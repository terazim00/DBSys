#ifndef TABLE_H
#define TABLE_H

#include "common.h"
#include "record.h"
#include "block.h"
#include <string>
#include <vector>
#include <fstream>

// TPC-H PART 테이블 스키마
struct PartRecord {
    int_t partkey;
    std::string name;
    std::string mfgr;
    std::string brand;
    std::string type;
    int_t size;
    std::string container;
    decimal_t retailprice;
    std::string comment;

    // Record로 변환
    Record toRecord() const;

    // Record에서 생성
    static PartRecord fromRecord(const Record& rec);

    // CSV 라인에서 파싱
    static PartRecord fromCSV(const std::string& line);
};

// TPC-H PARTSUPP 테이블 스키마
struct PartSuppRecord {
    int_t partkey;
    int_t suppkey;
    int_t availqty;
    decimal_t supplycost;
    std::string comment;

    // Record로 변환
    Record toRecord() const;

    // Record에서 생성
    static PartSuppRecord fromRecord(const Record& rec);

    // CSV 라인에서 파싱
    static PartSuppRecord fromCSV(const std::string& line);
};

// Join 결과 레코드
struct JoinResultRecord {
    PartRecord part;
    PartSuppRecord partsupp;

    // Record로 변환
    Record toRecord() const;
};

// 테이블 리더 클래스
class TableReader {
private:
    std::string filename;
    std::ifstream file;
    size_t block_size;
    Statistics* stats;

public:
    TableReader(const std::string& fname, size_t blk_size = DEFAULT_BLOCK_SIZE,
                Statistics* st = nullptr);
    ~TableReader();

    // 다음 블록 읽기
    bool readBlock(Block* block);

    // 파일 처음으로 되돌리기
    void reset();

    // 파일이 열려있는지 확인
    bool isOpen() const { return file.is_open(); }
};

// 테이블 라이터 클래스
class TableWriter {
private:
    std::string filename;
    std::ofstream file;
    Statistics* stats;

public:
    TableWriter(const std::string& fname, Statistics* st = nullptr);
    ~TableWriter();

    // 블록 쓰기
    bool writeBlock(const Block* block);

    // 파일이 열려있는지 확인
    bool isOpen() const { return file.is_open(); }
};

// CSV 파일을 블록 기반 파일로 변환
void convertCSVToBlocks(const std::string& csv_file,
                        const std::string& block_file,
                        const std::string& table_type,
                        size_t block_size = DEFAULT_BLOCK_SIZE);

#endif // TABLE_H
