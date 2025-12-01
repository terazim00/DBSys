#ifndef JOIN_H
#define JOIN_H

#include "common.h"
#include "table.h"
#include "buffer.h"
#include <string>

// Block Nested Loops Join 실행자
class BlockNestedLoopsJoin {
private:
    std::string outer_table_file;
    std::string inner_table_file;
    std::string output_file;
    std::string outer_table_type;  // 테이블 타입 (예: "PART", "PARTSUPP", "SUPPLIER")
    std::string inner_table_type;  // 테이블 타입 (예: "PART", "PARTSUPP", "SUPPLIER")
    std::string join_key;          // 조인 키 필드명 (예: "partkey", "suppkey")
    size_t buffer_size;            // 버퍼 크기 (블록 개수)
    size_t block_size;             // 블록 크기 (바이트)
    Statistics stats;

    // 조인 수행 헬퍼 함수
    void performJoin();

    // 일반화된 조인 함수
    void joinTables(TableReader& outer_reader,
                    TableReader& inner_reader,
                    TableWriter& writer,
                    BufferManager& buffer_mgr);

    // 레코드에서 조인 키 값 추출
    int_t getJoinKeyValue(const Record& rec, const std::string& table_type);

    // 두 레코드를 병합하여 조인 결과 생성
    Record mergeRecords(const Record& outer_rec, const Record& inner_rec);

public:
    BlockNestedLoopsJoin(const std::string& outer_file,
                         const std::string& inner_file,
                         const std::string& out_file,
                         const std::string& outer_type,
                         const std::string& inner_type,
                         const std::string& join_key_name,
                         size_t buf_size = 10,
                         size_t blk_size = DEFAULT_BLOCK_SIZE);

    // 조인 실행
    void execute();

    // 통계 정보 가져오기
    const Statistics& getStatistics() const { return stats; }
};

#endif // JOIN_H
