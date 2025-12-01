#include "block.h"
#include <cstring>
#include <stdexcept>

Block::Block(size_t size) : block_size(size), used_size(0) {
    data = new char[block_size];
    std::memset(data, 0, block_size);
}

Block::~Block() {
    delete[] data;
}

Block::Block(Block&& other) noexcept
    : data(other.data), block_size(other.block_size), used_size(other.used_size) {
    other.data = nullptr;
    other.block_size = 0;
    other.used_size = 0;
}

Block& Block::operator=(Block&& other) noexcept {
    if (this != &other) {
        delete[] data;
        data = other.data;
        block_size = other.block_size;
        used_size = other.used_size;
        other.data = nullptr;
        other.block_size = 0;
        other.used_size = 0;
    }
    return *this;
}

bool Block::append(const char* record_data, size_t record_size) {
    // 크기 체크 (4 bytes for size + record data)
    size_t required_size = sizeof(uint32_t) + record_size;

    if (isFull(required_size)) {
        return false;
    }

    // 레코드 크기 저장
    uint32_t size_val = static_cast<uint32_t>(record_size);
    std::memcpy(data + used_size, &size_val, sizeof(uint32_t));
    used_size += sizeof(uint32_t);

    // 레코드 데이터 저장
    std::memcpy(data + used_size, record_data, record_size);
    used_size += record_size;

    return true;
}

void Block::clear() {
    used_size = 0;
    std::memset(data, 0, block_size);
}
