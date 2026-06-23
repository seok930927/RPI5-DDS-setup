# HelloWorld Pub/Sub 예제 (시나리오 1-1용)

Fast DDS v3.6.1 설치 검증 및 기본 통신 테스트용 최소 Publisher/Subscriber.

- 타입: `HelloWorld { uint32 index; string message; }` (HelloWorld.idl 기반, 생성 파일 포함)
- Publisher: `Hello DDS 1~20`을 250ms 간격으로 송신
- Subscriber: 수신한 샘플을 출력, 20개 받으면 종료 (최대 15초 대기)

## 빌드

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
```

## 실행 (단독 머신 테스트)

터미널 2개로:
```bash
# 터미널 A
./build/subscriber
# 터미널 B
./build/publisher
```

또는 한 번에:
```bash
./build/subscriber & ; ./build/publisher ; wait
```

## 실행 (두 노드: RPI4 Pub ↔ RPI5 Sub)

같은 LAN, 같은 도메인(0)이면 별도 설정 없이 멀티캐스트 Discovery로 자동 매칭됨.
```bash
# RPI5
./build/subscriber
# RPI4
./build/publisher
```

## 검증된 결과 (2026-06-23, RPI5 단독)

- 매칭: Pub/Sub 양쪽 "매칭 수 = 1"
- 송신 20개 → 수신 20개 (유실 0)
- 로그: `logs/08_run_test.log`
