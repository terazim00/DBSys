#!/bin/bash

# 성능 벤치마크 스크립트
# 다양한 버퍼 크기와 블록 크기로 조인 성능을 측정합니다.

set -e

PROGRAM="./dbsys"
OUTER_TABLE="data/part.dat"
INNER_TABLE="data/partsupp.dat"
OUTPUT_DIR="output"
RESULTS_FILE="benchmark_results.txt"

# 프로그램 존재 확인
if [ ! -f "$PROGRAM" ]; then
    echo "Error: Program not found. Please run 'make' first."
    exit 1
fi

# 데이터 파일 존재 확인
if [ ! -f "$OUTER_TABLE" ] || [ ! -f "$INNER_TABLE" ]; then
    echo "Error: Data files not found."
    echo "Please convert CSV files to block format first."
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

echo "======================================"
echo "Block Nested Loops Join Benchmark"
echo "======================================"
echo ""
echo "Configuration:"
echo "  Outer Table: $OUTER_TABLE"
echo "  Inner Table: $INNER_TABLE"
echo "  Output Directory: $OUTPUT_DIR"
echo ""
echo "Starting benchmark..."
echo ""

# 결과 파일 헤더
cat > "$RESULTS_FILE" << EOF
Block Nested Loops Join - Performance Benchmark Results
Generated on: $(date)
========================================================

Configuration:
  Outer Table: $OUTER_TABLE (PART)
  Inner Table: $INNER_TABLE (PARTSUPP)

EOF

# 버퍼 크기 테스트
BUFFER_SIZES=(2 5 10 20 50 100)

echo "Test 1: Varying Buffer Size (Block Size = 4096 bytes)"
echo "========================================================" | tee -a "$RESULTS_FILE"
echo "" | tee -a "$RESULTS_FILE"

for bufsize in "${BUFFER_SIZES[@]}"; do
    echo "Testing with buffer size: $bufsize blocks..." | tee -a "$RESULTS_FILE"

    output_file="$OUTPUT_DIR/result_buf${bufsize}.dat"

    # 조인 실행 및 결과 저장
    $PROGRAM --join \
        --outer-table "$OUTER_TABLE" \
        --inner-table "$INNER_TABLE" \
        --outer-type PART \
        --inner-type PARTSUPP \
        --output "$output_file" \
        --buffer-size "$bufsize" \
        --block-size 4096 2>&1 | tee -a "$RESULTS_FILE"

    echo "" | tee -a "$RESULTS_FILE"
    echo "---" | tee -a "$RESULTS_FILE"
    echo "" | tee -a "$RESULTS_FILE"
done

# 블록 크기 테스트
BLOCK_SIZES=(1024 2048 4096 8192 16384)

echo "" | tee -a "$RESULTS_FILE"
echo "Test 2: Varying Block Size (Buffer Size = 10 blocks)"
echo "========================================================" | tee -a "$RESULTS_FILE"
echo "" | tee -a "$RESULTS_FILE"

for blksize in "${BLOCK_SIZES[@]}"; do
    echo "Testing with block size: $blksize bytes..." | tee -a "$RESULTS_FILE"

    # 블록 크기에 따라 데이터 재변환 필요 (여기서는 스킵)
    # 실제 사용시에는 각 블록 크기마다 데이터를 재변환해야 함

    output_file="$OUTPUT_DIR/result_blk${blksize}.dat"

    # 조인 실행 및 결과 저장
    # 주의: 블록 크기가 다르면 데이터 파일도 다시 생성해야 함
    # 여기서는 기본 블록 크기(4096)로 생성된 파일 사용
    if [ "$blksize" -eq 4096 ]; then
        $PROGRAM --join \
            --outer-table "$OUTER_TABLE" \
            --inner-table "$INNER_TABLE" \
            --outer-type PART \
            --inner-type PARTSUPP \
            --output "$output_file" \
            --buffer-size 10 \
            --block-size "$blksize" 2>&1 | tee -a "$RESULTS_FILE"

        echo "" | tee -a "$RESULTS_FILE"
        echo "---" | tee -a "$RESULTS_FILE"
        echo "" | tee -a "$RESULTS_FILE"
    else
        echo "Note: Skipped (requires data re-conversion)" | tee -a "$RESULTS_FILE"
        echo "" | tee -a "$RESULTS_FILE"
    fi
done

echo "" | tee -a "$RESULTS_FILE"
echo "======================================"
echo "Benchmark completed!"
echo "======================================"
echo ""
echo "Results saved to: $RESULTS_FILE"
echo ""

# 결과 요약 생성
echo "Generating summary..."
cat >> "$RESULTS_FILE" << EOF

Performance Summary:
====================

Buffer Size Analysis:
  - Smaller buffer sizes result in more inner table scans
  - Larger buffer sizes reduce I/O but increase memory usage
  - Optimal buffer size depends on available memory and data size

Block Size Analysis:
  - Larger blocks reduce the number of I/O operations
  - Smaller blocks allow finer-grained memory management
  - Trade-off between I/O efficiency and memory utilization

Recommendations:
  1. For large datasets: Use larger buffer sizes (50-100 blocks)
  2. For memory-constrained systems: Use smaller buffer sizes (5-10 blocks)
  3. Balance block size based on typical record size and I/O patterns

EOF

echo "Summary appended to results file."
echo ""
echo "You can view the full results with:"
echo "  cat $RESULTS_FILE"
echo "  or"
echo "  less $RESULTS_FILE"
