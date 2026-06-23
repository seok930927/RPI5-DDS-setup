#!/bin/bash
# DDS 테스트 프로그램을 tcpdump 캡처와 함께 실행하는 래퍼.
# 캡처 시작 → 프로그램 실행 → (끝나거나 Ctrl+C 시) 캡처 정지·저장까지 자동.
#
# 사용법:
#   ./run_with_capture.sh auto_sub                 # RPI5: 자동 시나리오 팔로워 + 캡처
#   ./run_with_capture.sh auto_pub                 # RPI4: 자동 시나리오 오케스트레이터 + 캡처
#   ./run_with_capture.sh subscriber --reliability reliable   # 수동 프로그램도 가능
#
# 환경변수:
#   IFACE=eth0     캡처 인터페이스 (기본 eth0)
#   FILTER='udp'   tcpdump 필터 (기본 udp = 모든 DDS 트래픽)
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD="$SCRIPT_DIR/build"
CAP_DIR="$REPO_ROOT/captures"
mkdir -p "$CAP_DIR"

if [ $# -lt 1 ]; then
    echo "사용법: $0 <auto_sub|auto_pub|publisher|subscriber> [프로그램 인자...]"
    exit 1
fi

ROLE="$1"; shift
IFACE="${IFACE:-eth0}"
FILTER="${FILTER:-udp}"

if [ ! -x "$BUILD/$ROLE" ]; then
    echo "[오류] $BUILD/$ROLE 없음. 먼저 빌드하세요: (cd $SCRIPT_DIR && cmake --build build)"
    exit 1
fi

STAMP="$(date +%Y%m%d_%H%M%S)"
PCAP="$CAP_DIR/dds_${ROLE}_$(hostname)_${STAMP}.pcap"

stop_capture() {
    sudo pkill -F "$PCAP.pid" 2>/dev/null || sudo pkill -f "tcpdump.*$PCAP" 2>/dev/null
}
trap 'stop_capture' INT TERM

echo "================================================================"
echo " 캡처 인터페이스 : $IFACE   필터: '$FILTER'"
echo " 저장 파일       : $PCAP"
echo " 실행 프로그램   : $ROLE $*"
echo "================================================================"

# tcpdump 시작 (전체 패킷 -s 0, root 권한). pid 기록.
sudo sh -c "echo \$\$ > '$PCAP.pid'; exec tcpdump -i '$IFACE' -s 0 -w '$PCAP' $FILTER" >/dev/null 2>&1 &
# tcpdump가 파일을 만들고 소켓 열 시간 확보 (SPDP 첫 패킷 놓치지 않게)
for _ in 1 2 3 4 5 6 7 8 9 10; do [ -f "$PCAP" ] && break; sleep 0.3; done
sleep 1
echo "[capture] tcpdump 가동. 프로그램 실행 ↓"
echo

# 프로그램 실행 (포그라운드)
"$BUILD/$ROLE" "$@"
RC=$?

echo
echo "[capture] 프로그램 종료(rc=$RC). 캡처 정지..."
stop_capture
sleep 1
# 파일 소유권을 현재 사용자로 (tcpdump가 root로 만들었으므로)
sudo chown "$(id -un):$(id -gn)" "$PCAP" 2>/dev/null
rm -f "$PCAP.pid"

SIZE="$(du -h "$PCAP" 2>/dev/null | cut -f1)"
echo "[capture] 저장 완료: $PCAP ($SIZE)"

# tshark 있으면 RTPS 요약
if command -v tshark >/dev/null 2>&1; then
    echo "---- RTPS 요약 ----"
    echo "총 RTPS 패킷 : $(tshark -r "$PCAP" -Y rtps 2>/dev/null | wc -l)"
    echo "DATA(0x15)   : $(tshark -r "$PCAP" -Y 'rtps && rtps.sm.id==0x15' 2>/dev/null | wc -l)"
    echo "HEARTBEAT    : $(tshark -r "$PCAP" -Y 'rtps && rtps.sm.id==0x07' 2>/dev/null | wc -l)"
    echo "ACKNACK      : $(tshark -r "$PCAP" -Y 'rtps && rtps.sm.id==0x06' 2>/dev/null | wc -l)"
fi
echo "Wireshark로 열기: $PCAP  (필터: rtps)"
