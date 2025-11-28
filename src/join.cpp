#include "join.h"
#include <iostream>
#include <chrono>
#include <vector>

/**
 * ============================================================================
 * Block Nested Loops Join 구현
 * ============================================================================
 *
 * 알고리즘 개요:
 * - Outer 테이블 R을 (B-1)개 블록 단위로 읽음
 * - 각 outer 블록 세트에 대해 inner 테이블 S를 전체 스캔
 * - R.PARTKEY = S.PARTKEY 조건으로 조인
 * - 결과를 출력 블록에 버퍼링 후 디스크에 쓰기
 *
 * 시간 복잡도: O((|R| / (B-1)) × |S|) - R, S는 블록 개수
 * I/O 복잡도: |R| + (|R| / (B-1)) × |S|
 *   - R 읽기: |R| 블록
 *   - S 읽기: (|R| / (B-1)) × |S| 블록 (R의 각 청크마다 S를 전체 스캔)
 *
 * 메모리 사용: B × block_size (기본: 10 × 4KB = 40KB)
 *   - Outer 버퍼: B-1 블록
 *   - Inner 버퍼: 1 블록
 *   - Output 버퍼: 별도 관리
 */

// ============================================================================
// 생성자: 조인 파라미터 초기화 및 검증
// ============================================================================
BlockNestedLoopsJoin::BlockNestedLoopsJoin(
    const std::string& outer_file,
    const std::string& inner_file,
    const std::string& out_file,
    const std::string& outer_type,
    const std::string& inner_type,
    size_t buf_size,
    size_t blk_size)
    : outer_table_file(outer_file),
      inner_table_file(inner_file),
      output_file(out_file),
      outer_table_type(outer_type),
      inner_table_type(inner_type),
      buffer_size(buf_size),
      block_size(blk_size) {

    // 버퍼 크기 검증: 최소 2개 필요 (outer 1개 + inner 1개)
    if (buffer_size < 2) {
        throw std::runtime_error("Buffer size must be at least 2 blocks");
    }
}

// ============================================================================
// 조인 실행 메인 함수: 시간 측정 및 통계 출력
// ============================================================================
void BlockNestedLoopsJoin::execute() {
    // ========== 단계 1: 시작 시간 기록 ==========
    auto start_time = std::chrono::high_resolution_clock::now();

    // ========== 단계 2: 실제 조인 수행 ==========
    performJoin();

    // ========== 단계 3: 종료 시간 기록 및 경과 시간 계산 ==========
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    stats.elapsed_time = elapsed.count();

    // ========== 단계 4: 메모리 사용량 계산 ==========
    // 총 메모리 = 버퍼 개수 × 블록 크기
    stats.memory_usage = buffer_size * block_size;

    // ========== 단계 5: 성능 통계 출력 ==========
    std::cout << "\n=== Join Statistics ===" << std::endl;
    std::cout << "Block Reads: " << stats.block_reads << std::endl;
    std::cout << "Block Writes: " << stats.block_writes << std::endl;
    std::cout << "Output Records: " << stats.output_records << std::endl;
    std::cout << "Elapsed Time: " << stats.elapsed_time << " seconds" << std::endl;
    std::cout << "Memory Usage: " << stats.memory_usage << " bytes ("
              << (stats.memory_usage / 1024.0 / 1024.0) << " MB)" << std::endl;
}

// ============================================================================
// 조인 수행 함수: 테이블 리더/라이터 초기화 및 조인 타입 분기
// ============================================================================
void BlockNestedLoopsJoin::performJoin() {
    // ========== 단계 1: 파일 리더/라이터 생성 ==========
    // 통계 객체를 전달하여 I/O 카운트 자동 추적
    TableReader outer_reader(outer_table_file, block_size, &stats);
    TableReader inner_reader(inner_table_file, block_size, &stats);
    TableWriter writer(output_file, &stats);

    // ========== 단계 2: 버퍼 풀 생성 ==========
    // buffer_size 개의 블록을 사전 할당
    BufferManager buffer_mgr(buffer_size, block_size);

    // ========== 단계 3: 테이블 타입에 따라 조인 수행 ==========
    // PART와 PARTSUPP 조인만 지원
    if ((outer_table_type == "PART" && inner_table_type == "PARTSUPP") ||
        (outer_table_type == "PARTSUPP" && inner_table_type == "PART")) {

        bool part_is_outer = (outer_table_type == "PART");
        joinPartAndPartSupp(outer_reader, inner_reader, writer, buffer_mgr, part_is_outer);
    } else {
        throw std::runtime_error("Unsupported table types for join");
    }
}

