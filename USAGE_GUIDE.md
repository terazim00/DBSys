# TPC-H Block Nested Loops Join - 사용 가이드

## 🚀 빠른 시작

### 1. 프로젝트 빌드

```bash
# 메인 프로그램 빌드
make

# 또는 최적화 빌드
make clean && make

# 디버그 모드 빌드
make debug
```

### 2. CSV 파일을 블록 파일로 변환

TPC-H 데이터를 사용하기 전에 먼저 블록 형식으로 변환해야 합니다.

```bash
# PART 테이블 변환
./dbsys --convert-csv \
  --csv-file data/part.tbl \
  --block-file data/part.dat \
  --table-type PART

# PARTSUPP 테이블 변환
./dbsys --convert-csv \
  --csv-file data/partsupp.tbl \
  --block-file data/partsupp.dat \
  --table-type PARTSUPP
```

### 3. Block Nested Loops Join 실행

```bash
./dbsys --join \
  --outer-table data/part.dat \
  --inner-table data/partsupp.dat \
  --outer-type PART \
  --inner-type PARTSUPP \
  --output output/result.dat \
  --buffer-size 20
```

---

## 📋 명령줄 옵션 상세 설명

### CSV 변환 옵션

| 옵션 | 설명 | 필수 | 기본값 |
|-----|------|-----|--------|
| `--convert-csv` | CSV 변환 모드 활성화 | ✅ | - |
| `--csv-file FILE` | 입력 CSV 파일 경로 | ✅ | - |
| `--block-file FILE` | 출력 블록 파일 경로 | ✅ | - |
| `--table-type TYPE` | 테이블 타입 (PART/PARTSUPP) | ✅ | - |
| `--block-size SIZE` | 블록 크기 (바이트) | ❌ | 4096 |

**예제**:
```bash
./dbsys --convert-csv \
  --csv-file data/part.tbl \
  --block-file data/part.dat \
  --table-type PART \
  --block-size 8192  # 8KB 블록
```

### Join 실행 옵션

| 옵션 | 설명 | 필수 | 기본값 |
|-----|------|-----|--------|
| `--join` | Join 모드 활성화 | ✅ | - |
| `--outer-table FILE` | Outer 테이블 파일 (블록 형식) | ✅ | - |
| `--inner-table FILE` | Inner 테이블 파일 (블록 형식) | ✅ | - |
| `--outer-type TYPE` | Outer 테이블 타입 | ✅ | - |
| `--inner-type TYPE` | Inner 테이블 타입 | ✅ | - |
| `--output FILE` | 출력 파일 경로 | ✅ | - |
| `--buffer-size NUM` | 버퍼 블록 개수 | ❌ | 10 |
| `--block-size SIZE` | 블록 크기 (바이트) | ❌ | 4096 |

**예제**:
```bash
./dbsys --join \
  --outer-table data/part.dat \
  --inner-table data/partsupp.dat \
  --outer-type PART \
  --inner-type PARTSUPP \
  --output output/result.dat \
  --buffer-size 50 \
  --block-size 8192
```

---

## 🛠️ 편의 스크립트

프로젝트에는 더 편리한 사용을 위한 스크립트가 포함되어 있습니다.

### 1. Join 실행 스크립트 (`scripts/run_join.sh`)

간단하게 조인을 실행할 수 있습니다.

```bash
# 기본 파일 경로로 실행
./scripts/run_join.sh

# 커스텀 파일 경로
./scripts/run_join.sh data/part.dat data/partsupp.dat output/myresult.dat 20

# 인자 설명:
# $1: PART 파일 경로 (기본: data/part.dat)
# $2: PARTSUPP 파일 경로 (기본: data/partsupp.dat)
# $3: 출력 파일 경로 (기본: output/result.dat)
# $4: 버퍼 크기 (기본: 10)
```

**출력 예시**:
```
========================================
  Block Nested Loops Join Runner
========================================

Configuration:
  PART File:     data/part.dat
  PARTSUPP File: data/partsupp.dat
  Output File:   output/result.dat
  Buffer Size:   20 blocks
  Block Size:    4096 bytes
  Memory:        0.08 MB

Executing join...

=== Block Nested Loops Join ===
...
=== Join Statistics ===
Block Reads: 780
Block Writes: 50
Output Records: 1900
Elapsed Time: 0.234 seconds
Memory Usage: 81920 bytes (0.078 MB)

========================================
  Join completed successfully!
========================================

Output file information:
  Path: output/result.dat
  Size: 156K
```

### 2. 벤치마크 스크립트 (`scripts/benchmark_join.sh`)

