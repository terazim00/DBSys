#include "../include/optimized_join.h"
#include "../include/file_manager.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <set>
#include <cstdlib>

/**
 * ============================================================================
 * 종합 성능 테스트 프로그램
 * ============================================================================
 *
 * 테스트 시나리오:
 * 1. 작은 데이터셋으로 정확성 검증
 * 2. 버퍼 크기별 성능 측정
 * 3. 알고리즘별 비교 (BNLJ vs Hash Join)
 * 4. 메모리 누수 체크 (간단한 버전)
 */

// ============================================================================
// 1. 정확성 검증
// ============================================================================
class CorrectnessValidator {
public:
    static bool validateJoinResult(const std::string& result_file) {
        std::cout << "\n=== Validating Join Result ===" << std::endl;

        FileManager fm;
        size_t total_records = 0;
        size_t invalid_records = 0;
        std::set<std::pair<int, int>> unique_pairs;

        fm.readBlockFile(result_file, [&](const Record& rec) {
            total_records++;

            // 조인 결과는 14개 필드를 가져야 함
            if (rec.getFieldCount() != 14) {
                invalid_records++;
                return;
            }

            try {
                // PART.PARTKEY와 PARTSUPP.PARTKEY가 일치해야 함
                int part_key = std::stoi(rec.getField(0));
                int partsupp_key = std::stoi(rec.getField(9));

                if (part_key != partsupp_key) {
                    std::cerr << "ERROR: Key mismatch! PART.PARTKEY=" << part_key
                              << " != PARTSUPP.PARTKEY=" << partsupp_key << std::endl;
                    invalid_records++;
                }

                // 중복 체크
                int suppkey = std::stoi(rec.getField(10));
                unique_pairs.insert({part_key, suppkey});

            } catch (const std::exception& e) {
                std::cerr << "ERROR: Failed to parse record: " << e.what() << std::endl;
                invalid_records++;
            }
        });

        std::cout << "Total Records:   " << total_records << std::endl;
        std::cout << "Invalid Records: " << invalid_records << std::endl;
        std::cout << "Unique Pairs:    " << unique_pairs.size() << std::endl;

        bool valid = (invalid_records == 0);
        std::cout << (valid ? "✓ PASSED" : "✗ FAILED") << std::endl;

        return valid;
    }

    static bool compareResults(const std::string& file1, const std::string& file2) {
        std::cout << "\n=== Comparing Two Result Files ===" << std::endl;

        FileManager fm;

        // 파일1의 모든 레코드를 세트에 저장
        std::set<std::string> records1;
        fm.readBlockFile(file1, [&](const Record& rec) {
            std::string key = rec.getField(0) + "|" + rec.getField(10);
            records1.insert(key);
        });

        // 파일2의 레코드가 파일1에 모두 있는지 확인
        size_t matching = 0;
        size_t missing = 0;

        fm.readBlockFile(file2, [&](const Record& rec) {
            std::string key = rec.getField(0) + "|" + rec.getField(10);
            if (records1.count(key)) {
                matching++;
            } else {
                missing++;
            }
        });

        std::cout << "File 1 records: " << records1.size() << std::endl;
        std::cout << "Matching:       " << matching << std::endl;
        std::cout << "Missing:        " << missing << std::endl;

        bool same = (records1.size() == matching && missing == 0);
        std::cout << (same ? "✓ IDENTICAL" : "✗ DIFFERENT") << std::endl;

        return same;
    }
};

