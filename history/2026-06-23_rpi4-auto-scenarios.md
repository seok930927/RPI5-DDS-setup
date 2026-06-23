# 작업 히스토리 — 자동 시나리오 러너 2노드 실행 (RPI4 Publisher)

- **날짜**: 2026-06-23
- **머신**: RPI4 (Publisher, 192.168.11.6) ↔ RPI5 (Subscriber, 192.168.11.211)
- **목적**: `examples/dds_test`의 자동 러너(`auto_pub`/`auto_sub`)로 PHASE 1~3 전 시나리오(11개)를 실기기 2노드에서 한 번에 검증
- **결과**: ✅ **11 / 11 PASS** (RPI5 단독 loopback 기준치와 완전 일치)

---

## 구성 / 실행

| 기기 | 바이너리 | 역할 |
|------|----------|------|
| RPI4 | `examples/dds_test/build/auto_pub` | 오케스트레이터 (시나리오 지휘) |
| RPI5 | `examples/dds_test/build/auto_sub` | 팔로워 (QoS별 Reader 생성·수신·회신) |

- 제어 토픽 `DDSTestControl`로 동기화. 같은 LAN·도메인0 멀티캐스트 Discovery.
- 실행 순서: RPI5 `auto_sub` 먼저 → RPI4 `auto_pub` (5초 뒤 자동 시작).
- 빌드: `logs/20_rpi4_auto_build.log` / 실행 로그: `logs/21_rpi4_auto_run.log`

## 결과표 (RPI4 출력)

| 시나리오 | 설명 | 수신 | 기대 | 판정 |
|----------|------|------|------|------|
| 1-1/2-1 | 기본통신·Discovery | 10 | 10 | PASS |
| 2-2 | HEARTBEAT/ACKNACK (RELIABLE) | 10 | 10 | PASS |
| 3-1-BE | Reliability BEST_EFFORT | 10 | 10 | PASS |
| 3-1-RE | Reliability RELIABLE | 10 | 10 | PASS |
| 3-2-VOL | Durability VOLATILE (늦은조인) | 0 | 0 | PASS |
| 3-2-TL | Durability TRANSIENT_LOCAL (늦은조인) | 10 | 10 | PASS |
| 3-3-KL5 | History KEEP_LAST depth=5 (늦은조인) | 5 | 5 | PASS |
| 3-3-KA | History KEEP_ALL (늦은조인) | 20 | 20 | PASS |
| 3-4-a | Mismatch Pub=BE/Sub=RE | 0 | 0 | PASS |
| 3-4-b | Mismatch Pub=VOL/Sub=TL | 0 | 0 | PASS |
| 3-4-c | Compatible Pub=RE/Sub=BE | 10 | 10 | PASS |
| | **합계** | | | **PASS 11 / 11** |

## 해석

- **Reliability**: BEST_EFFORT/RELIABLE 양쪽 정상 전달. RELIABLE에서 HEARTBEAT/ACKNACK 교환.
- **Durability**: 늦은 조인 시 VOLATILE=0개 / TRANSIENT_LOCAL=과거 데이터 수신. 의도대로 동작.
- **History**: KEEP_LAST(depth=5)=최근 5개 / KEEP_ALL=전체 20개.
- **Mismatch**: BE↔RE, VOLATILE↔TRANSIENT_LOCAL 비호환 조합은 매칭 실패(에러 없이 수신 0 = 조용한 실패). RE(Pub)↔BE(Sub)는 호환되어 정상 수신.
- RPI5 단독 loopback 결과와 동일 → 실기기 2노드에서도 QoS 의미론이 그대로 재현됨.

## 현재 상태 / 다음 할 일

- [x] PHASE 1~3 전 시나리오 2노드 자동 검증 (11/11 PASS)
- [ ] (선택) Wireshark로 각 시나리오의 SPDP/SEDP/DATA/HEARTBEAT/ACKNACK 패킷 캡처·대조 (`docs/wireshark_분석가이드.md`)
- [ ] (선택) `DDS_테스트_시나리오.md` 결과 기록표 갱신
