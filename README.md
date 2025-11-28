# Block Nested Loops Join Implementation

TPC-H PART와 PARTSUPP 테이블에 대한 Block Nested Loops Join 구현

## 프로젝트 개요

이 프로젝트는 데이터베이스 시스템의 핵심 연산인 Join을 Block Nested Loops 알고리즘으로 구현한 것입니다. C++로 작성되었으며, 고정 크기 블록에 가변 길이 레코드를 저장하는 방식을 사용합니다.

## 주요 기능

- ✅ **고정 크기 블록 관리**: 4KB 기본 블록 크기 (조절 가능)
- ✅ **가변 길이 레코드 지원**: 효율적인 직렬화/역직렬화
- ✅ **Block Nested Loops Join**: 전통적인 조인 알고리즘 구현
- ✅ **버퍼 관리**: 설정 가능한 버퍼 크기로 메모리 효율성 조절
- ✅ **성능 측정**: 수행 시간, I/O 횟수, 메모리 사용량 측정
- ✅ **TPC-H 호환**: PART와 PARTSUPP 테이블 스키마 지원

## 시스템 아키텍처

```
┌─────────────────────────────────────────────────┐
│              Application Layer                  │
│         (main.cpp - CLI Interface)              │
└─────────────────────────────────────────────────┘
                      │
┌─────────────────────────────────────────────────┐
│            Join Execution Layer                 │
│    (BlockNestedLoopsJoin - join.cpp)           │
└─────────────────────────────────────────────────┘
                      │
        ┌─────────────┴─────────────┐
        │                           │
┌───────────────┐          ┌────────────────┐
│ Buffer Manager│          │ Table I/O      │
│ (buffer.cpp)  │          │ (table.cpp)    │
└───────────────┘          └────────────────┘
        │                           │
        │                           │
┌───────────────────────────────────────────┐
│      Block & Record Management            │
│   (block.cpp, record.cpp)                 │
└───────────────────────────────────────────┘
                      │
┌─────────────────────────────────────────────────┐
│              Disk Storage                       │
│        (Block-based Binary Files)               │
└─────────────────────────────────────────────────┘
```

## 디렉토리 구조

```
DBSys/
├── include/              # 헤더 파일
│   ├── common.h         # 공통 타입 및 상수 정의
│   ├── block.h          # 블록 관리
│   ├── record.h         # 레코드 직렬화/역직렬화
│   ├── table.h          # 테이블 스키마 및 I/O
│   ├── buffer.h         # 버퍼 관리
│   └── join.h           # Join 알고리즘
├── src/                 # 구현 파일
│   ├── block.cpp
│   ├── record.cpp
│   ├── table.cpp
│   ├── buffer.cpp
│   ├── join.cpp
│   └── main.cpp
├── data/                # 데이터 파일 (.tbl, .dat)
├── output/              # 결과 파일
├── scripts/             # 유틸리티 스크립트
├── Makefile            # 빌드 스크립트
└── README.md           # 이 문서
```

## 빌드 방법

### 요구사항
- C++11 이상 지원 컴파일러 (g++)
- Make
- Ubuntu/Linux 환경

### 컴파일
```bash
make                # 최적화 빌드
make debug          # 디버그 빌드
make clean          # 빌드 파일 삭제
```

## 사용 방법

### 1. CSV 파일을 블록 형식으로 변환

TPC-H 데이터를 사용하기 전에 CSV(또는 .tbl) 파일을 블록 기반 바이너리 파일로 변환해야 합니다.

```bash
# PART 테이블 변환
./dbsys --convert-csv \
  --csv-file data/part.tbl \
  --block-file data/part.dat \
  --table-type PART \
  --block-size 4096

# PARTSUPP 테이블 변환
./dbsys --convert-csv \
  --csv-file data/partsupp.tbl \
  --block-file data/partsupp.dat \
  --table-type PARTSUPP \
  --block-size 4096
```

### 2. Block Nested Loops Join 실행

```bash
./dbsys --join \
  --outer-table data/part.dat \
  --inner-table data/partsupp.dat \
  --outer-type PART \
  --inner-type PARTSUPP \
  --output output/result.dat \
  --buffer-size 10 \
  --block-size 4096
```

### 3. Join 실행 및 성능 측정

**기본 Join 실행:**
```bash
./dbsys --join \
  --outer-table data/part.dat \
  --inner-table data/partsupp.dat \
  --outer-type PART \
  --inner-type PARTSUPP \
  --output output/result.dat \
  --buffer-size 10
```

**샘플 데이터로 빠른 테스트:**
```bash
./dbsys --join \
  --outer-table data/part_sample.dat \
  --inner-table data/partsupp_sample.dat \
  --outer-type PART \
  --inner-type PARTSUPP \
  --output output/result.dat
```

## 성능 측정 명령어

### 다양한 버퍼 크기 테스트

버퍼 크기가 성능에 미치는 영향을 측정:

