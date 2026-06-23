# DDS 테스트 시나리오 (RPI4 + RPI5)

## 테스트 환경

| 항목 | 내용 |
|------|------|
| 노드 A | Raspberry Pi 4 (Publisher) |
| 노드 B | Raspberry Pi 5 (Subscriber) |
| DDS 구현체 | eProsima Fast DDS (소스빌드) |
| 네트워크 | 공유기 LAN (포트 미러링) |
| 패킷 분석 | Windows PC - Wireshark |
| 프로토콜 | RTPS over UDP Multicast |

> **환경 점검 사항**
> - 스위치/공유기의 **IGMP Snooping**이 켜져 있으면 SPDP 멀티캐스트가 미러 포트로 넘어오지 않을 수 있음. SPDP가 안 보이면 가장 먼저 의심할 것.
> - Wireshark 기본 필터: `rtps`
> - 매칭 실패 사유 확인용 로그: `export FASTDDS_LOG_LEVEL=Warning`

---

## PHASE 1 — 기본 통신 확인

### 시나리오 1-1 : Publisher / Subscriber 기본 통신
- RPI4에서 Publisher, RPI5에서 Subscriber 실행
- 메시지 정상 송수신 확인

### 시나리오 1-2 : Multicast Discovery 확인
- 두 노드가 멀티캐스트로 서로를 발견(Discovery)하는지 확인
- Discovery 완료 후 유니캐스트로 전환되는지 확인
- Wireshark에서 SPDP 패킷 확인

---

## PHASE 2 — RTPS 패킷 분석

### 시나리오 2-1 : Discovery 시퀀스 분석
- SPDP → SEDP → Matching → DATA 순서로 패킷 흐름 확인
- 각 단계별 패킷 타입 및 내용 분석

| 단계 | 패킷 타입 | 설명 |
|------|-----------|------|
| 1 | SPDP | 참여자 발견 - 멀티캐스트 |
| 2 | SEDP | 엔드포인트 발견 - 유니캐스트 |
| 3 | Matching | Writer ↔ Reader 매칭 |
| 4 | DATA | 실제 데이터 전송 - 유니캐스트 |

### 시나리오 2-2 : HEARTBEAT / ACKNACK 분석
- Publisher의 HEARTBEAT 패킷 주기 확인
- Subscriber의 ACKNACK 응답 확인
- Sequence Number 흐름 확인

> **팁**: Fast DDS 기본 `heartbeat_period`가 길어 관찰이 어려울 수 있음. 테스트 시 100ms 정도로 짧게 설정하면 HEARTBEAT/ACKNACK 교환 관찰이 쉬움.

---

## PHASE 3 — QoS 정책 테스트

### 시나리오 3-1 : Reliability QoS
- BEST_EFFORT / RELIABLE 두 가지 설정으로 각각 테스트
- BEST_EFFORT : HEARTBEAT 없음 확인
- RELIABLE : HEARTBEAT / ACKNACK 교환 확인

| 설정 | 예상 동작 |
|------|-----------|
| BEST_EFFORT | 재전송 없음, HEARTBEAT 없음 |
| RELIABLE | 재전송 있음, HEARTBEAT/ACKNACK 교환 |

### 시나리오 3-2 : Durability QoS
- Publisher 먼저 실행 후 데이터 송신
- Subscriber 늦게 참여 시 이전 데이터 수신 여부 확인

| 설정 | 예상 동작 |
|------|-----------|
| VOLATILE | 늦은 Subscriber 이전 데이터 못 받음 |
| TRANSIENT_LOCAL | 늦은 Subscriber 이전 데이터 수신 |

> **전제 조건**: TRANSIENT_LOCAL로 과거 데이터를 전달하려면 Reliability를 **RELIABLE**로 설정해야 함. (BEST_EFFORT + TRANSIENT_LOCAL은 과거 샘플 전달 보장 안 됨) → 본 시나리오는 `RELIABLE + TRANSIENT_LOCAL`로 고정.

### 시나리오 3-3 : History QoS
- Subscriber 늦게 참여 시 수신 데이터 개수 확인

| 설정 | 예상 동작 |
|------|-----------|
| KEEP_LAST(depth=5) | 최근 5개만 수신 |
| KEEP_ALL | 전체 데이터 수신 |

> **전제 조건**: 본 시나리오는 `Durability = TRANSIENT_LOCAL` 전제 하에서만 의미가 있음. VOLATILE이면 늦은 Subscriber는 History 설정과 무관하게 **0개** 수신. History depth는 Writer가 보관하는 샘플 수를 제한함.

### 시나리오 3-4 : QoS Mismatch
- Publisher / Subscriber QoS 불일치 시 Matching 실패 확인
- 에러 없이 조용히 실패하는 동작 확인 (로그 레벨을 올리면 사유 출력됨)

| Publisher | Subscriber | 예상 결과 |
|-----------|------------|-----------|
| BEST_EFFORT | RELIABLE | Matching 실패 |
| VOLATILE | TRANSIENT_LOCAL | Matching 실패 |
| RELIABLE | BEST_EFFORT | Matching 성공 (호환) |

> **RxO 규칙 (Requested ≤ Offered)**
> - Reliability 강도: `RELIABLE > BEST_EFFORT` → Publisher가 같거나 강해야 매칭
> - Durability 강도: `VOLATILE < TRANSIENT_LOCAL < TRANSIENT < PERSISTENT` → Publisher가 같거나 강해야 매칭

---

## 테스트 결과 기록

| 시나리오 | 예상 결과 | 실제 결과 | Wireshark 확인 | 비고 |
|----------|-----------|-----------|----------------|------|
| 1-1 기본 통신 | 메시지 수신 | | | |
| 1-2 Multicast Discovery | SPDP 패킷 확인 | | | |
| 2-1 Discovery 시퀀스 | SPDP→SEDP→DATA | | | |
| 2-2 HEARTBEAT/ACKNACK | 패킷 교환 확인 | | | |
| 3-1 Reliability | BE/RE 차이 확인 | | | |
| 3-2 Durability | 늦은 수신 차이 | | | |
| 3-3 History | depth 제한 확인 | | | |
| 3-4 QoS Mismatch | 조용한 실패 확인 | | | |
