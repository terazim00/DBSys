#include "table.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

// Helper function to trim whitespace from strings
static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}

// Helper function to safely convert string to int
static int safe_stoi(const std::string& str, const std::string& field_name) {
    std::string trimmed = trim(str);
    if (trimmed.empty()) {
        throw std::runtime_error("Empty field for " + field_name);
    }
    try {
        return std::stoi(trimmed);
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid integer in " + field_name + ": '" + trimmed + "'");
    }
}

// Helper function to safely convert string to float
static float safe_stof(const std::string& str, const std::string& field_name) {
    std::string trimmed = trim(str);
    if (trimmed.empty()) {
        throw std::runtime_error("Empty field for " + field_name);
    }
    try {
        return std::stof(trimmed);
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid float in " + field_name + ": '" + trimmed + "'");
    }
}

// Helper function to extract supplier key from strings like "Supplier#000009978"
static int extract_supplier_key(const std::string& str, const std::string& field_name) {
    std::string trimmed = trim(str);
    if (trimmed.empty()) {
        throw std::runtime_error("Empty field for " + field_name);
    }

    // Check if it's in the format "Supplier#NNNNNN"
    if (trimmed.find("Supplier#") == 0) {
        // Extract the numeric part after "Supplier#"
        std::string num_part = trimmed.substr(9); // "Supplier#" is 9 characters
        try {
            return std::stoi(num_part);
        } catch (const std::exception& e) {
            throw std::runtime_error("Invalid supplier key format in " + field_name + ": '" + trimmed + "'");
        }
    }

    // Otherwise, try to parse as regular integer
    return safe_stoi(trimmed, field_name);
}

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
        throw std::runtime_error("Invalid PART record: expected 9 fields, got " + std::to_string(rec.getFieldCount()));
    }
    part.partkey = safe_stoi(rec.getField(0), "PART.partkey");
    part.name = rec.getField(1);
    part.mfgr = rec.getField(2);
    part.brand = rec.getField(3);
    part.type = rec.getField(4);
    part.size = safe_stoi(rec.getField(5), "PART.size");
    part.container = rec.getField(6);
    part.retailprice = safe_stof(rec.getField(7), "PART.retailprice");
    part.comment = rec.getField(8);
    return part;
}

PartRecord PartRecord::fromCSV(const std::string& line) {
    PartRecord part;
    std::stringstream ss(line);
    std::string field;

    std::getline(ss, field, '|');
    part.partkey = safe_stoi(field, "PART.partkey (CSV)");

    std::getline(ss, part.name, '|');
    std::getline(ss, part.mfgr, '|');
    std::getline(ss, part.brand, '|');
    std::getline(ss, part.type, '|');

    std::getline(ss, field, '|');
    part.size = safe_stoi(field, "PART.size (CSV)");

    std::getline(ss, part.container, '|');

    std::getline(ss, field, '|');
    part.retailprice = safe_stof(field, "PART.retailprice (CSV)");

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
        throw std::runtime_error("Invalid PARTSUPP record: expected 5 fields, got " + std::to_string(rec.getFieldCount()));
    }
    partsupp.partkey = safe_stoi(rec.getField(0), "PARTSUPP.partkey");
    partsupp.suppkey = safe_stoi(rec.getField(1), "PARTSUPP.suppkey");
    partsupp.availqty = safe_stoi(rec.getField(2), "PARTSUPP.availqty");
    partsupp.supplycost = safe_stof(rec.getField(3), "PARTSUPP.supplycost");
    partsupp.comment = rec.getField(4);
    return partsupp;
}

PartSuppRecord PartSuppRecord::fromCSV(const std::string& line) {
    PartSuppRecord partsupp;
    std::stringstream ss(line);
    std::string field;

    std::getline(ss, field, '|');
    partsupp.partkey = safe_stoi(field, "PARTSUPP.partkey (CSV)");

    std::getline(ss, field, '|');
    partsupp.suppkey = extract_supplier_key(field, "PARTSUPP.suppkey (CSV)");

    std::getline(ss, field, '|');
    partsupp.availqty = safe_stoi(field, "PARTSUPP.availqty (CSV)");

    std::getline(ss, field, '|');
    partsupp.supplycost = safe_stof(field, "PARTSUPP.supplycost (CSV)");

    std::getline(ss, partsupp.comment, '|');

    return partsupp;
}

