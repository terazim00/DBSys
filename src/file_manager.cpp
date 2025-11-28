#include "file_manager.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdexcept>

// ============================================================================
// 생성자 & 소멸자
// ============================================================================

FileManager::FileManager(size_t blk_size, size_t buf_count)
    : block_size(blk_size), buffer_count(buf_count) {
    // 버퍼 풀 생성
    buffer_mgr = std::make_unique<BufferManager>(buffer_count, block_size);
}

// ============================================================================
// CSV 변환 기능
// ============================================================================

size_t FileManager::convertCSV(const std::string& csv_file,
                               const std::string& block_file,
                               const std::string& table_type) {
    try {
        // 기존 convertCSVToBlocks 함수 활용
        std::ifstream input(csv_file);
        if (!input.is_open()) {
            throw std::runtime_error("Failed to open CSV file: " + csv_file);
        }

        TableWriter writer(block_file, &stats);
        Block block(block_size);
        RecordWriter rec_writer(&block);

        std::string line;
        size_t record_count = 0;

        while (std::getline(input, line)) {
            if (line.empty()) continue;

            try {
                Record record;

                if (table_type == "PART") {
                    PartRecord part = PartRecord::fromCSV(line);
                    record = part.toRecord();
                } else if (table_type == "PARTSUPP") {
                    PartSuppRecord partsupp = PartSuppRecord::fromCSV(line);
                    record = partsupp.toRecord();
                } else {
                    throw std::runtime_error("Unknown table type: " + table_type);
                }

                // 블록에 레코드 쓰기
                if (!rec_writer.writeRecord(record)) {
                    // 블록이 가득 차면 디스크에 쓰고 새 블록 시작
                    writer.writeBlock(&block);
                    block.clear();

                    // 새 블록에 레코드 쓰기
                    if (!rec_writer.writeRecord(record)) {
                        throw std::runtime_error("Record too large for block");
                    }
                }

                record_count++;
            } catch (const std::exception& e) {
                std::cerr << "Warning: Error parsing line: " << e.what() << std::endl;
            }
        }

        // 마지막 블록 쓰기
        if (!block.isEmpty()) {
            writer.writeBlock(&block);
        }

        input.close();
        return record_count;

    } catch (const std::exception& e) {
        throw std::runtime_error("convertCSV failed: " + std::string(e.what()));
    }
}

// ============================================================================
// 블록 파일 읽기 기능
// ============================================================================

size_t FileManager::readBlockFile(const std::string& block_file,
                                  std::function<void(const Record&)> callback) {
    try {
        TableReader reader(block_file, block_size, &stats);

        if (!reader.isOpen()) {
            throw std::runtime_error("Failed to open file: " + block_file);
        }

        Block block(block_size);
        size_t record_count = 0;

        // 모든 블록 읽기
        while (reader.readBlock(&block)) {
            RecordReader rec_reader(&block);

            // 블록의 모든 레코드 읽기
            while (rec_reader.hasNext()) {
                Record record = rec_reader.readNext();
                callback(record);
                record_count++;
            }
        }

        return record_count;

    } catch (const std::exception& e) {
        throw std::runtime_error("readBlockFile failed: " + std::string(e.what()));
    }
}

size_t FileManager::readPartRecords(const std::string& block_file,
                                    std::function<void(const PartRecord&)> callback) {
    return readBlockFile(block_file, [&callback](const Record& record) {
        try {
            PartRecord part = PartRecord::fromRecord(record);
            callback(part);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to parse PART record: " << e.what() << std::endl;
        }
    });
}

size_t FileManager::readPartSuppRecords(const std::string& block_file,
                                        std::function<void(const PartSuppRecord&)> callback) {
    return readBlockFile(block_file, [&callback](const Record& record) {
        try {
            PartSuppRecord partsupp = PartSuppRecord::fromRecord(record);
            callback(partsupp);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to parse PARTSUPP record: " << e.what() << std::endl;
        }
    });
}

// ============================================================================
// 블록 파일 쓰기 기능
// ============================================================================

size_t FileManager::writePartRecords(const std::string& block_file,
                                     const std::vector<PartRecord>& records) {
    try {
        TableWriter writer(block_file, &stats);

        if (!writer.isOpen()) {
            throw std::runtime_error("Failed to open file: " + block_file);
        }

        Block block(block_size);
        RecordWriter rec_writer(&block);
        size_t written_count = 0;

        for (const auto& part : records) {
            Record record = part.toRecord();

            // 블록에 레코드 쓰기
            if (!rec_writer.writeRecord(record)) {
                // 블록이 가득 차면 디스크에 쓰기
                writer.writeBlock(&block);
                block.clear();

                // 새 블록에 레코드 쓰기
                if (!rec_writer.writeRecord(record)) {
                    throw std::runtime_error("Record too large for block");
                }
            }

            written_count++;
        }

        // 마지막 블록 쓰기
        if (!block.isEmpty()) {
            writer.writeBlock(&block);
        }

        return written_count;

    } catch (const std::exception& e) {
        throw std::runtime_error("writePartRecords failed: " + std::string(e.what()));
    }
}

