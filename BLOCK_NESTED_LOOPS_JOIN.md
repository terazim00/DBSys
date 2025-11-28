# Block Nested Loops Join 구현 가이드

## 📚 개요

**Block Nested Loops Join (BNLJ)**은 두 테이블을 조인하는 전통적인 알고리즘으로, 제한된 메모리 환경에서 효율적으로 동작합니다. 이 구현은 TPC-H PART와 PARTSUPP 테이블을 PARTKEY로 조인합니다.

## 🎯 알고리즘 핵심 아이디어

### 기본 개념

```
┌─────────────────────────────────────────────────┐
│         Buffer Pool (B blocks)                  │
├─────────────────────────────────────────────────┤
│ Outer Blocks (B-1)    │  Inner Block (1)        │
│ [Block 0][Block 1]... │  [Block B-1]            │
└─────────────────────────────────────────────────┘

Algorithm:
for each chunk of (B-1) blocks from R (outer):
    load (B-1) blocks into buffer
    for each block from S (inner):
        load 1 block into buffer
        for each record r in outer blocks:
            for each record s in inner block:
                if r.key == s.key:
                    output <r, s>
```

### 왜 "Block" Nested Loops인가?

| 비교 | Simple Nested Loops | **Block Nested Loops** |
|-----|-------------------|----------------------|
| **Outer 단위** | 1개 레코드 | **(B-1)개 블록** |
| **Inner 스캔 횟수** | \|R\| 레코드 | \|R\| / (B-1) 블록 |
| **I/O 복잡도** | \|R\| + \|R\| × \|S\| | \|R\| + (\|R\| / (B-1)) × \|S\| |
| **성능** | ❌ 매우 느림 | ✅ **훨씬 빠름** |

**핵심**: Outer 테이블을 여러 블록 단위로 읽으면 Inner 테이블 스캔 횟수가 **획기적으로 감소**합니다!

## 🏗️ 클래스 구조

### **BlockNestedLoopsJoin 클래스** (`include/join.h`)

```cpp
class BlockNestedLoopsJoin {
private:
    // 파일 경로
    std::string outer_table_file;    // Outer 테이블 (예: part.dat)
    std::string inner_table_file;    // Inner 테이블 (예: partsupp.dat)
    std::string output_file;         // 결과 파일

    // 테이블 타입
    std::string outer_table_type;    // "PART" 또는 "PARTSUPP"
    std::string inner_table_type;    // "PART" 또는 "PARTSUPP"

    // 성능 파라미터
    size_t buffer_size;              // 버퍼 블록 개수 (기본: 10)
    size_t block_size;               // 블록 크기 (기본: 4096)

    // 통계
    Statistics stats;                // I/O 카운트, 시간 측정

    // 내부 메서드
    void performJoin();
    void joinPartAndPartSupp(...);

public:
    BlockNestedLoopsJoin(...);
    void execute();                  // 조인 실행 + 통계 출력
    const Statistics& getStatistics() const;
};
```

### **JoinResultRecord 구조체** (`include/table.h`)

```cpp
struct JoinResultRecord {
    PartRecord part;           // PART 테이블 레코드
    PartSuppRecord partsupp;   // PARTSUPP 테이블 레코드

    // Record로 변환 (직렬화)
    Record toRecord() const {
        std::vector<std::string> fields;

        // PART 필드 (9개)
        fields.push_back(std::to_string(part.partkey));
        fields.push_back(part.name);
        fields.push_back(part.mfgr);
        fields.push_back(part.brand);
        fields.push_back(part.type);
        fields.push_back(std::to_string(part.size));
        fields.push_back(part.container);
        fields.push_back(std::to_string(part.retailprice));
        fields.push_back(part.comment);

        // PARTSUPP 필드 (5개)
        fields.push_back(std::to_string(partsupp.partkey));
        fields.push_back(std::to_string(partsupp.suppkey));
        fields.push_back(std::to_string(partsupp.availqty));
        fields.push_back(std::to_string(partsupp.supplycost));
        fields.push_back(partsupp.comment);

        return Record(fields);  // 총 14개 필드
    }
};
```

## 🔄 알고리즘 단계별 설명

### **단계 0: 초기화** (`execute()`)

