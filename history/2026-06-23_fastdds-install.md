# 작업 히스토리 — Fast DDS v3.6.1 소스빌드 설치 (RPI5)

- **날짜**: 2026-06-23
- **머신**: Raspberry Pi 5 (BCM2712), Debian 12 (bookworm), aarch64, RAM 8GB / 4코어
- **목적**: DDS 테스트 시나리오(RPI4 Publisher ↔ RPI5 Subscriber)를 위해 이 머신(RPI5, Subscriber 역할)에 Fast DDS 설치
- **구현체/버전**: eProsima Fast DDS **v3.6.1** + Fast-CDR **v2.3.6** + foonathan_memory (master)
- **작업 디렉토리**: `/home/lihan/fastdds_ws`
- **설치 prefix**: `/usr/local`
- **로그 위치**: `/home/lihan/claude/logs/`
- **결과**: ✅ 빌드·설치·동작 검증까지 전부 성공

---

## 타임라인 (시간순)

| 시각(BST) | 단계 | 내용 | 로그 | 결과 |
|-----------|------|------|------|------|
| 02:09 | 0. 환경 파악 | OS/HW/도구/sudo/네트워크/기존설치 확인 | `00_environment.log` | ✅ |
| 02:09:47 ~ 02:10:53 | 1. apt 의존성 | libasio-dev, libtinyxml2-dev, libssl-dev 등 설치 | `01_apt_deps.log` | ✅ |
| 02:11:03 ~ 02:11:09 | 2. 소스 클론 | foonathan_memory / Fast-CDR v2.3.6 / Fast-DDS v3.6.1 | `02_clone.log` | ✅ |
| 02:11:17 ~ 02:11:45 | 3. foonathan_memory | cmake 빌드 + 설치 | `03_foonathan.log` | ✅ |
| 02:11:52 ~ 02:11:55 | 4. Fast-CDR | cmake 빌드 + 설치 + ldconfig | `04_fastcdr.log` | ✅ |
| 02:11:xx ~ 02:19:56 | 5. Fast-DDS | cmake 빌드(백그라운드, 약 8분) + 설치 + ldconfig | `05_fastdds.log` | ✅ |
| 02:20:27 ~ 02:20:31 | 6. 동작 검증 | DomainParticipant 생성 스모크 테스트 | `06_verify.log` | ✅ `OK` |

---

## 의존성 구조 (빌드 순서의 이유)

```
Fast-DDS              ← DDS/RTPS 구현 (Pub/Sub, Discovery, QoS)
  └ Fast-CDR          ← CDR 직렬화
  └ foonathan_memory  ← 고성능 메모리 할당자
  + asio / tinyxml2 / openssl (시스템 라이브러리)
```
아래에서 위로(foonathan → Fast-CDR → Fast-DDS) 순서로 빌드해야 함.
버전 짝: **Fast-DDS 3.x ↔ Fast-CDR 2.x**.

---

## 단계별 상세

### 0. 환경 파악
- `uname -a`, `/etc/os-release`, `nproc`, `free -h`, `df -h`로 확인
- 칩 `BCM2712` → Raspberry Pi 5 확정 (시나리오상 Subscriber 노드)
- gcc/g++ 12.2, cmake 3.25, git, make 기존 존재 / passwordless sudo OK / github 접속 OK
- `ldconfig -p | grep fastdds` → 기존 설치 없음 (신규 빌드 필요)

### 1. apt 의존성
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git \
     libasio-dev libtinyxml2-dev libssl-dev python3-pip
```
- libasio-dev: RTPS UDP 비동기 I/O
- libtinyxml2-dev: QoS XML 프로파일 파싱
- libssl-dev: DDS-Security(OpenSSL)

### 2. 소스 클론
```bash
cd /home/lihan/fastdds_ws
git clone https://github.com/eProsima/foonathan_memory_vendor.git
git clone --branch v2.3.6 --depth 1 https://github.com/eProsima/Fast-CDR.git
git clone --branch v3.6.1 --depth 1 https://github.com/eProsima/Fast-DDS.git
```

### 3. foonathan_memory
```bash
cd foonathan_memory_vendor
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
sudo cmake --install build
```
- 산출물: `/usr/local/lib/libfoonathan_memory-0.7.4.a` + 헤더
- 로그의 `long double` 경고는 GCC ABI 안내(오류 아님)

### 4. Fast-CDR v2.3.6
```bash
cd Fast-CDR
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
sudo cmake --install build && sudo ldconfig
```
- 산출물: `/usr/local/lib/libfastcdr.so.2.3.6` (+ `.so.2`, `.so`), 헤더 `/usr/local/include/fastcdr`

### 5. Fast-DDS v3.6.1 (백그라운드 실행)
```bash
cd Fast-DDS
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
cmake --build build --parallel 4
sudo cmake --install build && sudo ldconfig
```
- `-DBUILD_SHARED_LIBS=ON`: 공유 라이브러리(.so) 생성
- 컴파일된 핵심 모듈: PDP(=SPDP 참여자 discovery), EDP(=SEDP 엔드포인트 discovery), WLP(liveliness) 등
- 산출물:
  - `/usr/local/lib/libfastdds.so.3.6`
  - 헤더 `/usr/local/include/fastdds`
  - CMake config `/usr/local/share/fastdds/cmake/` (`find_package(fastdds)`)
  - CLI `/usr/local/bin/fastdds`, `fast-discovery-server`

### 6. 동작 검증
- `/home/lihan/fastdds_ws/smoketest/` 에 최소 프로그램 작성
- 도메인 0에 DomainParticipant 생성 → 출력 `OK: DomainParticipant created on domain 0`, `exit=0`
- 헤더 인클루드 / .so 링크 / 런타임 로딩 3가지 모두 정상 확인

---

## 설치 결과 요약

| 항목 | 경로/값 |
|------|---------|
| Fast-DDS 라이브러리 | `/usr/local/lib/libfastdds.so.3.6` |
| Fast-CDR 라이브러리 | `/usr/local/lib/libfastcdr.so.2` |
| foonathan_memory | `/usr/local/lib/libfoonathan_memory-0.7.4.a` |
| 헤더 | `/usr/local/include/{fastdds,fastcdr,foonathan}` |
| CMake config | `/usr/local/share/fastdds/cmake/`, `/usr/local/lib/cmake/fastcdr/` |
| CLI 도구 | `/usr/local/bin/fastdds`, `fast-discovery-server` |

---

## 현재 상태 / 다음 할 일

- [x] RPI5 (이 머신, Subscriber 노드) — Fast DDS 설치 완료
- [ ] RPI4 (Publisher 노드) — 동일 설치 필요 → **설치 스크립트로 묶으면 편함**
- [ ] 시나리오 1-1용 Publisher/Subscriber 코드 + IDL + CMake
- [ ] PHASE 3용 QoS XML 프로파일 세트
- [ ] Wireshark 디스플레이 필터 모음 (SPDP/SEDP/HEARTBEAT/ACKNACK)
