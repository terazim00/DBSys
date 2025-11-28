#include "buffer.h"
#include <stdexcept>

BufferManager::BufferManager(size_t num_buffers, size_t blk_size)
    : buffer_count(num_buffers), block_size(blk_size) {

    if (buffer_count == 0) {
        throw std::runtime_error("Buffer count must be at least 1");
    }

    // 버퍼 할당
    for (size_t i = 0; i < buffer_count; ++i) {
        buffers.push_back(std::make_unique<Block>(block_size));
    }
}

Block* BufferManager::getBuffer(size_t idx) {
    if (idx >= buffer_count) {
        throw std::out_of_range("Buffer index out of range");
    }
    return buffers[idx].get();
}

const Block* BufferManager::getBuffer(size_t idx) const {
    if (idx >= buffer_count) {
        throw std::out_of_range("Buffer index out of range");
    }
    return buffers[idx].get();
}

void BufferManager::clearAll() {
    for (auto& buffer : buffers) {
        buffer->clear();
    }
}
