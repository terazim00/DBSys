#include "common.h"
#include "block.h"
#include "record.h"
#include "table.h"
#include "buffer.h"
#include "join.h"
#include <iostream>
#include <cstring>
#include <cstdlib>

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTION]...\n\n";
    std::cout << "Options:\n";
    std::cout << "  --convert-csv        Convert CSV files to block format\n";
    std::cout << "      --csv-file FILE      Input CSV file path\n";
    std::cout << "      --block-file FILE    Output block file path\n";
    std::cout << "      --table-type TYPE    Table type (PART or PARTSUPP)\n";
    std::cout << "      --block-size SIZE    Block size in bytes (default: 4096)\n\n";
    std::cout << "  --join               Perform Block Nested Loops Join\n";
    std::cout << "      --outer-table FILE   Outer table file (block format)\n";
    std::cout << "      --inner-table FILE   Inner table file (block format)\n";
    std::cout << "      --outer-type TYPE    Outer table type (PART or PARTSUPP)\n";
    std::cout << "      --inner-type TYPE    Inner table type (PART or PARTSUPP)\n";
    std::cout << "      --output FILE        Output file path\n";
    std::cout << "      --buffer-size NUM    Number of buffer blocks (default: 10)\n";
    std::cout << "      --block-size SIZE    Block size in bytes (default: 4096)\n\n";
    std::cout << "Examples:\n";
    std::cout << "  # Convert PART CSV to block format\n";
    std::cout << "  " << program_name << " --convert-csv --csv-file data/part.tbl \\\n";
    std::cout << "      --block-file data/part.dat --table-type PART\n\n";
    std::cout << "  # Perform join with buffer size of 20 blocks\n";
    std::cout << "  " << program_name << " --join --outer-table data/part.dat \\\n";
    std::cout << "      --inner-table data/partsupp.dat --outer-type PART \\\n";
    std::cout << "      --inner-type PARTSUPP --output output/result.dat \\\n";
    std::cout << "      --buffer-size 20\n";
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
        size_t buffer_size = 10;
        size_t block_size = DEFAULT_BLOCK_SIZE;

        // 인자 파싱
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "--convert-csv") {
                mode = "convert";
            } else if (arg == "--join") {
                mode = "join";
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
        else {
            std::cerr << "Error: Please specify either --convert-csv or --join\n";
            printUsage(argv[0]);
            return 1;
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
