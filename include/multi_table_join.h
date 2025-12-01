#ifndef MULTI_TABLE_JOIN_H
#define MULTI_TABLE_JOIN_H

#include "common.h"
#include "record.h"
#include "table.h"
#include "buffer.h"
#include <string>
#include <vector>
#include <unordered_map>

// 조인 조건 구조체
struct JoinCondition {
    size_t left_table_idx;      // 왼쪽 테이블 인덱스
    std::string left_field;     // 왼쪽 필드 이름
    size_t right_table_idx;     // 오른쪽 테이블 인덱스
    std::string right_field;    // 오른쪽 필드 이름

    JoinCondition(size_t left_idx, const std::string& left_f,
                  size_t right_idx, const std::string& right_f)
        : left_table_idx(left_idx), left_field(left_f),
          right_table_idx(right_idx), right_field(right_f) {}
};

// 테이블 정보 구조체
struct TableInfo {
    std::string filename;       // 파일 경로
    std::string table_type;     // 테이블 타입 (PART, PARTSUPP 등)
    std::vector<std::string> field_names;  // 필드 이름 리스트

    TableInfo(const std::string& file, const std::string& type)
        : filename(file), table_type(type) {
        // 테이블 타입에 따라 필드 이름 초기화
        initFieldNames();
    }

    void initFieldNames();
    int getFieldIndex(const std::string& field_name) const;
};

// 다중 테이블 조인 결과 클래스
class MultiJoinResult {
private:
    std::vector<Record> table_records;     // 각 테이블의 레코드
    std::vector<std::string> table_types;  // 테이블 타입들

public:
    MultiJoinResult() {}

    void addRecord(const Record& rec, const std::string& table_type) {
        table_records.push_back(rec);
        table_types.push_back(table_type);
    }

    // 전체 결과를 하나의 Record로 병합
    Record toRecord() const;

    // 특정 테이블의 레코드 가져오기
    const Record& getTableRecord(size_t idx) const { return table_records[idx]; }

    size_t getTableCount() const { return table_records.size(); }
};

// 다중 테이블 조인 실행자 (Left-Deep Join Plan 사용)
class MultiTableJoin {
private:
    std::vector<TableInfo> tables;         // 조인할 테이블들
    std::vector<JoinCondition> conditions; // 조인 조건들
    std::string output_file;               // 출력 파일
    size_t buffer_size;                    // 버퍼 크기 (블록 개수)
    size_t block_size;                     // 블록 크기 (바이트)
    Statistics stats;

    // 조인 수행 헬퍼 함수들
    void performJoin();

    // 두 테이블 조인 (Block Nested Loops Join)
    void joinTwoTables(const std::string& outer_file,
                       const std::string& inner_file,
                       const std::string& temp_output,
                       const TableInfo& outer_info,
                       const TableInfo& inner_info,
                       const JoinCondition& condition);

    // 레코드가 조인 조건을 만족하는지 확인
    bool matchesCondition(const Record& left_rec, const Record& right_rec,
                         const TableInfo& left_info, const TableInfo& right_info,
                         const JoinCondition& condition) const;

    // 두 레코드를 병합
    Record mergeRecords(const Record& left, const Record& right) const;

public:
    MultiTableJoin(size_t buf_size = 10, size_t blk_size = DEFAULT_BLOCK_SIZE);

    // 테이블 추가
    void addTable(const std::string& filename, const std::string& table_type);

    // 조인 조건 추가
    void addJoinCondition(size_t left_idx, const std::string& left_field,
                         size_t right_idx, const std::string& right_field);

    // 출력 파일 설정
    void setOutputFile(const std::string& output);

    // 조인 실행
    void execute();

    // 통계 정보 가져오기
    const Statistics& getStatistics() const { return stats; }

    // 조인 플랜 출력 (디버깅용)
    void printJoinPlan() const;
};

#endif // MULTI_TABLE_JOIN_H
