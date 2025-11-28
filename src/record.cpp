#include "record.h"
#include <cstring>
#include <stdexcept>

std::vector<char> Record::serialize() const {
    std::vector<char> buffer;

    // 각 필드를 직렬화
    for (const auto& field : fields) {
        // 필드 길이 (2 bytes)
        uint16_t field_len = static_cast<uint16_t>(field.size());
        buffer.insert(buffer.end(),
                      reinterpret_cast<const char*>(&field_len),
                      reinterpret_cast<const char*>(&field_len) + sizeof(uint16_t));

        // 필드 데이터
        buffer.insert(buffer.end(), field.begin(), field.end());
    }

    return buffer;
}

Record Record::deserialize(const char* data, size_t& offset) {
    Record record;
    size_t pos = offset;

    // 먼저 레코드 크기 읽기
    uint32_t record_size;
    std::memcpy(&record_size, data + pos, sizeof(uint32_t));
    pos += sizeof(uint32_t);

    size_t end_pos = pos + record_size;

    // 필드들 읽기
    while (pos < end_pos) {
        // 필드 길이 읽기
        uint16_t field_len;
        std::memcpy(&field_len, data + pos, sizeof(uint16_t));
        pos += sizeof(uint16_t);

        // 필드 데이터 읽기
        std::string field(data + pos, field_len);
        record.addField(field);
        pos += field_len;
    }

    offset = pos;
    return record;
}

size_t Record::getSerializedSize() const {
    size_t size = 0;
    for (const auto& field : fields) {
        size += sizeof(uint16_t) + field.size();
    }
    return size;
}

bool RecordReader::hasNext() const {
    // Need at least 4 bytes for record size
    if (current_offset + sizeof(uint32_t) > block->getUsedSize()) {
        return false;
    }

    // Check if we can read the full record
    const char* data = block->getData();
    uint32_t record_size;
    std::memcpy(&record_size, data + current_offset, sizeof(uint32_t));

    // Sanity check: record size should be reasonable (less than block size)
    if (record_size == 0 || record_size > block->getSize()) {
        return false;
    }

    // Check if we have enough bytes for the complete record
    size_t total_needed = current_offset + sizeof(uint32_t) + record_size;
    return total_needed <= block->getUsedSize();
}

Record RecordReader::readNext() {
    if (!hasNext()) {
        throw std::runtime_error("No more records in block");
    }

    const char* data = block->getData();
    Record record = Record::deserialize(data, current_offset);

    return record;
}

bool RecordWriter::writeRecord(const Record& record) {
    std::vector<char> serialized = record.serialize();
    return block->append(serialized.data(), serialized.size());
}
