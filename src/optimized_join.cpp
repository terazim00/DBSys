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
                             outer_table_type, inner_table_type, "partkey",
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
                             outer_table_type, inner_table_type, "partkey",
                             buffer_size, block_size);
    join.execute();
    stats = join.getStatistics();
}

// ============================================================================
// Sort-Merge Join 구현
// ============================================================================

SortMergeJoin::SortMergeJoin(
    const std::string& outer_file,
    const std::string& inner_file,
    const std::string& out_file,
    const std::string& outer_type,
    const std::string& inner_type,
    const std::string& join_key_name,
    size_t buf_size,
    size_t blk_size)
    : outer_table_file(outer_file),
      inner_table_file(inner_file),
      output_file(out_file),
      outer_table_type(outer_type),
      inner_table_type(inner_type),
      join_key(join_key_name),
      buffer_size(buf_size),
      block_size(blk_size) {
}

int_t SortMergeJoin::getJoinKeyValue(const Record& rec, const std::string& table_type) {
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
        }
    }
    throw std::runtime_error("Invalid join key '" + join_key + "' for table type '" + table_type + "'");
}

void SortMergeJoin::externalSort(
    const std::string& input_file,
    const std::string& output_file,
    const std::string& table_type) {

    std::cout << "Sorting " << input_file << "..." << std::endl;

    // 1단계: 메모리에 맞는 크기로 런(run) 생성 및 정렬
    TableReader reader(input_file, block_size, &stats);
    std::vector<std::string> run_files;
    size_t run_number = 0;

    while (true) {
        std::vector<std::pair<int_t, Record>> records;
        size_t records_loaded = 0;

        // 버퍼 크기만큼 레코드 읽기
        for (size_t i = 0; i < buffer_size; ++i) {
            Block block(block_size);
            if (!reader.readBlock(&block)) {
                break;
            }

            RecordReader rec_reader(&block);
            while (rec_reader.hasNext()) {
                Record rec = rec_reader.readNext();
                int_t key = getJoinKeyValue(rec, table_type);
                records.emplace_back(key, rec);
                records_loaded++;
            }
        }

        if (records.empty()) {
            break;
        }

        // 정렬
        std::sort(records.begin(), records.end(),
                 [](const auto& a, const auto& b) { return a.first < b.first; });

        // 런 파일 생성
        std::string run_file = output_file + ".run" + std::to_string(run_number++);
        run_files.push_back(run_file);

        TableWriter run_writer(run_file, &stats);
        Block output_block(block_size);
        RecordWriter output_writer(&output_block);

        for (const auto& [key, rec] : records) {
            if (!output_writer.writeRecord(rec)) {
                run_writer.writeBlock(&output_block);
                output_block.clear();
                if (!output_writer.writeRecord(rec)) {
                    throw std::runtime_error("Record too large for block");
                }
            }
        }

        if (!output_block.isEmpty()) {
            run_writer.writeBlock(&output_block);
        }
    }

    std::cout << "Created " << run_files.size() << " sorted runs" << std::endl;

    // 2단계: 런들을 병합하여 최종 정렬 파일 생성
    if (run_files.size() == 1) {
        std::rename(run_files[0].c_str(), output_file.c_str());
    } else {
        // 간단한 2-way 병합 (실제로는 K-way 병합이 더 효율적)
        // TODO: K-way 병합 구현하면 더 효율적
        std::cout << "Merging runs..." << std::endl;

        while (run_files.size() > 1) {
            std::vector<std::string> new_runs;

            for (size_t i = 0; i + 1 < run_files.size(); i += 2) {
                std::string merged_file = output_file + ".merged" + std::to_string(i / 2);
                new_runs.push_back(merged_file);

                TableReader r1(run_files[i], block_size, &stats);
                TableReader r2(run_files[i + 1], block_size, &stats);
                TableWriter writer(merged_file, &stats);

                Block b1(block_size), b2(block_size);
                RecordReader rr1(&b1), rr2(&b2);
                bool has1 = r1.readBlock(&b1);
                bool has2 = r2.readBlock(&b2);

                Record rec1, rec2;
                bool rec1_valid = has1 && rr1.hasNext();
                bool rec2_valid = has2 && rr2.hasNext();

                if (rec1_valid) rec1 = rr1.readNext();
                if (rec2_valid) rec2 = rr2.readNext();

                Block output_block(block_size);
                RecordWriter output_writer(&output_block);

                while (rec1_valid || rec2_valid) {
                    Record* selected = nullptr;

                    if (!rec1_valid) {
                        selected = &rec2;
                    } else if (!rec2_valid) {
                        selected = &rec1;
                    } else {
                        int_t key1 = getJoinKeyValue(rec1, table_type);
                        int_t key2 = getJoinKeyValue(rec2, table_type);
                        selected = (key1 <= key2) ? &rec1 : &rec2;
                    }

                    if (!output_writer.writeRecord(*selected)) {
                        writer.writeBlock(&output_block);
                        output_block.clear();
                        output_writer.writeRecord(*selected);
                    }

                    // 다음 레코드 읽기
                    if (selected == &rec1) {
                        rec1_valid = rr1.hasNext();
                        if (rec1_valid) {
                            rec1 = rr1.readNext();
                        } else if (r1.readBlock(&b1)) {
                            rr1.reset();
                            rec1_valid = rr1.hasNext();
                            if (rec1_valid) rec1 = rr1.readNext();
                        }
                    } else {
                        rec2_valid = rr2.hasNext();
                        if (rec2_valid) {
                            rec2 = rr2.readNext();
                        } else if (r2.readBlock(&b2)) {
                            rr2.reset();
                            rec2_valid = rr2.hasNext();
                            if (rec2_valid) rec2 = rr2.readNext();
                        }
                    }
                }

                if (!output_block.isEmpty()) {
                    writer.writeBlock(&output_block);
                }

                // 임시 파일 삭제
                std::remove(run_files[i].c_str());
                std::remove(run_files[i + 1].c_str());
            }

            if (run_files.size() % 2 == 1) {
                new_runs.push_back(run_files.back());
            }

            run_files = new_runs;
        }

        std::rename(run_files[0].c_str(), output_file.c_str());
    }

    std::cout << "Sorting completed: " << output_file << std::endl;
}

