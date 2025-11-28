#include "../include/file_manager.h"
#include <iostream>

/**
 * FileManager 간단 사용 예제
 *
 * 이 예제는 FileManager 클래스를 사용하여:
 * 1. CSV를 블록 파일로 변환
 * 2. 블록 파일 읽기
 * 3. 데이터 필터링 및 쓰기
 * 4. 통계 출력
 */

int main() {
    try {
        // ====================================================================
        // 1. FileManager 생성
        // ====================================================================
        std::cout << "=== Creating FileManager ===" << std::endl;

        // 4KB 블록, 10개 버퍼
        FileManager fm(4096, 10);

        std::cout << "Block Size: " << fm.getBlockSize() << " bytes" << std::endl;
        std::cout << "Buffer Count: " << fm.getBufferCount() << std::endl;
        std::cout << "Memory Usage: " << (fm.getMemoryUsage() / 1024.0) << " KB\n" << std::endl;

        // ====================================================================
        // 2. CSV를 블록 파일로 변환
        // ====================================================================
        std::cout << "=== Converting CSV to Block File ===" << std::endl;

        size_t converted = fm.convertCSV(
            "data/part.tbl",      // CSV 파일
            "data/part.dat",      // 블록 파일
            "PART"                // 테이블 타입
        );

        std::cout << "Converted " << converted << " records\n" << std::endl;

        // ====================================================================
        // 3. 블록 파일 정보 출력
        // ====================================================================
        fm.printFileInfo("data/part.dat");

        // ====================================================================
        // 4. 블록 파일 읽기 (간단한 방법)
        // ====================================================================
        std::cout << "\n=== Reading Records (First 5) ===" << std::endl;

        int count = 0;
        fm.readPartRecords("data/part.dat", [&count](const PartRecord& part) {
            if (count < 5) {
                std::cout << "Record " << (count + 1) << ":" << std::endl;
                std::cout << "  PARTKEY: " << part.partkey << std::endl;
                std::cout << "  NAME: " << part.name << std::endl;
                std::cout << "  SIZE: " << part.size << std::endl;
                std::cout << "  PRICE: " << part.retailprice << std::endl;
                std::cout << std::endl;
            }
            count++;
        });

        std::cout << "Total records read: " << count << "\n" << std::endl;

        // ====================================================================
        // 5. 데이터 필터링 및 새 파일로 저장
        // ====================================================================
        std::cout << "=== Filtering Records (SIZE > 30) ===" << std::endl;

        std::vector<PartRecord> filtered_records;

        fm.readPartRecords("data/part.dat", [&filtered_records](const PartRecord& part) {
            if (part.size > 30) {
                filtered_records.push_back(part);
            }
        });

        std::cout << "Found " << filtered_records.size() << " records matching criteria" << std::endl;

        // 필터링된 레코드를 새 파일로 저장
        size_t written = fm.writePartRecords("output/filtered_parts.dat", filtered_records);
        std::cout << "Wrote " << written << " records to output file\n" << std::endl;

        // ====================================================================
        // 6. 통계 출력
        // ====================================================================
        fm.printStatistics();

        std::cout << "\n=== Success! ===" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }
}
