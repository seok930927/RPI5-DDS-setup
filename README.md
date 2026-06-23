# RPI5-DDS-setup — Raspberry Pi 간 Fast DDS 통신 테스트

두 라즈베리파이(RPI4 ↔ RPI5) 사이에서 **eProsima Fast DDS**(RTPS over UDP)로 Pub/Sub 통신을 구성하고,
QoS 정책·Discovery·패킷 동작을 자동 시나리오와 패킷 캡처로 검증한 프로젝트.

## 구성

| 기기 | 역할 | IP |
|------|------|-----|
| RPI4 | Publisher | 192.168.11.6 |
| RPI5 | Subscriber | 192.168.11.211 |

- DDS 구현체: **Fast DDS v3.6.1** + Fast-CDR v2.3.6 (소스빌드, `/usr/local`)
- 프로토콜: RTPS 2.2 over UDP (멀티캐스트 Discovery + 유니캐스트 데이터)

## 폴더 구조

```
.
├── CLAUDE.md                   # RPI4 작업 인수인계서
├── DDS_테스트_시나리오.md       # 시나리오 정의 + 결과표
├── examples/
│   ├── helloworld/             # 기본 Pub/Sub 예제 (시나리오 1-1)
│   └── dds_test/               # 시나리오 통합 + 자동 러너 + 캡처 래퍼
│       ├── publisher / subscriber        # 옵션으로 QoS 바꾸는 수동 버전
│       ├── auto_pub / auto_sub           # 14개 시나리오 자동 진행 러너
│       └── run_with_capture.sh           # tcpdump 캡처까지 자동
├── captures/                   # 패킷 캡처 (.pcap)
├── docs/
│   ├── wireshark_분석가이드.md
│   └── 패킷분석_상세.md          # 실제 캡처 기반 상세 분석
├── logs/                       # 설치~테스트 로그
└── history/                    # 작업 히스토리 (시간순)
```

## 빠른 시작

```bash
# 1) 설치 (각 RPI에서, history/2026-06-23_fastdds-install.md 참고)
# 2) 빌드
cd examples/dds_test
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel $(nproc)

# 3) 자동 시나리오 14개 실행 (캡처 포함)
sudo apt-get install -y tcpdump        # 한 번만
# RPI5:
./run_with_capture.sh auto_sub
# RPI4:
./build/auto_pub
```
끝나면 RPI4 화면에 **PASS/FAIL 결과표**, `captures/`에 패킷 파일 생성.

## 검증한 시나리오 (자동, 14개)

| 그룹 | 시나리오 |
|------|----------|
| 기본 | 1-1 통신·Discovery, 2-2 HEARTBEAT/ACKNACK |
| Reliability | BEST_EFFORT / RELIABLE |
| Durability | VOLATILE / TRANSIENT_LOCAL (늦은조인) |
| History | KEEP_LAST(d5) / KEEP_ALL |
| QoS Mismatch | BE↔RE, VOL↔TL (실패), RE→BE (호환) |
| **DDS 고유** | Partition 일치/불일치, Content Filter(index>5) |

**결과: 14/14 PASS** (2노드 실통신 + 패킷 캡처로 확인)

## 핵심 메모

- 공유기 "포트 미러링"이 **인터넷(WAN) 트래픽 전용**이면 RPI 간 LAN 유니캐스트(DATA/HEARTBEAT/ACKNACK)를 모니터링 PC에서 못 잡음 → **각 RPI에서 직접 tcpdump**(`run_with_capture.sh`)로 캡처해야 전부 보임.
- 상세 패킷 분석은 `docs/패킷분석_상세.md` 참고.
- 작업 이력은 `history/` 참고.

## 다음 (TODO)

- [ ] Keyed Topic / Instance (IDL `@key`) — `fastddsgen` 설치 필요
- [ ] 추가 QoS: Deadline / Liveliness / Lifespan