void SortMergeJoin::mergeJoin(
    const std::string& sorted_outer,
    const std::string& sorted_inner,
    TableWriter& writer) {

    std::cout << "Performing merge join..." << std::endl;

    TableReader outer_reader(sorted_outer, block_size, &stats);
    TableReader inner_reader(sorted_inner, block_size, &stats);

    Block outer_block(block_size), inner_block(block_size);
    RecordReader outer_rec_reader(&outer_block), inner_rec_reader(&inner_block);

    bool has_outer = outer_reader.readBlock(&outer_block);
    bool has_inner = inner_reader.readBlock(&inner_block);

    if (!has_outer || !has_inner) {
        std::cout << "One or both tables are empty" << std::endl;
        return;
    }

    Block output_block(block_size);
    RecordWriter output_writer(&output_block);

    Record outer_rec, inner_rec;
    bool outer_valid = outer_rec_reader.hasNext();
    bool inner_valid = inner_rec_reader.hasNext();

    if (outer_valid) outer_rec = outer_rec_reader.readNext();
    if (inner_valid) inner_rec = inner_rec_reader.readNext();

    while (outer_valid && inner_valid) {
        int_t outer_key = getJoinKeyValue(outer_rec, outer_table_type);
        int_t inner_key = getJoinKeyValue(inner_rec, inner_table_type);

        if (outer_key == inner_key) {
            // 조인 성공 - 같은 키를 가진 모든 레코드 처리
            std::vector<Record> matching_inner;
            int_t current_key = inner_key;

            // Inner 테이블에서 같은 키를 가진 모든 레코드 수집
            while (inner_valid && getJoinKeyValue(inner_rec, inner_table_type) == current_key) {
                matching_inner.push_back(inner_rec);

                inner_valid = inner_rec_reader.hasNext();
                if (inner_valid) {
                    inner_rec = inner_rec_reader.readNext();
                } else if (inner_reader.readBlock(&inner_block)) {
                    inner_rec_reader.reset();
                    inner_valid = inner_rec_reader.hasNext();
                    if (inner_valid) inner_rec = inner_rec_reader.readNext();
                }
            }

            // Outer 테이블의 같은 키를 가진 모든 레코드와 조인
            while (outer_valid && getJoinKeyValue(outer_rec, outer_table_type) == current_key) {
                for (const auto& inner : matching_inner) {
                    Record result;
                    for (size_t i = 0; i < outer_rec.getFieldCount(); ++i) {
                        result.addField(outer_rec.getField(i));
                    }
                    for (size_t i = 0; i < inner.getFieldCount(); ++i) {
                        result.addField(inner.getField(i));
                    }

                    if (!output_writer.writeRecord(result)) {
                        writer.writeBlock(&output_block);
                        output_block.clear();
                        output_writer.writeRecord(result);
                    }
                    stats.output_records++;
                }

                outer_valid = outer_rec_reader.hasNext();
                if (outer_valid) {
                    outer_rec = outer_rec_reader.readNext();
                } else if (outer_reader.readBlock(&outer_block)) {
                    outer_rec_reader.reset();
                    outer_valid = outer_rec_reader.hasNext();
                    if (outer_valid) outer_rec = outer_rec_reader.readNext();
                }
            }

        } else if (outer_key < inner_key) {
            // Outer 진행
            outer_valid = outer_rec_reader.hasNext();
            if (outer_valid) {
                outer_rec = outer_rec_reader.readNext();
            } else if (outer_reader.readBlock(&outer_block)) {
                outer_rec_reader.reset();
                outer_valid = outer_rec_reader.hasNext();
                if (outer_valid) outer_rec = outer_rec_reader.readNext();
            }
        } else {
            // Inner 진행
            inner_valid = inner_rec_reader.hasNext();
            if (inner_valid) {
                inner_rec = inner_rec_reader.readNext();
            } else if (inner_reader.readBlock(&inner_block)) {
                inner_rec_reader.reset();
                inner_valid = inner_rec_reader.hasNext();
                if (inner_valid) inner_rec = inner_rec_reader.readNext();
            }
        }
    }

    if (!output_block.isEmpty()) {
        writer.writeBlock(&output_block);
    }

    std::cout << "Merge join completed" << std::endl;
}