다양한 버퍼 크기로 성능을 비교합니다.

```bash
# 기본 파일로 벤치마크
./scripts/benchmark_join.sh

# 커스텀 파일로 벤치마크
./scripts/benchmark_join.sh data/part.dat data/partsupp.dat
```

**출력 예시**:
```
========================================
  Block Nested Loops Join Benchmark
========================================

Starting benchmark...

┌──────────────┬──────────────┬──────────────┬──────────────┬──────────────┐
│ Buffer Size  │ Block Reads  │ Block Writes │    Time (s)  │  Memory (KB) │
├──────────────┼──────────────┼──────────────┼──────────────┼──────────────┤
│ 3            │ 1280800      │ 50           │ 1.456        │ 12.00        │
│ 5            │ 640800       │ 50           │ 0.789        │ 20.00        │
│ 10           │ 285600       │ 50           │ 0.345        │ 40.00        │
│ 20           │ 135200       │ 50           │ 0.178        │ 80.00        │
│ 50           │ 52000        │ 50           │ 0.089        │ 200.00       │
└──────────────┴──────────────┴──────────────┴──────────────┴──────────────┘

========================================
  Benchmark completed!
========================================

Results saved to:
  - output/benchmark/benchmark_results.txt
  - output/benchmark/benchmark_results.csv

Performance Analysis:
  Buffer size 3 → 50: 93.9% faster
```

결과는 다음 형식으로 저장됩니다:
- **TXT 파일**: 상세한 텍스트 보고서
- **CSV 파일**: 그래프 작성용 데이터

---

## 📊 실행 결과 해석

### Join Statistics 설명

```
=== Join Statistics ===
Block Reads: 780           # 읽은 블록 수 (I/O 카운트)
Block Writes: 50           # 쓴 블록 수 (출력 I/O)
Output Records: 1900       # 조인 결과 레코드 수
Elapsed Time: 0.234 seconds  # 전체 실행 시간
Memory Usage: 81920 bytes (0.078 MB)  # 사용한 메모리
```

### 성능 분석

#### **Block Reads (I/O 복잡도)**

```
Block Reads = |R| + (|R| / (B-1)) × |S|
```

- `|R|`: Outer 테이블 블록 수
- `|S|`: Inner 테이블 블록 수
- `B`: 버퍼 크기

**예시** (PART 800블록, PARTSUPP 3200블록, 버퍼 20):
```
Block Reads = 800 + (800 / 19) × 3200
            = 800 + 42 × 3200
            = 800 + 134,400
            = 135,200
```

#### **Memory Usage**

```
Memory = Buffer Size × Block Size
```

**예시**:
- 버퍼 10개 × 4KB = **40 KB**
- 버퍼 50개 × 4KB = **200 KB**

#### **성능 개선율**

버퍼 크기를 늘리면 I/O가 획기적으로 감소합니다:

| 버퍼 크기 | Block Reads | 개선율 |
|----------|-------------|--------|
| 3 | 1,280,800 | - |
| 5 | 640,800 | **50%** ⬇ |
| 10 | 285,600 | **78%** ⬇ |
| 20 | 135,200 | **89%** ⬇ |
| 50 | 52,000 | **96%** ⬇ |

---

## 🎯 일반적인 사용 시나리오

### 시나리오 1: 소규모 테스트

```bash
# 1. 샘플 데이터 생성 (join_demo 사용)
./join_demo

# 2. 조인 실행
./scripts/run_join.sh data/part_sample.dat data/partsupp_sample.dat
```

### 시나리오 2: 전체 TPC-H 데이터

```bash
# 1. CSV 변환
./dbsys --convert-csv --csv-file data/part.tbl \
  --block-file data/part.dat --table-type PART

./dbsys --convert-csv --csv-file data/partsupp.tbl \
  --block-file data/partsupp.dat --table-type PARTSUPP

# 2. 조인 실행 (큰 버퍼)
./scripts/run_join.sh data/part.dat data/partsupp.dat output/result.dat 100
```

### 시나리오 3: 성능 분석

```bash
# 1. 벤치마크 실행
./scripts/benchmark_join.sh

# 2. 결과 확인
cat output/benchmark/benchmark_results.txt

# 3. CSV를 스프레드시트나 Python으로 시각화
# output/benchmark/benchmark_results.csv
```

---

## 🔧 트러블슈팅

### 문제 1: "dbsys not found"

**원인**: 프로그램이 빌드되지 않음

**해결**:
```bash
make clean && make
```

### 문제 2: "Failed to open file"

**원인**: 입력 파일이 존재하지 않음