// ============================================================================
// 2. 메모리 누수 체크 (간단한 버전)
// ============================================================================
class MemoryLeakChecker {
public:
    static void checkMemoryLeak(int iterations = 10) {
        std::cout << "\n=== Memory Leak Check ===" << std::endl;
        std::cout << "Running " << iterations << " iterations..." << std::endl;

        // 초기 메모리 사용량 (간단한 근사치)
        size_t initial_memory = getCurrentMemoryUsage();

        for (int i = 0; i < iterations; ++i) {
            // 작은 조인 반복 실행
            try {
                BlockNestedLoopsJoin join(
                    "data/part_sample.dat",
                    "data/partsupp_sample.dat",
                    "output/leak_test_" + std::to_string(i) + ".dat",
                    "PART", "PARTSUPP", 10, 4096);

                join.execute();

            } catch (const std::exception& e) {
                std::cerr << "Error in iteration " << i << ": " << e.what() << std::endl;
            }

            if (i % 3 == 0) {
                size_t current_memory = getCurrentMemoryUsage();
                std::cout << "Iteration " << i << ": ~" << current_memory << " KB" << std::endl;
            }
        }

        size_t final_memory = getCurrentMemoryUsage();

        std::cout << "\nMemory Usage:" << std::endl;
        std::cout << "  Initial: ~" << initial_memory << " KB" << std::endl;
        std::cout << "  Final:   ~" << final_memory << " KB" << std::endl;
        std::cout << "  Diff:    ~" << (final_memory - initial_memory) << " KB" << std::endl;

        if (final_memory - initial_memory < 1000) {  // < 1MB 증가
            std::cout << "✓ PASSED (No significant memory leak detected)" << std::endl;
        } else {
            std::cout << "✗ WARNING (Possible memory leak)" << std::endl;
        }
    }

private:
    static size_t getCurrentMemoryUsage() {
        // 간단한 메모리 사용량 근사치 (Linux /proc/self/status)
#ifdef __linux__
        std::ifstream status("/proc/self/status");
        std::string line;
        while (std::getline(status, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                size_t pos = line.find_first_of("0123456789");
                if (pos != std::string::npos) {
                    return std::stoul(line.substr(pos));
                }
            }
        }
#endif
        return 0;  // 지원하지 않는 플랫폼
    }
};

// ============================================================================
// 3. 버퍼 크기별 성능 측정
// ============================================================================
void testBufferSizes() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Buffer Size Performance Test" << std::endl;
    std::cout << "========================================" << std::endl;

    std::vector<size_t> buffer_sizes = {3, 5, 10, 20, 50};

    std::cout << std::setw(12) << "Buffer Size"
              << std::setw(15) << "Time (s)"
              << std::setw(15) << "Block Reads"
              << std::setw(15) << "Speedup"
              << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    double baseline_time = 0.0;

    for (size_t i = 0; i < buffer_sizes.size(); ++i) {
        size_t buf_size = buffer_sizes[i];

        try {
            BlockNestedLoopsJoin join(
                "data/part_sample.dat",
                "data/partsupp_sample.dat",
                "output/test_buf" + std::to_string(buf_size) + ".dat",
                "PART", "PARTSUPP",
                buf_size, 4096);

            join.execute();

            const Statistics& stats = join.getStatistics();

            if (i == 0) {
                baseline_time = stats.elapsed_time;
            }

            double speedup = (baseline_time > 0) ? (baseline_time / stats.elapsed_time) : 1.0;

            std::cout << std::setw(12) << buf_size
                      << std::setw(15) << std::fixed << std::setprecision(4) << stats.elapsed_time
                      << std::setw(15) << stats.block_reads
                      << std::setw(15) << std::fixed << std::setprecision(2) << speedup << "x"
                      << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "Error with buffer size " << buf_size << ": " << e.what() << std::endl;
        }
    }

    std::cout << std::string(60, '=') << std::endl;
}

// ============================================================================
// 4. 알고리즘 비교
// ============================================================================
void compareAlgorithms() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Algorithm Comparison" << std::endl;
    std::cout << "========================================" << std::endl;

    std::vector<PerformanceResult> results;

    // Block Nested Loops (버퍼 10)
    try {
        std::cout << "\n--- Testing Block Nested Loops Join ---" << std::endl;
        auto result = PerformanceTester::testBlockNestedLoops(
            "data/part_sample.dat",
            "data/partsupp_sample.dat",
            "output/bnlj_result.dat",
            10);
        results.push_back(result);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // Hash Join
    try {
        std::cout << "\n--- Testing Hash Join ---" << std::endl;
        auto result = PerformanceTester::testHashJoin(
            "data/part_sample.dat",
            "data/partsupp_sample.dat",
            "output/hash_result.dat");
        results.push_back(result);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // 결과 비교
    if (results.size() >= 2) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  Performance Summary" << std::endl;
        std::cout << "========================================" << std::endl;

        std::cout << std::setw(30) << "Algorithm"
                  << std::setw(15) << "Time (s)"
                  << std::setw(15) << "Block Reads"
                  << std::setw(15) << "Speedup"
                  << std::endl;
        std::cout << std::string(75, '-') << std::endl;

        for (size_t i = 0; i < results.size(); ++i) {
            double speedup = (i == 0) ? 1.0 : results[i].getSpeedup(results[0]);

            std::cout << std::setw(30) << results[i].algorithm_name
                      << std::setw(15) << std::fixed << std::setprecision(4) << results[i].elapsed_time
                      << std::setw(15) << results[i].block_reads
                      << std::setw(15) << std::fixed << std::setprecision(2) << speedup << "x"
                      << std::endl;
        }

        std::cout << std::string(75, '=') << std::endl;

        // 결과 일치 여부 확인
        if (results.size() == 2) {
            bool same = CorrectnessValidator::compareResults(
                "output/bnlj_result.dat",
                "output/hash_result.dat");

            if (same) {
                std::cout << "\n✓ Both algorithms produced identical results!" << std::endl;
            } else {
                std::cout << "\n✗ WARNING: Results differ between algorithms!" << std::endl;
            }
        }
    }
}