void SortMergeJoin::execute() {
    auto start_time = std::chrono::high_resolution_clock::now();

    // 1단계: 두 테이블 정렬
    std::string sorted_outer = output_file + ".sorted_outer";
    std::string sorted_inner = output_file + ".sorted_inner";

    externalSort(outer_table_file, sorted_outer, outer_table_type);
    externalSort(inner_table_file, sorted_inner, inner_table_type);

    // 2단계: 병합 조인
    TableWriter writer(output_file, &stats);
    mergeJoin(sorted_outer, sorted_inner, writer);

    // 정렬된 임시 파일 삭제
    std::remove(sorted_outer.c_str());
    std::remove(sorted_inner.c_str());

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    stats.elapsed_time = elapsed.count();
    stats.memory_usage = buffer_size * block_size;

    std::cout << "\n=== Sort-Merge Join Statistics ===" << std::endl;
    std::cout << "Block Reads: " << stats.block_reads << std::endl;
    std::cout << "Block Writes: " << stats.block_writes << std::endl;
    std::cout << "Output Records: " << stats.output_records << std::endl;
    std::cout << "Elapsed Time: " << stats.elapsed_time << " seconds" << std::endl;
    std::cout << "Memory Usage: " << (stats.memory_usage / 1024.0 / 1024.0) << " MB" << std::endl;
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
                             "PART", "PARTSUPP", "partkey", buffer_size, 4096);
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

PerformanceResult PerformanceTester::testSortMergeJoin(
    const std::string& outer_file,
    const std::string& inner_file,
    const std::string& output_file,
    size_t buffer_size) {

    std::cout << "\n=== Testing Sort-Merge Join ===" << std::endl;

    SortMergeJoin join(outer_file, inner_file, output_file,
                      "PART", "PARTSUPP", "partkey", buffer_size, 4096);
    join.execute();

    const Statistics& stats = join.getStatistics();

    PerformanceResult result;
    result.algorithm_name = "Sort-Merge Join (buf=" + std::to_string(buffer_size) + ")";
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

    // 3. Sort-Merge Join
    try {
        auto result = testSortMergeJoin(
            outer_file, inner_file,
            output_dir + "/sort_merge_join.dat",
            10);
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
