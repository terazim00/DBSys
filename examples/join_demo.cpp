#include "../include/join.h"
#include "../include/file_manager.h"
#include <iostream>
#include <iomanip>

/**
 * Block Nested Loops Join 데모 프로그램
 *
 * 이 프로그램은 다음을 수행합니다:
 * 1. 샘플 PART 및 PARTSUPP 데이터 생성
 * 2. Block Nested Loops Join 실행
 * 3. 다양한 버퍼 크기로 성능 비교
 * 4. 조인 결과 검증
 */

// ============================================================================
// 샘플 데이터 생성 함수
// ============================================================================
void generateSampleData() {
    std::cout << "=== Generating Sample Data ===" << std::endl;

    FileManager fm(4096, 10);

    // ========== PART 테이블 샘플 데이터 생성 ==========
    std::vector<PartRecord> parts;

    for (int i = 1; i <= 100; ++i) {
        PartRecord part;
        part.partkey = i;
        part.name = "Part Name " + std::to_string(i);
        part.mfgr = "Manufacturer#" + std::to_string((i % 5) + 1);
        part.brand = "Brand#" + std::to_string((i % 5) + 1) + std::to_string((i % 5) + 1);
        part.type = "STANDARD ANODIZED STEEL";
        part.size = (i % 50) + 1;
        part.container = "SM BOX";
        part.retailprice = 900.0f + static_cast<float>(i);
        part.comment = "Sample comment for part " + std::to_string(i);

        parts.push_back(part);
    }

    size_t part_count = fm.writePartRecords("data/part_sample.dat", parts);
    std::cout << "Created " << part_count << " PART records" << std::endl;

    // ========== PARTSUPP 테이블 샘플 데이터 생성 ==========
    std::vector<PartSuppRecord> partsupps;

    // 각 PART에 대해 4개의 PARTSUPP 레코드 생성
    for (int partkey = 1; partkey <= 100; ++partkey) {
        for (int suppkey = 1; suppkey <= 4; ++suppkey) {
            PartSuppRecord partsupp;
            partsupp.partkey = partkey;
            partsupp.suppkey = suppkey;
            partsupp.availqty = (partkey * suppkey) % 1000;
            partsupp.supplycost = 10.0f + static_cast<float>(partkey * suppkey % 100);
            partsupp.comment = "Supplier " + std::to_string(suppkey) +
                              " for part " + std::to_string(partkey);

            partsupps.push_back(partsupp);
        }
    }

    size_t partsupp_count = fm.writePartSuppRecords("data/partsupp_sample.dat", partsupps);
    std::cout << "Created " << partsupp_count << " PARTSUPP records" << std::endl;

    std::cout << "\n✓ Sample data generated successfully!\n" << std::endl;
}

