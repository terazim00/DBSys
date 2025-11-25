#include "../include/common.h"
#include "../include/block.h"
#include "../include/record.h"
#include "../include/table.h"
#include "../include/buffer.h"
#include <iostream>
#include <stdexcept>

/**
 * FileManager 통합 예제
 *
 * 이 예제는 TPC-H 데이터를 읽고 쓰는 전체 워크플로우를 보여줍니다:
 * 1. CSV 파일을 블록 파일로 변환
 * 2. 블록 파일 읽기
 * 3. 버퍼 관리
 * 4. 에러 처리
 */

// ============================================================================
// 예제 1: CSV 파일을 블록 파일로 변환
// ============================================================================
void example1_convert_csv_to_blocks() {
    std::cout << "\n=== Example 1: Convert CSV to Block File ===" << std::endl;

    try {
        const std::string csv_file = "data/part.tbl";
        const std::string block_file = "data/part.dat";
        const std::string table_type = "PART";
        const size_t block_size = 4096;

        std::cout << "Converting " << csv_file << " to " << block_file << "..." << std::endl;

        // CSV → 블록 파일 변환
        convertCSVToBlocks(csv_file, block_file, table_type, block_size);

        std::cout << "✓ Conversion completed successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "✗ Error: " << e.what() << std::endl;
    }
}

// ============================================================================
// 예제 2: 블록 파일 읽기 (버퍼 없이)
// ============================================================================
void example2_read_block_file() {
    std::cout << "\n=== Example 2: Read Block File ===" << std::endl;

    try {
        const std::string block_file = "data/part.dat";
        const size_t block_size = 4096;

        // 통계 수집
        Statistics stats;

        // 파일 리더 생성
        TableReader reader(block_file, block_size, &stats);

        if (!reader.isOpen()) {
            throw std::runtime_error("Failed to open file: " + block_file);
        }

        // 블록 생성
        Block block(block_size);

        int block_count = 0;
        int record_count = 0;

        // 모든 블록 읽기
        while (reader.readBlock(&block)) {
            block_count++;

            // 블록에서 레코드 읽기
            RecordReader rec_reader(&block);

            while (rec_reader.hasNext()) {
                Record record = rec_reader.readNext();
                record_count++;

                // 첫 5개 레코드만 출력
                if (record_count <= 5) {
                    PartRecord part = PartRecord::fromRecord(record);
                    std::cout << "  Record " << record_count
                              << ": PARTKEY=" << part.partkey
                              << ", NAME=" << part.name.substr(0, 20) << "..."
                              << std::endl;
                }
            }
        }

        std::cout << "\n✓ Read " << record_count << " records from "
                  << block_count << " blocks" << std::endl;
        std::cout << "  Block I/Os: " << stats.block_reads << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "✗ Error: " << e.what() << std::endl;
    }
}

// ============================================================================
// 예제 3: 버퍼 풀을 사용한 블록 읽기
// ============================================================================
void example3_read_with_buffer_pool() {
    std::cout << "\n=== Example 3: Read with Buffer Pool ===" << std::endl;

    try {
        const std::string block_file = "data/part.dat";
        const size_t block_size = 4096;
        const size_t buffer_count = 10;

        // 버퍼 풀 생성
        BufferManager buffer_mgr(buffer_count, block_size);

        std::cout << "Buffer Pool Created:" << std::endl;
        std::cout << "  Buffer Count: " << buffer_mgr.getBufferCount() << std::endl;
        std::cout << "  Memory Usage: " << (buffer_mgr.getMemoryUsage() / 1024.0)
                  << " KB" << std::endl;

        // 통계 수집
        Statistics stats;
        TableReader reader(block_file, block_size, &stats);

        int blocks_read = 0;
        int total_records = 0;

        // 버퍼 풀의 모든 버퍼에 블록 로드
        for (size_t i = 0; i < buffer_mgr.getBufferCount(); ++i) {
            Block* buffer = buffer_mgr.getBuffer(i);

            if (reader.readBlock(buffer)) {
                blocks_read++;

                // 버퍼의 레코드 카운트
                RecordReader rec_reader(buffer);
                int records_in_block = 0;

                while (rec_reader.hasNext()) {
                    rec_reader.readNext();
                    records_in_block++;
                    total_records++;
                }

                std::cout << "  Buffer " << i << ": " << records_in_block
                          << " records (" << buffer->getUsedSize() << " bytes)"
                          << std::endl;
            } else {
                break;
            }
        }

        std::cout << "\n✓ Loaded " << blocks_read << " blocks into buffer pool"
                  << std::endl;
        std::cout << "  Total Records: " << total_records << std::endl;
        std::cout << "  Block I/Os: " << stats.block_reads << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "✗ Error: " << e.what() << std::endl;
    }
}

