# Block Nested Loops Join 구현 보고서

**과목명**: 데이터베이스 시스템
**학번**: [학번 입력]
**이름**: [이름 입력]
**제출일**: [날짜 입력]

---

## 목차

1. [개요](#1-개요)
2. [구현 세부사항](#2-구현-세부사항)
3. [성능 분석](#3-성능-분석)
4. [최적화 기법](#4-최적화-기법)
5. [결론](#5-결론)
6. [부록: 소스코드](#6-부록-소스코드)

---

## 1. 개요

### 1.1 프로젝트 목표

TPC-H 벤치마크의 PART와 PARTSUPP 테이블에 대한 Block Nested Loops Join 연산을 C++로 구현하고, 버퍼 크기에 따른 성능 특성을 분석한다.

### 1.2 개발 환경

- **프로그래밍 언어**: C++14
- **컴파일러**: g++ (GCC)
- **운영체제**: Linux
- **빌드 도구**: GNU Make

### 1.3 주요 기능

- 고정 크기 블록에 가변 길이 레코드 저장
- Block Nested Loops Join 알고리즘 구현
- 설정 가능한 버퍼 크기
- 성능 측정 (수행 시간, I/O 횟수, 메모리 사용량)

---

## 2. 구현 세부사항

### 2.1 시스템 아키텍처

```
Application Layer (main.cpp)
         ↓
Join Execution Layer (join.cpp)
         ↓
    ┌────┴────┐
Buffer Mgr  Table I/O
    └────┬────┘
         ↓
Block & Record Management
         ↓
    Disk Storage
```

#### 주요 컴포넌트:
1. **Block Manager**: 고정 크기 블록 관리
2. **Record Manager**: 가변 길이 레코드 직렬화/역직렬화
3. **Table I/O**: 디스크 읽기/쓰기
4. **Buffer Manager**: 메모리 버퍼 관리
5. **Join Executor**: Block Nested Loops Join 수행

### 2.2 데이터 구조

#### 2.2.1 블록 구조

각 블록은 고정 크기(기본 4KB)이며 다음과 같은 형식으로 구성됩니다:

```
┌─────────────────────────────────────────────────┐
│ Record 1 Size (4 bytes)                         │
│ Record 1 Data (variable length)                 │
├─────────────────────────────────────────────────┤
│ Record 2 Size (4 bytes)                         │
│ Record 2 Data (variable length)                 │
├─────────────────────────────────────────────────┤
│ ...                                             │
├─────────────────────────────────────────────────┤
│ Record N Size (4 bytes)                         │
│ Record N Data (variable length)                 │
├─────────────────────────────────────────────────┤
│ Unused Space                                    │
└─────────────────────────────────────────────────┘
```

**특징**:
- 고정 크기 블록으로 I/O 효율성 확보
- 가변 길이 레코드 지원으로 저장 공간 효율성 확보
- 각 레코드 앞에 크기 정보를 저장하여 역직렬화 가능

#### 2.2.2 레코드 직렬화

레코드는 다음과 같이 직렬화됩니다:

```
┌──────────────────────────────────────────────┐
│ Field 1 Length (2 bytes)                     │
│ Field 1 Data (variable length)               │
├──────────────────────────────────────────────┤
│ Field 2 Length (2 bytes)                     │
│ Field 2 Data (variable length)               │
├──────────────────────────────────────────────┤
│ ...                                          │
└──────────────────────────────────────────────┘
```

**장점**:
- 가변 길이 문자열 지원
- 스키마 변경에 유연
- 공간 효율적

### 2.3 Block Nested Loops Join 알고리즘

#### 2.3.1 알고리즘 개요

```cpp
// 의사 코드
Input: Outer table R, Inner table S, Buffer with B blocks
Output: Join result

for each set of (B-1) blocks from R:
    Load (B-1) blocks of R into buffer
    for each block from S:
        Load block of S into buffer
        for each tuple r in outer buffer blocks:
            for each tuple s in inner buffer block:
                if r.partkey == s.partkey:
                    output <r, s> to result
```

#### 2.3.2 버퍼 할당 전략

- **총 버퍼**: B개의 블록
- **Outer 테이블용**: B-1개의 블록
- **Inner 테이블용**: 1개의 블록
- **출력 버퍼**: 별도 메모리 공간

#### 2.3.3 복잡도 분석

**시간 복잡도**:
- 최선: O(|R| + |S|) - 한 테이블이 메모리에 완전히 들어가는 경우
- 평균: O((⌈|R|/(B-1)⌉ × |S|) + |R|)
- 최악: O(|R| × |S|) - B=2인 경우

**공간 복잡도**:
- O(B × block_size)

**I/O 복잡도**:
- Block Reads: |R| + ⌈|R|/(B-1)⌉ × |S|
- Block Writes: ⌈|Result|/block_size⌉

여기서:
- |R|: Outer 테이블의 블록 수
- |S|: Inner 테이블의 블록 수
- B: 버퍼 크기 (블록 개수)

### 2.4 주요 클래스 설명

#### Block 클래스
```cpp
class Block {
    char* data;          // 블록 데이터
    size_t block_size;   // 블록 크기
    size_t used_size;    // 사용된 크기

    // 메서드
    bool append(const char* record_data, size_t record_size);
    void clear();
    void setUsedSize(size_t size);
};
```

**역할**: 고정 크기 블록 관리 및 레코드 추가

#### Record 클래스
```cpp
class Record {
    std::vector<std::string> fields;

    std::vector<char> serialize() const;
    static Record deserialize(const char* data, size_t& offset);
    size_t getSerializedSize() const;
};
```

**역할**: 레코드 직렬화/역직렬화

#### BufferManager 클래스
```cpp
class BufferManager {
    std::vector<std::unique_ptr<Block>> buffers;
    size_t buffer_count;

    Block* getBuffer(size_t idx);
    void clearAll();
};
```

**역할**: 메모리 버퍼 풀 관리

#### BlockNestedLoopsJoin 클래스
```cpp
class BlockNestedLoopsJoin {
    void performJoin();
    void joinPartAndPartSupp(...);
    Statistics stats;
};
```

**역할**: Join 연산 실행 및 통계 수집

---

## 3. 성능 분석

### 3.1 실험 환경

- **데이터셋**: TPC-H Scale Factor [입력]
  - PART 테이블: [레코드 수] 레코드, [파일 크기] MB
  - PARTSUPP 테이블: [레코드 수] 레코드, [파일 크기] MB
- **블록 크기**: 4096 bytes
- **테스트 환경**: [CPU, RAM, 디스크 정보 입력]

### 3.2 버퍼 크기에 따른 성능 측정

#### 실험 결과

| 버퍼 크기 (블록) | 메모리 (MB) | Block Reads | Block Writes | 수행 시간 (초) | Output Records |
|-----------------|------------|-------------|--------------|---------------|----------------|
| 2               | 0.008      | [입력]      | [입력]       | [입력]        | [입력]         |
| 5               | 0.020      | [입력]      | [입력]       | [입력]        | [입력]         |
| 10              | 0.039      | [입력]      | [입력]       | [입력]        | [입력]         |
| 20              | 0.078      | [입력]      | [입력]       | [입력]        | [입력]         |
| 50              | 0.195      | [입력]      | [입력]       | [입력]        | [입력]         |
| 100             | 0.391      | [입력]      | [입력]       | [입력]        | [입력]         |

#### 그래프

```
[여기에 그래프 삽입]
- 버퍼 크기 vs 수행 시간
- 버퍼 크기 vs Block Reads
- 버퍼 크기 vs 메모리 사용량
```

### 3.3 성능 분석

#### 3.3.1 I/O 효율성

**관찰 결과**:
- 버퍼 크기가 증가할수록 Inner 테이블 스캔 횟수 감소
- Block Reads = |R| + ⌈|R|/(B-1)⌉ × |S| 공식과 일치
- [구체적인 분석 입력]

**이론적 분석**:
```
버퍼 크기 2:  Block Reads = |R| + (|R|/1) × |S| = |R| + |R|×|S|
버퍼 크기 10: Block Reads = |R| + (|R|/9) × |S|
버퍼 크기 50: Block Reads = |R| + (|R|/49) × |S|
```

#### 3.3.2 수행 시간 분석

**관찰 결과**:
- [분석 입력]
- CPU 시간 vs I/O 대기 시간 비율
- 버퍼 크기 증가에 따른 성능 향상 곡선

**성능 병목 요인**:
1. 디스크 I/O
2. 메모리 접근
3. CPU 연산 (레코드 비교)

#### 3.3.3 메모리 사용량

**트레이드오프**:
- 버퍼 크기 ↑ → I/O ↓, 메모리 사용량 ↑
- 최적 버퍼 크기: [분석 결과 입력]

### 3.4 블록 크기에 따른 성능 (선택 사항)

| 블록 크기 (bytes) | 블록 개수 | Block Reads | 수행 시간 (초) |
|------------------|----------|-------------|---------------|
| 1024             | [입력]   | [입력]      | [입력]        |
| 2048             | [입력]   | [입력]      | [입력]        |
| 4096             | [입력]   | [입력]      | [입력]        |
| 8192             | [입력]   | [입력]      | [입력]        |

**분석**:
- [블록 크기 영향 분석 입력]

---

## 4. 최적화 기법

### 4.1 구현된 최적화

#### 4.1.1 멀티 블록 버퍼링
- Outer 테이블에 여러 블록 할당
- Inner 테이블 스캔 횟수 최소화
- 효과: I/O 횟수를 ⌈|R|/(B-1)⌉배 감소

#### 4.1.2 효율적인 메모리 관리
- C++11 이동 시맨틱 (std::move) 활용
- std::unique_ptr을 통한 자동 메모리 해제
- 메모리 누수 방지

#### 4.1.3 블록 단위 I/O
- 한 번에 4KB 블록 단위로 읽기/쓰기
- 시스템 콜 오버헤드 감소
- 디스크 I/O 효율성 향상

### 4.2 추가 최적화 가능 항목

#### 4.2.1 해시 조인으로 확장
```
예상 복잡도: O(|R| + |S|)
예상 성능 향상: [분석]
```

#### 4.2.2 병렬 처리
- 멀티스레딩을 통한 CPU 활용도 증가
- Outer 블록들을 병렬로 처리
- 예상 성능 향상: [분석]

#### 4.2.3 인덱스 활용
- PARTKEY에 대한 인덱스 생성
- 조인 대신 인덱스 룩업 사용
- 예상 복잡도: O(|R| × log|S|)

#### 4.2.4 SIMD 최적화
- 벡터화 명령어로 여러 비교 동시 수행
- 예상 성능 향상: 2-4배

### 4.3 최적화 효과 비교

| 기법                | 예상 성능 향상 | 구현 복잡도 | 우선순위 |
|---------------------|---------------|------------|---------|
| 멀티 블록 버퍼링      | High          | Low        | ✅ 완료  |
| 효율적 메모리 관리    | Medium        | Low        | ✅ 완료  |
| 블록 단위 I/O        | High          | Low        | ✅ 완료  |
| 해시 조인           | Very High     | Medium     | 🔄 향후  |
| 병렬 처리           | High          | High       | 🔄 향후  |
| 인덱스 활용         | Very High     | Medium     | 🔄 향후  |
| SIMD 최적화         | Medium        | High       | 🔄 향후  |

---

## 5. 결론

### 5.1 구현 결과 요약

본 프로젝트에서는 TPC-H PART와 PARTSUPP 테이블에 대한 Block Nested Loops Join을 성공적으로 구현하였다. 주요 성과는 다음과 같다:

1. **고정 크기 블록에 가변 길이 레코드 저장**:
   - [결과 설명]

2. **Block Nested Loops Join 알고리즘 구현**:
   - [결과 설명]

3. **성능 측정 및 분석**:
   - [결과 설명]

### 5.2 성능 분석 결론

실험 결과, 다음과 같은 결론을 도출하였다:

1. **버퍼 크기의 영향**:
   - [결론 입력]

2. **I/O 최적화**:
   - [결론 입력]

3. **메모리-성능 트레이드오프**:
   - [결론 입력]

### 5.3 학습 내용

이번 프로젝트를 통해 다음을 학습하였다:

1. **데이터베이스 시스템의 내부 구조**:
   - 블록 기반 저장 관리
   - 버퍼 관리 전략
   - 조인 알고리즘 구현

2. **성능 최적화 기법**:
   - I/O 최소화
   - 메모리 효율적 활용
   - 알고리즘 복잡도 분석

3. **시스템 프로그래밍**:
   - C++를 이용한 저수준 I/O
   - 메모리 관리
   - 성능 측정 및 분석

### 5.4 향후 개선 방향

1. **알고리즘 확장**:
   - Hash Join 구현
   - Sort-Merge Join 구현
   - Grace Hash Join 구현

2. **최적화**:
   - 병렬 처리 추가
   - SIMD 명령어 활용
   - 캐시 친화적 데이터 구조

3. **기능 확장**:
   - 다중 조인 조건 지원
   - 다양한 조인 타입 (Left/Right/Full Outer Join)
   - 쿼리 최적화기 통합

---

## 6. 부록: 소스코드

### 6.1 프로젝트 구조
```
DBSys/
├── include/
│   ├── common.h
│   ├── block.h
│   ├── record.h
│   ├── table.h
│   ├── buffer.h
│   └── join.h
├── src/
│   ├── block.cpp
│   ├── record.cpp
│   ├── table.cpp
│   ├── buffer.cpp
│   ├── join.cpp
│   └── main.cpp
├── data/
├── output/
├── scripts/
├── Makefile
└── README.md
```

### 6.2 빌드 및 실행 방법

```bash
# 빌드
make

# CSV를 블록 형식으로 변환
./dbsys --convert-csv --csv-file data/part.tbl \
  --block-file data/part.dat --table-type PART

./dbsys --convert-csv --csv-file data/partsupp.tbl \
  --block-file data/partsupp.dat --table-type PARTSUPP

# Join 실행
./dbsys --join --outer-table data/part.dat \
  --inner-table data/partsupp.dat \
  --outer-type PART --inner-type PARTSUPP \
  --output output/result.dat --buffer-size 10

# 성능 테스트
./scripts/benchmark.sh
```

### 6.3 소스코드

전체 소스코드는 프로젝트 디렉토리에 포함되어 있습니다.

주요 파일:
- **include/join.h**: Join 알고리즘 인터페이스
- **src/join.cpp**: Block Nested Loops Join 구현
- **include/block.h**: 블록 관리 인터페이스
- **src/block.cpp**: 블록 관리 구현
- **include/buffer.h**: 버퍼 관리자 인터페이스
- **src/buffer.cpp**: 버퍼 관리자 구현

---

## 참고 문헌

1. Ramakrishnan, R., & Gehrke, J. (2003). Database Management Systems (3rd ed.). McGraw-Hill.
2. Silberschatz, A., Korth, H. F., & Sudarshan, S. (2019). Database System Concepts (7th ed.). McGraw-Hill.
3. TPC-H Benchmark Specification. http://www.tpc.org/tpch/
4. [추가 참고자료]

---

**제출 파일**:
- 본 보고서 (PDF)
- 소스코드 전문 (압축 파일 또는 GitHub 저장소 링크)
- 실행 결과 로그
- 성능 측정 데이터 및 그래프
