#ifndef BUFFER_H
#define BUFFER_H

#include "common.h"
#include "block.h"
#include <vector>
#include <memory>

// 버퍼 풀 관리자
class BufferManager {
private:
    std::vector<std::unique_ptr<Block>> buffers;
    size_t buffer_count;
    size_t block_size;

public:
    BufferManager(size_t num_buffers, size_t blk_size = DEFAULT_BLOCK_SIZE);

    // 버퍼 접근
    Block* getBuffer(size_t idx);
    const Block* getBuffer(size_t idx) const;

    // 버퍼 개수
    size_t getBufferCount() const { return buffer_count; }

    // 모든 버퍼 초기화
    void clearAll();

    // 메모리 사용량 계산
    size_t getMemoryUsage() const {
        return buffer_count * block_size;
    }
};

#endif // BUFFER_H