// SupplierRecord 구현
Record SupplierRecord::toRecord() const {
    std::vector<std::string> fields;
    fields.push_back(std::to_string(suppkey));
    fields.push_back(name);
    fields.push_back(address);
    fields.push_back(std::to_string(nationkey));
    fields.push_back(phone);
    fields.push_back(std::to_string(acctbal));
    fields.push_back(comment);
    return Record(fields);
}

SupplierRecord SupplierRecord::fromRecord(const Record& rec) {
    SupplierRecord supplier;
    if (rec.getFieldCount() < 7) {
        throw std::runtime_error("Invalid SUPPLIER record: expected 7 fields, got " + std::to_string(rec.getFieldCount()));
    }
    supplier.suppkey = safe_stoi(rec.getField(0), "SUPPLIER.suppkey");
    supplier.name = rec.getField(1);
    supplier.address = rec.getField(2);
    supplier.nationkey = safe_stoi(rec.getField(3), "SUPPLIER.nationkey");
    supplier.phone = rec.getField(4);
    supplier.acctbal = safe_stof(rec.getField(5), "SUPPLIER.acctbal");
    supplier.comment = rec.getField(6);
    return supplier;
}

SupplierRecord SupplierRecord::fromCSV(const std::string& line) {
    SupplierRecord supplier;
    std::stringstream ss(line);
    std::string field;

    std::getline(ss, field, '|');
    supplier.suppkey = safe_stoi(field, "SUPPLIER.suppkey (CSV)");

    std::getline(ss, supplier.name, '|');
    std::getline(ss, supplier.address, '|');

    std::getline(ss, field, '|');
    supplier.nationkey = safe_stoi(field, "SUPPLIER.nationkey (CSV)");

    std::getline(ss, supplier.phone, '|');

    std::getline(ss, field, '|');
    supplier.acctbal = safe_stof(field, "SUPPLIER.acctbal (CSV)");

    std::getline(ss, supplier.comment, '|');

    return supplier;
}

// CustomerRecord 구현
Record CustomerRecord::toRecord() const {
    std::vector<std::string> fields;
    fields.push_back(std::to_string(custkey));
    fields.push_back(name);
    fields.push_back(address);
    fields.push_back(std::to_string(nationkey));
    fields.push_back(phone);
    fields.push_back(std::to_string(acctbal));
    fields.push_back(mktsegment);
    fields.push_back(comment);
    return Record(fields);
}

CustomerRecord CustomerRecord::fromRecord(const Record& rec) {
    CustomerRecord customer;
    if (rec.getFieldCount() < 8) {
        throw std::runtime_error("Invalid CUSTOMER record: expected 8 fields, got " + std::to_string(rec.getFieldCount()));
    }
    customer.custkey = safe_stoi(rec.getField(0), "CUSTOMER.custkey");
    customer.name = rec.getField(1);
    customer.address = rec.getField(2);
    customer.nationkey = safe_stoi(rec.getField(3), "CUSTOMER.nationkey");
    customer.phone = rec.getField(4);
    customer.acctbal = safe_stof(rec.getField(5), "CUSTOMER.acctbal");
    customer.mktsegment = rec.getField(6);
    customer.comment = rec.getField(7);
    return customer;
}

CustomerRecord CustomerRecord::fromCSV(const std::string& line) {
    CustomerRecord customer;
    std::stringstream ss(line);
    std::string field;

    std::getline(ss, field, '|');
    customer.custkey = safe_stoi(field, "CUSTOMER.custkey (TBL)");

    std::getline(ss, customer.name, '|');
    std::getline(ss, customer.address, '|');

    std::getline(ss, field, '|');
    customer.nationkey = safe_stoi(field, "CUSTOMER.nationkey (TBL)");

    std::getline(ss, customer.phone, '|');

    std::getline(ss, field, '|');
    customer.acctbal = safe_stof(field, "CUSTOMER.acctbal (TBL)");

    std::getline(ss, customer.mktsegment, '|');
    std::getline(ss, customer.comment, '|');

    return customer;
}

// OrdersRecord 구현
Record OrdersRecord::toRecord() const {
    std::vector<std::string> fields;
    fields.push_back(std::to_string(orderkey));
    fields.push_back(std::to_string(custkey));
    fields.push_back(orderstatus);
    fields.push_back(std::to_string(totalprice));
    fields.push_back(orderdate);
    fields.push_back(orderpriority);
    fields.push_back(clerk);
    fields.push_back(std::to_string(shippriority));
    fields.push_back(comment);
    return Record(fields);
}