// ============================================================================
// 조인 실행 함수
// ============================================================================
void runJoin(size_t buffer_size) {
    std::cout << "=== Running Block Nested Loops Join ===" << std::endl;
    std::cout << "Buffer Size: " << buffer_size << " blocks" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    try {
        // BlockNestedLoopsJoin 생성
        BlockNestedLoopsJoin join(
            "data/part_sample.dat",      // Outer 테이블
            "data/partsupp_sample.dat",  // Inner 테이블
            "output/join_result.dat",    // 출력 파일
            "PART",                       // Outer 타입
            "PARTSUPP",                   // Inner 타입
            buffer_size,                  // 버퍼 크기
            4096                          // 블록 크기
        );

        // 조인 실행
        join.execute();

        // 통계 가져오기
        const Statistics& stats = join.getStatistics();

        // 상세 통계 출력
        std::cout << "\n=== Detailed Statistics ===" << std::endl;
        std::cout << std::setw(25) << "Block Reads: " << stats.block_reads << std::endl;
        std::cout << std::setw(25) << "Block Writes: " << stats.block_writes << std::endl;
        std::cout << std::setw(25) << "Output Records: " << stats.output_records << std::endl;
        std::cout << std::setw(25) << "Elapsed Time: "
                  << std::fixed << std::setprecision(4)
                  << stats.elapsed_time << " seconds" << std::endl;
        std::cout << std::setw(25) << "Memory Usage: "
                  << (stats.memory_usage / 1024.0) << " KB" << std::endl;

        // I/O 효율성 계산
        double io_efficiency = (stats.block_writes > 0) ?
            static_cast<double>(stats.output_records) / stats.block_writes : 0.0;
        std::cout << std::setw(25) << "Records per Block Write: "
                  << std::fixed << std::setprecision(2)
                  << io_efficiency << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// ============================================================================
// 조인 결과 검증 함수
// ============================================================================
void verifyJoinResult() {
    std::cout << "\n=== Verifying Join Result ===" << std::endl;

    try {
        FileManager fm;

        size_t record_count = 0;
        size_t valid_records = 0;

        fm.readBlockFile("output/join_result.dat", [&](const Record& rec) {
            record_count++;

            // 조인 결과는 14개 필드를 가져야 함 (PART 9개 + PARTSUPP 5개)
            if (rec.getFieldCount() == 14) {
                valid_records++;

                // 첫 5개 레코드만 출력
                if (record_count <= 5) {
                    std::cout << "\nRecord " << record_count << ":" << std::endl;
                    std::cout << "  PART.PARTKEY: " << rec.getField(0) << std::endl;
                    std::cout << "  PART.NAME: " << rec.getField(1).substr(0, 30) << "..." << std::endl;
                    std::cout << "  PARTSUPP.PARTKEY: " << rec.getField(9) << std::endl;
                    std::cout << "  PARTSUPP.SUPPKEY: " << rec.getField(10) << std::endl;

                    // 조인 키 일치 확인
                    if (rec.getField(0) == rec.getField(9)) {
                        std::cout << "  ✓ Join keys match!" << std::endl;
                    } else {
                        std::cout << "  ✗ Join keys DO NOT match!" << std::endl;
                    }
                }
            }
        });

        std::cout << "\n=== Verification Summary ===" << std::endl;
        std::cout << "Total Records: " << record_count << std::endl;
        std::cout << "Valid Records: " << valid_records << std::endl;
        std::cout << "Invalid Records: " << (record_count - valid_records) << std::endl;

        if (valid_records == record_count && record_count > 0) {
            std::cout << "✓ All records are valid!" << std::endl;
        } else {
            std::cout << "✗ Some records are invalid!" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// ============================================================================
// 버퍼 크기별 성능 비교 함수
// ============================================================================
void compareBufferSizes() {
    std::cout << "\n=== Buffer Size Performance Comparison ===" << std::endl;
    std::cout << std::string(80, '=') << std::endl;

    std::vector<size_t> buffer_sizes = {3, 5, 10, 20};

    std::cout << std::setw(12) << "Buffer Size"
              << std::setw(15) << "Block Reads"
              << std::setw(15) << "Block Writes"
              << std::setw(15) << "Time (sec)"
              << std::setw(15) << "Memory (KB)"
              << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    for (size_t buffer_size : buffer_sizes) {
        try {
            BlockNestedLoopsJoin join(
                "data/part_sample.dat",
                "data/partsupp_sample.dat",
                "output/join_result_buf" + std::to_string(buffer_size) + ".dat",
                "PART",
                "PARTSUPP",
                buffer_size,
                4096
            );

            join.execute();
            const Statistics& stats = join.getStatistics();

            std::cout << std::setw(12) << buffer_size
                      << std::setw(15) << stats.block_reads
                      << std::setw(15) << stats.block_writes
                      << std::setw(15) << std::fixed << std::setprecision(4) << stats.elapsed_time
                      << std::setw(15) << std::fixed << std::setprecision(1)
                      << (stats.memory_usage / 1024.0)
                      << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Error with buffer size " << buffer_size << ": "
                      << e.what() << std::endl;
        }
    }

    std::cout << std::string(80, '=') << std::endl;
}

// ============================================================================
// 메인 함수
// ============================================================================
int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "  Block Nested Loops Join Demo" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        if (argc > 1 && std::string(argv[1]) == "--compare") {
            // 성능 비교 모드
            std::cout << "\nMode: Performance Comparison\n" << std::endl;
            generateSampleData();
            compareBufferSizes();

        } else if (argc > 1 && std::string(argv[1]) == "--verify") {
            // 검증 모드
            std::cout << "\nMode: Verification\n" << std::endl;
            verifyJoinResult();

        } else {
            // 기본 모드: 데이터 생성 + 조인 + 검증
            std::cout << "\nMode: Full Demo\n" << std::endl;

            // 1. 샘플 데이터 생성
            generateSampleData();

            // 2. 조인 실행 (버퍼 크기 10)
            runJoin(10);

            // 3. 결과 검증
            verifyJoinResult();
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "  Demo completed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\nFatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
