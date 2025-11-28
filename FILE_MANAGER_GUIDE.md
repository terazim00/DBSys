# FileManager 사용 가이드

## 📚 개요

`FileManager`는 TPC-H 데이터 파일을 관리하기 위한 통합 인터페이스입니다. 기존의 여러 클래스(`Block`, `TableReader`, `TableWriter`, `BufferManager`)를 래핑하여 더 간단하고 직관적인 API를 제공합니다.

## 🎯 주요 기능

| 기능 | 설명 | 메서드 |
|-----|------|--------|
| **CSV 변환** | TPC-H .tbl 파일을 블록 파일로 변환 | `convertCSV()` |
| **파일 읽기** | 블록 파일에서 레코드 읽기 | `readPartRecords()`, `readPartSuppRecords()` |
| **파일 쓰기** | 레코드를 블록 파일로 저장 | `writePartRecords()`, `writePartSuppRecords()` |
| **버퍼 관리** | 메모리 버퍼 풀 관리 | `getBuffer()`, `clearBuffers()` |
| **통계** | I/O 성능 측정 | `getStatistics()`, `printStatistics()` |
| **유틸리티** | 파일 정보 조회 | `countRecords()`, `printFileInfo()` |

## 🔧 클래스 구조

### 기본 정보

```cpp
class FileManager {
public:
    // 생성자: 블록 크기, 버퍼 개수 지정
    FileManager(size_t block_size = 4096, size_t buffer_count = 10);

    // CSV 변환
    size_t convertCSV(const std::string& csv_file,
                     const std::string& block_file,
                     const std::string& table_type);

    // 타입 안전 읽기
    size_t readPartRecords(const std::string& block_file,
                          std::function<void(const PartRecord&)> callback);

    // 타입 안전 쓰기
    size_t writePartRecords(const std::string& block_file,
                           const std::vector<PartRecord>& records);

    // 통계 및 정보
    void printStatistics() const;
    void printFileInfo(const std::string& block_file);
};
```

## 📖 기본 사용법

### 1. FileManager 생성

```cpp
#include "file_manager.h"

// 4KB 블록, 10개 버퍼 (기본값)
FileManager fm;

// 또는 커스텀 설정
FileManager fm(8192, 20);  // 8KB 블록, 20개 버퍼
```

**메모리 사용량**: `블록 크기 × 버퍼 개수`
- 4KB × 10 = **40 KB**
- 8KB × 20 = **160 KB**

### 2. CSV 파일을 블록 파일로 변환

```cpp
try {
    // PART 테이블 변환
    size_t count = fm.convertCSV(
        "data/part.tbl",      // 입력 CSV 파일
        "data/part.dat",      // 출력 블록 파일
        "PART"                // 테이블 타입
    );

    std::cout << "Converted " << count << " records" << std::endl;

    // PARTSUPP 테이블 변환
    fm.convertCSV("data/partsupp.tbl", "data/partsupp.dat", "PARTSUPP");

} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

**지원 테이블 타입**:
- `"PART"` - TPC-H PART 테이블
- `"PARTSUPP"` - TPC-H PARTSUPP 테이블

### 3. 블록 파일 읽기

#### 방법 A: 타입 안전 읽기 (권장)

```cpp
// PART 레코드 읽기
fm.readPartRecords("data/part.dat", [](const PartRecord& part) {
    std::cout << "PARTKEY: " << part.partkey << std::endl;
    std::cout << "NAME: " << part.name << std::endl;
    std::cout << "SIZE: " << part.size << std::endl;
});

// PARTSUPP 레코드 읽기
fm.readPartSuppRecords("data/partsupp.dat", [](const PartSuppRecord& ps) {
    std::cout << "PARTKEY: " << ps.partkey << std::endl;
    std::cout << "SUPPKEY: " << ps.suppkey << std::endl;
});
```

#### 방법 B: 범용 레코드 읽기

```cpp
fm.readBlockFile("data/part.dat", [](const Record& record) {
    // 수동으로 파싱
    PartRecord part = PartRecord::fromRecord(record);
    // 처리...
});
```

### 4. 데이터 필터링 및 저장

```cpp
// 조건을 만족하는 레코드 수집
std::vector<PartRecord> filtered;

fm.readPartRecords("data/part.dat", [&filtered](const PartRecord& part) {
    // SIZE가 30보다 큰 레코드만
    if (part.size > 30) {
        filtered.push_back(part);
    }
});

// 필터링된 레코드를 새 파일로 저장
size_t written = fm.writePartRecords("output/large_parts.dat", filtered);
std::cout << "Wrote " << written << " records" << std::endl;
```

### 5. 통계 및 파일 정보

```cpp
// 파일 정보 출력
fm.printFileInfo("data/part.dat");
// 출력:
// === File Information ===
// File: data/part.dat
// File Size: 31.5 MB
// Block Size: 4096 bytes
// Total Blocks: 7890
// Total Records: 200000
// Avg Records/Block: 25.35
// Storage Efficiency: 100.0%

