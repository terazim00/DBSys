#include "optimized_join.h"
#include <iostream>
#include <chrono>
#include <algorithm>

// ============================================================================
// Hash Join 구현 (일반화 버전)
// ============================================================================

HashJoin::HashJoin(
    const std::string& build_file,
    const std::string& probe_file,
    const std::string& out_file,
    const std::string& build_type,
    const std::string& probe_type,
    const std::string& join_key_name,
    size_t blk_size)
    : build_table_file(build_file),
      probe_table_file(probe_file),
      output_file(out_file),
      build_table_type(build_type),
      probe_table_type(probe_type),
      join_key(join_key_name),
      block_size(blk_size) {
}

int_t HashJoin::getJoinKeyValue(const Record& rec, const std::string& table_type) {
    if (table_type == "PART") {
        PartRecord part = PartRecord::fromRecord(rec);
        if (join_key == "partkey") {
            return part.partkey;
        }
    } else if (table_type == "PARTSUPP") {
        PartSuppRecord partsupp = PartSuppRecord::fromRecord(rec);
        if (join_key == "partkey") {
            return partsupp.partkey;
        } else if (join_key == "suppkey") {
            return partsupp.suppkey;
        }
    } else if (table_type == "SUPPLIER") {
        SupplierRecord supplier = SupplierRecord::fromRecord(rec);
        if (join_key == "suppkey") {
            return supplier.suppkey;
        } else if (join_key == "nationkey") {
            return supplier.nationkey;
        }
    } else if (table_type == "CUSTOMER") {
        CustomerRecord customer = CustomerRecord::fromRecord(rec);
        if (join_key == "custkey") {
            return customer.custkey;
        } else if (join_key == "nationkey") {
            return customer.nationkey;
        }
    } else if (table_type == "ORDERS") {
        OrdersRecord orders = OrdersRecord::fromRecord(rec);
        if (join_key == "orderkey") {
            return orders.orderkey;
        } else if (join_key == "custkey") {
            return orders.custkey;
        }
    } else if (table_type == "LINEITEM") {
        LineItemRecord lineitem = LineItemRecord::fromRecord(rec);
        if (join_key == "orderkey") {
            return lineitem.orderkey;
        } else if (join_key == "partkey") {
            return lineitem.partkey;
        } else if (join_key == "suppkey") {
            return lineitem.suppkey;
        }
    } else if (table_type == "NATION") {
        NationRecord nation = NationRecord::fromRecord(rec);
        if (join_key == "nationkey") {
            return nation.nationkey;
        } else if (join_key == "regionkey") {
            return nation.regionkey;
        }
    } else if (table_type == "REGION") {
        RegionRecord region = RegionRecord::fromRecord(rec);
        if (join_key == "regionkey") {
            return region.regionkey;
        }
    }

    throw std::runtime_error("Invalid join key '" + join_key + "' for table type '" + table_type + "'");
}

void HashJoin::buildHashTable() {
    std::cout << "Building hash table from " << build_table_file << "..." << std::endl;

    TableReader reader(build_table_file, block_size, &stats);
    Block block(block_size);

    size_t records_loaded = 0;

    // Build 테이블의 모든 레코드를 읽어 해시 테이블 구축
    while (reader.readBlock(&block)) {
        RecordReader rec_reader(&block);

        while (rec_reader.hasNext()) {
            Record record = rec_reader.readNext();

            // 조인 키 값 추출
            int_t key = getJoinKeyValue(record, build_table_type);

            // 해시 테이블에 레코드 추가
            hash_table[key].push_back(record);
            records_loaded++;
        }

        block.clear();
    }

    std::cout << "Hash table built: " << records_loaded << " records, "
              << hash_table.size() << " unique keys" << std::endl;
}

