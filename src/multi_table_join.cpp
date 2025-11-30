#include "multi_table_join.h"
#include "block.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <sstream>

// TableInfo 구현
void TableInfo::initFieldNames() {
    if (table_type == "PART") {
        field_names = {"partkey", "name", "mfgr", "brand", "type",
                      "size", "container", "retailprice", "comment"};
    } else if (table_type == "PARTSUPP") {
        field_names = {"partkey", "suppkey", "availqty", "supplycost", "comment"};
    } else if (table_type == "SUPPLIER") {
        field_names = {"suppkey", "name", "address", "nationkey", "phone", "acctbal", "comment"};
    }
    // 추가 테이블 타입은 여기에 확장
}

int TableInfo::getFieldIndex(const std::string& field_name) const {
    for (size_t i = 0; i < field_names.size(); ++i) {
        if (field_names[i] == field_name) {
            return static_cast<int>(i);
        }
    }
    return -1;  // 필드를 찾지 못함
}

// MultiJoinResult 구현
Record MultiJoinResult::toRecord() const {
    Record result;
    // 모든 테이블의 필드를 순서대로 병합
    for (const auto& rec : table_records) {
        for (size_t i = 0; i < rec.getFieldCount(); ++i) {
            result.addField(rec.getField(i));
        }
    }
    return result;
}

// MultiTableJoin 구현
MultiTableJoin::MultiTableJoin(size_t buf_size, size_t blk_size)
    : buffer_size(buf_size), block_size(blk_size) {
}

void MultiTableJoin::addTable(const std::string& filename, const std::string& table_type) {
    tables.emplace_back(filename, table_type);
}

void MultiTableJoin::addJoinCondition(size_t left_idx, const std::string& left_field,
                                      size_t right_idx, const std::string& right_field) {
    conditions.emplace_back(left_idx, left_field, right_idx, right_field);
}

void MultiTableJoin::setOutputFile(const std::string& output) {
    output_file = output;
}

void MultiTableJoin::printJoinPlan() const {
    std::cout << "\n=== Multi-Table Join Plan ===" << std::endl;
    std::cout << "Tables:" << std::endl;
    for (size_t i = 0; i < tables.size(); ++i) {
        std::cout << "  [" << i << "] " << tables[i].table_type
                  << " (" << tables[i].filename << ")" << std::endl;
    }
    std::cout << "\nJoin Conditions:" << std::endl;
    for (const auto& cond : conditions) {
        std::cout << "  T" << cond.left_table_idx << "." << cond.left_field
                  << " = T" << cond.right_table_idx << "." << cond.right_field
                  << std::endl;
    }
    std::cout << "\nJoin Strategy: Left-Deep Plan with Block Nested Loops Join" << std::endl;
    std::cout << "Buffer Size: " << buffer_size << " blocks" << std::endl;
    std::cout << "==============================\n" << std::endl;
}

bool MultiTableJoin::matchesCondition(const Record& left_rec, const Record& right_rec,
                                      const TableInfo& left_info, const TableInfo& right_info,
                                      const JoinCondition& condition) const {
    int left_idx = left_info.getFieldIndex(condition.left_field);
    int right_idx = right_info.getFieldIndex(condition.right_field);

    if (left_idx < 0 || right_idx < 0) {
        return false;  // 필드를 찾지 못함
    }

    std::string left_val = left_rec.getField(left_idx);
    std::string right_val = right_rec.getField(right_idx);

    return left_val == right_val;
}

Record MultiTableJoin::mergeRecords(const Record& left, const Record& right) const {
    Record merged;
    // 왼쪽 레코드의 모든 필드 추가
    for (size_t i = 0; i < left.getFieldCount(); ++i) {
        merged.addField(left.getField(i));
    }
    // 오른쪽 레코드의 모든 필드 추가
    for (size_t i = 0; i < right.getFieldCount(); ++i) {
        merged.addField(right.getField(i));
    }
    return merged;
}

