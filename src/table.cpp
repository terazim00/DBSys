#include "table.h"
#include <sstream>
#include <iostream>
#include <algorithm>

// PartRecord 구현
Record PartRecord::toRecord() const {
    std::vector<std::string> fields;
    fields.push_back(std::to_string(partkey));
    fields.push_back(name);
    fields.push_back(mfgr);
    fields.push_back(brand);
    fields.push_back(type);
    fields.push_back(std::to_string(size));
    fields.push_back(container);
    fields.push_back(std::to_string(retailprice));
    fields.push_back(comment);
    return Record(fields);
}

PartRecord PartRecord::fromRecord(const Record& rec) {
    PartRecord part;
    if (rec.getFieldCount() < 9) {
        throw std::runtime_error("Invalid PART record");
    }
    part.partkey = std::stoi(rec.getField(0));
    part.name = rec.getField(1);
    part.mfgr = rec.getField(2);
    part.brand = rec.getField(3);
    part.type = rec.getField(4);
    part.size = std::stoi(rec.getField(5));
    part.container = rec.getField(6);
    part.retailprice = std::stof(rec.getField(7));
    part.comment = rec.getField(8);
    return part;
}

PartRecord PartRecord::fromCSV(const std::string& line) {
    PartRecord part;
    std::stringstream ss(line);
    std::string field;

    std::getline(ss, field, '|');
    part.partkey = std::stoi(field);

    std::getline(ss, part.name, '|');
    std::getline(ss, part.mfgr, '|');
    std::getline(ss, part.brand, '|');
    std::getline(ss, part.type, '|');

    std::getline(ss, field, '|');
    part.size = std::stoi(field);

    std::getline(ss, part.container, '|');

    std::getline(ss, field, '|');
    part.retailprice = std::stof(field);

    std::getline(ss, part.comment, '|');

    return part;
}

// PartSuppRecord 구현
Record PartSuppRecord::toRecord() const {
    std::vector<std::string> fields;
    fields.push_back(std::to_string(partkey));
    fields.push_back(std::to_string(suppkey));
    fields.push_back(std::to_string(availqty));
    fields.push_back(std::to_string(supplycost));
    fields.push_back(comment);
    return Record(fields);
}

PartSuppRecord PartSuppRecord::fromRecord(const Record& rec) {
    PartSuppRecord partsupp;
    if (rec.getFieldCount() < 5) {
        throw std::runtime_error("Invalid PARTSUPP record");
    }
    partsupp.partkey = std::stoi(rec.getField(0));
    partsupp.suppkey = std::stoi(rec.getField(1));
    partsupp.availqty = std::stoi(rec.getField(2));
    partsupp.supplycost = std::stof(rec.getField(3));
    partsupp.comment = rec.getField(4);
    return partsupp;
}

PartSuppRecord PartSuppRecord::fromCSV(const std::string& line) {
    PartSuppRecord partsupp;
    std::stringstream ss(line);
    std::string field;

    std::getline(ss, field, '|');
    partsupp.partkey = std::stoi(field);

    std::getline(ss, field, '|');
    partsupp.suppkey = std::stoi(field);

    std::getline(ss, field, '|');
    partsupp.availqty = std::stoi(field);

    std::getline(ss, field, '|');
    partsupp.supplycost = std::stof(field);

    std::getline(ss, partsupp.comment, '|');

    return partsupp;
}

// JoinResultRecord 구현
Record JoinResultRecord::toRecord() const {
    std::vector<std::string> fields;

    // PART 필드
    fields.push_back(std::to_string(part.partkey));
    fields.push_back(part.name);
    fields.push_back(part.mfgr);
    fields.push_back(part.brand);
    fields.push_back(part.type);
    fields.push_back(std::to_string(part.size));
    fields.push_back(part.container);
    fields.push_back(std::to_string(part.retailprice));
    fields.push_back(part.comment);

    // PARTSUPP 필드
    fields.push_back(std::to_string(partsupp.partkey));
    fields.push_back(std::to_string(partsupp.suppkey));
    fields.push_back(std::to_string(partsupp.availqty));
    fields.push_back(std::to_string(partsupp.supplycost));
    fields.push_back(partsupp.comment);

    return Record(fields);
}

// TableReader 구현
TableReader::TableReader(const std::string& fname, size_t blk_size, Statistics* st)
    : filename(fname), block_size(blk_size), stats(st) {
    file.open(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
}

TableReader::~TableReader() {
    if (file.is_open()) {
        file.close();
    }
}

bool TableReader::readBlock(Block* block) {
    if (!file.is_open() || file.eof()) {
        return false;
    }

    block->clear();

    // 블록 크기만큼 읽기
    file.read(block->getData(), block->getSize());
    std::streamsize bytes_read = file.gcount();

    if (bytes_read == 0) {
        return false;
    }

    // 실제로 읽은 바이트 수를 used_size로 설정
    block->setUsedSize(static_cast<size_t>(bytes_read));

    if (stats) {
        stats->block_reads++;
    }

    return true;
}

void TableReader::reset() {
    file.clear();
    file.seekg(0, std::ios::beg);
}

// TableWriter 구현
TableWriter::TableWriter(const std::string& fname, Statistics* st)
    : filename(fname), stats(st) {
    file.open(filename, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
}

TableWriter::~TableWriter() {
    if (file.is_open()) {
        file.close();
    }
}

bool TableWriter::writeBlock(const Block* block) {
    if (!file.is_open() || block->isEmpty()) {
        return false;
    }

    // 사용된 크기만큼만 쓰기
    file.write(block->getData(), block->getUsedSize());

    if (stats) {
        stats->block_writes++;
    }

    return file.good();
}

// CSV를 블록 파일로 변환
void convertCSVToBlocks(const std::string& csv_file,
                        const std::string& block_file,
                        const std::string& table_type,
                        size_t block_size) {
    std::ifstream input(csv_file);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open CSV file: " + csv_file);
    }

    TableWriter writer(block_file, nullptr);
    Block block(block_size);
    RecordWriter rec_writer(&block);

    std::string line;
    int record_count = 0;

    while (std::getline(input, line)) {
        if (line.empty()) continue;

        try {
            Record record;

            if (table_type == "PART") {
                PartRecord part = PartRecord::fromCSV(line);
                record = part.toRecord();
            } else if (table_type == "PARTSUPP") {
                PartSuppRecord partsupp = PartSuppRecord::fromCSV(line);
                record = partsupp.toRecord();
            } else {
                throw std::runtime_error("Unknown table type: " + table_type);
            }

            // 블록에 레코드 쓰기
            if (!rec_writer.writeRecord(record)) {
                // 블록이 가득 차면 디스크에 쓰고 새 블록 시작
                writer.writeBlock(&block);
                block.clear();

                // 새 블록에 레코드 쓰기
                if (!rec_writer.writeRecord(record)) {
                    throw std::runtime_error("Record too large for block");
                }
            }

            record_count++;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing line: " << line << "\nError: " << e.what() << std::endl;
        }
    }

    // 마지막 블록 쓰기
    if (!block.isEmpty()) {
        writer.writeBlock(&block);
    }

    input.close();
    std::cout << "Converted " << record_count << " records from " << csv_file
              << " to " << block_file << std::endl;
}
