#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "common.h"
#include "block.h"
#include "record.h"
#include "table.h"
#include "buffer.h"
#include <string>
#include <memory>
#include <functional>

/**
 * FileManager - TPC-H 데이터 파일 관리를 위한 통합 인터페이스
 *
 * 기능:
 * - TPC-H CSV 파일 읽기 및 블록 파일로 변환
 * - 블록 단위 파일 I/O
 * - 버퍼 풀 관리
 * - 에러 처리
 *
 * 사용 예제:
 *   FileManager fm(4096, 10);  // 4KB 블록, 10개 버퍼
 *   fm.convertCSV("part.tbl", "part.dat", "PART");
 *   fm.readBlockFile("part.dat", [](const PartRecord& record) {
 *       std::cout << record.name << std::endl;
 *   });
 */
class FileManager {
private:
    size_t block_size;         // 블록 크기 (바이트)
    size_t buffer_count;       // 버퍼 개수
    Statistics stats;          // 성능 통계

    std::unique_ptr<BufferManager> buffer_mgr;  // 버퍼 풀 관리자

public:
    /**
     * 생성자
     *
     * @param blk_size 블록 크기 (바이트, 기본값: 4096)
     * @param buf_count 버퍼 개수 (기본값: 10)
     */
    FileManager(size_t blk_size = DEFAULT_BLOCK_SIZE, size_t buf_count = 10);

    ~FileManager() = default;

    // ========================================================================
    // CSV 변환 기능
    // ========================================================================

    /**
     * CSV 파일을 블록 파일로 변환
     *
     * @param csv_file CSV 파일 경로 (.tbl 형식)
     * @param block_file 출력 블록 파일 경로
     * @param table_type 테이블 타입 ("PART" 또는 "PARTSUPP")
     * @return 변환된 레코드 개수
     * @throws std::runtime_error 파일 오류 또는 잘못된 형식
     */
    size_t convertCSV(const std::string& csv_file,
                     const std::string& block_file,
                     const std::string& table_type);

    // ========================================================================
    // 블록 파일 읽기 기능
    // ========================================================================

    /**
     * 블록 파일에서 모든 레코드를 읽어 콜백 함수 실행
     *
     * @param block_file 블록 파일 경로
     * @param callback 각 레코드에 대해 호출될 함수
     * @return 읽은 레코드 개수
     * @throws std::runtime_error 파일 오류
     *
     * 사용 예제:
     *   fm.readBlockFile("part.dat", [](const Record& rec) {
     *       PartRecord part = PartRecord::fromRecord(rec);
     *       std::cout << part.name << std::endl;
     *   });
     */
    size_t readBlockFile(const std::string& block_file,
                        std::function<void(const Record&)> callback);

    /**
     * 블록 파일에서 PART 레코드 읽기 (타입 안전)
     *
     * @param block_file 블록 파일 경로
     * @param callback 각 PART 레코드에 대해 호출될 함수
     * @return 읽은 레코드 개수
     */
    size_t readPartRecords(const std::string& block_file,
                          std::function<void(const PartRecord&)> callback);

    /**
     * 블록 파일에서 PARTSUPP 레코드 읽기 (타입 안전)
     *
     * @param block_file 블록 파일 경로
     * @param callback 각 PARTSUPP 레코드에 대해 호출될 함수
     * @return 읽은 레코드 개수
     */
    size_t readPartSuppRecords(const std::string& block_file,
                              std::function<void(const PartSuppRecord&)> callback);

    // ========================================================================
    // 블록 파일 쓰기 기능
    // ========================================================================

    /**
     * PART 레코드를 블록 파일로 쓰기
     *
     * @param block_file 출력 블록 파일 경로
     * @param records PART 레코드 벡터
     * @return 쓴 레코드 개수
     * @throws std::runtime_error 파일 오류
     */
    size_t writePartRecords(const std::string& block_file,
                           const std::vector<PartRecord>& records);

    /**
     * PARTSUPP 레코드를 블록 파일로 쓰기
     *
     * @param block_file 출력 블록 파일 경로
     * @param records PARTSUPP 레코드 벡터
     * @return 쓴 레코드 개수
     */
    size_t writePartSuppRecords(const std::string& block_file,
                               const std::vector<PartSuppRecord>& records);

    // ========================================================================
    // 버퍼 관리 기능
    // ========================================================================

    /**
     * 버퍼 풀 초기화
     */
    void clearBuffers();

    /**
     * 버퍼 풀 재초기화 (크기 변경)
     *
     * @param new_buffer_count 새로운 버퍼 개수
     */
    void resizeBuffers(size_t new_buffer_count);

    /**
     * 특정 버퍼 접근
     *
     * @param idx 버퍼 인덱스
     * @return 버퍼 포인터
     * @throws std::out_of_range 인덱스 범위 초과
     */
    Block* getBuffer(size_t idx);

    // ========================================================================
    // 통계 및 정보
    // ========================================================================

    /**
     * 성능 통계 가져오기
     */
    const Statistics& getStatistics() const { return stats; }

    /**
     * 통계 초기화
     */
    void resetStatistics();

    /**
     * 통계 출력
     */
    void printStatistics() const;

    /**
     * 블록 크기 가져오기
     */
    size_t getBlockSize() const { return block_size; }

    /**
     * 버퍼 개수 가져오기
     */
    size_t getBufferCount() const { return buffer_count; }

    /**
     * 메모리 사용량 가져오기 (바이트)
     */
    size_t getMemoryUsage() const;

    // ========================================================================
    // 유틸리티 함수
    // ========================================================================

    /**
     * 블록 파일의 레코드 개수 카운트
     *
     * @param block_file 블록 파일 경로
     * @return 레코드 개수
     */
    size_t countRecords(const std::string& block_file);

    /**
     * 블록 파일의 블록 개수 카운트
     *
     * @param block_file 블록 파일 경로
     * @return 블록 개수
     */
    size_t countBlocks(const std::string& block_file);

    /**
     * 블록 파일 정보 출력
     *
     * @param block_file 블록 파일 경로
     */
    void printFileInfo(const std::string& block_file);
};

#endif // FILE_MANAGER_H
