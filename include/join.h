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
    std::string outer_table_type;  // "PART" or "PARTSUPP"
    std::string inner_table_type;  // "PART" or "PARTSUPP"
    size_t buffer_size;            // 버퍼 크기 (블록 개수)
    size_t block_size;             // 블록 크기 (바이트)
    Statistics stats;

    // 조인 수행 헬퍼 함수
    void performJoin();

    // PART와 PARTSUPP 조인
    void joinPartAndPartSupp(TableReader& outer_reader,
                             TableReader& inner_reader,
                             TableWriter& writer,
                             BufferManager& buffer_mgr,
                             bool part_is_outer);

public:
    BlockNestedLoopsJoin(const std::string& outer_file,
                         const std::string& inner_file,
                         const std::string& out_file,
                         const std::string& outer_type,
                         const std::string& inner_type,
                         size_t buf_size = 10,
                         size_t blk_size = DEFAULT_BLOCK_SIZE);

    // 조인 실행
    void execute();

    // 통계 정보 가져오기
    const Statistics& getStatistics() const { return stats; }
};

#endif // JOIN_H