```bash
# 버퍼 크기별 테스트 (5, 10, 20, 50, 100 블록)
for bufsize in 5 10 20 50 100; do
  echo "=== Testing with buffer size: $bufsize ==="
  ./dbsys --join \
    --outer-table data/part.dat \
    --inner-table data/partsupp.dat \
    --outer-type PART \
    --inner-type PARTSUPP \
    --output output/result_buf${bufsize}.dat \
    --buffer-size ${bufsize}
  echo ""
done
```

### 블록 크기별 테스트

블록 크기가 성능에 미치는 영향을 측정:

```bash
# 블록 크기별 테스트 (2KB, 4KB, 8KB, 16KB)
for blocksize in 2048 4096 8192 16384; do
  echo "=== Testing with block size: $blocksize ==="

  # 먼저 해당 블록 크기로 데이터 재변환
  ./dbsys --convert-csv \
    --csv-file data/part.tbl \
    --block-file data/part_${blocksize}.dat \
    --table-type PART \
    --block-size ${blocksize}

  ./dbsys --convert-csv \
    --csv-file data/partsupp.tbl \
    --block-file data/partsupp_${blocksize}.dat \
    --table-type PARTSUPP \
    --block-size ${blocksize}

  # Join 실행
  ./dbsys --join \
    --outer-table data/part_${blocksize}.dat \
    --inner-table data/partsupp_${blocksize}.dat \
    --outer-type PART \
    --inner-type PARTSUPP \
    --output output/result_block${blocksize}.dat \
    --buffer-size 10 \
    --block-size ${blocksize}
  echo ""
done
```

### 성능 결과 저장 및 분석

```bash
# 결과를 파일로 저장
./dbsys --join \
  --outer-table data/part.dat \
  --inner-table data/partsupp.dat \
  --outer-type PART \
  --inner-type PARTSUPP \
  --output output/result.dat \
  --buffer-size 10 | tee performance_result.txt

# 여러 설정 테스트 후 결과 비교
echo "Buffer Size,Block Reads,Block Writes,Output Records,Time (s)" > benchmark.csv
for bufsize in 5 10 20 50 100; do
  ./dbsys --join \
    --outer-table data/part.dat \
    --inner-table data/partsupp.dat \
    --outer-type PART \
    --inner-type PARTSUPP \
    --output output/result_buf${bufsize}.dat \
    --buffer-size ${bufsize} | grep -E "Block Reads|Block Writes|Output Records|Elapsed Time" | \
    awk -v bs=$bufsize 'BEGIN{ORS=","} {print $NF} END{print ""}' >> benchmark.csv
done
```

### 실행 시간 측정

```bash
# time 명령어로 전체 실행 시간 측정
time ./dbsys --join \
  --outer-table data/part.dat \
  --inner-table data/partsupp.dat \
  --outer-type PART \
  --inner-type PARTSUPP \
  --output output/result.dat \
  --buffer-size 10

# 반복 실행하여 평균 측정
for i in {1..5}; do
  echo "Run $i:"
  /usr/bin/time -v ./dbsys --join \
    --outer-table data/part.dat \
    --inner-table data/partsupp.dat \
    --outer-type PART \
    --inner-type PARTSUPP \
    --output output/result_run${i}.dat \
    --buffer-size 10 2>&1 | grep "Elapsed"
done
```

## 명령줄 옵션

### CSV 변환 옵션
- `--convert-csv`: CSV 변환 모드 활성화
- `--csv-file FILE`: 입력 CSV 파일 경로
- `--block-file FILE`: 출력 블록 파일 경로
- `--table-type TYPE`: 테이블 타입 (PART 또는 PARTSUPP)
- `--block-size SIZE`: 블록 크기 (바이트, 기본값: 4096)

### Join 실행 옵션
- `--join`: Join 실행 모드 활성화
- `--outer-table FILE`: Outer 테이블 파일 (블록 형식)
- `--inner-table FILE`: Inner 테이블 파일 (블록 형식)
- `--outer-type TYPE`: Outer 테이블 타입
- `--inner-type TYPE`: Inner 테이블 타입
- `--output FILE`: 출력 파일 경로
- `--buffer-size NUM`: 버퍼 블록 개수 (기본값: 10)
- `--block-size SIZE`: 블록 크기 (바이트, 기본값: 4096)

## 구현 세부사항

### 1. 블록 구조

각 블록은 고정 크기(기본 4KB)이며, 다음과 같은 형식으로 여러 레코드를 포함합니다:

```
[Record 1 Size (4B)][Record 1 Data]
[Record 2 Size (4B)][Record 2 Data]
...
[Record N Size (4B)][Record N Data]
[Unused Space]
```

### 2. 레코드 직렬화 형식

가변 길이 레코드는 다음과 같이 직렬화됩니다:

```
[Field 1 Length (2B)][Field 1 Data]
[Field 2 Length (2B)][Field 2 Data]
...
[Field N Length (2B)][Field N Data]
```

### 3. Block Nested Loops Join 알고리즘

