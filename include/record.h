#ifndef RECORD_H
#define RECORD_H

#include "common.h"
#include "block.h"
#include <string>
#include <vector>
#include <cstring>

// 가변 길이 레코드 형식
// [record_size(4 bytes)][field1_len(2 bytes)][field1_data][field2_len][field2_data]...

class Record {
private:
    std::vector<std::string> fields;

public:
    Record() {}
    Record(const std::vector<std::string>& f) : fields(f) {}

    // 필드 추가
    void addField(const std::string& field) { fields.push_back(field); }

    // 필드 접근
    const std::string& getField(size_t idx) const { return fields[idx]; }
    size_t getFieldCount() const { return fields.size(); }
    const std::vector<std::string>& getFields() const { return fields; }

    // 레코드를 바이트 배열로 직렬화
    std::vector<char> serialize() const;

    // 바이트 배열에서 레코드 역직렬화
    static Record deserialize(const char* data, size_t& offset);

    // 레코드의 직렬화된 크기 계산
    size_t getSerializedSize() const;
};

// 레코드 리더 클래스 - 블록에서 레코드 읽기
class RecordReader {
private:
    const Block* block;
    size_t current_offset;

public:
    RecordReader(const Block* blk) : block(blk), current_offset(0) {}

    // 다음 레코드 읽기
    bool hasNext() const;
    Record readNext();

    // 리더 초기화
    void reset() { current_offset = 0; }
};

// 레코드 라이터 클래스 - 블록에 레코드 쓰기
class RecordWriter {
private:
    Block* block;

public:
    RecordWriter(Block* blk) : block(blk) {}

    // 레코드 쓰기
    bool writeRecord(const Record& record);
};

#endif // RECORD_H