```cpp
void BlockNestedLoopsJoin::execute() {
    // 1. 시작 시간 기록
    auto start_time = std::chrono::high_resolution_clock::now();

    // 2. 조인 수행
    performJoin();

    // 3. 종료 시간 및 통계 계산
    auto end_time = std::chrono::high_resolution_clock::now();
    stats.elapsed_time = (end_time - start_time).count();
    stats.memory_usage = buffer_size * block_size;

    // 4. 통계 출력
    printStatistics();
}
```

### **단계 1: 파일 및 버퍼 초기화** (`performJoin()`)

```cpp
void BlockNestedLoopsJoin::performJoin() {
    // 테이블 리더/라이터 생성
    TableReader outer_reader(outer_table_file, block_size, &stats);
    TableReader inner_reader(inner_table_file, block_size, &stats);
    TableWriter writer(output_file, &stats);

    // 버퍼 풀 생성 (B개 블록)
    BufferManager buffer_mgr(buffer_size, block_size);

    // 조인 수행
    joinPartAndPartSupp(outer_reader, inner_reader, writer, buffer_mgr, ...);
}
```

### **단계 2: 버퍼 할당 전략** (`joinPartAndPartSupp()`)

```cpp
// 총 B개 버퍼를 다음과 같이 분할:
size_t outer_buffer_count = buffer_size - 1;  // B-1 개 (Outer)
// 나머지 1개는 Inner용

// 예: buffer_size = 10
//   → outer_buffer_count = 9
//   → inner_buffer_count = 1
```

**버퍼 할당 예시 (B=10)**:

```
┌────────────────────────────────────────────────┐
│             Buffer Pool (10 blocks)            │
├────────────────────────────────────────────────┤
│ [0][1][2][3][4][5][6][7][8]    │     [9]      │
│    Outer Blocks (9개)          │ Inner (1개)  │
└────────────────────────────────────────────────┘
```

### **단계 3: Outer 테이블 로드** (외부 루프)

```cpp
while (has_outer_blocks) {
    // ===== 3.1: (B-1)개 블록을 버퍼에 로드 =====
    std::vector<Record> outer_records;
    size_t loaded_blocks = 0;

    for (size_t i = 0; i < outer_buffer_count; ++i) {
        Block* outer_block = buffer_mgr.getBuffer(i);
        outer_block->clear();

        if (outer_reader.readBlock(outer_block)) {
            loaded_blocks++;

            // 블록에서 레코드 추출
            RecordReader reader(outer_block);
            while (reader.hasNext()) {
                outer_records.push_back(reader.readNext());
            }
        } else {
            break;  // 더 이상 블록이 없음
        }
    }

    // ===== 3.2: 종료 조건 확인 =====
    if (loaded_blocks == 0) {
        has_outer_blocks = false;
        break;
    }

    // ... Inner 테이블 스캔 (다음 단계)
}
```

**메모리 사용**:
- Outer 블록들: 버퍼에 저장 (블록 데이터)
- Outer 레코드들: `outer_records` 벡터 (역직렬화된 레코드)

**I/O**: (B-1)개 블록 읽기 → 통계 카운터 증가

### **단계 4: Inner 테이블 스캔** (내부 루프)

```cpp
// ===== 4.1: Inner 테이블 파일 포인터 리셋 =====
inner_reader.reset();  // 매번 처음부터 읽기

Block* inner_block = buffer_mgr.getBuffer(buffer_size - 1);

// ===== 4.2: Inner 테이블의 모든 블록 순회 =====
while (inner_reader.readBlock(inner_block)) {

    // Inner 블록에서 레코드 추출
    std::vector<Record> inner_records;
    RecordReader inner_rec_reader(inner_block);

    while (inner_rec_reader.hasNext()) {
        inner_records.push_back(inner_rec_reader.readNext());
    }

    // ... 조인 수행 (다음 단계)

    inner_block->clear();
}
```

**핵심**:
- Outer 블록 청크마다 Inner 테이블을 **완전히 스캔**해야 함
- `inner_reader.reset()`으로 파일 포인터를 처음으로 되돌림

**I/O**: |S| 블록 읽기 (Inner 테이블 전체)

### **단계 5: 레코드 쌍 비교 및 조인** (Nested Loop)