void HashJoin::probeAndJoin(TableWriter& writer) {
    std::cout << "Probing " << probe_table_file << "..." << std::endl;

    TableReader reader(probe_table_file, block_size, &stats);
    Block input_block(block_size);
    Block output_block(block_size);
    RecordWriter output_writer(&output_block);

    size_t probed_records = 0;

    // Probe 테이블을 스캔하며 해시 테이블에서 매칭
    while (reader.readBlock(&input_block)) {
        RecordReader rec_reader(&input_block);

        while (rec_reader.hasNext()) {
            Record probe_record = rec_reader.readNext();
            probed_records++;

            // Probe 레코드의 조인 키 값 추출
            int_t probe_key = getJoinKeyValue(probe_record, probe_table_type);

            // 해시 테이블에서 매칭되는 레코드 찾기
            auto it = hash_table.find(probe_key);

            if (it != hash_table.end()) {
                // 매칭되는 모든 Build 레코드와 조인
                for (const auto& build_record : it->second) {
                    // 레코드 병합
                    Record result;

                    // Build 레코드 필드 추가
                    for (size_t i = 0; i < build_record.getFieldCount(); ++i) {
                        result.addField(build_record.getField(i));
                    }

                    // Probe 레코드 필드 추가
                    for (size_t i = 0; i < probe_record.getFieldCount(); ++i) {
                        result.addField(probe_record.getField(i));
                    }

                    // 결과 쓰기
                    if (!output_writer.writeRecord(result)) {
                        writer.writeBlock(&output_block);
                        output_block.clear();

                        if (!output_writer.writeRecord(result)) {
                            throw std::runtime_error("Result record too large for block");
                        }
                    }

                    stats.output_records++;
                }
            }
        }

        input_block.clear();
    }

    // 마지막 출력 블록 플러시
    if (!output_block.isEmpty()) {
        writer.writeBlock(&output_block);
    }

    std::cout << "Probed " << probed_records << " records" << std::endl;
}

void HashJoin::execute() {
    auto start_time = std::chrono::high_resolution_clock::now();

    std::cout << "\n=== Hash Join Execution ===" << std::endl;
    std::cout << "Build Table: " << build_table_file << " (" << build_table_type << ")" << std::endl;
    std::cout << "Probe Table: " << probe_table_file << " (" << probe_table_type << ")" << std::endl;
    std::cout << "Join Key: " << join_key << std::endl;
    std::cout << "Output: " << output_file << std::endl;

    // Build Phase
    buildHashTable();

    // Probe Phase
    TableWriter writer(output_file, &stats);
    probeAndJoin(writer);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    stats.elapsed_time = elapsed.count();

    // 메모리 사용량 추정 (해시 테이블 + 블록)
    size_t hash_memory = 0;
    for (const auto& pair : hash_table) {
        hash_memory += sizeof(int_t);  // 키
        hash_memory += pair.second.size() * 100;  // 대략적인 레코드 크기
    }
    stats.memory_usage = hash_memory + 2 * block_size;

    std::cout << "\n=== Hash Join Statistics ===" << std::endl;
    std::cout << "Block Reads: " << stats.block_reads << std::endl;
    std::cout << "Block Writes: " << stats.block_writes << std::endl;
    std::cout << "Output Records: " << stats.output_records << std::endl;
    std::cout << "Elapsed Time: " << stats.elapsed_time << " seconds" << std::endl;
    std::cout << "Memory Usage: " << (stats.memory_usage / 1024.0 / 1024.0) << " MB" << std::endl;
    std::cout << "Hash Table Size: " << hash_table.size() << " keys" << std::endl;
}

// ============================================================================
// 성능 비교 유틸리티
// ============================================================================

void PerformanceResult::print() const {
    std::cout << "\n--- " << algorithm_name << " ---" << std::endl;
    std::cout << "Elapsed Time:   " << elapsed_time << " seconds" << std::endl;
    std::cout << "Block Reads:    " << block_reads << std::endl;
    std::cout << "Block Writes:   " << block_writes << std::endl;
    std::cout << "Output Records: " << output_records << std::endl;
    std::cout << "Memory Usage:   " << (memory_usage / 1024.0 / 1024.0) << " MB" << std::endl;
}

