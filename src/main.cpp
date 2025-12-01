#include "common.h"
#include "block.h"
#include "record.h"
#include "table.h"
#include "buffer.h"
#include "join.h"
#include "multi_table_join.h"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <vector>

// 쉼표로 구분된 문자열 파싱
std::vector<std::string> splitByComma(const std::string& str) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        result.push_back(item);
    }
    return result;
}

// 조인 조건 파싱 (예: "0.partkey=1.partkey;1.suppkey=2.suppkey")
std::vector<JoinCondition> parseJoinConditions(const std::string& str) {
    std::vector<JoinCondition> conditions;
    std::stringstream ss(str);
    std::string condition_str;

    // 세미콜론으로 분리
    while (std::getline(ss, condition_str, ';')) {
        // 등호로 분리
        size_t eq_pos = condition_str.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string left = condition_str.substr(0, eq_pos);
        std::string right = condition_str.substr(eq_pos + 1);

        // 왼쪽 파싱 (예: "0.partkey")
        size_t left_dot = left.find('.');
        if (left_dot == std::string::npos) continue;
        size_t left_idx = std::atoi(left.substr(0, left_dot).c_str());
        std::string left_field = left.substr(left_dot + 1);

        // 오른쪽 파싱 (예: "1.partkey")
        size_t right_dot = right.find('.');
        if (right_dot == std::string::npos) continue;
        size_t right_idx = std::atoi(right.substr(0, right_dot).c_str());
        std::string right_field = right.substr(right_dot + 1);

        conditions.emplace_back(left_idx, left_field, right_idx, right_field);
    }

    return conditions;
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTION]...\n\n";
    std::cout << "Options:\n";
    std::cout << "  --convert-csv        Convert CSV files to block format\n";
    std::cout << "      --csv-file FILE      Input CSV file path\n";
    std::cout << "      --block-file FILE    Output block file path\n";
    std::cout << "      --table-type TYPE    Table type (PART, PARTSUPP, or SUPPLIER)\n";
    std::cout << "      --block-size SIZE    Block size in bytes (default: 4096)\n\n";
    std::cout << "  --join               Perform Block Nested Loops Join (2 tables)\n";
    std::cout << "      --outer-table FILE   Outer table file (block format)\n";
    std::cout << "      --inner-table FILE   Inner table file (block format)\n";
    std::cout << "      --outer-type TYPE    Outer table type (PART, PARTSUPP, or SUPPLIER)\n";
    std::cout << "      --inner-type TYPE    Inner table type (PART, PARTSUPP, or SUPPLIER)\n";
    std::cout << "      --output FILE        Output file path\n";
    std::cout << "      --buffer-size NUM    Number of buffer blocks (default: 10)\n";
    std::cout << "      --block-size SIZE    Block size in bytes (default: 4096)\n\n";
    std::cout << "  --multi-join         Perform Multi-Table Join (3+ tables)\n";
    std::cout << "      --tables FILES       Comma-separated table files (e.g., t1.dat,t2.dat,t3.dat)\n";
    std::cout << "      --table-types TYPES  Comma-separated types (e.g., PART,PARTSUPP,SUPPLIER)\n";
    std::cout << "      --join-conditions C  Join conditions (e.g., \"0.partkey=1.partkey;1.suppkey=2.suppkey\")\n";
    std::cout << "                           Format: \"TABLE_IDX.FIELD=TABLE_IDX.FIELD;...\" \n";
    std::cout << "      --output FILE        Output file path\n";
    std::cout << "      --buffer-size NUM    Number of buffer blocks (default: 10)\n";
    std::cout << "      --block-size SIZE    Block size in bytes (default: 4096)\n\n";
    std::cout << "Examples:\n";
    std::cout << "  # Convert PART CSV to block format\n";
    std::cout << "  " << program_name << " --convert-csv --csv-file data/part.tbl \\\n";
    std::cout << "      --block-file data/part.dat --table-type PART\n\n";
    std::cout << "  # Perform 2-table join with buffer size of 20 blocks\n";
    std::cout << "  " << program_name << " --join --outer-table data/part.dat \\\n";
    std::cout << "      --inner-table data/partsupp.dat --outer-type PART \\\n";
    std::cout << "      --inner-type PARTSUPP --output output/result.dat \\\n";
    std::cout << "      --buffer-size 20\n\n";
    std::cout << "  # Perform 3-table join\n";
    std::cout << "  " << program_name << " --multi-join \\\n";
    std::cout << "      --tables data/part.dat,data/partsupp.dat,data/supplier.dat \\\n";
    std::cout << "      --table-types PART,PARTSUPP,SUPPLIER \\\n";
    std::cout << "      --join-conditions \"0.partkey=1.partkey;1.suppkey=2.suppkey\" \\\n";
    std::cout << "      --output output/multi_result.dat --buffer-size 20\n";
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            printUsage(argv[0]);
            return 1;
        }

        std::string mode;
        std::string csv_file, block_file, table_type;
        std::string outer_table, inner_table, outer_type, inner_type, output_file;
        std::string tables_str, table_types_str, join_conditions_str;
        size_t buffer_size = 10;
        size_t block_size = DEFAULT_BLOCK_SIZE;

        // 인자 파싱
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "--convert-csv") {
                mode = "convert";
            } else if (arg == "--join") {
                mode = "join";
            } else if (arg == "--multi-join") {
                mode = "multi-join";
            } else if (arg == "--csv-file" && i + 1 < argc) {
                csv_file = argv[++i];
            } else if (arg == "--block-file" && i + 1 < argc) {
                block_file = argv[++i];
            } else if (arg == "--table-type" && i + 1 < argc) {
                table_type = argv[++i];
            } else if (arg == "--outer-table" && i + 1 < argc) {
                outer_table = argv[++i];
            } else if (arg == "--inner-table" && i + 1 < argc) {
                inner_table = argv[++i];
            } else if (arg == "--outer-type" && i + 1 < argc) {
                outer_type = argv[++i];
            } else if (arg == "--inner-type" && i + 1 < argc) {
                inner_type = argv[++i];
            } else if (arg == "--output" && i + 1 < argc) {
                output_file = argv[++i];
            } else if (arg == "--tables" && i + 1 < argc) {
                tables_str = argv[++i];
            } else if (arg == "--table-types" && i + 1 < argc) {
                table_types_str = argv[++i];
            } else if (arg == "--join-conditions" && i + 1 < argc) {
                join_conditions_str = argv[++i];
            } else if (arg == "--buffer-size" && i + 1 < argc) {
                buffer_size = std::atoi(argv[++i]);
            } else if (arg == "--block-size" && i + 1 < argc) {
                block_size = std::atoi(argv[++i]);
            } else if (arg == "--help" || arg == "-h") {
                printUsage(argv[0]);
                return 0;
            } else {
                std::cerr << "Unknown option: " << arg << std::endl;
                printUsage(argv[0]);
                return 1;
            }
        }

        // CSV 변환 모드
        if (mode == "convert") {
            if (csv_file.empty() || block_file.empty() || table_type.empty()) {
                std::cerr << "Error: Missing required arguments for CSV conversion\n";
                printUsage(argv[0]);
                return 1;
            }

            std::cout << "Converting CSV to block format...\n";
            std::cout << "Input: " << csv_file << "\n";
            std::cout << "Output: " << block_file << "\n";
            std::cout << "Table Type: " << table_type << "\n";
            std::cout << "Block Size: " << block_size << " bytes\n\n";

            convertCSVToBlocks(csv_file, block_file, table_type, block_size);

            std::cout << "Conversion completed successfully!\n";
        }
        // Join 모드
        else if (mode == "join") {
            if (outer_table.empty() || inner_table.empty() ||
                outer_type.empty() || inner_type.empty() || output_file.empty()) {
                std::cerr << "Error: Missing required arguments for join\n";
                printUsage(argv[0]);
                return 1;
            }

            std::cout << "=== Block Nested Loops Join ===" << std::endl;
            std::cout << "Outer Table: " << outer_table << " (" << outer_type << ")" << std::endl;
            std::cout << "Inner Table: " << inner_table << " (" << inner_type << ")" << std::endl;
            std::cout << "Output File: " << output_file << std::endl;
            std::cout << "Buffer Size: " << buffer_size << " blocks" << std::endl;
            std::cout << "Block Size: " << block_size << " bytes" << std::endl;
            std::cout << "Total Memory: " << (buffer_size * block_size / 1024.0 / 1024.0)
                      << " MB" << std::endl;
            std::cout << "\nExecuting join...\n" << std::endl;

            BlockNestedLoopsJoin join(outer_table, inner_table, output_file,
                                     outer_type, inner_type,
                                     buffer_size, block_size);
            join.execute();

            std::cout << "\nJoin completed successfully!\n";
        }
        // Multi-table join 모드
        else if (mode == "multi-join") {
            if (tables_str.empty() || table_types_str.empty() ||
                join_conditions_str.empty() || output_file.empty()) {
                std::cerr << "Error: Missing required arguments for multi-join\n";
                printUsage(argv[0]);
                return 1;
            }

            // 문자열 파싱
            std::vector<std::string> table_files = splitByComma(tables_str);
            std::vector<std::string> table_types = splitByComma(table_types_str);
            std::vector<JoinCondition> join_conditions = parseJoinConditions(join_conditions_str);

            // 검증
            if (table_files.size() != table_types.size()) {
                std::cerr << "Error: Number of tables and table types must match\n";
                return 1;
            }

            if (table_files.size() < 2) {
                std::cerr << "Error: At least 2 tables are required for join\n";
                return 1;
            }

            if (join_conditions.size() != table_files.size() - 1) {
                std::cerr << "Error: Number of join conditions must be (number of tables - 1)\n";
                std::cerr << "Expected: " << (table_files.size() - 1)
                          << ", Got: " << join_conditions.size() << "\n";
                return 1;
            }

            // Multi-table join 실행
            MultiTableJoin multi_join(buffer_size, block_size);

            // 테이블 추가
            for (size_t i = 0; i < table_files.size(); ++i) {
                multi_join.addTable(table_files[i], table_types[i]);
            }

            // 조인 조건 추가
            for (const auto& cond : join_conditions) {
                multi_join.addJoinCondition(cond.left_table_idx, cond.left_field,
                                           cond.right_table_idx, cond.right_field);
            }

            // 출력 파일 설정
            multi_join.setOutputFile(output_file);

            // 실행
            multi_join.execute();

            std::cout << "\nMulti-table join completed successfully!\n";
        }
        else {
            std::cerr << "Error: Please specify either --convert-csv, --join, or --multi-join\n";
            printUsage(argv[0]);
            return 1;
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
