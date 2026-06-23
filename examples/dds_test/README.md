# DDS 시나리오 통합 테스트 프로그램 (RPI4 ↔ RPI5)

하나의 Publisher/Subscriber 프로그램 쌍으로 **PHASE 1~3 모든 시나리오**를 수행한다.
QoS·동작을 커맨드라인 옵션으로 주고, **RPI4에서 `publisher`, RPI5에서 `subscriber`**를 실행해 상호작용시킨다.

- RPI4 = Publisher (192.168.11.6)
- RPI5 = Subscriber (192.168.11.211)
- 같은 LAN·도메인0 → 멀티캐스트 Discovery 자동 매칭

## 빌드 (양쪽 각각)

```bash
cd examples/dds_test
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel $(nproc)
```

## 옵션

```
--reliability reliable|best_effort   (기본 reliable)
--durability  volatile|transient_local (기본 volatile)
--history     keep_last|keep_all      (기본 keep_last)
--depth N                             (기본 10)
--samples N        [Pub] 송신 개수 / [Sub] 이 개수 받으면 조기 종료 (기본 20)
--interval MS      [Pub] 송신 간격 (기본 250)
--keep-alive SEC   [Pub] 송신 후 유지 시간 (기본 2) — 늦은 조인/HEARTBEAT 관찰용
--timeout SEC      [Sub] 최대 대기 (기본 300)
--domain N / --topic NAME / --name STR / --help
```

---

## 시나리오별 실행 명령

> 표기: **R5** = RPI5에서, **R4** = RPI4에서. 별다른 말 없으면 R5(sub) 먼저 띄우고 R4(pub) 실행.

### 1-1 / 1-2 / 2-1 — 기본 통신 & Discovery
```bash
# R5
./build/subscriber
# R4
./build/publisher
```
- Wireshark로 SPDP(멀티캐스트 239.255.0.1) → SEDP → DATA(유니캐스트) 순서 확인.

### 2-2 — HEARTBEAT / ACKNACK (RELIABLE 필요)
```bash
# R5
./build/subscriber --reliability reliable
# R4
./build/publisher --reliability reliable --keep-alive 10
```
- Wireshark 필터 `rtps.sm.id==0x07`(HEARTBEAT), `rtps.sm.id==0x06`(ACKNACK).

### 3-1 — Reliability
```bash
# BEST_EFFORT (HEARTBEAT 없음)
# R5: ./build/subscriber --reliability best_effort
# R4: ./build/publisher  --reliability best_effort

# RELIABLE (HEARTBEAT/ACKNACK 교환)
# R5: ./build/subscriber --reliability reliable
# R4: ./build/publisher  --reliability reliable
```

### 3-2 — Durability (늦은 Subscriber)
**Publisher 먼저 실행** → 송신 후 → **Subscriber 나중에 조인**.
```bash
# VOLATILE: 늦은 Sub는 이전 데이터 못 받음 (0개)
# R4(먼저): ./build/publisher  --durability volatile --keep-alive 30
# R5(나중): ./build/subscriber --durability volatile

# TRANSIENT_LOCAL: 늦은 Sub도 이전 데이터 수신
# R4(먼저): ./build/publisher  --reliability reliable --durability transient_local --keep-alive 30
# R5(나중): ./build/subscriber --reliability reliable --durability transient_local
```

### 3-3 — History (늦은 Subscriber, TRANSIENT_LOCAL 전제)
```bash
# KEEP_LAST depth=5: 늦은 Sub는 최근 5개만
# R4(먼저): ./build/publisher  --reliability reliable --durability transient_local --history keep_last --depth 5 --samples 20 --keep-alive 30
# R5(나중): ./build/subscriber --reliability reliable --durability transient_local --history keep_last --depth 5 --samples 20

# KEEP_ALL: 늦은 Sub가 전체 20개
# R4(먼저): ./build/publisher  --reliability reliable --durability transient_local --history keep_all --samples 20 --keep-alive 30
# R5(나중): ./build/subscriber --reliability reliable --durability transient_local --history keep_all --samples 20
```

### 3-4 — QoS Mismatch (조용한 실패)
```bash
# (a) Pub BEST_EFFORT / Sub RELIABLE → 매칭 실패 (수신 0)
# R5: ./build/subscriber --reliability reliable
# R4: ./build/publisher  --reliability best_effort

# (b) Pub VOLATILE / Sub TRANSIENT_LOCAL → 매칭 실패
# R5: ./build/subscriber --durability transient_local --reliability reliable
# R4: ./build/publisher  --durability volatile --reliability reliable

# (c) Pub RELIABLE / Sub BEST_EFFORT → 매칭 성공 (호환)
# R5: ./build/subscriber --reliability best_effort
# R4: ./build/publisher  --reliability reliable
```
- 매칭 실패 시 양쪽 모두 "매칭 수=1" 메시지가 안 뜨고, Sub 총 수신 0. 에러는 안 남.

---

---

