#!/usr/bin/env bash
set -euo pipefail

MODE="${1:-}"
CONFIG="${CONFIG:-Release}"

if [[ "$MODE" != "dft" && "$MODE" != "fft" && "$MODE" != "DFT" && "$MODE" != "FFT" ]]; then
  echo "usage: ./scripts/lab4_task2.sh dft|fft"
  echo "optional: CONFIG=Debug ./scripts/lab4_task2.sh fft"
  exit 1
fi

MODE_UPPER="$(echo "$MODE" | tr '[:lower:]' '[:upper:]')"

ROOT="$HOME/Documents/GitHub/iot-lab"
TASK="$ROOT/lab4/task2"
PROG="/Applications/STMicroelectronics/STM32Cube/STM32CubeProgrammer/STM32CubeProgrammer.app/Contents/Resources/bin/STM32_Programmer_CLI"
PORT="/dev/cu.usbmodem21403"
BAUD="115200"
TS="$(date +%Y%m%d_%H%M%S)"
REPORT_DIR="$ROOT/reports/lab4_task2"
LOG="$REPORT_DIR/${TS}_${MODE_UPPER}_${CONFIG}.log"

mkdir -p "$REPORT_DIR"

echo "== Lab 4 Task 2: $MODE_UPPER / $CONFIG =="
echo "== log: $LOG =="

echo "== build =="
cd "$TASK"
rm -rf "build/$CONFIG" build

if cmake --list-presets | grep -q "\"$CONFIG\""; then
  cmake --preset "$CONFIG" -DLAB4_TASK2_MODE="$MODE_UPPER"
  cmake --build --preset "$CONFIG"
  ELF="$TASK/build/$CONFIG/task3.elf"
else
  echo "No CMake preset named $CONFIG; falling back to Debug preset with CMAKE_BUILD_TYPE=$CONFIG"
  cmake --preset Debug -DCMAKE_BUILD_TYPE="$CONFIG" -DLAB4_TASK2_MODE="$MODE_UPPER"
  cmake --build --preset Debug
  ELF="$TASK/build/Debug/task3.elf"
fi

echo
echo "== flash =="
"$PROG" -c port=SWD -w "$ELF" -rst

echo
echo "== serial logging =="
echo "Opening screen on $PORT @ $BAUD"
echo "Log file: $LOG"
echo "Collect a few lines, then quit with: Ctrl-A, K, y"
echo

sleep 2

# screen log mode. Old macOS screen does not support -Logfile.
# It writes to screenlog.0 in the current directory, so run it inside REPORT_DIR
# and rename the log after the user exits.
(
  cd "$REPORT_DIR"
  rm -f screenlog.0
  screen -L "$PORT" "$BAUD" || true

  if [[ -f screenlog.0 ]]; then
    mv screenlog.0 "$LOG"
  fi
)

echo
echo "== saved log =="
echo "$LOG"

echo
echo "== measurement lines =="
grep -a "mode=$MODE_UPPER" "$LOG" || true
