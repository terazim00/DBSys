#include "join.h"
#include <iostream>
#include <chrono>
#include <vector>

BlockNestedLoopsJoin::BlockNestedLoopsJoin(
    const std::string& outer_file,
    const std::string& inner_file,
    const std::string& out_file,
    const std::string& outer_type,
    const std::string& inner_type,
    size_t buf_size,
    size_t blk_size)
    : outer_table_file(outer_file),
      inner_table_file(inner_file),
      output_file(out_file),
      outer_table_type(outer_type),
      inner_table_type(inner_type),
      buffer_size(buf_size),
      block_size(blk_size) {

    if (buffer_size < 2) {
        throw std::runtime_error("Buffer size must be at least 2 blocks");
    }
}

void BlockNestedLoopsJoin::execute() {
    auto start_time = std::chrono::high_resolution_clock::now();

    performJoin();

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    stats.elapsed_time = elapsed.count();

    // 메모리 사용량 계산
    stats.memory_usage = buffer_size * block_size;

    std::cout << "\n=== Join Statistics ===" << std::endl;
    std::cout << "Block Reads: " << stats.block_reads << std::endl;
    std::cout << "Block Writes: " << stats.block_writes << std::endl;
    std::cout << "Output Records: " << stats.output_records << std::endl;
    std::cout << "Elapsed Time: " << stats.elapsed_time << " seconds" << std::endl;
    std::cout << "Memory Usage: " << stats.memory_usage << " bytes ("
              << (stats.memory_usage / 1024.0 / 1024.0) << " MB)" << std::endl;
}

void BlockNestedLoopsJoin::performJoin() {
    TableReader outer_reader(outer_table_file, block_size, &stats);
    TableReader inner_reader(inner_table_file, block_size, &stats);
    TableWriter writer(output_file, &stats);

    // 버퍼 관리자 생성
    BufferManager buffer_mgr(buffer_size, block_size);

    // PART와 PARTSUPP 조인
    if ((outer_table_type == "PART" && inner_table_type == "PARTSUPP") ||
        (outer_table_type == "PARTSUPP" && inner_table_type == "PART")) {
        bool part_is_outer = (outer_table_type == "PART");
        joinPartAndPartSupp(outer_reader, inner_reader, writer, buffer_mgr, part_is_outer);
    } else {
        throw std::runtime_error("Unsupported table types for join");
    }
}

void BlockNestedLoopsJoin::joinPartAndPartSupp(
    TableReader& outer_reader,
    TableReader& inner_reader,
    TableWriter& writer,
    BufferManager& buffer_mgr,
    bool part_is_outer) {

    // 버퍼 할당: buffer_size - 1 개를 outer 테이블용, 1개를 inner 테이블용
    size_t outer_buffer_count = buffer_size - 1;

    Block output_block(block_size);
    RecordWriter output_writer(&output_block);

    // Outer 테이블 블록들을 버퍼에 로드
    bool has_outer_blocks = true;

    while (has_outer_blocks) {
        // Outer 테이블 블록들을 버퍼에 로드
        std::vector<Record> outer_records;
        size_t loaded_blocks = 0;

        for (size_t i = 0; i < outer_buffer_count; ++i) {
            Block* outer_block = buffer_mgr.getBuffer(i);
            outer_block->clear();

            if (outer_reader.readBlock(outer_block)) {
                loaded_blocks++;

                // 블록에서 레코드 읽기
                RecordReader reader(outer_block);
                while (reader.hasNext()) {
                    outer_records.push_back(reader.readNext());
                }
            } else {
                break;
            }
        }

        if (loaded_blocks == 0) {
            has_outer_blocks = false;
            break;
        }

        // Inner 테이블을 처음부터 스캔
        inner_reader.reset();
        Block* inner_block = buffer_mgr.getBuffer(buffer_size - 1);

        while (inner_reader.readBlock(inner_block)) {
            // Inner 블록에서 레코드 읽기
            std::vector<Record> inner_records;
            RecordReader inner_rec_reader(inner_block);

            while (inner_rec_reader.hasNext()) {
                inner_records.push_back(inner_rec_reader.readNext());
            }

            // 조인 수행: outer_records x inner_records
            for (const auto& outer_rec : outer_records) {
                for (const auto& inner_rec : inner_records) {
                    try {
                        int_t outer_key, inner_key;

                        if (part_is_outer) {
                            // PART x PARTSUPP
                            PartRecord part = PartRecord::fromRecord(outer_rec);
                            PartSuppRecord partsupp = PartSuppRecord::fromRecord(inner_rec);
                            outer_key = part.partkey;
                            inner_key = partsupp.partkey;

                            // 조인 조건 확인
                            if (outer_key == inner_key) {
                                JoinResultRecord result;
                                result.part = part;
                                result.partsupp = partsupp;

                                Record result_rec = result.toRecord();

                                // 출력 블록에 쓰기
                                if (!output_writer.writeRecord(result_rec)) {
                                    // 블록이 가득 차면 디스크에 쓰기
                                    writer.writeBlock(&output_block);
                                    output_block.clear();

                                    if (!output_writer.writeRecord(result_rec)) {
                                        throw std::runtime_error("Result record too large");
                                    }
                                }

                                stats.output_records++;
                            }
                        } else {
                            // PARTSUPP x PART
                            PartSuppRecord partsupp = PartSuppRecord::fromRecord(outer_rec);
                            PartRecord part = PartRecord::fromRecord(inner_rec);
                            outer_key = partsupp.partkey;
                            inner_key = part.partkey;

                            // 조인 조건 확인
                            if (outer_key == inner_key) {
                                JoinResultRecord result;
                                result.part = part;
                                result.partsupp = partsupp;

                                Record result_rec = result.toRecord();

                                // 출력 블록에 쓰기
                                if (!output_writer.writeRecord(result_rec)) {
                                    // 블록이 가득 차면 디스크에 쓰기
                                    writer.writeBlock(&output_block);
                                    output_block.clear();

                                    if (!output_writer.writeRecord(result_rec)) {
                                        throw std::runtime_error("Result record too large");
                                    }
                                }

                                stats.output_records++;
                            }
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error during join: " << e.what() << std::endl;
                    }
                }
            }

            inner_block->clear();
        }
    }

    // 마지막 출력 블록 쓰기
    if (!output_block.isEmpty()) {
        writer.writeBlock(&output_block);
    }
}
