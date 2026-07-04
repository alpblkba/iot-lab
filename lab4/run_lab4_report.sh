#!/usr/bin/env bash
set -euo pipefail

# Lab 4 benchmark/report harness for macOS + STM32 B-U585I-IOT02A.
# Place this file in the lab4 root directory, next to task1/ and task2/.
# Usage:
#   ./run_lab4_report.sh
# Optional env vars:
#   PORT=/dev/cu.usbmodem21403 CONFIG=Release ./run_lab4_report.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAB4_ROOT="${LAB4_ROOT:-$SCRIPT_DIR}"
PORT="${PORT:-/dev/cu.usbmodem21403}"
BAUD="${BAUD:-115200}"
CONFIG="${CONFIG:-Release}"
RUN_ID="$(date +%Y%m%d_%H%M%S)"
RUN_DIR="$LAB4_ROOT/reports/lab4_full/$RUN_ID"
PROG="${PROG:-}"

TASK1_DIR="$LAB4_ROOT/task1"
TASK2_DIR="$LAB4_ROOT/task2"
GENERATOR="$LAB4_ROOT/generate_lab4_markdown.py"

mkdir -p "$RUN_DIR"

log() {
  printf '\n== %s ==\n' "$*"
}

find_programmer() {
  if [[ -n "$PROG" && -x "$PROG" ]]; then
    echo "$PROG"
    return
  fi

  if command -v STM32_Programmer_CLI >/dev/null 2>&1; then
    command -v STM32_Programmer_CLI
    return
  fi

  local mac_app="/Applications/STMicroelectronics/STM32Cube/STM32CubeProgrammer/STM32CubeProgrammer.app/Contents/Resources/bin/STM32_Programmer_CLI"
  if [[ -x "$mac_app" ]]; then
    echo "$mac_app"
    return
  fi

  echo "Could not find STM32_Programmer_CLI. Set PROG=/path/to/STM32_Programmer_CLI" >&2
  exit 1
}

PROG="$(find_programmer)"

require_layout() {
  [[ -d "$TASK1_DIR" ]] || { echo "Missing task1 directory: $TASK1_DIR" >&2; exit 1; }
  [[ -d "$TASK2_DIR" ]] || { echo "Missing task2 directory: $TASK2_DIR" >&2; exit 1; }
  [[ -f "$GENERATOR" ]] || { echo "Missing generator: $GENERATOR" >&2; exit 1; }
}

build_project() {
  local task_dir="$1"
  local mode="${2:-}"
  local name
  name="$(basename "$task_dir")"

  echo "== build: $task_dir mode=${mode:-none} ==" >&2

  pushd "$task_dir" >/dev/null

  rm -rf build

  if [[ -n "$mode" ]]; then
    if cmake --list-presets | grep -q "\"$CONFIG\""; then
      {
        cmake --preset "$CONFIG" -DLAB4_TASK2_MODE="$mode"
        cmake --build --preset "$CONFIG"
      } 2>&1 | tee "$RUN_DIR/${name}_${mode}_build.log" >&2
    else
      {
        echo "No preset named $CONFIG; using Debug preset with CMAKE_BUILD_TYPE=$CONFIG"
        cmake --preset Debug -DCMAKE_BUILD_TYPE="$CONFIG" -DLAB4_TASK2_MODE="$mode"
        cmake --build --preset Debug
      } 2>&1 | tee "$RUN_DIR/${name}_${mode}_build.log" >&2
    fi
  else
    if cmake --list-presets | grep -q "\"$CONFIG\""; then
      {
        cmake --preset "$CONFIG"
        cmake --build --preset "$CONFIG"
      } 2>&1 | tee "$RUN_DIR/${name}_build.log" >&2
    else
      {
        echo "No preset named $CONFIG; using Debug preset with CMAKE_BUILD_TYPE=$CONFIG"
        cmake --preset Debug -DCMAKE_BUILD_TYPE="$CONFIG"
        cmake --build --preset Debug
      } 2>&1 | tee "$RUN_DIR/${name}_build.log" >&2
    fi
  fi

  local elf
  elf="$(find build -name "*.elf" -type f | head -n 1 || true)"

  if [[ -z "$elf" ]]; then
    echo "No ELF produced for $task_dir" >&2
    popd >/dev/null
    return 1
  fi

  python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "$elf"

  popd >/dev/null
}