// 성능 통계 출력
fm.printStatistics();
// 출력:
// === FileManager Statistics ===
//      Block Reads: 7890
//     Block Writes: 1234
//   Output Records: 200000
//     Elapsed Time: 1.234 seconds
//      Memory Usage: 0.039 MB

// 레코드 개수만 세기
size_t count = fm.countRecords("data/part.dat");
std::cout << "Total records: " << count << std::endl;
```

## 🎨 고급 사용 패턴

### 패턴 1: 데이터 변환

```cpp
// PART 레코드를 읽어서 수정 후 저장
std::vector<PartRecord> modified_parts;

fm.readPartRecords("data/part.dat", [&modified_parts](const PartRecord& part) {
    PartRecord modified = part;

    // 가격 10% 인상
    modified.retailprice *= 1.1f;

    modified_parts.push_back(modified);
});

// 수정된 데이터 저장
fm.writePartRecords("output/updated_parts.dat", modified_parts);
```

### 패턴 2: 조인 연산

```cpp
// PART와 PARTSUPP를 메모리에서 조인
std::map<int, PartRecord> part_map;
std::vector<JoinResultRecord> join_results;

// 1. PART 레코드를 맵에 저장
fm.readPartRecords("data/part.dat", [&part_map](const PartRecord& part) {
    part_map[part.partkey] = part;
});

// 2. PARTSUPP를 읽으며 조인
fm.readPartSuppRecords("data/partsupp.dat", [&](const PartSuppRecord& ps) {
    auto it = part_map.find(ps.partkey);
    if (it != part_map.end()) {
        JoinResultRecord result;
        result.part = it->second;
        result.partsupp = ps;
        join_results.push_back(result);
    }
});

std::cout << "Join produced " << join_results.size() << " records" << std::endl;
```

### 패턴 3: 집계 연산

```cpp
// 통계 계산
int total_records = 0;
double total_price = 0.0;
int max_size = 0;
std::map<std::string, int> brand_count;

fm.readPartRecords("data/part.dat", [&](const PartRecord& part) {
    total_records++;
    total_price += part.retailprice;
    max_size = std::max(max_size, part.size);
    brand_count[part.brand]++;
});

// 결과 출력
std::cout << "Total Records: " << total_records << std::endl;
std::cout << "Average Price: " << (total_price / total_records) << std::endl;
std::cout << "Max Size: " << max_size << std::endl;
std::cout << "Unique Brands: " << brand_count.size() << std::endl;
```

### 패턴 4: 배치 처리

```cpp
// 대용량 파일을 배치로 처리
const size_t BATCH_SIZE = 1000;
std::vector<PartRecord> batch;

fm.readPartRecords("data/part.dat", [&batch](const PartRecord& part) {
    batch.push_back(part);

    // 배치가 가득 차면 처리
    if (batch.size() >= BATCH_SIZE) {
        processBatch(batch);  // 사용자 정의 함수
        batch.clear();
    }
});

// 남은 레코드 처리
if (!batch.empty()) {
    processBatch(batch);
}
```

### 패턴 5: 버퍼 직접 관리

```cpp
FileManager fm(4096, 5);  // 5개 버퍼

// 버퍼 직접 접근
for (size_t i = 0; i < fm.getBufferCount(); ++i) {
    Block* buffer = fm.getBuffer(i);

    // 버퍼 사용...
    std::cout << "Buffer " << i
              << " size: " << buffer->getSize()
              << " bytes" << std::endl;
}

// 버퍼 초기화
fm.clearBuffers();

// 버퍼 크기 재설정
fm.resizeBuffers(20);  // 20개 버퍼로 확장
```

## ⚠️ 에러 처리

### 1. 파일 오류

```cpp
try {
    fm.convertCSV("nonexistent.tbl", "output.dat", "PART");
} catch (const std::runtime_error& e) {
    // "Failed to open CSV file: nonexistent.tbl"
    std::cerr << "Error: " << e.what() << std::endl;
}
```

### 2. 잘못된 테이블 타입

```cpp
try {
    fm.convertCSV("part.tbl", "output.dat", "INVALID");
} catch (const std::runtime_error& e) {
    // "Unknown table type: INVALID"
    std::cerr << "Error: " << e.what() << std::endl;
}
```

### 3. 블록 크기 초과

```cpp
// 레코드가 블록보다 큰 경우
try {
    FileManager small_fm(512);  // 512 바이트 블록

    std::vector<PartRecord> records;
    PartRecord huge;
    huge.name = std::string(10000, 'X');  // 매우 긴 문자열
    records.push_back(huge);

    small_fm.writePartRecords("output.dat", records);
} catch (const std::runtime_error& e) {
    // "Record too large for block"
    std::cerr << "Error: " << e.what() << std::endl;
}
```

### 4. 버퍼 인덱스 오류

```cpp
try {
    FileManager fm(4096, 5);  // 5개 버퍼 (인덱스 0-4)
    Block* buffer = fm.getBuffer(10);  // 범위 초과
} catch (const std::out_of_range& e) {
    // "Buffer index out of range: 10"
    std::cerr << "Error: " << e.what() << std::endl;
}
```

## 🚀 성능 최적화 팁

### 1. 적절한 버퍼 크기 선택

```cpp
// 메모리가 충분한 경우
FileManager fm_large(4096, 100);  // 400 KB 메모리

