#!/bin/bash

# 전체 설정 및 테스트 스크립트
# 프로젝트 빌드부터 데이터 생성, 조인 실행까지 모든 과정을 자동화

set -e

echo "======================================"
echo "DBSys - Block Nested Loops Join"
echo "Setup and Test Script"
echo "======================================"
echo ""

# 1. 빌드
echo "Step 1: Building project..."
make clean
make
echo "✓ Build completed"
echo ""

# 2. 샘플 데이터 생성
echo "Step 2: Generating sample data..."
./scripts/generate_sample_data.sh
echo "✓ Sample data generated"
echo ""

# 3. CSV를 블록 형식으로 변환
echo "Step 3: Converting CSV to block format..."

echo "Converting PART table..."
./dbsys --convert-csv \
    --csv-file data/part.tbl \
    --block-file data/part.dat \
    --table-type PART \
    --block-size 4096

echo ""
echo "Converting PARTSUPP table..."
./dbsys --convert-csv \
    --csv-file data/partsupp.tbl \
    --block-file data/partsupp.dat \
    --table-type PARTSUPP \
    --block-size 4096

echo "✓ Conversion completed"
echo ""

# 4. 조인 실행 (샘플)
echo "Step 4: Running sample join..."
./dbsys --join \
    --outer-table data/part.dat \
    --inner-table data/partsupp.dat \
    --outer-type PART \
    --inner-type PARTSUPP \
    --output output/result.dat \
    --buffer-size 10 \
    --block-size 4096

echo "✓ Join completed"
echo ""

# 5. 결과 확인
echo "Step 5: Verifying results..."
if [ -f "output/result.dat" ]; then
    result_size=$(stat -f%z "output/result.dat" 2>/dev/null || stat -c%s "output/result.dat" 2>/dev/null)
    echo "Result file created: output/result.dat"
    echo "Result file size: $result_size bytes"
    echo "✓ Results verified"
else
    echo "✗ Result file not found!"
    exit 1
fi

echo ""
echo "======================================"
echo "Setup and Test Completed Successfully!"
echo "======================================"
echo ""
echo "Next steps:"
echo "  1. Run performance benchmark: ./scripts/benchmark.sh"
echo "  2. Try different buffer sizes: ./dbsys --join ... --buffer-size <N>"
echo "  3. Use your own TPC-H data for larger tests"
echo ""
