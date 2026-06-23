# Wireshark RTPS 패킷 분석 가이드 (PHASE 2)

Windows PC(포트 미러링)에서 RPI4↔RPI5 RTPS 패킷을 캡처·분석하는 절차.

- 분석 대상: 시나리오 1-2(Multicast Discovery), 2-1(Discovery 시퀀스), 2-2(HEARTBEAT/ACKNACK)
- 전제: 포트 미러링 동작 중, RPI4=192.168.11.6(Pub), RPI5=192.168.11.211(Sub)

---

## 0. 기본 디스플레이 필터

| 목적 | 필터 |
|------|------|
| 모든 RTPS | `rtps` |
| 두 라파이 간만 | `rtps && (ip.addr==192.168.11.6 && ip.addr==192.168.11.211)` |
| SPDP (참여자 발견, 멀티캐스트) | `rtps.traffic_nature == 1` 또는 `rtps.sm.wrEntityId == 0x000100c2` |
| SEDP (엔드포인트 발견) | `rtps.sm.wrEntityId == 0x000003c2 || rtps.sm.wrEntityId == 0x000004c2` |
| DATA 서브메시지 | `rtps.sm.id == 0x15` |
| HEARTBEAT | `rtps.sm.id == 0x07` |
| ACKNACK | `rtps.sm.id == 0x06` |
| 멀티캐스트 주소(SPDP 송신) | `ip.dst == 239.255.0.1` |

> RTPS 서브메시지 ID 참고: DATA=0x15, HEARTBEAT=0x07, ACKNACK=0x06, INFO_TS=0x09, INFO_DST=0x0e
> 빌트인 엔드포인트 EntityId: SPDP writer=0x000100c2, SEDP pub writer=0x000003c2, SEDP sub writer=0x000004c2

---

## 1. 캡처 절차 (중요: 순서)

Discovery(SPDP)는 **참여자 시작 시점에** 나오므로, **반드시 캡처를 먼저 시작**한 뒤 프로그램을 실행해야 함.

1. Windows Wireshark에서 미러링 받는 인터페이스 선택 → 캡처 시작
2. 디스플레이 필터에 `rtps` 입력
3. **RPI5**: `examples/helloworld/build/subscriber` 실행 (대기)
4. **RPI4**: `examples/helloworld/build/publisher` 실행
5. 20개 송신 끝나면 캡처 정지 → 저장(.pcapng)

---

## 2. 시나리오별 확인 포인트

### 시나리오 1-2 : Multicast Discovery
- 필터: `ip.dst == 239.255.0.1` (또는 SPDP 필터)
- 확인: 두 노드가 **멀티캐스트 주소 239.255.0.1:7400번대**로 SPDP DATA(p) 전송
- Discovery 후 실제 DATA는 **유니캐스트**(상대 IP 직접)로 가는지 대조

### 시나리오 2-1 : Discovery 시퀀스 (SPDP→SEDP→DATA)
순서대로 패킷 흐름 확인:

| 단계 | 패킷 | 필터 | 전송 |
|------|------|------|------|
| 1 | SPDP (DATA(p)) | `rtps.sm.wrEntityId == 0x000100c2` | 멀티캐스트 239.255.0.1 |
| 2 | SEDP (DATA(w)/DATA(r)) | `rtps.sm.wrEntityId == 0x000003c2 \|\| rtps.sm.wrEntityId == 0x000004c2` | 유니캐스트 |
| 3 | (매칭) | — | 프로그램 로그 "매칭 수=1" |
| 4 | DATA (사용자 데이터) | `rtps.sm.id == 0x15 && rtps.sm.wrEntityId > 0x01000000` 등 | 유니캐스트 |

- "Follow"보다는 시간순 정렬해서 SPDP가 먼저, 그다음 SEDP, 마지막 사용자 DATA 순인지 본다.

### 시나리오 2-2 : HEARTBEAT / ACKNACK
> ⚠️ **현재 예제 기본 QoS(Reader=BEST_EFFORT)로는 HEARTBEAT/ACKNACK이 거의 안 보임.**
> Reader를 RELIABLE로 바꿔야 함 (아래 참고).

- 필터: `rtps.sm.id == 0x07` (HEARTBEAT), `rtps.sm.id == 0x06` (ACKNACK)
- 확인: Publisher가 주기적으로 HEARTBEAT 송신 → Subscriber가 ACKNACK 응답
- Sequence Number 흐름 추적 (HEARTBEAT의 firstSN/lastSN, ACKNACK의 readerSNState)

---

## 3. RELIABLE 설정 (시나리오 2-2 및 PHASE 3 대비)

현재 `subscriber.cpp`는 `DATAREADER_QOS_DEFAULT`(BEST_EFFORT) 사용.
HEARTBEAT/ACKNACK을 보려면 Reader reliability를 RELIABLE로:

```cpp
DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
DataReader* reader = subscriber->create_datareader(topic, rqos, &listener);
```

> PHASE 3에서는 이 QoS들을 XML 프로파일로 빼서 코드 재빌드 없이 전환할 예정.

---

## 4. 결과 기록
- 캡처 파일(.pcapng)과 스크린샷을 `logs/` 또는 별도 폴더에 저장
- `DDS_테스트_시나리오.md` 결과표의 "Wireshark 확인" 칸 채우기