// 메모리가 제한적인 경우
FileManager fm_small(4096, 5);   // 20 KB 메모리
```

**권장 사항**:
- **읽기 전용 작업**: 버퍼 크기 10-20
- **조인 연산**: 버퍼 크기 50-100
- **메모리 제한 환경**: 버퍼 크기 3-5

### 2. 블록 크기 조정

```cpp
// 작은 레코드 (PARTSUPP)
FileManager fm_small(2048, 10);   // 2KB 블록

// 큰 레코드 (PART)
FileManager fm_large(8192, 10);   // 8KB 블록
```

**권장 사항**:
- **일반적**: 4096 bytes (4KB)
- **작은 레코드**: 2048 bytes (2KB)
- **큰 레코드**: 8192 bytes (8KB)

### 3. 스트리밍 처리

```cpp
// ❌ 나쁜 방법: 모든 데이터를 메모리에 로드
std::vector<PartRecord> all_records;
fm.readPartRecords("data/part.dat", [&all_records](const PartRecord& part) {
    all_records.push_back(part);  // 메모리 사용량 증가
});

// ✅ 좋은 방법: 스트리밍 처리
fm.readPartRecords("data/part.dat", [](const PartRecord& part) {
    processRecord(part);  // 즉시 처리
    // 레코드를 메모리에 유지하지 않음
});
```

## 📊 실전 예제

### 완전한 데이터 처리 파이프라인

```cpp
#include "file_manager.h"
#include <iostream>
#include <map>

int main() {
    try {
        FileManager fm(4096, 10);

        // 1. CSV 변환
        std::cout << "Step 1: Converting CSV files..." << std::endl;
        fm.convertCSV("data/part.tbl", "data/part.dat", "PART");
        fm.convertCSV("data/partsupp.tbl", "data/partsupp.dat", "PARTSUPP");

        // 2. 데이터 분석
        std::cout << "\nStep 2: Analyzing data..." << std::endl;
        std::map<std::string, int> manufacturer_count;
        double avg_price = 0.0;
        int count = 0;

        fm.readPartRecords("data/part.dat", [&](const PartRecord& part) {
            manufacturer_count[part.mfgr]++;
            avg_price += part.retailprice;
            count++;
        });

        avg_price /= count;

        std::cout << "Total Parts: " << count << std::endl;
        std::cout << "Average Price: $" << avg_price << std::endl;
        std::cout << "Unique Manufacturers: " << manufacturer_count.size() << std::endl;

        // 3. 필터링 및 저장
        std::cout << "\nStep 3: Filtering expensive parts..." << std::endl;
        std::vector<PartRecord> expensive_parts;

        fm.readPartRecords("data/part.dat", [&](const PartRecord& part) {
            if (part.retailprice > avg_price) {
                expensive_parts.push_back(part);
            }
        });

        size_t written = fm.writePartRecords("output/expensive_parts.dat", expensive_parts);
        std::cout << "Saved " << written << " expensive parts" << std::endl;

        // 4. 통계 출력
        std::cout << "\nStep 4: Statistics" << std::endl;
        fm.printStatistics();

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

## 🔗 관련 클래스

`FileManager`는 다음 클래스들을 내부적으로 사용합니다:

- **Block** (block.h): 고정 크기 블록 관리
- **Record** (record.h): 가변 길이 레코드
- **TableReader** (table.h): 블록 파일 읽기
- **TableWriter** (table.h): 블록 파일 쓰기
- **BufferManager** (buffer.h): 버퍼 풀 관리
- **PartRecord** (table.h): PART 테이블 스키마
- **PartSuppRecord** (table.h): PARTSUPP 테이블 스키마

필요한 경우 이들 클래스를 직접 사용할 수도 있습니다.

## 📦 빌드 방법

```bash
# 메인 프로그램 빌드
make

# 예제 프로그램 빌드
make examples

# 예제 실행
./simple_usage
./file_manager_example
```

## 📚 추가 자료

- [README.md](README.md) - 프로젝트 개요
- [examples/simple_usage.cpp](examples/simple_usage.cpp) - 간단한 사용 예제
- [examples/file_manager_example.cpp](examples/file_manager_example.cpp) - 전체 예제 모음

## ❓ FAQ

**Q: CSV 파일이 없으면 어떻게 하나요?**
A: TPC-H 벤치마크 도구로 생성하거나 샘플 데이터를 사용하세요. (README.md 참조)

**Q: 메모리 사용량을 줄이려면?**
A: 버퍼 개수를 줄이세요: `FileManager fm(4096, 3);`

**Q: 더 빠른 성능을 원한다면?**
A: 버퍼 크기를 늘리고 블록 크기를 키우세요: `FileManager fm(8192, 50);`

**Q: 다른 테이블 타입을 추가하려면?**
A: table.h에 새 구조체를 추가하고 fromCSV(), toRecord() 메서드를 구현하세요.