## ★ 자동 시나리오 러너 (한 번 켜두면 전부 자동 진행)

수동으로 시나리오마다 옵션을 바꿔 실행하는 대신, **두 기기에서 한 번씩만 실행하면 11개 시나리오가 순서대로 자동 진행**되고 마지막에 PASS/FAIL 결과표가 나온다.

- **RPI4 = `auto_pub`** (오케스트레이터: 시나리오 목록을 갖고 지휘)
- **RPI5 = `auto_sub`** (팔로워: 명령 받아 QoS별 Reader 생성·수신·회신)
- 별도 **제어 토픽(DDSTestControl)** 으로 동기화. RPI4가 진행/정지 신호를 보내고 RPI5가 결과를 회신.

### 실행 (RPI5 먼저, RPI4 나중)
```bash
# RPI5
./build/auto_sub
# RPI4 (5초 뒤 자동 시작)
./build/auto_pub
```
> 같은 도메인이어야 함. 다른 테스트와 섞이지 않게 하려면 양쪽에 `--domain N` 동일 지정.

### 자동 진행되는 시나리오 (11개)
1-1/2-1 기본통신 · 2-2 HEARTBEAT/ACKNACK · 3-1 BE/RE · 3-2 VOLATILE/TRANSIENT_LOCAL(늦은조인) ·
3-3 KEEP_LAST(d5)/KEEP_ALL(늦은조인) · 3-4 Mismatch(BE/RE, VOL/TL)/Compatible(RE/BE)

### 결과 예시 (RPI5 단독 loopback 검증, 2026-06-23)
```
시나리오     설명                          수신  기대  판정
1-1/2-1     기본통신·Discovery             10    10   PASS
2-2         HEARTBEAT/ACKNACK(RELIABLE)    10    10   PASS
3-1-BE      Reliability BEST_EFFORT        10    10   PASS
3-1-RE      Reliability RELIABLE           10    10   PASS
3-2-VOL     Durability VOLATILE(늦은조인)   0     0   PASS
3-2-TL      Durability TRANSIENT_LOCAL      10    10   PASS
3-3-KL5     History KEEP_LAST depth=5        5     5   PASS
3-3-KA      History KEEP_ALL               20    20   PASS
3-4-a       Mismatch Pub=BE/Sub=RE          0     0   PASS
3-4-b       Mismatch Pub=VOL/Sub=TL         0     0   PASS
3-4-c       Compatible Pub=RE/Sub=BE       10    10   PASS
PASS 11 / 11
```

> Wireshark로 패킷을 보려면, auto_pub/auto_sub 실행 전에 캡처를 시작해두면 각 시나리오의 SPDP/SEDP/DATA/HEARTBEAT/ACKNACK가 시간순으로 잡힌다.

### 패킷 캡처까지 자동으로 (run_with_capture.sh)

공유기 포트 미러링이 **인터넷(WAN) 트래픽만** 미러하는 경우(일반 가정용 공유기) RPI 간 LAN 유니캐스트(DATA/HEARTBEAT/ACKNACK)는 모니터링 PC에서 안 잡힌다.
→ **각 RPI에서 직접 tcpdump로 캡처**하면 그 노드가 주고받는 모든 RTPS가 100% 잡힌다. 이를 자동화한 래퍼:

```bash
# 사전: tcpdump 설치 (한 번만)
sudo apt-get install -y tcpdump

# RPI5: 캡처와 함께 팔로워 실행
./run_with_capture.sh auto_sub
# RPI4: 캡처와 함께 오케스트레이터 실행
./run_with_capture.sh auto_pub
```
- 캡처 시작 → 프로그램 실행 → 종료(또는 Ctrl+C) 시 자동 정지·저장
- 저장 위치: `captures/dds_<role>_<host>_<날짜>.pcap` (패킷 전체 `-s 0`)
- 끝나면 RTPS/DATA/HEARTBEAT/ACKNACK 개수 요약 출력 (tshark 있을 때)
- 환경변수: `IFACE=eth0`(기본), `FILTER='udp'`(기본, 모든 DDS)
- 만들어진 `.pcap`는 Wireshark에서 `rtps` 필터로 바로 분석 가능

> 미러링이 LAN 유니캐스트를 잡는 관리형 스위치라면 모니터링 PC Wireshark로 봐도 되지만,
> 인터넷 전용 미러밖에 없으면 이 방식이 유일하게 확실하다.

---

## (수동) 개별 실행용 publisher / subscriber

아래는 시나리오를 하나씩 직접 돌려보고 싶을 때 쓰는 단순 버전이다.

## RPI5 단독 검증 결과 (2026-06-23)
- RELIABLE 양쪽: 매칭=1, 5/5 수신 ✅
- Mismatch(BE/RE): 매칭 안 됨, 0개 (조용한 실패) ✅
- Durability TRANSIENT_LOCAL 늦은 조인: 과거 데이터 수신 ✅ / VOLATILE: 0개 ✅
