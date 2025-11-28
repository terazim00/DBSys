#!/bin/bash

################################################################################
# Block Nested Loops Join 성능 벤치마크 스크립트
#
# 다양한 버퍼 크기로 조인을 실행하여 성능을 비교합니다.
#
# 사용법:
#   ./scripts/benchmark_join.sh [PART_FILE] [PARTSUPP_FILE]
################################################################################

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 기본값 설정
PART_FILE="${1:-data/part.dat}"
PARTSUPP_FILE="${2:-data/partsupp.dat}"
OUTPUT_DIR="output/benchmark"

# 테스트할 버퍼 크기들
BUFFER_SIZES=(3 5 10 20 50)

# 스크립트 디렉토리
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
DBSYS_BIN="$PROJECT_DIR/dbsys"

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Block Nested Loops Join Benchmark${NC}"
echo -e "${CYAN}========================================${NC}"
echo ""

# 프로그램 확인
if [ ! -f "$DBSYS_BIN" ]; then
    echo -e "${RED}Error: dbsys not found! Run 'make' first.${NC}"
    exit 1
fi

# 입력 파일 확인
if [ ! -f "$PART_FILE" ] || [ ! -f "$PARTSUPP_FILE" ]; then
    echo -e "${RED}Error: Input files not found!${NC}"
    exit 1
fi

# 출력 디렉토리 생성
mkdir -p "$OUTPUT_DIR"

# 결과 파일
RESULT_FILE="$OUTPUT_DIR/benchmark_results.txt"
CSV_FILE="$OUTPUT_DIR/benchmark_results.csv"

# 결과 파일 초기화
echo "Block Nested Loops Join Benchmark Results" > "$RESULT_FILE"
echo "Date: $(date)" >> "$RESULT_FILE"
echo "PART File: $PART_FILE" >> "$RESULT_FILE"
echo "PARTSUPP File: $PARTSUPP_FILE" >> "$RESULT_FILE"
echo "" >> "$RESULT_FILE"

# CSV 헤더
echo "BufferSize,BlockReads,BlockWrites,OutputRecords,ElapsedTime,MemoryKB" > "$CSV_FILE"

echo -e "${GREEN}Starting benchmark...${NC}"
echo ""
echo -e "${BLUE}┌──────────────┬──────────────┬──────────────┬──────────────┬──────────────┐${NC}"
echo -e "${BLUE}│ Buffer Size  │ Block Reads  │ Block Writes │    Time (s)  │  Memory (KB) │${NC}"
echo -e "${BLUE}├──────────────┼──────────────┼──────────────┼──────────────┼──────────────┤${NC}"

# 각 버퍼 크기에 대해 테스트
for buffer_size in "${BUFFER_SIZES[@]}"; do
    OUTPUT_FILE="$OUTPUT_DIR/result_buf${buffer_size}.dat"

    # 임시 파일에 출력 저장
    TEMP_OUTPUT=$(mktemp)

    # 조인 실행
    "$DBSYS_BIN" --join \
        --outer-table "$PART_FILE" \
        --inner-table "$PARTSUPP_FILE" \
        --outer-type PART \
        --inner-type PARTSUPP \
        --output "$OUTPUT_FILE" \
        --buffer-size "$buffer_size" \
        2>&1 | tee "$TEMP_OUTPUT" > /dev/null

    # 통계 추출
    BLOCK_READS=$(grep "Block Reads:" "$TEMP_OUTPUT" | awk '{print $3}')
    BLOCK_WRITES=$(grep "Block Writes:" "$TEMP_OUTPUT" | awk '{print $3}')
    OUTPUT_RECORDS=$(grep "Output Records:" "$TEMP_OUTPUT" | awk '{print $3}')
    ELAPSED_TIME=$(grep "Elapsed Time:" "$TEMP_OUTPUT" | awk '{print $3}')
    MEMORY_BYTES=$(grep "Memory Usage:" "$TEMP_OUTPUT" | awk '{print $3}')
    MEMORY_KB=$(echo "scale=2; $MEMORY_BYTES / 1024" | bc)

    # 결과 출력
    printf "${BLUE}│${NC} %-12s ${BLUE}│${NC} %-12s ${BLUE}│${NC} %-12s ${BLUE}│${NC} %-12s ${BLUE}│${NC} %-12s ${BLUE}│${NC}\n" \
        "$buffer_size" "$BLOCK_READS" "$BLOCK_WRITES" "$ELAPSED_TIME" "$MEMORY_KB"

    # 파일에 저장
    echo "Buffer Size: $buffer_size" >> "$RESULT_FILE"
    echo "  Block Reads:    $BLOCK_READS" >> "$RESULT_FILE"
    echo "  Block Writes:   $BLOCK_WRITES" >> "$RESULT_FILE"
    echo "  Output Records: $OUTPUT_RECORDS" >> "$RESULT_FILE"
    echo "  Elapsed Time:   $ELAPSED_TIME seconds" >> "$RESULT_FILE"
    echo "  Memory Usage:   $MEMORY_KB KB" >> "$RESULT_FILE"
    echo "" >> "$RESULT_FILE"

    # CSV에 저장
    echo "$buffer_size,$BLOCK_READS,$BLOCK_WRITES,$OUTPUT_RECORDS,$ELAPSED_TIME,$MEMORY_KB" >> "$CSV_FILE"

    # 임시 파일 삭제
    rm "$TEMP_OUTPUT"
done

echo -e "${BLUE}└──────────────┴──────────────┴──────────────┴──────────────┴──────────────┘${NC}"
echo ""

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  Benchmark completed!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo -e "${YELLOW}Results saved to:${NC}"
echo "  - $RESULT_FILE"
echo "  - $CSV_FILE"
echo ""

# 성능 개선 분석
echo -e "${CYAN}Performance Analysis:${NC}"

# 첫 번째와 마지막 버퍼 크기의 시간 비교
FIRST_TIME=$(grep "^${BUFFER_SIZES[0]}," "$CSV_FILE" | cut -d',' -f5)
LAST_TIME=$(grep "^${BUFFER_SIZES[-1]}," "$CSV_FILE" | cut -d',' -f5)

if [ -n "$FIRST_TIME" ] && [ -n "$LAST_TIME" ]; then
    IMPROVEMENT=$(echo "scale=1; ($FIRST_TIME - $LAST_TIME) / $FIRST_TIME * 100" | bc)
    echo "  Buffer size ${BUFFER_SIZES[0]} → ${BUFFER_SIZES[-1]}: ${IMPROVEMENT}% faster"
fi

echo ""
echo -e "${BLUE}Tip: You can plot the results using the CSV file${NC}"