**해결**:
```bash
# 파일 경로 확인
ls -la data/

# CSV 변환이 필요한 경우
./dbsys --convert-csv --csv-file data/part.tbl \
  --block-file data/part.dat --table-type PART
```

### 문제 3: 느린 성능

**원인**: 버퍼 크기가 너무 작음

**해결**:
```bash
# 버퍼 크기 증가
./dbsys --join ... --buffer-size 50  # 10 → 50

# 또는 블록 크기 증가
./dbsys --join ... --block-size 8192  # 4KB → 8KB
```

### 문제 4: 메모리 부족

**원인**: 버퍼 크기가 너무 큼

**해결**:
```bash
# 버퍼 크기 감소
./dbsys --join ... --buffer-size 5  # 10 → 5

# 또는 블록 크기 감소
./dbsys --join ... --block-size 2048  # 4KB → 2KB
```

---

## 📈 성능 최적화 팁

### 1. 적절한 버퍼 크기 선택

**메모리가 충분한 경우**:
```bash
--buffer-size 50  # 200 KB 메모리
```

**메모리가 제한적인 경우**:
```bash
--buffer-size 5   # 20 KB 메모리
```

### 2. 작은 테이블을 Outer로 선택

```bash
# ✅ 좋은 방법: PART (작음) → Outer
./dbsys --join --outer-table data/part.dat \
  --inner-table data/partsupp.dat

# ❌ 나쁜 방법: PARTSUPP (큼) → Outer
./dbsys --join --outer-table data/partsupp.dat \
  --inner-table data/part.dat
```

### 3. 블록 크기 조정

**큰 레코드 (PART)**:
```bash
--block-size 8192  # 8KB
```

**작은 레코드 (PARTSUPP)**:
```bash
--block-size 4096  # 4KB (기본값)
```

---

## 🧪 예제 프로그램

### 1. simple_usage (기본 파일 I/O)

```bash
make examples
./simple_usage
```

**기능**:
- CSV → 블록 파일 변환
- 블록 파일 읽기
- 데이터 필터링 및 저장
- 통계 출력

### 2. file_manager_example (고급 패턴)

```bash
./file_manager_example
```

**기능**:
- 6가지 사용 패턴 데모
- 에러 처리 예제
- 전체 워크플로우

### 3. join_demo (Join 데모)

```bash
# 전체 데모
./join_demo

# 성능 비교
./join_demo --compare

# 결과 검증
./join_demo --verify
```

**기능**:
- 샘플 데이터 생성
- 조인 실행
- 결과 검증
- 버퍼 크기별 성능 비교

---

## 📚 추가 자료

| 문서 | 설명 |
|-----|------|
| **README.md** | 프로젝트 전체 개요 |
| **BLOCK_NESTED_LOOPS_JOIN.md** | 알고리즘 상세 설명 (800줄) |
| **FILE_MANAGER_GUIDE.md** | FileManager API 가이드 |
| **USAGE_GUIDE.md** | 이 문서 |

---

## 🎓 학습 가이드

### 단계 1: 기본 실행

```bash
make
./join_demo
```

### 단계 2: 실제 데이터 사용

```bash
# CSV 변환
./dbsys --convert-csv --csv-file data/part.tbl \
  --block-file data/part.dat --table-type PART

# 조인 실행
./scripts/run_join.sh
```

### 단계 3: 성능 분석

```bash
./scripts/benchmark_join.sh
cat output/benchmark/benchmark_results.txt
```

### 단계 4: 코드 이해

- `src/join.cpp` - 알고리즘 구현 (상세한 주석)
- `BLOCK_NESTED_LOOPS_JOIN.md` - 알고리즘 이론
- `examples/join_demo.cpp` - 실전 예제

---

## 🔍 자주 묻는 질문 (FAQ)

**Q: 버퍼 크기를 얼마로 설정해야 하나요?**

A:
- 개발/테스트: 10-20
- 소규모 데이터: 20-50
- 대규모 데이터: 50-100

**Q: 조인 결과가 너무 오래 걸립니다.**

A: 버퍼 크기를 늘리거나 작은 테이블을 Outer로 선택하세요.

**Q: 출력 파일 형식은 무엇인가요?**

A: 블록 형식의 바이너리 파일입니다. `FileManager`로 읽을 수 있습니다.

**Q: CSV 파일이 없습니다.**

A: `join_demo`로 샘플 데이터를 생성하거나 TPC-H 도구를 사용하세요.

---

**작성일**: 2025
**버전**: 1.0
**프로젝트**: TPC-H Block Nested Loops Join
