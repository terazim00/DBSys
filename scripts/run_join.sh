#!/bin/bash

################################################################################
# Block Nested Loops Join 실행 스크립트
#
# 사용법:
#   ./scripts/run_join.sh [PART_FILE] [PARTSUPP_FILE] [OUTPUT_FILE] [BUFFER_SIZE]
#
# 예제:
#   ./scripts/run_join.sh data/part.dat data/partsupp.dat output/result.dat 20
################################################################################

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 기본값 설정
DEFAULT_PART_FILE="data/part.dat"
DEFAULT_PARTSUPP_FILE="data/partsupp.dat"
DEFAULT_OUTPUT_FILE="output/result.dat"
DEFAULT_BUFFER_SIZE=10
DEFAULT_BLOCK_SIZE=4096

# 인자 파싱
PART_FILE="${1:-$DEFAULT_PART_FILE}"
PARTSUPP_FILE="${2:-$DEFAULT_PARTSUPP_FILE}"
OUTPUT_FILE="${3:-$DEFAULT_OUTPUT_FILE}"
BUFFER_SIZE="${4:-$DEFAULT_BUFFER_SIZE}"
BLOCK_SIZE="${5:-$DEFAULT_BLOCK_SIZE}"

# 스크립트 디렉토리 찾기
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
DBSYS_BIN="$PROJECT_DIR/dbsys"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Block Nested Loops Join Runner${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# 프로그램 존재 확인
if [ ! -f "$DBSYS_BIN" ]; then
    echo -e "${RED}Error: dbsys executable not found!${NC}"
    echo -e "${YELLOW}Please run 'make' first to build the program.${NC}"
    exit 1
fi

# 입력 파일 확인
if [ ! -f "$PART_FILE" ]; then
    echo -e "${RED}Error: PART file not found: $PART_FILE${NC}"
    exit 1
fi

if [ ! -f "$PARTSUPP_FILE" ]; then
    echo -e "${RED}Error: PARTSUPP file not found: $PARTSUPP_FILE${NC}"
    exit 1
fi

# 출력 디렉토리 확인 및 생성
OUTPUT_DIR=$(dirname "$OUTPUT_FILE")
if [ ! -d "$OUTPUT_DIR" ]; then
    echo -e "${YELLOW}Creating output directory: $OUTPUT_DIR${NC}"
    mkdir -p "$OUTPUT_DIR"
fi

# 파라미터 출력
echo -e "${GREEN}Configuration:${NC}"
echo "  PART File:     $PART_FILE"
echo "  PARTSUPP File: $PARTSUPP_FILE"
echo "  Output File:   $OUTPUT_FILE"
echo "  Buffer Size:   $BUFFER_SIZE blocks"
echo "  Block Size:    $BLOCK_SIZE bytes"
echo "  Memory:        $(echo "scale=2; $BUFFER_SIZE * $BLOCK_SIZE / 1024 / 1024" | bc) MB"
echo ""

# 조인 실행
echo -e "${GREEN}Executing join...${NC}"
echo ""

"$DBSYS_BIN" --join \
    --outer-table "$PART_FILE" \
    --inner-table "$PARTSUPP_FILE" \
    --outer-type PART \
    --inner-type PARTSUPP \
    --output "$OUTPUT_FILE" \
    --buffer-size "$BUFFER_SIZE" \
    --block-size "$BLOCK_SIZE"

# 결과 확인
if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  Join completed successfully!${NC}"
    echo -e "${GREEN}========================================${NC}"

    # 출력 파일 정보
    if [ -f "$OUTPUT_FILE" ]; then
        FILE_SIZE=$(du -h "$OUTPUT_FILE" | cut -f1)
        echo ""
        echo -e "${BLUE}Output file information:${NC}"
        echo "  Path: $OUTPUT_FILE"
        echo "  Size: $FILE_SIZE"
    fi
else
    echo ""
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}  Join failed!${NC}"
    echo -e "${RED}========================================${NC}"
    exit 1
fi