```cpp
// ===== 5.1: 모든 레코드 쌍 비교 =====
for (const auto& outer_rec : outer_records) {
    for (const auto& inner_rec : inner_records) {
        try {
            // ===== 5.2: 타입에 따라 레코드 파싱 =====
            PartRecord part = PartRecord::fromRecord(outer_rec);
            PartSuppRecord partsupp = PartSuppRecord::fromRecord(inner_rec);

            // ===== 5.3: 조인 조건 검사 =====
            if (part.partkey == partsupp.partkey) {

                // ===== 5.4: 조인 결과 생성 =====
                JoinResultRecord result;
                result.part = part;
                result.partsupp = partsupp;

                Record result_rec = result.toRecord();

                // ===== 5.5: 출력 블록에 쓰기 =====
                if (!output_writer.writeRecord(result_rec)) {
                    // 블록이 가득 참 → 디스크에 플러시
                    writer.writeBlock(&output_block);
                    output_block.clear();

                    // 새 블록에 다시 쓰기
                    output_writer.writeRecord(result_rec);
                }

                stats.output_records++;
            }

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}
```

**시간 복잡도**:
- Outer 레코드 개수: O(outer_records.size())
- Inner 레코드 개수: O(inner_records.size())
- **총**: O(outer_records × inner_records) per inner block

### **단계 6: 출력 플러시**

```cpp
// 마지막 출력 블록 플러시
if (!output_block.isEmpty()) {
    writer.writeBlock(&output_block);
}
```

## 📊 성능 분석

### **I/O 복잡도**

| 연산 | 횟수 | 설명 |
|-----|------|------|
| **Outer 테이블 읽기** | \|R\| | 한 번만 순차 읽기 |
| **Inner 테이블 읽기** | (\|R\| / (B-1)) × \|S\| | Outer 청크마다 전체 스캔 |
| **결과 쓰기** | \|Output\| / records_per_block | 조인 결과 크기 의존 |

**총 I/O**: `|R| + (|R| / (B-1)) × |S| + |Output|`

### **실제 예시: TPC-H Scale Factor 0.1**

**데이터 크기**:
- PART 테이블: 20,000 레코드 → ~800 블록 (4KB)
- PARTSUPP 테이블: 80,000 레코드 → ~3,200 블록

**버퍼 크기별 I/O 비교**:

| 버퍼 크기 (B) | Outer 청크 수 | Inner 스캔 횟수 | 총 Block Reads |
|--------------|--------------|---------------|---------------|
| **B=5** | 800/4 = 200 | 200 | 800 + 200×3,200 = **640,800** |
| **B=10** | 800/9 ≈ 89 | 89 | 800 + 89×3,200 = **285,600** |
| **B=20** | 800/19 ≈ 42 | 42 | 800 + 42×3,200 = **135,200** |
| **B=50** | 800/49 ≈ 16 | 16 | 800 + 16×3,200 = **52,000** |
| **B=100** | 800/99 ≈ 8 | 8 | 800 + 8×3,200 = **26,400** |

**결론**: 버퍼 크기를 2배로 늘리면 I/O가 거의 절반으로 감소!

### **시간 복잡도**

**CPU 연산**:
- 레코드 비교: O(|R| × |S|) - 모든 레코드 쌍 비교
- 레코드 파싱: O(|R| + |S|)
- 조인 결과 생성: O(|Output|)

**총 시간**: I/O 시간 + CPU 시간
- **I/O 지배적**: 대부분의 경우 I/O가 병목
- **버퍼 크기 증가 → I/O 감소 → 전체 시간 단축**

### **메모리 사용량**

| 항목 | 크기 | 설명 |
|-----|------|------|
| **버퍼 풀** | B × block_size | 고정 할당 |
| **Outer 레코드** | outer_records.size() × avg_record_size | 가변 |
| **Inner 레코드** | inner_records.size() × avg_record_size | 가변 |
| **출력 블록** | 1 × block_size | 고정 |

**예시 (B=10, block_size=4KB)**:
- 버퍼 풀: 10 × 4KB = **40 KB**
- Outer 레코드 (블록당 ~25개): 9 × 25 × 164B ≈ **37 KB**
- Inner 레코드 (블록당 ~18개): 1 × 18 × 215B ≈ **4 KB**
- **총**: ~81 KB

## 🔧 버퍼 관리 전략

### **BufferManager 클래스**

```cpp
class BufferManager {
private:
    std::vector<std::unique_ptr<Block>> buffers;
    size_t buffer_count;
    size_t block_size;

public:
    BufferManager(size_t num_buffers, size_t blk_size);

    // 버퍼 접근
    Block* getBuffer(size_t idx);

    // 모든 버퍼 초기화
    void clearAll();

    // 메모리 사용량
    size_t getMemoryUsage() const;
};
```

