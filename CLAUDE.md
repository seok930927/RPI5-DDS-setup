# 인수인계서 — RPI4 작업용 (RPI5 → RPI4)

> 이 문서는 **RPI4에서 새로 시작하는 Claude Code 세션**을 위한 인수인계서입니다.
> RPI5에서 진행한 작업의 맥락을 그대로 이어받아 RPI4 작업을 진행하세요.
> 상세 기록은 `history/` 폴더에 있습니다. **작업 전 `history/`를 먼저 읽으세요.**

---

## 1. 이 프로젝트가 뭔가

eProsima **Fast DDS**(RTPS over UDP)로 두 라즈베리파이 간 Pub/Sub 통신을 테스트하고,
Windows PC의 Wireshark로 RTPS 패킷(Discovery/QoS 등)을 분석하는 프로젝트.

- 목표 시나리오: **`DDS_테스트_시나리오.md`** 참고 (PHASE 1~3)
- 역할 분담:
  - **RPI4 = Publisher** ← **지금 이 기기에서 작업**
  - **RPI5 = Subscriber** (이미 설치·검증 완료됨)

## 2. 네트워크 / 기기 정보

| 기기 | 역할 | IP | 상태 |
|------|------|-----|------|
| RPI5 | Subscriber | `192.168.11.211` | ✅ Fast DDS 설치·검증 완료 |
| RPI4 | Publisher | `192.168.11.6` | ✅ Fast DDS 설치·단독검증·2노드 통신 완료 (STEP1~3, 2026-06-23) |
| Windows PC | Wireshark 캡처 | - | 포트 미러링 |

## 3. 지금까지 된 것 (RPI5)

- Fast DDS **v3.6.1** + Fast-CDR **v2.3.6** + foonathan_memory 소스빌드·설치 완료 (`/usr/local`)
- 단독 통신 테스트 통과: 송신 20개 → 수신 20개 (유실 0)
- 상세: `history/2026-06-23_fastdds-install.md`, `history/2026-06-23_standalone-test.md`

## 4. RPI4에서 할 일 (순서대로)

### STEP 1. Fast DDS 설치 (가장 먼저)
RPI5와 **동일한 과정**을 RPI4에서 반복. **정확한 명령·버전은 `history/2026-06-23_fastdds-install.md`에 단계별로 다 있음.** 요약:

1. apt 의존성: `build-essential cmake git libasio-dev libtinyxml2-dev libssl-dev`
2. 소스 클론 (작업폴더 예: `~/fastdds_ws`):
   - `foonathan_memory_vendor` (master)
   - `Fast-CDR` → `--branch v2.3.6`
   - `Fast-DDS` → `--branch v3.6.1`
3. 빌드·설치 순서: **foonathan_memory → Fast-CDR → Fast-DDS** (각각 `cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release` → `cmake --build build` → `sudo cmake --install build` → `sudo ldconfig`)
4. Fast-DDS는 `-DBUILD_SHARED_LIBS=ON`

> ⚠️ **RPI4 메모리 주의**: RPI4는 RAM이 1~4GB일 수 있음. `free -h`로 확인하고, 적으면 `cmake --build build --parallel 2`(또는 1)로 낮출 것. 부족하면 swap 늘리기. (RPI5는 8GB라 `--parallel 4` 사용했음)

> ⚠️ **cmake 버전**: 설치된 cmake가 3.28 미만이면 Fast-DDS **공식 예제**(`examples/cpp/hello_world`)는 빌드 불가. 우리는 그래서 예제 대신 `examples/helloworld/`(이 레포)에 자체 코드를 만들어 씀(CMake 3.16 호환).

### STEP 2. 설치 검증
`history/2026-06-23_standalone-test.md`대로 `examples/helloworld/`를 빌드해서 RPI4 단독으로 송수신 확인:
```bash
cd examples/helloworld
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel $(nproc)
./build/subscriber & ./build/publisher ; wait
```

### STEP 3. 2노드 통신 테스트 (시나리오 1-1) ← 진짜 목표
- **RPI5(192.168.11.211)**에서 Subscriber 실행: `examples/helloworld/build/subscriber`
- **RPI4(이 기기)**에서 Publisher 실행: `examples/helloworld/build/publisher`
- 같은 LAN·같은 도메인(0)이면 멀티캐스트 Discovery로 자동 매칭됨
- RPI4가 보낸 20개를 RPI5가 받으면 성공

### STEP 4. 그 다음 (시나리오 진행)
- PHASE 1-2: Wireshark로 SPDP→SEDP→DATA, HEARTBEAT/ACKNACK 패킷 분석
- PHASE 3: QoS XML 프로파일 만들어 Reliability/Durability/History/Mismatch 테스트
- `DDS_테스트_시나리오.md`의 결과 기록표 채우기

## 5. 레포 구조

```
.
├── CLAUDE.md                  # 이 인수인계서 (자동 로딩됨)
├── DDS_테스트_시나리오.md      # 목표 시나리오 + 결과 기록표
├── examples/helloworld/       # Pub/Sub 예제 (시나리오 1-1용, RPI4=Pub)
├── logs/                      # RPI5 설치/빌드/테스트 원본 로그
└── history/                   # 작업 히스토리 (시간순) ★먼저 읽기
    ├── README.md              # 히스토리 인덱스
    ├── 2026-06-23_fastdds-install.md   # 설치 단계별 명령 ← STEP1에서 그대로 사용
    └── 2026-06-23_standalone-test.md   # 검증 방법
```

## 6. 작업 규칙 (이어서 지킬 것)

- **로그 남기기**: 설치·빌드 명령은 출력을 `logs/`에 저장 (RPI4 것은 예: `logs/10_rpi4_install.log` 식으로 새 번호)
- **히스토리 작성**: 작업 끝나면 `history/YYYY-MM-DD_작업명.md` 추가하고 `history/README.md` 표에 한 줄 등록
- **커밋 메시지에 Claude 관련 기록(Co-Authored-By 등) 넣지 말 것** (사용자 요청)
- **git 사용자**: seok930927 / lihan@wiznet.io

## 7. Git 푸시 인증 (중요)

- 이 레포는 **공개(public)지만 푸시는 인증 필요** (읽기/클론은 인증 불필요).
- RPI5는 **Deploy Key**(레포 전용 SSH 키, 쓰기 허용)로 푸시함. 그 키는 RPI5 전용이라 **RPI4에서는 못 씀**.
- RPI4에서 푸시하려면 **RPI4용 인증을 따로** 만들어야 함. 권장: RPI4에서 `ssh-keygen`으로 새 키 생성 → 그 공개키를 GitHub 레포 **Settings → Deploy keys → Add deploy key**에 등록(✅ Allow write access 체크) → remote를 SSH(`git@github.com:seok930927/RPI5-DDS-setup.git`)로 설정.
  - (Deploy Key는 한 키당 한 레포만. RPI5 키와 별개로 RPI4 키를 추가하면 됨.)
- 클론만 할 거면 인증 없이: `git clone https://github.com/seok930927/RPI5-DDS-setup.git`
```
