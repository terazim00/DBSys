#ifndef OPTIMIZED_JOIN_H
#define OPTIMIZED_JOIN_H

#include "common.h"
#include "table.h"
#include "buffer.h"
#include "join.h"
#include <string>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

/**
 * ============================================================================
 * 최적화된 조인 구현들
 * ============================================================================
 */

// ============================================================================
// 1. 해시 조인 (Hash Join)
// ============================================================================
/**
 * 해시 조인 구현
 *
 * 알고리즘:
 * 1. Build Phase: 작은 테이블(PART)을 해시 테이블에 로드
 * 2. Probe Phase: 큰 테이블(PARTSUPP)을 스캔하며 매칭
 *
 * 시간 복잡도: O(|R| + |S|) - 이상적인 경우
 * I/O 복잡도: |R| + |S| (각 테이블을 한 번씩만 스캔)
 * 메모리 요구: PART 테이블 전체를 메모리에 로드
 *
 * 장점:
 * - Block Nested Loops보다 훨씬 빠름 (특히 큰 데이터셋)
 * - Inner 테이블 반복 스캔 불필요
 *
 * 단점:
 * - 작은 테이블이 메모리에 들어가야 함
 * - 해시 테이블 구축 오버헤드
 */
class HashJoin {
private:
    std::string build_table_file;   // 작은 테이블 (메모리에 로드)
    std::string probe_table_file;   // 큰 테이블 (스캔)
    std::string output_file;
    std::string build_table_type;
    std::string probe_table_type;
    size_t block_size;
    Statistics stats;

    // 해시 테이블: PARTKEY → PartRecord 리스트
    std::unordered_map<int_t, std::vector<PartRecord>> hash_table;

    void buildHashTable();
    void probeAndJoin(TableWriter& writer);

public:
    HashJoin(const std::string& build_file,
             const std::string& probe_file,
             const std::string& out_file,
             const std::string& build_type,
             const std::string& probe_type,
             size_t blk_size = DEFAULT_BLOCK_SIZE);

    void execute();
    const Statistics& getStatistics() const { return stats; }
};

// ============================================================================
// 2. 멀티스레드 Block Nested Loops Join
// ============================================================================
/**
 * 멀티스레드 BNLJ 구현
 *
 * 최적화 전략:
 * - Producer-Consumer 패턴
 * - I/O 스레드: 블록 읽기/쓰기
 * - Worker 스레드: 조인 연산
 *
 * 성능 개선: 1.5x - 2x (듀얼 코어 기준)
 */
class MultithreadedJoin {
private:
    std::string outer_table_file;
    std::string inner_table_file;
    std::string output_file;
    std::string outer_table_type;
    std::string inner_table_type;
    size_t buffer_size;
    size_t block_size;
    size_t num_threads;
    Statistics stats;

    // 스레드 동기화
    std::mutex queue_mutex;
    std::condition_variable cv_producer;
    std::condition_variable cv_consumer;
    std::queue<std::vector<Record>> work_queue;
    bool done_reading;

    void readerThread(TableReader& reader);
    void workerThread(TableReader& inner_reader,
                     TableWriter& writer,
                     int thread_id);

public:
    MultithreadedJoin(const std::string& outer_file,
                      const std::string& inner_file,
                      const std::string& out_file,
                      const std::string& outer_type,
                      const std::string& inner_type,
                      size_t buf_size = 10,
                      size_t blk_size = DEFAULT_BLOCK_SIZE,
                      size_t threads = 2);

    void execute();
    const Statistics& getStatistics() const { return stats; }
};

// ============================================================================
// 3. 프리페칭 최적화 BNLJ
// ============================================================================
/**
 * 프리페칭을 적용한 BNLJ
 *
 * 최적화:
 * - 다음 블록을 미리 읽어둠 (비동기 I/O)
 * - CPU가 조인 연산하는 동안 I/O 병렬 수행
 *
 * 성능 개선: 1.2x - 1.5x
 */
class PrefetchingJoin {
private:
    std::string outer_table_file;
    std::string inner_table_file;
    std::string output_file;
    std::string outer_table_type;
    std::string inner_table_type;
    size_t buffer_size;
    size_t block_size;
    Statistics stats;

    // 프리페칭 버퍼
    std::unique_ptr<Block> prefetch_buffer;
    std::thread prefetch_thread;
    std::mutex prefetch_mutex;
    bool prefetch_ready;

    void prefetchNextBlock(TableReader& reader);

public:
    PrefetchingJoin(const std::string& outer_file,
                    const std::string& inner_file,
                    const std::string& out_file,
                    const std::string& outer_type,
                    const std::string& inner_type,
                    size_t buf_size = 10,
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
        size_t buffer_size);

    static PerformanceResult testHashJoin(
        const std::string& build_file,
        const std::string& probe_file,
        const std::string& output_file);

    static PerformanceResult testMultithreaded(
        const std::string& outer_file,
        const std::string& inner_file,
        const std::string& output_file,
        size_t buffer_size,
        size_t num_threads);

    static void compareAll(
        const std::string& outer_file,
        const std::string& inner_file,
        const std::string& output_dir);
};

#endif // OPTIMIZED_JOIN_H