// ============================================================================
// 예제 4: 블록 파일 쓰기
// ============================================================================
void example4_write_block_file() {
    std::cout << "\n=== Example 4: Write Block File ===" << std::endl;

    try {
        const std::string output_file = "output/example_output.dat";
        const size_t block_size = 4096;

        // 통계 수집
        Statistics stats;

        // 파일 라이터 생성
        TableWriter writer(output_file, &stats);

        if (!writer.isOpen()) {
            throw std::runtime_error("Failed to open file: " + output_file);
        }

        // 블록 및 레코드 라이터 생성
        Block block(block_size);
        RecordWriter rec_writer(&block);

        // 샘플 레코드 생성 및 쓰기
        int records_written = 0;
        int blocks_written = 0;

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
            part.comment = "This is a sample comment for testing";

            Record record = part.toRecord();

            // 블록에 레코드 쓰기
            if (!rec_writer.writeRecord(record)) {
                // 블록이 가득 차면 디스크에 쓰기
                writer.writeBlock(&block);
                blocks_written++;
                block.clear();

                // 새 블록에 레코드 쓰기
                if (!rec_writer.writeRecord(record)) {
                    throw std::runtime_error("Record too large for block");
                }
            }

            records_written++;
        }

        // 마지막 블록 쓰기
        if (!block.isEmpty()) {
            writer.writeBlock(&block);
            blocks_written++;
        }

        std::cout << "✓ Wrote " << records_written << " records to "
                  << blocks_written << " blocks" << std::endl;
        std::cout << "  Output File: " << output_file << std::endl;
        std::cout << "  Block Writes: " << stats.block_writes << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "✗ Error: " << e.what() << std::endl;
    }
}