// ============================================================================
// PART와 PARTSUPP 조인 함수: Block Nested Loops Join 알고리즘 구현
// ============================================================================
void BlockNestedLoopsJoin::joinPartAndPartSupp(
    TableReader& outer_reader,
    TableReader& inner_reader,
    TableWriter& writer,
    BufferManager& buffer_mgr,
    bool part_is_outer) {

    // =========================================================================
    // 버퍼 할당 전략
    // =========================================================================
    // 총 버퍼 B개를 다음과 같이 분할:
    //   - Outer 테이블용: B-1 개 (한 번에 여러 블록 로드)
    //   - Inner 테이블용: 1 개 (한 번에 1개 블록만 로드)
    //
    // 이유: Outer 테이블을 많이 로드할수록 Inner 테이블 스캔 횟수 감소
    // =========================================================================
    size_t outer_buffer_count = buffer_size - 1;

    // ========== 출력 블록 초기화 ==========
    // 조인 결과를 버퍼링하여 디스크 쓰기 횟수 최소화
    Block output_block(block_size);
    RecordWriter output_writer(&output_block);

    // =========================================================================
    // Block Nested Loops Join 메인 루프
    // =========================================================================
    bool has_outer_blocks = true;

    // ========== 외부 루프: Outer 테이블을 (B-1)개 블록씩 처리 ==========
    while (has_outer_blocks) {

        // =====================================================================
        // 단계 1: Outer 테이블 블록들을 버퍼에 로드
        // =====================================================================
        std::vector<Record> outer_records;  // 메모리에 레코드 저장
        size_t loaded_blocks = 0;

        // (B-1)개 블록을 순차적으로 읽기
        for (size_t i = 0; i < outer_buffer_count; ++i) {
            Block* outer_block = buffer_mgr.getBuffer(i);
            outer_block->clear();  // 이전 데이터 제거

            // 디스크에서 블록 읽기
            if (outer_reader.readBlock(outer_block)) {
                loaded_blocks++;

                // 블록에서 모든 레코드를 추출하여 메모리에 저장
                RecordReader reader(outer_block);
                while (reader.hasNext()) {
                    outer_records.push_back(reader.readNext());
                }
            } else {
                // 더 이상 읽을 블록이 없으면 종료
                break;
            }
        }

        // 읽은 블록이 없으면 outer 테이블 끝
        if (loaded_blocks == 0) {
            has_outer_blocks = false;
            break;
        }

        std::cout << "Loaded " << loaded_blocks << " outer blocks ("
                  << outer_records.size() << " records)" << std::endl;

        // =====================================================================
        // 단계 2: Inner 테이블을 처음부터 끝까지 스캔
        // =====================================================================
        // 중요: Outer 블록 청크마다 Inner 테이블을 완전히 스캔해야 함
        inner_reader.reset();  // 파일 포인터를 처음으로 되돌림

        // Inner 테이블용 버퍼 (마지막 버퍼 사용)
        Block* inner_block = buffer_mgr.getBuffer(buffer_size - 1);

        size_t inner_blocks_scanned = 0;

        // Inner 테이블의 모든 블록 순회
        while (inner_reader.readBlock(inner_block)) {
            inner_blocks_scanned++;

            // -----------------------------------------------------------------
            // 단계 2.1: Inner 블록에서 레코드 추출
            // -----------------------------------------------------------------
            std::vector<Record> inner_records;
            RecordReader inner_rec_reader(inner_block);

            while (inner_rec_reader.hasNext()) {
                inner_records.push_back(inner_rec_reader.readNext());
            }

            // -----------------------------------------------------------------
            // 단계 2.2: 조인 수행 (Nested Loop)
            // -----------------------------------------------------------------
            // Outer 레코드들 × Inner 레코드들 - 모든 쌍 비교
            for (const auto& outer_rec : outer_records) {
                for (const auto& inner_rec : inner_records) {
                    try {
                        int_t outer_key, inner_key;

                        // =============================================================
                        // 조인 조건 검사 및 결과 생성
                        // =============================================================
                        if (part_is_outer) {
                            // Case 1: PART (outer) × PARTSUPP (inner)
                            PartRecord part = PartRecord::fromRecord(outer_rec);
                            PartSuppRecord partsupp = PartSuppRecord::fromRecord(inner_rec);
                            outer_key = part.partkey;
                            inner_key = partsupp.partkey;

                            // 조인 조건: R.PARTKEY = S.PARTKEY
                            if (outer_key == inner_key) {
                                // 조인 결과 레코드 생성
                                JoinResultRecord result;
                                result.part = part;
                                result.partsupp = partsupp;

                                Record result_rec = result.toRecord();

                                // ---------------------------------------------
                                // 출력 블록에 쓰기 (버퍼링)
                                // ---------------------------------------------
                                if (!output_writer.writeRecord(result_rec)) {
                                    // 블록이 가득 차면 디스크에 플러시
                                    writer.writeBlock(&output_block);
                                    output_block.clear();

                                    // 새 블록에 다시 쓰기
                                    if (!output_writer.writeRecord(result_rec)) {
                                        throw std::runtime_error("Result record too large");
                                    }
                                }

                                stats.output_records++;
                            }
                        } else {
                            // Case 2: PARTSUPP (outer) × PART (inner)
                            PartSuppRecord partsupp = PartSuppRecord::fromRecord(outer_rec);
                            PartRecord part = PartRecord::fromRecord(inner_rec);
                            outer_key = partsupp.partkey;
                            inner_key = part.partkey;

                            // 조인 조건: R.PARTKEY = S.PARTKEY
                            if (outer_key == inner_key) {
                                // 조인 결과 레코드 생성
                                JoinResultRecord result;
                                result.part = part;
                                result.partsupp = partsupp;

                                Record result_rec = result.toRecord();

                                // ---------------------------------------------
                                // 출력 블록에 쓰기 (버퍼링)
                                // ---------------------------------------------
                                if (!output_writer.writeRecord(result_rec)) {
                                    // 블록이 가득 차면 디스크에 플러시
                                    writer.writeBlock(&output_block);
                                    output_block.clear();

                                    // 새 블록에 다시 쓰기
                                    if (!output_writer.writeRecord(result_rec)) {
                                        throw std::runtime_error("Result record too large");
                                    }
                                }

                                stats.output_records++;
                            }
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error during join: " << e.what() << std::endl;
                    }
                }
            }

            // Inner 블록 정리 (다음 블록 준비)
            inner_block->clear();
        }

        std::cout << "Scanned " << inner_blocks_scanned << " inner blocks" << std::endl;
    }

    // =========================================================================
    // 단계 3: 마지막 출력 블록 플러시
    // =========================================================================
    // 버퍼에 남아있는 레코드들을 디스크에 쓰기
    if (!output_block.isEmpty()) {
        writer.writeBlock(&output_block);
    }

    std::cout << "\nJoin completed!" << std::endl;
}