flash_elf() {
  local elf="$1"
  local name="$2"

  log "flash $name"
  "$PROG" -c port=SWD -w "$elf" 2>&1 | tee "$RUN_DIR/${name}_flash.log"
}

reset_board() {
  "$PROG" -c port=SWD -rst >/dev/null 2>&1 || true
}

capture_screen_log() {
  local name="$1"
  local seconds="$2"
  local min_pattern="$3"
  local min_count="$4"
  local session="lab4_${name}_${RUN_ID}_$$"
  local log_file="$RUN_DIR/${name}_uart.log"

  log "capture $name UART (${seconds}s)"
  echo "port=$PORT baud=$BAUD log=$log_file"

  (
    cd "$RUN_DIR"
    rm -f screenlog.0

    # macOS screen supports -L but often not -Logfile.
    # It writes screenlog.0 in the current directory.
    screen -dmS "$session" -L "$PORT" "$BAUD"
    sleep 0.7

    # Clean reset while screen is already attached, so early UART output is captured.
    reset_board

    sleep "$seconds"

    screen -S "$session" -X quit >/dev/null 2>&1 || true
    sleep 0.5

    if [[ -f screenlog.0 ]]; then
      mv screenlog.0 "$log_file"
    else
      : > "$log_file"
    fi
  )

  local count
  count="$(grep -a -c "$min_pattern" "$log_file" || true)"
  echo "captured_lines_matching_${min_pattern}=$count" | tee -a "$RUN_DIR/${name}_capture.log"

  if (( count < min_count )); then
    echo "WARNING: expected at least $min_count matching lines for $name, got $count" | tee -a "$RUN_DIR/${name}_capture.log"
    echo "You can increase capture time with TASK1_SECONDS, DFT_SECONDS, or FFT_SECONDS." | tee -a "$RUN_DIR/${name}_capture.log"
  fi

  echo "saved $log_file"
}

write_metadata() {
  cat > "$RUN_DIR/metadata.env" <<EOF
RUN_ID=$RUN_ID
LAB4_ROOT=$LAB4_ROOT
CONFIG=$CONFIG
PORT=$PORT
BAUD=$BAUD
PROG=$PROG
TASK1_SECONDS=${TASK1_SECONDS:-6}
DFT_SECONDS=${DFT_SECONDS:-18}
FFT_SECONDS=${FFT_SECONDS:-6}
EOF
}

main() {
  require_layout
  write_metadata

  log "Lab 4 full benchmark run"
  echo "run_id=$RUN_ID"
  echo "run_dir=$RUN_DIR"
  echo "config=$CONFIG"
  echo "port=$PORT"

  # Avoid old interactive screen sessions holding the serial port.
  pkill -f "screen $PORT" >/dev/null 2>&1 || true

  # Task 1: benchmark AXPY, CONV, SAD.
  local task1_elf
  task1_elf="$(build_project "$TASK1_DIR" "")"
  flash_elf "$task1_elf" task1
  capture_screen_log task1 "${TASK1_SECONDS:-6}" "sink=" 1

  # Task 2 DFT: 5 entries.
  local task2_dft_elf
  task2_dft_elf="$(build_project "$TASK2_DIR" "DFT")"
  flash_elf "$task2_dft_elf" task2_dft
  capture_screen_log task2_dft "${DFT_SECONDS:-18}" "mode=DFT" 5

  # Task 2 FFT: 20 entries.
  local task2_fft_elf
  task2_fft_elf="$(build_project "$TASK2_DIR" "FFT")"
  flash_elf "$task2_fft_elf" task2_fft
  capture_screen_log task2_fft "${FFT_SECONDS:-6}" "mode=FFT" 20

  log "generate markdown report"
  python3 "$GENERATOR" "$RUN_DIR" | tee "$RUN_DIR/generator.log"

  log "done"
  echo "Markdown report: $RUN_DIR/lab4_benchmark_report.md"
  echo "Raw logs:        $RUN_DIR"
}

main "$@"
