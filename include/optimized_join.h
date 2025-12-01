#ifndef OPTIMIZED_JOIN_H
#define OPTIMIZED_JOIN_H

#include "common.h"
#include "table.h"
#include "buffer.h"
#include "join.h"
#include <string>
#include <unordered_map>
#include <vector>

/**
 * ============================================================================
 * 최적화된 조인 구현
 * ============================================================================
 */

// ============================================================================
// Hash Join (일반화 버전)
// ============================================================================
/**
 * Hash Join 구현 - 모든 테이블/키 조합 지원
 *
 * 알고리즘:
 * 1. Build Phase: 작은 테이블을 해시 테이블에 로드
 * 2. Probe Phase: 큰 테이블을 스캔하며 매칭
 *
 * 시간 복잡도: O(|R| + |S|) - 이상적인 경우
 * I/O 복잡도: |R| + |S| (각 테이블을 한 번씩만 스캔)
 * 메모리 요구: Build 테이블 전체를 메모리에 로드
 *
 * 장점:
 * - Block Nested Loops보다 훨씬 빠름 (5-10배)
 * - Inner 테이블 반복 스캔 불필요
 * - I/O 90-99% 감소
 *
 * 단점:
 * - Build 테이블이 메모리에 들어가야 함
 * - 해시 테이블 구축 오버헤드
 * - Equi-join만 지원
 */
class HashJoin {
private:
    std::string build_table_file;   // Build 테이블 (메모리에 로드)
    std::string probe_table_file;   // Probe 테이블 (스캔)
    std::string output_file;
    std::string build_table_type;   // 테이블 타입 (PART, PARTSUPP, SUPPLIER)
    std::string probe_table_type;   // 테이블 타입
    std::string join_key;           // 조인 키 (partkey, suppkey 등)
    size_t block_size;
    Statistics stats;

    // 해시 테이블: JOIN_KEY → Record 리스트
    std::unordered_map<int_t, std::vector<Record>> hash_table;

    void buildHashTable();
    void probeAndJoin(TableWriter& writer);

    // 레코드에서 조인 키 값 추출
    int_t getJoinKeyValue(const Record& rec, const std::string& table_type);

public:
    HashJoin(const std::string& build_file,
             const std::string& probe_file,
             const std::string& out_file,
             const std::string& build_type,
             const std::string& probe_type,
             const std::string& join_key_name,
             size_t blk_size = DEFAULT_BLOCK_SIZE);

    void execute();
    const Statistics& getStatistics() const { return stats; }
};

// ============================================================================
// 성능 비교 유틸리티
// ============================================================================
struct PerformanceResult {
    std::string algorithm_name;
    double elapsed_time;
    size_t block_reads;
    size_t block_writes;
    size_t output_records;
    size_t memory_usage;

    void print() const;
    double getSpeedup(const PerformanceResult& baseline) const;
};

class PerformanceTester {
public:
    static PerformanceResult testBlockNestedLoops(
        const std::string& outer_file,
        const std::string& inner_file,
        const std::string& output_file,
        const std::string& outer_type,
        const std::string& inner_type,
        const std::string& join_key,
        size_t buffer_size);

    static PerformanceResult testHashJoin(
        const std::string& build_file,
        const std::string& probe_file,
        const std::string& output_file,
        const std::string& build_type,
        const std::string& probe_type,
        const std::string& join_key);

    static void compareAll(
        const std::string& outer_file,
        const std::string& inner_file,
        const std::string& output_dir,
        const std::string& outer_type,
        const std::string& inner_type,
        const std::string& join_key);
};

#endif // OPTIMIZED_JOIN_H
