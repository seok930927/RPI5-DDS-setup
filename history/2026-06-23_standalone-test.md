# 작업 히스토리 — RPI5 단독 통신 테스트 (설치 검증)

- **날짜**: 2026-06-23
- **머신**: Raspberry Pi 5 (Subscriber 노드)
- **목적**: 설치한 Fast DDS v3.6.1이 실제로 송수신까지 동작하는지 단독(같은 머신)으로 검증
- **결과**: ✅ 송신 20개 → 수신 20개 (유실 0), Discovery·매칭·직렬화 전부 정상

---

## 진행 내용

### 1. 공식 HelloWorld 예제 직접 빌드 시도 → 실패
- `Fast-DDS/examples/cpp/hello_world`의 CMakeLists가 **CMake 3.28 이상** 요구
- 현재 설치된 cmake는 3.25.1 → 빌드 불가
- 대응: 예제를 통째로 쓰는 대신 **생성된 타입 파일만 재사용**해 자체 Pub/Sub 작성

### 2. 자체 Pub/Sub 작성
- 재사용한 타입 파일: `HelloWorld.hpp`, `HelloWorldPubSubTypes.cxx/.hpp`, `HelloWorldCdrAux.hpp/.ipp`, `HelloWorldTypeObjectSupport.cxx/.hpp`
- 타입: `HelloWorld { uint32 index; string message; }`
- `publisher.cpp`: 매칭 대기 후 `Hello DDS 1~20`을 250ms 간격 송신
- `subscriber.cpp`: 리스너의 `on_data_available`에서 take, 20개 수신 시 종료
- `CMakeLists.txt`: `cmake_minimum_required(3.16)`, `find_package(fastdds/fastcdr)`
- 위치(레포): `examples/helloworld/`

### 3. 빌드 → `logs/07_helloworld_build.log`
- publisher / subscriber 두 바이너리 정상 생성

### 4. 단독 통신 테스트 → `logs/08_run_test.log` (+ `08_pub.log`, `08_sub.log`)
- subscriber 백그라운드 실행 후 publisher 실행
- 결과:
  - Pub/Sub 양쪽 "매칭 수 = 1" (Discovery·EDP 매칭 성공)
  - Publisher 송신 20개 (index 1~20)
  - Subscriber 수신 **20개 전부**, 종료코드 0

---

## 결론

설치가 단순히 "파일만 깔린" 수준이 아니라 **RTPS 스택 전체(참여자 발견 → 엔드포인트 매칭 → 데이터 전송 → 역직렬화)가 정상 동작**함을 확인.
이 코드는 그대로 RPI4(Pub) ↔ RPI5(Sub) 2노드 테스트(시나리오 1-1)에 재사용 가능.

## 비고
- 같은 머신 2프로세스지만 실제 RTPS 스택을 그대로 사용 (loopback/멀티캐스트)
- 빌드 산출물(`build/`)은 레포에 포함하지 않음 (`.gitignore` 처리)