```cpp
// 의사 코드
for each set of (buffer_size - 1) blocks from outer table:
    load blocks into buffer
    for each block from inner table:
        load block into buffer
        for each record r in outer blocks:
            for each record s in inner block:
                if r.partkey == s.partkey:
                    output <r, s> to result
```

**시간 복잡도**: O((|R| / (B-1)) * |S|)
- |R|: Outer 테이블 블록 수
- |S|: Inner 테이블 블록 수
- B: 버퍼 크기 (블록 개수)

**I/O 복잡도**: |R| + (|R| / (B-1)) * |S|

### 4. 버퍼 관리

- **총 버퍼**: B개의 블록
- **Outer 테이블용**: B-1 개의 블록
- **Inner 테이블용**: 1개의 블록
- **출력용**: 별도 1개의 블록 (메모리에서 관리)

## 성능 분석

프로그램은 실행 후 다음과 같은 통계를 출력합니다:

```
=== Join Statistics ===
Block Reads: 12345
Block Writes: 567
Output Records: 98765
Elapsed Time: 1.234 seconds
Memory Usage: 40960 bytes (0.039 MB)
```

### 성능에 영향을 미치는 요소

1. **버퍼 크기**:
   - 버퍼가 클수록 Inner 테이블 스캔 횟수 감소
   - 메모리 사용량 증가

2. **블록 크기**:
   - 큰 블록: I/O 횟수 감소, 레코드 처리 오버헤드 증가
   - 작은 블록: I/O 횟수 증가, 레코드 처리 효율적

3. **테이블 크기**:
   - 작은 테이블을 Outer로 선택하면 성능 향상

## TPC-H 테스트 데이터 준비

### 방법 1: TPC-H 데이터 생성

```bash
# TPC-H dbgen 도구 다운로드 및 컴파일
git clone https://github.com/electrum/tpch-dbgen.git
cd tpch-dbgen
make

# 데이터 생성 (Scale Factor에 따라 크기 조절)
./dbgen -s 0.1          # 0.1 = 약 100MB
./dbgen -s 1            # 1 = 약 1GB

# 생성된 .tbl 파일을 DBSys data/ 디렉토리로 복사
cp part.tbl partsupp.tbl ../DBSys/data/
cd ../DBSys
```

**Scale Factor 옵션:**
- `-s 0.01`: 매우 작은 테스트 데이터 (~10MB)
- `-s 0.1`: 소규모 테스트 데이터 (~100MB)
- `-s 1`: 표준 크기 (1GB)
- `-s 10`: 대규모 벤치마크 (10GB)

### 방법 2: 기존 TPC-H 파일 사용

이미 TPC-H .tbl 파일을 가지고 있다면 data/ 디렉토리에 복사:

```bash
cp /path/to/part.tbl data/
cp /path/to/partsupp.tbl data/
```

### 방법 3: 샘플 데이터로 빠른 테스트

포함된 샘플 데이터를 사용하여 바로 테스트 가능:
```bash
# data/part_sample.dat, data/partsupp_sample.dat 파일이 이미 포함되어 있습니다
# 아래 "실행 예제" 섹션 참고
```

## 빠른 시작 가이드

전체 과정을 한눈에:

```bash
# 1. 빌드
make

# 2. TPC-H 데이터가 있다면 data/ 디렉토리에 복사
cp /path/to/part.tbl data/
cp /path/to/partsupp.tbl data/

# 3. CSV → Block 변환
./dbsys --convert-csv --csv-file data/part.tbl --block-file data/part.dat --table-type PART
./dbsys --convert-csv --csv-file data/partsupp.tbl --block-file data/partsupp.dat --table-type PARTSUPP

# 4. Join 실행
./dbsys --join \
  --outer-table data/part.dat \
  --inner-table data/partsupp.dat \
  --outer-type PART \
  --inner-type PARTSUPP \
  --output output/result.dat \
  --buffer-size 10

# 5. 샘플 데이터로 빠른 테스트 (데이터 변환 없이 바로 실행)
./dbsys --join \
  --outer-table data/part_sample.dat \
  --inner-table data/partsupp_sample.dat \
  --outer-type PART \
  --inner-type PARTSUPP \
  --output output/result.dat
```

## 트러블슈팅

### 컴파일 에러
```bash
# C++11 표준 확인
g++ --version

# 명시적으로 C++11 표준 지정
make clean && make CXXFLAGS="-std=c++11 -Iinclude"
```

### output 디렉토리가 없다는 에러
```bash
mkdir -p output
```

### 메모리 부족 에러
```bash
# 버퍼 크기를 줄이기
./dbsys --join ... --buffer-size 5
```

## 라이선스

Educational purposes only.

## 참고 자료

- [TPC-H Benchmark](http://www.tpc.org/tpch/)
- Database System Concepts (Silberschatz, Korth, Sudarshan)
- [Block Nested Loops Join](https://en.wikipedia.org/wiki/Nested_loop_join#Block_nested_loop)

## 저자

Database Systems Course Project