OrdersRecord OrdersRecord::fromRecord(const Record& rec) {
    OrdersRecord orders;
    if (rec.getFieldCount() < 9) {
        throw std::runtime_error("Invalid ORDERS record: expected 9 fields, got " + std::to_string(rec.getFieldCount()));
    }
    orders.orderkey = safe_stoi(rec.getField(0), "ORDERS.orderkey");
    orders.custkey = safe_stoi(rec.getField(1), "ORDERS.custkey");
    orders.orderstatus = rec.getField(2);
    orders.totalprice = safe_stof(rec.getField(3), "ORDERS.totalprice");
    orders.orderdate = rec.getField(4);
    orders.orderpriority = rec.getField(5);
    orders.clerk = rec.getField(6);
    orders.shippriority = safe_stoi(rec.getField(7), "ORDERS.shippriority");
    orders.comment = rec.getField(8);
    return orders;
}

OrdersRecord OrdersRecord::fromCSV(const std::string& line) {
    OrdersRecord orders;
    std::stringstream ss(line);
    std::string field;

    std::getline(ss, field, '|');
    orders.orderkey = safe_stoi(field, "ORDERS.orderkey (TBL)");

    std::getline(ss, field, '|');
    orders.custkey = safe_stoi(field, "ORDERS.custkey (TBL)");

    std::getline(ss, orders.orderstatus, '|');

    std::getline(ss, field, '|');
    orders.totalprice = safe_stof(field, "ORDERS.totalprice (TBL)");

    std::getline(ss, orders.orderdate, '|');
    std::getline(ss, orders.orderpriority, '|');
    std::getline(ss, orders.clerk, '|');

    std::getline(ss, field, '|');
    orders.shippriority = safe_stoi(field, "ORDERS.shippriority (TBL)");

    std::getline(ss, orders.comment, '|');

    return orders;
}

// LineItemRecord 구현
Record LineItemRecord::toRecord() const {
    std::vector<std::string> fields;
    fields.push_back(std::to_string(orderkey));
    fields.push_back(std::to_string(partkey));
    fields.push_back(std::to_string(suppkey));
    fields.push_back(std::to_string(linenumber));
    fields.push_back(std::to_string(quantity));
    fields.push_back(std::to_string(extendedprice));
    fields.push_back(std::to_string(discount));
    fields.push_back(std::to_string(tax));
    fields.push_back(returnflag);
    fields.push_back(linestatus);
    fields.push_back(shipdate);
    fields.push_back(commitdate);
    fields.push_back(receiptdate);
    fields.push_back(shipinstruct);
    fields.push_back(shipmode);
    fields.push_back(comment);
    return Record(fields);
}

LineItemRecord LineItemRecord::fromRecord(const Record& rec) {
    LineItemRecord lineitem;
    if (rec.getFieldCount() < 16) {
        throw std::runtime_error("Invalid LINEITEM record: expected 16 fields, got " + std::to_string(rec.getFieldCount()));
    }
    lineitem.orderkey = safe_stoi(rec.getField(0), "LINEITEM.orderkey");
    lineitem.partkey = safe_stoi(rec.getField(1), "LINEITEM.partkey");
    lineitem.suppkey = safe_stoi(rec.getField(2), "LINEITEM.suppkey");
    lineitem.linenumber = safe_stoi(rec.getField(3), "LINEITEM.linenumber");
    lineitem.quantity = safe_stof(rec.getField(4), "LINEITEM.quantity");
    lineitem.extendedprice = safe_stof(rec.getField(5), "LINEITEM.extendedprice");
    lineitem.discount = safe_stof(rec.getField(6), "LINEITEM.discount");
    lineitem.tax = safe_stof(rec.getField(7), "LINEITEM.tax");
    lineitem.returnflag = rec.getField(8);
    lineitem.linestatus = rec.getField(9);
    lineitem.shipdate = rec.getField(10);
    lineitem.commitdate = rec.getField(11);
    lineitem.receiptdate = rec.getField(12);
    lineitem.shipinstruct = rec.getField(13);
    lineitem.shipmode = rec.getField(14);
    lineitem.comment = rec.getField(15);
    return lineitem;
}