### **버퍼 할당 정책**

```cpp
// 정책 1: 고정 분할
//   - Outer: B-1 블록 (고정)
//   - Inner: 1 블록 (고정)
size_t outer_buffer_count = buffer_size - 1;

// 정책 2: 동적 분할 (미구현)
//   - 테이블 크기에 따라 조정
//   - 작은 테이블을 더 많이 로드
```

### **버퍼 재사용**

```cpp
// Outer 블록들은 청크마다 재사용
for (size_t i = 0; i < outer_buffer_count; ++i) {
    Block* outer_block = buffer_mgr.getBuffer(i);
    outer_block->clear();  // 이전 데이터 제거
    outer_reader.readBlock(outer_block);
}

// Inner 블록은 매 스캔마다 재사용
Block* inner_block = buffer_mgr.getBuffer(buffer_size - 1);
while (inner_reader.readBlock(inner_block)) {
    // 처리...
    inner_block->clear();  // 다음 블록 준비
}
```

## 📈 성능 최적화 팁

### **1. 작은 테이블을 Outer로 선택**

```bash
# PART (20K records) vs PARTSUPP (80K records)
# 선택 1: PART를 Outer로 (권장)
./dbsys --join --outer-table part.dat --inner-table partsupp.dat

# 선택 2: PARTSUPP를 Outer로 (느림)
./dbsys --join --outer-table partsupp.dat --inner-table part.dat
```

**이유**: Outer 청크 수 = |R| / (B-1)가 작을수록 Inner 스캔 횟수 감소

### **2. 버퍼 크기 조정**

```cpp
// 메모리가 충분한 경우
BlockNestedLoopsJoin join(..., 100, 4096);  // 100개 블록 = 400 KB

// 메모리가 제한적인 경우
BlockNestedLoopsJoin join(..., 5, 4096);    // 5개 블록 = 20 KB
```

**권장**:
- **개발/테스트**: 10-20 블록
- **프로덕션 (작은 데이터)**: 50-100 블록
- **프로덕션 (대용량 데이터)**: 100-500 블록

### **3. 블록 크기 조정**

```cpp
// 큰 레코드 (PART)
BlockNestedLoopsJoin join(..., 10, 8192);  // 8KB 블록

// 작은 레코드 (PARTSUPP)
BlockNestedLoopsJoin join(..., 10, 2048);  // 2KB 블록
```

### **4. 출력 버퍼링**

```cpp
// 출력 블록을 버퍼링하여 쓰기 횟수 최소화
if (!output_writer.writeRecord(result_rec)) {
    writer.writeBlock(&output_block);  // 블록이 가득 찰 때만 쓰기
    output_block.clear();
}
```

## 🧪 사용 예제

### **예제 1: 기본 사용**

```cpp
#include "join.h"

int main() {
    // PART × PARTSUPP 조인
    BlockNestedLoopsJoin join(
        "data/part.dat",          // Outer 테이블
        "data/partsupp.dat",      // Inner 테이블
        "output/result.dat",      // 출력 파일
        "PART",                   // Outer 타입
        "PARTSUPP",               // Inner 타입
        10,                       // 버퍼 10개 블록
        4096                      // 블록 4KB
    );

    // 조인 실행
    join.execute();

    // 출력:
    // === Join Statistics ===
    // Block Reads: 4000
    // Block Writes: 500
    // Output Records: 80000
    // Elapsed Time: 2.345 seconds
    // Memory Usage: 40960 bytes (0.039 MB)

    return 0;
}
```

### **예제 2: 커맨드라인 사용**

```bash
# CSV 파일을 블록 파일로 변환
./dbsys --convert-csv --csv-file data/part.tbl \
  --block-file data/part.dat --table-type PART

./dbsys --convert-csv --csv-file data/partsupp.tbl \
  --block-file data/partsupp.dat --table-type PARTSUPP

# Block Nested Loops Join 실행
./dbsys --join \
  --outer-table data/part.dat \
  --inner-table data/partsupp.dat \
  --outer-type PART \
  --inner-type PARTSUPP \
  --output output/result.dat \
  --buffer-size 20 \
  --block-size 4096
```

### **예제 3: 성능 테스트**