void MultiTableJoin::joinTwoTables(const std::string& outer_file,
                                   const std::string& inner_file,
                                   const std::string& temp_output,
                                   const TableInfo& outer_info,
                                   const TableInfo& inner_info,
                                   const JoinCondition& condition) {
    // Block Nested Loops Join 알고리즘 구현
    TableReader outer_reader(outer_file, block_size, &stats);
    TableReader inner_reader(inner_file, block_size, &stats);
    TableWriter writer(temp_output, &stats);

    if (!outer_reader.isOpen() || !inner_reader.isOpen() || !writer.isOpen()) {
        std::cerr << "파일 열기 실패" << std::endl;
        return;
    }

    // 버퍼 관리자 생성
    BufferManager buffer_mgr(buffer_size, block_size);

    // 출력용 블록
    Block output_block(block_size);

    bool has_outer_blocks = true;

    while (has_outer_blocks) {
        // 외부 테이블에서 (buffer_size - 1)개의 블록을 메모리에 로드
        std::vector<Record> outer_records;
        size_t blocks_loaded = 0;

        for (size_t i = 0; i < buffer_size - 1; ++i) {
            Block* outer_block = buffer_mgr.getBuffer(i);
            outer_block->clear();

            if (!outer_reader.readBlock(outer_block)) {
                has_outer_blocks = false;
                break;
            }

            blocks_loaded++;

            // 블록에서 레코드 추출
            RecordReader reader(outer_block);
            while (reader.hasNext()) {
                Record rec = reader.readNext();
                outer_records.push_back(rec);
            }
        }

        if (outer_records.empty()) {
            break;
        }

        // 내부 테이블 전체 스캔
        inner_reader.reset();

        Block* inner_block = buffer_mgr.getBuffer(buffer_size - 1);

        while (inner_reader.readBlock(inner_block)) {
            // 내부 블록에서 레코드 추출
            std::vector<Record> inner_records;
            RecordReader reader(inner_block);
            while (reader.hasNext()) {
                Record rec = reader.readNext();
                inner_records.push_back(rec);
            }

            // 모든 외부 레코드와 내부 레코드 비교
            for (const auto& outer_rec : outer_records) {
                for (const auto& inner_rec : inner_records) {
                    // 조인 조건 확인
                    if (matchesCondition(outer_rec, inner_rec,
                                       outer_info, inner_info, condition)) {
                        // 레코드 병합
                        Record merged = mergeRecords(outer_rec, inner_rec);

                        // 출력 블록에 쓰기
                        RecordWriter record_writer(&output_block);
                        if (!record_writer.writeRecord(merged)) {
                            // 블록이 가득 찼으면 디스크에 쓰고 초기화
                            writer.writeBlock(&output_block);
                            output_block.clear();
                            record_writer.writeRecord(merged);
                        }

                        stats.output_records++;
                    }
                }
            }

            inner_block->clear();
        }
    }

    // 남은 출력 블록 플러시
    if (output_block.getUsedSize() > 0) {
        writer.writeBlock(&output_block);
    }
}

void MultiTableJoin::performJoin() {
    if (tables.size() < 2) {
        std::cerr << "최소 2개 이상의 테이블이 필요합니다." << std::endl;
        return;
    }

    if (conditions.size() != tables.size() - 1) {
        std::cerr << "조인 조건 개수가 올바르지 않습니다. "
                  << "(테이블 개수 - 1 = 조인 조건 개수)" << std::endl;
        return;
    }

    // Left-Deep Join Plan 실행
    // ((T0 ⋈ T1) ⋈ T2) ⋈ T3 ...

    std::string current_result = tables[0].filename;
    TableInfo current_info = tables[0];

    for (size_t i = 1; i < tables.size(); ++i) {
        std::string temp_output;
        if (i == tables.size() - 1) {
            // 마지막 조인 결과는 최종 출력 파일로
            temp_output = output_file;
        } else {
            // 중간 결과는 임시 파일로
            temp_output = "temp_join_" + std::to_string(i) + ".dat";
        }

        std::cout << "단계 " << i << ": " << current_info.table_type
                  << " ⋈ " << tables[i].table_type << std::endl;

        // 두 테이블 조인
        joinTwoTables(current_result, tables[i].filename, temp_output,
                     current_info, tables[i], conditions[i - 1]);

        // 임시 파일 삭제 (첫 번째 테이블이 아니고 중간 결과인 경우)
        if (i > 1 && i < tables.size()) {
            // 이전 중간 결과 파일 삭제
            std::remove(current_result.c_str());
        }

        // 다음 반복을 위해 현재 결과 업데이트
        current_result = temp_output;

        // 병합된 테이블 정보 업데이트
        TableInfo merged_info(temp_output,
                             current_info.table_type + "_" + tables[i].table_type);
        // 필드 이름 병합
        merged_info.field_names = current_info.field_names;
        merged_info.field_names.insert(merged_info.field_names.end(),
                                      tables[i].field_names.begin(),
                                      tables[i].field_names.end());
        current_info = merged_info;
    }

    std::cout << "다중 테이블 조인 완료!" << std::endl;
}

void MultiTableJoin::execute() {
    printJoinPlan();

    auto start = std::chrono::high_resolution_clock::now();

    performJoin();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    stats.elapsed_time = elapsed.count();
    stats.memory_usage = buffer_size * block_size;

    // 통계 출력
    std::cout << "\n=== Join Statistics ===" << std::endl;
    std::cout << "Block Reads: " << stats.block_reads << std::endl;
    std::cout << "Block Writes: " << stats.block_writes << std::endl;
    std::cout << "Output Records: " << stats.output_records << std::endl;
    std::cout << "Elapsed Time: " << stats.elapsed_time << " seconds" << std::endl;
    std::cout << "Memory Usage: " << stats.memory_usage
              << " bytes (" << (stats.memory_usage / 1024.0 / 1024.0) << " MB)" << std::endl;
}