// ============================================================================
// 5. 샘플 데이터 생성
// ============================================================================
void generateSampleData() {
    std::cout << "\n=== Generating Sample Data ===" << std::endl;

    FileManager fm(4096, 10);

    // PART 테이블 (100 레코드)
    std::vector<PartRecord> parts;
    for (int i = 1; i <= 100; ++i) {
        PartRecord part;
        part.partkey = i;
        part.name = "Part " + std::to_string(i);
        part.mfgr = "Manufacturer#" + std::to_string((i % 5) + 1);
        part.brand = "Brand#" + std::to_string((i % 5) + 1);
        part.type = "TYPE" + std::to_string(i % 3);
        part.size = (i % 50) + 1;
        part.container = "CONTAINER";
        part.retailprice = 1000.0f + i;
        part.comment = "Comment for part " + std::to_string(i);

        parts.push_back(part);
    }

    fm.writePartRecords("data/part_sample.dat", parts);
    std::cout << "Created " << parts.size() << " PART records" << std::endl;

    // PARTSUPP 테이블 (400 레코드 = 100 parts × 4 suppliers)
    std::vector<PartSuppRecord> partsupps;
    for (int partkey = 1; partkey <= 100; ++partkey) {
        for (int suppkey = 1; suppkey <= 4; ++suppkey) {
            PartSuppRecord ps;
            ps.partkey = partkey;
            ps.suppkey = suppkey;
            ps.availqty = (partkey * suppkey) % 1000;
            ps.supplycost = 50.0f + (partkey % 100);
            ps.comment = "Supplier " + std::to_string(suppkey) +
                        " for part " + std::to_string(partkey);

            partsupps.push_back(ps);
        }
    }

    fm.writePartSuppRecords("data/partsupp_sample.dat", partsupps);
    std::cout << "Created " << partsupps.size() << " PARTSUPP records" << std::endl;

    std::cout << "Expected join result: " << partsupps.size() << " records\n" << std::endl;
}

// ============================================================================
// 메인 함수
// ============================================================================
int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "  Comprehensive Performance Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    // 명령줄 인자 파싱
    std::string mode = (argc > 1) ? argv[1] : "all";

    try {
        if (mode == "generate" || mode == "all") {
            generateSampleData();
        }

        if (mode == "correctness" || mode == "all") {
            // 정확성 검증
            generateSampleData();

            BlockNestedLoopsJoin join(
                "data/part_sample.dat",
                "data/partsupp_sample.dat",
                "output/correctness_test.dat",
                "PART", "PARTSUPP", 10, 4096);
            join.execute();

            CorrectnessValidator::validateJoinResult("output/correctness_test.dat");
        }

        if (mode == "buffer" || mode == "all") {
            // 버퍼 크기 테스트
            generateSampleData();
            testBufferSizes();
        }

        if (mode == "compare" || mode == "all") {
            // 알고리즘 비교
            generateSampleData();
            compareAlgorithms();
        }

        if (mode == "memory" || mode == "all") {
            // 메모리 누수 체크
            generateSampleData();
            MemoryLeakChecker::checkMemoryLeak(5);
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "  All tests completed!" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\nFatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