// ============================================================================
// 예제 5: 에러 처리 종합
// ============================================================================
void example5_error_handling() {
    std::cout << "\n=== Example 5: Error Handling ===" << std::endl;

    // 1. 존재하지 않는 파일 읽기
    std::cout << "\n1. Reading non-existent file:" << std::endl;
    try {
        TableReader reader("nonexistent.dat");
        std::cout << "✗ Should have thrown exception!" << std::endl;
    } catch (const std::runtime_error& e) {
        std::cout << "✓ Caught expected error: " << e.what() << std::endl;
    }

    // 2. 블록에 너무 큰 레코드 추가
    std::cout << "\n2. Adding too large record to block:" << std::endl;
    try {
        Block block(1024);  // 작은 블록 (1KB)

        // 매우 큰 레코드 생성
        std::string huge_field(2000, 'X');  // 2KB 필드
        Record huge_record;
        huge_record.addField(huge_field);

        RecordWriter writer(&block);
        bool success = writer.writeRecord(huge_record);

        if (!success) {
            std::cout << "✓ Block correctly rejected oversized record" << std::endl;
        } else {
            std::cout << "✗ Block should have rejected oversized record!" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cout << "✗ Unexpected error: " << e.what() << std::endl;
    }

    // 3. 잘못된 테이블 타입
    std::cout << "\n3. Converting with invalid table type:" << std::endl;
    try {
        convertCSVToBlocks("data/part.tbl", "output/test.dat",
                          "INVALID_TYPE", 4096);
        std::cout << "✗ Should have thrown exception!" << std::endl;
    } catch (const std::runtime_error& e) {
        std::cout << "✓ Caught expected error: " << e.what() << std::endl;
    }

    // 4. 버퍼 인덱스 범위 체크
    std::cout << "\n4. Accessing buffer with invalid index:" << std::endl;
    try {
        BufferManager buffer_mgr(5, 4096);

        // 유효한 인덱스 (0-4)
        Block* valid_buffer = buffer_mgr.getBuffer(0);
        if (valid_buffer != nullptr) {
            std::cout << "✓ Valid index access successful" << std::endl;
        }

        // 참고: 실제 구현에서는 범위 체크를 추가해야 함
        // Block* invalid_buffer = buffer_mgr.getBuffer(10);  // 범위 초과

    } catch (const std::exception& e) {
        std::cout << "✓ Caught expected error: " << e.what() << std::endl;
    }
}

// ============================================================================
// 예제 6: 전체 워크플로우 (CSV → 블록 파일 → 처리 → 출력)
// ============================================================================
void example6_complete_workflow() {
    std::cout << "\n=== Example 6: Complete Workflow ===" << std::endl;

    try {
        // 단계 1: CSV → 블록 파일
        std::cout << "\nStep 1: Converting CSV to block format..." << std::endl;
        convertCSVToBlocks("data/part.tbl", "data/part.dat", "PART", 4096);
        std::cout << "✓ Conversion complete" << std::endl;

        // 단계 2: 버퍼 풀 생성
        std::cout << "\nStep 2: Creating buffer pool..." << std::endl;
        const size_t buffer_count = 10;
        const size_t block_size = 4096;
        BufferManager buffer_mgr(buffer_count, block_size);
        std::cout << "✓ Buffer pool created ("
                  << (buffer_mgr.getMemoryUsage() / 1024.0) << " KB)"
                  << std::endl;

        // 단계 3: 블록 파일 읽기 및 필터링
        std::cout << "\nStep 3: Reading and filtering data..." << std::endl;
        Statistics read_stats;
        TableReader reader("data/part.dat", block_size, &read_stats);

        // 출력 파일 준비
        Statistics write_stats;
        TableWriter writer("output/filtered_output.dat", &write_stats);
        Block output_block(block_size);
        RecordWriter rec_writer(&output_block);

        Block input_block(block_size);
        int total_records = 0;
        int filtered_records = 0;

        // 모든 블록 처리
        while (reader.readBlock(&input_block)) {
            RecordReader rec_reader(&input_block);

            while (rec_reader.hasNext()) {
                Record record = rec_reader.readNext();
                PartRecord part = PartRecord::fromRecord(record);
                total_records++;

                // 필터 조건: SIZE > 25
                if (part.size > 25) {
                    filtered_records++;

                    // 출력 블록에 쓰기
                    if (!rec_writer.writeRecord(record)) {
                        writer.writeBlock(&output_block);
                        output_block.clear();
                        rec_writer.writeRecord(record);
                    }
                }
            }
        }

        // 마지막 출력 블록 쓰기
        if (!output_block.isEmpty()) {
            writer.writeBlock(&output_block);
        }

        std::cout << "✓ Processing complete" << std::endl;
        std::cout << "\nResults:" << std::endl;
        std::cout << "  Total Records Read: " << total_records << std::endl;
        std::cout << "  Filtered Records: " << filtered_records << std::endl;
        std::cout << "  Block Reads: " << read_stats.block_reads << std::endl;
        std::cout << "  Block Writes: " << write_stats.block_writes << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "✗ Error in workflow: " << e.what() << std::endl;
    }
}

// ============================================================================
// Main Function
// ============================================================================
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  TPC-H File Manager Examples" << std::endl;
    std::cout << "========================================" << std::endl;

    // 주석을 해제하여 원하는 예제 실행

    // example1_convert_csv_to_blocks();
    // example2_read_block_file();
    // example3_read_with_buffer_pool();
    // example4_write_block_file();
    example5_error_handling();
    // example6_complete_workflow();

    std::cout << "\n========================================" << std::endl;
    std::cout << "  All examples completed!" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
