#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// 블록 크기 (기본 4KB)
#define DEFAULT_BLOCK_SIZE 4096

// 타입 정의
typedef int32_t int_t;
typedef uint32_t uint_t;
typedef float decimal_t;

// 성능 측정을 위한 통계
struct Statistics {
    size_t block_reads;
    size_t block_writes;
    size_t output_records;
    double elapsed_time;
    size_t memory_usage;

    Statistics() : block_reads(0), block_writes(0), output_records(0),
                   elapsed_time(0.0), memory_usage(0) {}
};

#endif // COMMON_H
