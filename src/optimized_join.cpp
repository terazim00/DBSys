#include "optimized_join.h"
#include <iostream>
#include <chrono>
#include <algorithm>

// ============================================================================
// 1. 해시 조인 구현
// ============================================================================

HashJoin::HashJoin(
    const std::string& build_file,
    const std::string& probe_file,
    const std::string& out_file,
    const std::string& build_type,
    const std::string& probe_type,
    size_t blk_size)
    : build_table_file(build_file),
      probe_table_file(probe_file),
      output_file(out_file),
      build_table_type(build_type),
      probe_table_type(probe_type),
      block_size(blk_size) {
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

            if (build_table_type == "PART") {
                PartRecord part = PartRecord::fromRecord(record);
                hash_table[part.partkey].push_back(part);
                records_loaded++;
            }
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
            Record record = rec_reader.readNext();
            probed_records++;

            if (probe_table_type == "PARTSUPP") {
                PartSuppRecord partsupp = PartSuppRecord::fromRecord(record);

                // 해시 테이블에서 매칭되는 PART 레코드 찾기
                auto it = hash_table.find(partsupp.partkey);

                if (it != hash_table.end()) {
                    // 매칭되는 모든 PART 레코드와 조인
                    for (const auto& part : it->second) {
                        JoinResultRecord result;
                        result.part = part;
                        result.partsupp = partsupp;

                        Record result_rec = result.toRecord();

                        if (!output_writer.writeRecord(result_rec)) {
                            writer.writeBlock(&output_block);
                            output_block.clear();

                            if (!output_writer.writeRecord(result_rec)) {
                                throw std::runtime_error("Result record too large");
                            }
                        }

                        stats.output_records++;
                    }
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

    // Build Phase
    buildHashTable();

    // Probe Phase
    TableWriter writer(output_file, &stats);
    probeAndJoin(writer);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    stats.elapsed_time = elapsed.count();

    // 메모리 사용량 (해시 테이블 + 블록)
    stats.memory_usage = hash_table.size() * sizeof(std::pair<int_t, std::vector<PartRecord>>)
                        + 2 * block_size;

    std::cout << "\n=== Hash Join Statistics ===" << std::endl;
    std::cout << "Block Reads: " << stats.block_reads << std::endl;
    std::cout << "Block Writes: " << stats.block_writes << std::endl;
    std::cout << "Output Records: " << stats.output_records << std::endl;
    std::cout << "Elapsed Time: " << stats.elapsed_time << " seconds" << std::endl;
    std::cout << "Memory Usage: " << (stats.memory_usage / 1024.0 / 1024.0) << " MB" << std::endl;
}

// ============================================================================
// 2. 멀티스레드 BNLJ 구현 (간단한 버전)
// ============================================================================

MultithreadedJoin::MultithreadedJoin(
    const std::string& outer_file,
    const std::string& inner_file,
    const std::string& out_file,
    const std::string& outer_type,
    const std::string& inner_type,
    size_t buf_size,
    size_t blk_size,
    size_t threads)
    : outer_table_file(outer_file),
      inner_table_file(inner_file),
      output_file(out_file),
      outer_table_type(outer_type),
      inner_table_type(inner_type),
      buffer_size(buf_size),
      block_size(blk_size),
      num_threads(threads),
      done_reading(false) {
}

void MultithreadedJoin::execute() {
    std::cout << "Multithreaded join not fully implemented yet." << std::endl;
    std::cout << "This requires more complex synchronization." << std::endl;

    // 현재는 단일 스레드 BNLJ로 fallback
    BlockNestedLoopsJoin join(outer_table_file, inner_table_file, output_file,
                             outer_table_type, inner_table_type,
                             buffer_size, block_size);
    join.execute();
    stats = join.getStatistics();
}

// ============================================================================
// 3. 프리페칭 BNLJ 구현 (간단한 버전)
// ============================================================================

PrefetchingJoin::PrefetchingJoin(
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
      block_size(blk_size),
      prefetch_ready(false) {
}

void PrefetchingJoin::execute() {
    std::cout << "Prefetching join not fully implemented yet." << std::endl;
    std::cout << "This requires asynchronous I/O support." << std::endl;

    // 현재는 단일 스레드 BNLJ로 fallback
    BlockNestedLoopsJoin join(outer_table_file, inner_table_file, output_file,
                             outer_table_type, inner_table_type,
                             buffer_size, block_size);
    join.execute();
    stats = join.getStatistics();
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
    size_t buffer_size) {

    std::cout << "\n=== Testing Block Nested Loops Join ===" << std::endl;

    BlockNestedLoopsJoin join(outer_file, inner_file, output_file,
                             "PART", "PARTSUPP", buffer_size, 4096);
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
    const std::string& output_file) {

    std::cout << "\n=== Testing Hash Join ===" << std::endl;

    HashJoin join(build_file, probe_file, output_file,
                 "PART", "PARTSUPP", 4096);
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

PerformanceResult PerformanceTester::testMultithreaded(
    const std::string& outer_file,
    const std::string& inner_file,
    const std::string& output_file,
    size_t buffer_size,
    size_t num_threads) {

    std::cout << "\n=== Testing Multithreaded Join ===" << std::endl;

    MultithreadedJoin join(outer_file, inner_file, output_file,
                          "PART", "PARTSUPP",
                          buffer_size, 4096, num_threads);
    join.execute();

    const Statistics& stats = join.getStatistics();

    PerformanceResult result;
    result.algorithm_name = "Multithreaded (threads=" + std::to_string(num_threads) + ")";
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
    const std::string& output_dir) {

    std::cout << "\n========================================" << std::endl;
    std::cout << "  Performance Comparison" << std::endl;
    std::cout << "========================================" << std::endl;

    std::vector<PerformanceResult> results;

    // 1. Block Nested Loops (다양한 버퍼 크기)
    for (size_t buf_size : {5, 10, 20}) {
        try {
            auto result = testBlockNestedLoops(
                outer_file, inner_file,
                output_dir + "/bnlj_buf" + std::to_string(buf_size) + ".dat",
                buf_size);
            results.push_back(result);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    // 2. Hash Join
    try {
        auto result = testHashJoin(
            outer_file, inner_file,
            output_dir + "/hash_join.dat");
        results.push_back(result);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
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