```bash
# 다양한 버퍼 크기로 성능 비교
for bufsize in 5 10 20 50 100; do
  echo "Testing with buffer size: $bufsize"
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

**예상 출력**:

```
Testing with buffer size: 5
Block Reads: 640800
Elapsed Time: 8.234 seconds

Testing with buffer size: 10
Block Reads: 285600
Elapsed Time: 3.567 seconds

Testing with buffer size: 20
Block Reads: 135200
Elapsed Time: 1.789 seconds

Testing with buffer size: 50
Block Reads: 52000
Elapsed Time: 0.678 seconds

Testing with buffer size: 100
Block Reads: 26400
Elapsed Time: 0.345 seconds
```

## 🔍 디버깅 및 트러블슈팅

### **문제 1: 메모리 부족**

**증상**:
```
terminate called after throwing an instance of 'std::bad_alloc'
```

**해결**:
```cpp
// 버퍼 크기 줄이기
BlockNestedLoopsJoin join(..., 3, 4096);  // 최소 2개 필요
```

### **문제 2: 느린 성능**

**원인 분석**:
```cpp
// 통계 확인
const Statistics& stats = join.getStatistics();
std::cout << "Block Reads: " << stats.block_reads << std::endl;

// Block Reads가 매우 크면 → 버퍼 크기 증가 필요
```

**해결**:
```cpp
// 버퍼 크기 증가
BlockNestedLoopsJoin join(..., 50, 4096);  // 10 → 50
```

### **문제 3: 잘못된 조인 결과**

**검증**:
```cpp
// 조인 결과 확인
FileManager fm;
fm.readBlockFile("output/result.dat", [](const Record& rec) {
    // 14개 필드 확인 (PART 9개 + PARTSUPP 5개)
    if (rec.getFieldCount() != 14) {
        std::cerr << "Invalid result record!" << std::endl;
    }
});
```

## 📚 참고 자료

### **교과서 알고리즘과의 비교**

```
// 교과서 의사 코드 (Silberschatz et al.)
for each block b_r in R do
    for each block b_s in S do
        for each tuple r in b_r do
            for each tuple s in b_s do
                if r.key = s.key then
                    output(r, s)

// 이 구현의 변형점
1. (B-1)개 블록을 한 번에 로드 (청크 단위 처리)
2. 레코드를 메모리에 추출하여 접근 속도 향상
3. 출력 버퍼링으로 쓰기 횟수 최소화
```

### **관련 알고리즘**

| 알고리즘 | 시간 복잡도 | 메모리 요구 | 적용 시나리오 |
|---------|-----------|-----------|------------|
| **Block Nested Loops** | O((R/(B-1))×S) | B 블록 | 제한된 메모리 |
| **Hash Join** | O(R+S) | R 전체 | 메모리 충분 |
| **Sort-Merge Join** | O(RlogR+SlogS) | 2 블록 | 정렬된 데이터 |
| **Index Nested Loops** | O(R×log(S)) | 1 블록 + 인덱스 | 인덱스 존재 |

### **추가 개선 가능 항목**

1. **인덱스 활용**: PARTKEY에 B-Tree 인덱스 생성
2. **병렬 처리**: 멀티 스레드로 블록 청크 병렬 처리
3. **하이브리드 조인**: 작은 테이블은 해시, 큰 테이블은 BNLJ
4. **Bloom Filter**: 조인 전 필터링으로 불필요한 비교 제거

## 🎓 정리

### **핵심 포인트**

1. ✅ **블록 단위 처리**: (B-1)개 블록을 한 번에 로드하여 Inner 스캔 횟수 감소
2. ✅ **버퍼 관리**: 제한된 메모리로 대용량 조인 가능
3. ✅ **I/O 최적화**: 버퍼 크기에 따라 I/O 획기적 감소
4. ✅ **성능 측정**: 상세한 통계로 튜닝 가능
5. ✅ **확장 가능**: 다양한 테이블 타입 및 조인 조건 지원 가능

### **실무 적용**

- **작은 데이터셋 (<100MB)**: BNLJ 충분히 효율적
- **중간 데이터셋 (100MB-1GB)**: 버퍼 크기 조정 중요
- **대용량 데이터셋 (>1GB)**: Hash Join 또는 Sort-Merge Join 고려

---

**작성자**: Database Systems Course
**날짜**: 2025
**버전**: 1.0
