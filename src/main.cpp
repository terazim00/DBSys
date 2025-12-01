#include "common.h"
#include "block.h"
#include "record.h"
#include "table.h"
#include "buffer.h"
#include "join.h"
#include "optimized_join.h"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <vector>

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTION]...\n\n";
    std::cout << "Options:\n";
    std::cout << "  --convert            Convert TBL files to block format (.dat)\n";
    std::cout << "      --input-file FILE    Input TBL file path (pipe-delimited)\n";
    std::cout << "      --output-file FILE   Output block file path (.dat)\n";
    std::cout << "      --table-type TYPE    Table type: PART, PARTSUPP, SUPPLIER,\n";
    std::cout << "                           CUSTOMER, ORDERS, LINEITEM, NATION, REGION\n";
    std::cout << "      --block-size SIZE    Block size in bytes (default: 4096)\n\n";
    std::cout << "  --join               Perform Block Nested Loops Join (2 tables)\n";
    std::cout << "      --outer-table FILE   Outer table file (block format)\n";
    std::cout << "      --inner-table FILE   Inner table file (block format)\n";
    std::cout << "      --outer-type TYPE    Outer table type (any TPC-H table)\n";
    std::cout << "      --inner-type TYPE    Inner table type (any TPC-H table)\n";
    std::cout << "      --join-key KEY       Join key: partkey, suppkey, custkey,\n";
    std::cout << "                           orderkey, nationkey, regionkey\n";
    std::cout << "      --output FILE        Output file path\n";
    std::cout << "      --buffer-size NUM    Number of buffer blocks (default: 10)\n";
    std::cout << "      --block-size SIZE    Block size in bytes (default: 4096)\n\n";
    std::cout << "  --hash-join          Perform Hash Join (2 tables)\n";
    std::cout << "      --build-table FILE   Build table file (smaller table, block format)\n";
    std::cout << "      --probe-table FILE   Probe table file (larger table, block format)\n";
    std::cout << "      --build-type TYPE    Build table type (any TPC-H table)\n";
    std::cout << "      --probe-type TYPE    Probe table type (any TPC-H table)\n";
    std::cout << "      --join-key KEY       Join key (see --join for options)\n";
    std::cout << "      --output FILE        Output file path\n";
    std::cout << "      --block-size SIZE    Block size in bytes (default: 4096)\n\n";
    std::cout << "  --compare-all        Compare BNLJ and Hash Join performance\n";
    std::cout << "      --outer-table FILE   First table file (block format)\n";
    std::cout << "      --inner-table FILE   Second table file (block format)\n";
    std::cout << "      --outer-type TYPE    First table type (any TPC-H table)\n";
    std::cout << "      --inner-type TYPE    Second table type (any TPC-H table)\n";
    std::cout << "      --join-key KEY       Join key (see --join for options)\n";
    std::cout << "      --output-dir DIR     Output directory for result files\n\n";
    std::cout << "Examples:\n";
    std::cout << "  # Convert TBL files to block format\n";
    std::cout << "  " << program_name << " --convert --input-file data/part.tbl \\\n";
    std::cout << "      --output-file data/part.dat --table-type PART\n";
    std::cout << "  " << program_name << " --convert --input-file data/orders.tbl \\\n";
    std::cout << "      --output-file data/orders.dat --table-type ORDERS\n\n";
    std::cout << "  # BNLJ: PART ⋈ PARTSUPP on partkey\n";
    std::cout << "  " << program_name << " --join --outer-table data/part.dat \\\n";
    std::cout << "      --inner-table data/partsupp.dat --outer-type PART \\\n";
    std::cout << "      --inner-type PARTSUPP --join-key partkey \\\n";
    std::cout << "      --output output/result.dat --buffer-size 20\n\n";
    std::cout << "  # BNLJ: CUSTOMER ⋈ ORDERS on custkey\n";
    std::cout << "  " << program_name << " --join --outer-table data/customer.dat \\\n";
    std::cout << "      --inner-table data/orders.dat --outer-type CUSTOMER \\\n";
    std::cout << "      --inner-type ORDERS --join-key custkey \\\n";
    std::cout << "      --output output/cust_orders.dat --buffer-size 20\n\n";
    std::cout << "  # Hash Join: PART (build) ⋈ PARTSUPP (probe) on partkey\n";
    std::cout << "  " << program_name << " --hash-join --build-table data/part.dat \\\n";
    std::cout << "      --probe-table data/partsupp.dat --build-type PART \\\n";
    std::cout << "      --probe-type PARTSUPP --join-key partkey \\\n";
    std::cout << "      --output output/hash_result.dat\n\n";
    std::cout << "  # Hash Join: ORDERS (build) ⋈ LINEITEM (probe) on orderkey\n";
    std::cout << "  " << program_name << " --hash-join --build-table data/orders.dat \\\n";
    std::cout << "      --probe-table data/lineitem.dat --build-type ORDERS \\\n";
    std::cout << "      --probe-type LINEITEM --join-key orderkey \\\n";
    std::cout << "      --output output/orders_lineitem.dat\n\n";
    std::cout << "  # Compare BNLJ vs Hash Join performance\n";
    std::cout << "  " << program_name << " --compare-all --outer-table data/part.dat \\\n";
    std::cout << "      --inner-table data/partsupp.dat --outer-type PART \\\n";
    std::cout << "      --inner-type PARTSUPP --join-key partkey \\\n";
    std::cout << "      --output-dir output\n";
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            printUsage(argv[0]);
            return 1;
        }

        std::string mode;
        std::string input_file, output_file_convert, table_type;
        std::string outer_table, inner_table, outer_type, inner_type, output_file;
        std::string build_table, probe_table, build_type, probe_type;
        std::string output_dir;
        std::string join_key;
        size_t buffer_size = 10;
        size_t block_size = DEFAULT_BLOCK_SIZE;

        // 인자 파싱
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "--convert") {
                mode = "convert";
            } else if (arg == "--join") {
                mode = "join";
            } else if (arg == "--hash-join") {
                mode = "hash-join";
            } else if (arg == "--compare-all") {
                mode = "compare-all";
            } else if (arg == "--input-file" && i + 1 < argc) {
                input_file = argv[++i];
            } else if (arg == "--output-file" && i + 1 < argc) {
                output_file_convert = argv[++i];
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
            } else if (arg == "--build-table" && i + 1 < argc) {
                build_table = argv[++i];
            } else if (arg == "--probe-table" && i + 1 < argc) {
                probe_table = argv[++i];
            } else if (arg == "--build-type" && i + 1 < argc) {
                build_type = argv[++i];
            } else if (arg == "--probe-type" && i + 1 < argc) {
                probe_type = argv[++i];
            } else if (arg == "--join-key" && i + 1 < argc) {
                join_key = argv[++i];
            } else if (arg == "--output" && i + 1 < argc) {
                output_file = argv[++i];
            } else if (arg == "--output-dir" && i + 1 < argc) {
                output_dir = argv[++i];
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

        // TBL 변환 모드
        if (mode == "convert") {
            if (input_file.empty() || output_file_convert.empty() || table_type.empty()) {
                std::cerr << "Error: Missing required arguments for TBL conversion\n";
                std::cerr << "Required: --input-file, --output-file, --table-type\n";
                printUsage(argv[0]);
                return 1;
            }

            std::cout << "Converting TBL to block format...\n";
            std::cout << "Input: " << input_file << "\n";
            std::cout << "Output: " << output_file_convert << "\n";
            std::cout << "Table Type: " << table_type << "\n";
            std::cout << "Block Size: " << block_size << " bytes\n\n";

            convertTBLToBlocks(input_file, output_file_convert, table_type, block_size);

            std::cout << "Conversion completed successfully!\n";
        }
        // Join 모드
        else if (mode == "join") {
            if (outer_table.empty() || inner_table.empty() ||
                outer_type.empty() || inner_type.empty() ||
                join_key.empty() || output_file.empty()) {
                std::cerr << "Error: Missing required arguments for join\n";
                std::cerr << "Required: --outer-table, --inner-table, --outer-type, --inner-type, --join-key, --output\n";
                printUsage(argv[0]);
                return 1;
            }

            std::cout << "=== Block Nested Loops Join ===" << std::endl;
            std::cout << "Outer Table: " << outer_table << " (" << outer_type << ")" << std::endl;
            std::cout << "Inner Table: " << inner_table << " (" << inner_type << ")" << std::endl;
            std::cout << "Join Key: " << join_key << std::endl;
            std::cout << "Output File: " << output_file << std::endl;
            std::cout << "Buffer Size: " << buffer_size << " blocks" << std::endl;
            std::cout << "Block Size: " << block_size << " bytes" << std::endl;
            std::cout << "Total Memory: " << (buffer_size * block_size / 1024.0 / 1024.0)
                      << " MB" << std::endl;
            std::cout << "\nExecuting join...\n" << std::endl;

            BlockNestedLoopsJoin join(outer_table, inner_table, output_file,
                                     outer_type, inner_type, join_key,
                                     buffer_size, block_size);
            join.execute();

            std::cout << "\nJoin completed successfully!\n";
        }
        // Hash Join 모드
        else if (mode == "hash-join") {
            if (build_table.empty() || probe_table.empty() ||
                build_type.empty() || probe_type.empty() ||
                join_key.empty() || output_file.empty()) {
                std::cerr << "Error: Missing required arguments for hash join\n";
                std::cerr << "Required: --build-table, --probe-table, --build-type, --probe-type, --join-key, --output\n";
                printUsage(argv[0]);
                return 1;
            }

            std::cout << "=== Hash Join ===" << std::endl;
            std::cout << "Build Table: " << build_table << " (" << build_type << ")" << std::endl;
            std::cout << "Probe Table: " << probe_table << " (" << probe_type << ")" << std::endl;
            std::cout << "Join Key: " << join_key << std::endl;
            std::cout << "Output File: " << output_file << std::endl;
            std::cout << "Block Size: " << block_size << " bytes" << std::endl;
            std::cout << "\nExecuting hash join...\n" << std::endl;

            HashJoin join(build_table, probe_table, output_file,
                         build_type, probe_type, join_key, block_size);
            join.execute();

            std::cout << "\nHash Join completed successfully!\n";
        }
        // 성능 비교 모드
        else if (mode == "compare-all") {
            if (outer_table.empty() || inner_table.empty() ||
                outer_type.empty() || inner_type.empty() ||
                join_key.empty() || output_dir.empty()) {
                std::cerr << "Error: Missing required arguments for performance comparison\n";
                std::cerr << "Required: --outer-table, --inner-table, --outer-type, --inner-type, --join-key, --output-dir\n";
                printUsage(argv[0]);
                return 1;
            }

            std::cout << "=== Performance Comparison ===" << std::endl;
            std::cout << "Table 1: " << outer_table << " (" << outer_type << ")" << std::endl;
            std::cout << "Table 2: " << inner_table << " (" << inner_type << ")" << std::endl;
            std::cout << "Join Key: " << join_key << std::endl;
            std::cout << "Output Directory: " << output_dir << std::endl;
            std::cout << "\nRunning performance tests...\n" << std::endl;

            PerformanceTester::compareAll(outer_table, inner_table, output_dir,
                                         outer_type, inner_type, join_key);

            std::cout << "\nPerformance comparison completed!\n";
        }
        else {
            std::cerr << "Error: Please specify one of: --convert, --join, --hash-join, --compare-all\n";
            printUsage(argv[0]);
            return 1;
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