double PerformanceResult::getSpeedup(const PerformanceResult& baseline) const {
    if (elapsed_time > 0) {
        return baseline.elapsed_time / elapsed_time;
    }
    return 1.0;
}

PerformanceResult PerformanceTester::testBlockNestedLoops(
    const std::string& outer_file,
    const std::string& inner_file,
    const std::string& output_file,
    const std::string& outer_type,
    const std::string& inner_type,
    const std::string& join_key,
    size_t buffer_size) {

    std::cout << "\n=== Testing Block Nested Loops Join ===" << std::endl;

    BlockNestedLoopsJoin join(outer_file, inner_file, output_file,
                             outer_type, inner_type, join_key, buffer_size, 4096);
    join.execute();

    const Statistics& stats = join.getStatistics();

    PerformanceResult result;
    result.algorithm_name = "Block Nested Loops (buf=" + std::to_string(buffer_size) + ")";
    result.elapsed_time = stats.elapsed_time;
    result.block_reads = stats.block_reads;
    result.block_writes = stats.block_writes;
    result.output_records = stats.output_records;
    result.memory_usage = stats.memory_usage;

    return result;
}

PerformanceResult PerformanceTester::testHashJoin(
    const std::string& build_file,
    const std::string& probe_file,
    const std::string& output_file,
    const std::string& build_type,
    const std::string& probe_type,
    const std::string& join_key) {

    std::cout << "\n=== Testing Hash Join ===" << std::endl;

    HashJoin join(build_file, probe_file, output_file,
                 build_type, probe_type, join_key, 4096);
    join.execute();

    const Statistics& stats = join.getStatistics();

    PerformanceResult result;
    result.algorithm_name = "Hash Join";
    result.elapsed_time = stats.elapsed_time;
    result.block_reads = stats.block_reads;
    result.block_writes = stats.block_writes;
    result.output_records = stats.output_records;
    result.memory_usage = stats.memory_usage;

    return result;
}

void PerformanceTester::compareAll(
    const std::string& outer_file,
    const std::string& inner_file,
    const std::string& output_dir,
    const std::string& outer_type,
    const std::string& inner_type,
    const std::string& join_key) {

    std::cout << "\n========================================" << std::endl;
    std::cout << "  Performance Comparison" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Tables: " << outer_type << " ⋈ " << inner_type << std::endl;
    std::cout << "Join Key: " << join_key << std::endl;

    std::vector<PerformanceResult> results;

    // 1. Block Nested Loops (다양한 버퍼 크기)
    for (size_t buf_size : {5, 10, 20, 50}) {
        try {
            auto result = testBlockNestedLoops(
                outer_file, inner_file,
                output_dir + "/bnlj_buf" + std::to_string(buf_size) + ".dat",
                outer_type, inner_type, join_key,
                buf_size);
            results.push_back(result);
        } catch (const std::exception& e) {
            std::cerr << "Error in BNLJ (buf=" << buf_size << "): " << e.what() << std::endl;
        }
    }

    // 2. Hash Join
    try {
        // 작은 테이블을 Build로 선택
        auto result = testHashJoin(
            outer_file, inner_file,
            output_dir + "/hash_join.dat",
            outer_type, inner_type, join_key);
        results.push_back(result);
    } catch (const std::exception& e) {
        std::cerr << "Error in Hash Join: " << e.what() << std::endl;
    }

    // 결과 출력
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Summary" << std::endl;
    std::cout << "========================================" << std::endl;

    for (const auto& result : results) {
        result.print();
    }

    // 기준 대비 성능 향상
    if (results.size() > 1) {
        std::cout << "\n=== Speedup Comparison ===" << std::endl;
        const auto& baseline = results[0];

        for (size_t i = 1; i < results.size(); ++i) {
            double speedup = results[i].getSpeedup(baseline);
            std::cout << results[i].algorithm_name << " vs " << baseline.algorithm_name
                      << ": " << speedup << "x speedup" << std::endl;
        }
    }
}