size_t FileManager::writePartSuppRecords(const std::string& block_file,
                                         const std::vector<PartSuppRecord>& records) {
    try {
        TableWriter writer(block_file, &stats);

        if (!writer.isOpen()) {
            throw std::runtime_error("Failed to open file: " + block_file);
        }

        Block block(block_size);
        RecordWriter rec_writer(&block);
        size_t written_count = 0;

        for (const auto& partsupp : records) {
            Record record = partsupp.toRecord();

            // 블록에 레코드 쓰기
            if (!rec_writer.writeRecord(record)) {
                // 블록이 가득 차면 디스크에 쓰기
                writer.writeBlock(&block);
                block.clear();

                // 새 블록에 레코드 쓰기
                if (!rec_writer.writeRecord(record)) {
                    throw std::runtime_error("Record too large for block");
                }
            }

            written_count++;
        }

        // 마지막 블록 쓰기
        if (!block.isEmpty()) {
            writer.writeBlock(&block);
        }

        return written_count;

    } catch (const std::exception& e) {
        throw std::runtime_error("writePartSuppRecords failed: " + std::string(e.what()));
    }
}

// ============================================================================
// 버퍼 관리 기능
// ============================================================================

void FileManager::clearBuffers() {
    if (buffer_mgr) {
        buffer_mgr->clearAll();
    }
}

void FileManager::resizeBuffers(size_t new_buffer_count) {
    buffer_count = new_buffer_count;
    buffer_mgr = std::make_unique<BufferManager>(buffer_count, block_size);
}

Block* FileManager::getBuffer(size_t idx) {
    if (!buffer_mgr) {
        throw std::runtime_error("Buffer manager not initialized");
    }

    if (idx >= buffer_count) {
        throw std::out_of_range("Buffer index out of range: " + std::to_string(idx));
    }

    return buffer_mgr->getBuffer(idx);
}

// ============================================================================
// 통계 및 정보
// ============================================================================

void FileManager::resetStatistics() {
    stats = Statistics();
}

void FileManager::printStatistics() const {
    std::cout << "\n=== FileManager Statistics ===" << std::endl;
    std::cout << std::setw(20) << "Block Reads: " << stats.block_reads << std::endl;
    std::cout << std::setw(20) << "Block Writes: " << stats.block_writes << std::endl;
    std::cout << std::setw(20) << "Output Records: " << stats.output_records << std::endl;
    std::cout << std::setw(20) << "Elapsed Time: " << std::fixed << std::setprecision(3)
              << stats.elapsed_time << " seconds" << std::endl;
    std::cout << std::setw(20) << "Memory Usage: "
              << (stats.memory_usage / 1024.0 / 1024.0) << " MB" << std::endl;
}

size_t FileManager::getMemoryUsage() const {
    if (buffer_mgr) {
        return buffer_mgr->getMemoryUsage();
    }
    return 0;
}

// ============================================================================
// 유틸리티 함수
// ============================================================================

size_t FileManager::countRecords(const std::string& block_file) {
    size_t count = 0;

    readBlockFile(block_file, [&count](const Record&) {
        count++;
    });

    return count;
}

size_t FileManager::countBlocks(const std::string& block_file) {
    try {
        Statistics local_stats;
        TableReader reader(block_file, block_size, &local_stats);

        if (!reader.isOpen()) {
            throw std::runtime_error("Failed to open file: " + block_file);
        }

        Block block(block_size);
        size_t block_count = 0;

        while (reader.readBlock(&block)) {
            block_count++;
        }

        return block_count;

    } catch (const std::exception& e) {
        throw std::runtime_error("countBlocks failed: " + std::string(e.what()));
    }
}

void FileManager::printFileInfo(const std::string& block_file) {
    try {
        std::cout << "\n=== File Information ===" << std::endl;
        std::cout << "File: " << block_file << std::endl;

        // 파일 크기 확인
        std::ifstream file(block_file, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + block_file);
        }

        std::streamsize file_size = file.tellg();
        file.close();

        // 블록 및 레코드 개수 카운트
        size_t num_blocks = countBlocks(block_file);
        size_t num_records = countRecords(block_file);

        // 정보 출력
        std::cout << std::setw(20) << "File Size: "
                  << (file_size / 1024.0 / 1024.0) << " MB" << std::endl;
        std::cout << std::setw(20) << "Block Size: " << block_size << " bytes" << std::endl;
        std::cout << std::setw(20) << "Total Blocks: " << num_blocks << std::endl;
        std::cout << std::setw(20) << "Total Records: " << num_records << std::endl;
        std::cout << std::setw(20) << "Avg Records/Block: "
                  << std::fixed << std::setprecision(2)
                  << (num_blocks > 0 ? static_cast<double>(num_records) / num_blocks : 0.0)
                  << std::endl;
        std::cout << std::setw(20) << "Storage Efficiency: "
                  << std::fixed << std::setprecision(1)
                  << (file_size > 0 ? (num_blocks * block_size * 100.0) / file_size : 0.0)
                  << "%" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
