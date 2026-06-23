# 작업 히스토리 인덱스

DDS 테스트 프로젝트의 작업 기록을 시간순으로 관리하는 폴더입니다.
새 작업을 할 때마다 `YYYY-MM-DD_작업명.md` 파일을 추가하고 아래 표에 한 줄 등록합니다.

## 기록 목록

| 날짜 | 작업 | 파일 | 상태 |
|------|------|------|------|
| 2026-06-23 | Fast DDS v3.6.1 소스빌드 설치 (RPI5) | [2026-06-23_fastdds-install.md](2026-06-23_fastdds-install.md) | ✅ 완료 |
| 2026-06-23 | RPI5 단독 통신 테스트 (설치 검증) | [2026-06-23_standalone-test.md](2026-06-23_standalone-test.md) | ✅ 완료 |
| 2026-06-23 | RPI4 Fast DDS 설치 + 단독 검증 (Publisher, STEP1~2) | [2026-06-23_rpi4-fastdds-install.md](2026-06-23_rpi4-fastdds-install.md) | ✅ 완료 |
| 2026-06-23 | 2노드 실통신 테스트 (시나리오 1-1) | [2026-06-23_2node-test.md](2026-06-23_2node-test.md) | ✅ 성공 |
| 2026-06-23 | 자동 시나리오 러너 2노드 실행 (PHASE 1~3, 11개) | [2026-06-23_rpi4-auto-scenarios.md](2026-06-23_rpi4-auto-scenarios.md) | ✅ 11/11 PASS |

## 폴더 구조

```
/home/lihan/claude/
├── DDS_테스트_시나리오.md      # 테스트 시나리오 문서
├── examples/
│   └── helloworld/             # Pub/Sub 예제 (시나리오 1-1용)
├── logs/                       # 설치/빌드/테스트 raw 로그
└── history/                    # 작업 히스토리 (이 폴더)
    ├── README.md               # 이 인덱스
    ├── 2026-06-23_fastdds-install.md
    └── 2026-06-23_standalone-test.md
```

## 기록 규칙

- **logs/** : 명령 실행의 원본 출력(raw). 자동 생성.
- **history/** : 사람이 읽는 작업 요약 — 무엇을/왜/어떻게 했는지, 타임라인, 다음 할 일.
- 파일명: `YYYY-MM-DD_작업명.md`
- 각 기록에 관련 로그 파일을 링크/참조해 둘 것.
