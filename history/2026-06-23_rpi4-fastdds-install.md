# 작업 히스토리 — Fast DDS v3.6.1 설치 + 단독 검증 (RPI4)

- **날짜**: 2026-06-23
- **머신**: Raspberry Pi 4 Model B Rev 1.2, Debian 12 (bookworm), aarch64, RAM 3.7GB / 4코어
- **역할**: Publisher 노드 (`192.168.11.6`)
- **목적**: RPI5(Subscriber)와 동일하게 Fast DDS를 설치하고, 단독 송수신으로 설치를 검증 (STEP1 + STEP2)
- **구현체/버전**: eProsima Fast DDS **v3.6.1** + Fast-CDR **v2.3.6** + foonathan_memory (master)
- **작업 디렉토리**: `/home/lihan/fastdds_ws`
- **설치 prefix**: `/usr/local`
- **결과**: ✅ 빌드·설치·단독 송수신(20/20)까지 전부 성공

---

## 타임라인 (시간순, BST)

| 시각 | 단계 | 내용 | 로그 | 결과 |
|------|------|------|------|------|
| 04:17 | 1. apt 의존성 | build-essential, **cmake(신규 설치)**, libasio/tinyxml2/ssl-dev 등 | `10_rpi4_apt_deps.log` | ✅ |
| 04:18 | 2. 소스 클론 | foonathan_memory / Fast-CDR v2.3.6 / Fast-DDS v3.6.1 | `11_rpi4_clone.log` | ✅ |
| 04:19 | 3. foonathan_memory | cmake 빌드(`--parallel 4`) + 설치 | `12_rpi4_foonathan.log` | ✅ |
| 04:20 | 4. Fast-CDR | cmake 빌드(`--parallel 4`) + 설치 + ldconfig | `13_rpi4_fastcdr.log` | ✅ |
| 04:21~04:57 | 5. Fast-DDS | cmake 빌드(**`--parallel 2`**, 약 36분) + 설치 + ldconfig | `14_rpi4_fastdds.log` | ✅ |
| 04:59 | 6. 동작 검증 | DomainParticipant 생성 스모크 테스트 | `15_rpi4_verify.log` | ✅ `OK` |
| 05:00 | 7. helloworld 빌드 | publisher/subscriber 바이너리 생성 | `16_rpi4_helloworld_build.log` | ✅ |
| 05:00 | 8. 단독 송수신 | sub 백그라운드 → pub 실행, 20개 송수신 | `17_rpi4_run_test.log` (+`17_rpi4_pub.log`, `17_rpi4_sub.log`) | ✅ 20/20 |

---

## RPI5와 달랐던 점 (RPI4 특이사항)

- **cmake 미설치 상태**였음 → apt로 신규 설치(3.25.1). RPI5와 동일 버전이라 `examples/cpp/hello_world`(3.28+ 요구)는 여전히 불가, 자체 `examples/helloworld/` 사용 방침 유지.
- **메모리 3.7GB + swap 511MB**뿐 → Fast-DDS 빌드만 `--parallel 2`로 낮춤 (foonathan/Fast-CDR는 가벼워 `--parallel 4`).
  - 빌드 내내 메모리 모니터링: free 최저 ~108MB, **available은 항상 1.7GB+ 유지, swap 사용 0**. OOM·에러 0건.
  - RPI5(8GB, `--parallel 4`)는 ~8분, RPI4(`--parallel 2`)는 ~36분 소요.
- `/usr/bin/time` 미설치라 빌드 1회차가 rc=127로 무동작 → time 제거 후 재실행(`14_rpi4_fastdds.log` 안에 두 시도 모두 기록됨).

---

## 설치 결과 요약

| 항목 | 경로/값 |
|------|---------|
| Fast-DDS 라이브러리 | `/usr/local/lib/libfastdds.so.3.6.1.0` (→ `.so.3.6` → `.so`) |
| Fast-CDR 라이브러리 | `/usr/local/lib/libfastcdr.so.2.3.6` (→ `.so.2` → `.so`) |
| foonathan_memory | `/usr/local/lib/libfoonathan_memory-0.7.4.a` |
| 헤더 | `/usr/local/include/{fastdds,fastcdr,foonathan}` |
| helloworld 바이너리 | `examples/helloworld/build/{publisher,subscriber}` |

---

## 검증 결과

- **스모크**: `OK: DomainParticipant created on domain 0`, exit 0
- **단독 송수신**: Publisher 20개 송신(index 1~20) → Subscriber **20개 전부 수신**, 양쪽 rc=0
  - Discovery·EDP 매칭 → DATA 전송 → 역직렬화까지 RTPS 스택 전체 정상

---

## 현재 상태 / 다음 할 일

- [x] STEP1 — RPI4(Publisher) Fast DDS v3.6.1 설치 완료
- [x] STEP2 — RPI4 단독 송수신 검증 완료 (20/20)
- [x] STEP3 — 2노드 통신 (시나리오 1-1) 성공: RPI4 송신 20 → RPI5 수신 20 (유실 0). 상세 [2026-06-23_2node-test.md](2026-06-23_2node-test.md), RPI4 로그 `logs/18_rpi4_2node_pub.log`
- [ ] STEP4 — Wireshark로 SPDP/SEDP/DATA/HEARTBEAT/ACKNACK 분석, QoS XML 프로파일 테스트 (RPI5가 PHASE 2 가이드 `docs/wireshark_분석가이드.md` 작성함)
