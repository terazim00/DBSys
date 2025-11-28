#ifndef BLOCK_H
#define BLOCK_H

#include "common.h"
#include <vector>
#include <cstddef>

// 고정 크기 블록 클래스
class Block {
private:
    char* data;           // 블록 데이터
    size_t block_size;    // 블록 크기
    size_t used_size;     // 사용된 크기

public:
    Block(size_t size = DEFAULT_BLOCK_SIZE);
    ~Block();

    // 복사 방지
    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;

    // 이동 생성자/대입 연산자
    Block(Block&& other) noexcept;
    Block& operator=(Block&& other) noexcept;

    // 블록에 데이터 추가
    bool append(const char* record_data, size_t record_size);

    // 블록 초기화
    void clear();

    // 블록 데이터 접근
    const char* getData() const { return data; }
    char* getData() { return data; }
    size_t getSize() const { return block_size; }
    size_t getUsedSize() const { return used_size; }
    size_t getFreeSize() const { return block_size - used_size; }
    bool isEmpty() const { return used_size == 0; }
    bool isFull(size_t required_size) const {
        return getFreeSize() < required_size;
    }

    // 사용된 크기 설정 (파일에서 읽을 때 사용)
    void setUsedSize(size_t size) { used_size = size; }
};

// 블록 관리자 클래스
class BlockManager {
private:
    size_t block_size;

public:
    BlockManager(size_t size = DEFAULT_BLOCK_SIZE) : block_size(size) {}

    // 블록 생성
    Block* createBlock() { return new Block(block_size); }

    size_t getBlockSize() const { return block_size; }
};

#endif // BLOCK_H