LineItemRecord LineItemRecord::fromCSV(const std::string& line) {
    LineItemRecord lineitem;
    std::stringstream ss(line);
    std::string field;

    std::getline(ss, field, '|');
    lineitem.orderkey = safe_stoi(field, "LINEITEM.orderkey (TBL)");

    std::getline(ss, field, '|');
    lineitem.partkey = safe_stoi(field, "LINEITEM.partkey (TBL)");

    std::getline(ss, field, '|');
    lineitem.suppkey = safe_stoi(field, "LINEITEM.suppkey (TBL)");

    std::getline(ss, field, '|');
    lineitem.linenumber = safe_stoi(field, "LINEITEM.linenumber (TBL)");

    std::getline(ss, field, '|');
    lineitem.quantity = safe_stof(field, "LINEITEM.quantity (TBL)");

    std::getline(ss, field, '|');
    lineitem.extendedprice = safe_stof(field, "LINEITEM.extendedprice (TBL)");

    std::getline(ss, field, '|');
    lineitem.discount = safe_stof(field, "LINEITEM.discount (TBL)");

    std::getline(ss, field, '|');
    lineitem.tax = safe_stof(field, "LINEITEM.tax (TBL)");

    std::getline(ss, lineitem.returnflag, '|');
    std::getline(ss, lineitem.linestatus, '|');
    std::getline(ss, lineitem.shipdate, '|');
    std::getline(ss, lineitem.commitdate, '|');
    std::getline(ss, lineitem.receiptdate, '|');
    std::getline(ss, lineitem.shipinstruct, '|');
    std::getline(ss, lineitem.shipmode, '|');
    std::getline(ss, lineitem.comment, '|');

    return lineitem;
}

// NationRecord 구현
Record NationRecord::toRecord() const {
    std::vector<std::string> fields;
    fields.push_back(std::to_string(nationkey));
    fields.push_back(name);
    fields.push_back(std::to_string(regionkey));
    fields.push_back(comment);
    return Record(fields);
}

NationRecord NationRecord::fromRecord(const Record& rec) {
    NationRecord nation;
    if (rec.getFieldCount() < 4) {
        throw std::runtime_error("Invalid NATION record: expected 4 fields, got " + std::to_string(rec.getFieldCount()));
    }
    nation.nationkey = safe_stoi(rec.getField(0), "NATION.nationkey");
    nation.name = rec.getField(1);
    nation.regionkey = safe_stoi(rec.getField(2), "NATION.regionkey");
    nation.comment = rec.getField(3);
    return nation;
}

NationRecord NationRecord::fromCSV(const std::string& line) {
    NationRecord nation;
    std::stringstream ss(line);
    std::string field;

    std::getline(ss, field, '|');
    nation.nationkey = safe_stoi(field, "NATION.nationkey (TBL)");

    std::getline(ss, nation.name, '|');

    std::getline(ss, field, '|');
    nation.regionkey = safe_stoi(field, "NATION.regionkey (TBL)");

    std::getline(ss, nation.comment, '|');

    return nation;
}

// RegionRecord 구현
Record RegionRecord::toRecord() const {
    std::vector<std::string> fields;
    fields.push_back(std::to_string(regionkey));
    fields.push_back(name);
    fields.push_back(comment);
    return Record(fields);
}

RegionRecord RegionRecord::fromRecord(const Record& rec) {
    RegionRecord region;
    if (rec.getFieldCount() < 3) {
        throw std::runtime_error("Invalid REGION record: expected 3 fields, got " + std::to_string(rec.getFieldCount()));
    }
    region.regionkey = safe_stoi(rec.getField(0), "REGION.regionkey");
    region.name = rec.getField(1);
    region.comment = rec.getField(2);
    return region;
}

RegionRecord RegionRecord::fromCSV(const std::string& line) {
    RegionRecord region;
    std::stringstream ss(line);
    std::string field;

    std::getline(ss, field, '|');
    region.regionkey = safe_stoi(field, "REGION.regionkey (TBL)");

    std::getline(ss, region.name, '|');
    std::getline(ss, region.comment, '|');

    return region;
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
            } else if (table_type == "SUPPLIER") {
                SupplierRecord supplier = SupplierRecord::fromCSV(line);
                record = supplier.toRecord();
            } else if (table_type == "CUSTOMER") {
                CustomerRecord customer = CustomerRecord::fromCSV(line);
                record = customer.toRecord();
            } else if (table_type == "ORDERS") {
                OrdersRecord orders = OrdersRecord::fromCSV(line);
                record = orders.toRecord();
            } else if (table_type == "LINEITEM") {
                LineItemRecord lineitem = LineItemRecord::fromCSV(line);
                record = lineitem.toRecord();
            } else if (table_type == "NATION") {
                NationRecord nation = NationRecord::fromCSV(line);
                record = nation.toRecord();
            } else if (table_type == "REGION") {
                RegionRecord region = RegionRecord::fromCSV(line);
                record = region.toRecord();
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
