#!/usr/bin/env bash
set -euo pipefail

ROOT="$HOME/Documents/GitHub/iot-lab"
REPORT_DIR="$ROOT/reports/lab4_task2"
OUT="$REPORT_DIR/summary.md"

mkdir -p "$REPORT_DIR"

awk '
BEGIN {
  print "# Lab 4 Task 2 Benchmark Summary"
  print ""
  print "Block size: 512 samples"
  print ""
  print "Sample rate: 16 kHz"
  print ""
  print "Real-time budget: 32 ms = 5,120,000 cycles at 160 MHz"
  print ""
}

FNR == 1 {
  expected = ""
  if (FILENAME ~ /_DFT_/) expected = "DFT"
  if (FILENAME ~ /_FFT_/) expected = "FFT"
}

{
  gsub(/\033\[[0-9;]*[A-Za-z]/, "", $0)

  if ($0 !~ /mode=(DFT|FFT)/) next

  mode = ""
  cycles = ""
  time_ms = ""
  load = ""
  overrun = ""
  errors = ""
  peak = ""

  for (i = 1; i <= NF; i++) {
    if ($i ~ /^mode=/) {
      split($i, a, "="); mode = a[2]
    }
    if ($i ~ /^cycles=/) {
      split($i, a, "="); cycles = a[2] + 0
    }
    if ($i ~ /^time=/) {
      split($i, a, "="); time_ms = a[2] + 0.0
    }
    if ($i ~ /^load=/) {
      split($i, a, "="); gsub(/%/, "", a[2]); load = a[2] + 0.0
    }
    if ($i ~ /^overrun=/) {
      split($i, a, "="); overrun = a[2] + 0
    }
    if ($i ~ /^errors=/) {
      split($i, a, "="); errors = a[2] + 0
    }
    if ($i ~ /^peak=/) {
      split($i, a, "="); gsub(/Hz/, "", a[2]); peak = a[2] + 0
    }
  }

  # Important: ignore stale FFT lines inside DFT logs, and stale DFT lines inside FFT logs.
  if (expected != "" && mode != expected) next
  if (mode == "" || cycles == "") next

  n[mode]++
  sum_cycles[mode] += cycles
  sum_time[mode] += time_ms
  sum_load[mode] += load
  sum_peak[mode] += peak

  if (n[mode] == 1 || cycles < min_cycles[mode]) min_cycles[mode] = cycles
  if (cycles > max_cycles[mode]) max_cycles[mode] = cycles

  if (n[mode] == 1) first_overrun[mode] = overrun
  last_overrun[mode] = overrun

  if (errors > max_errors[mode]) max_errors[mode] = errors
}

END {
  print "| Mode | N | Avg cycles | Min cycles | Max cycles | Avg time ms | Avg load % | Overrun first→last | Max errors | Avg peak Hz |"
  print "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|"

  modes[1] = "DFT"
  modes[2] = "FFT"

  for (j = 1; j <= 2; j++) {
    m = modes[j]
    if (n[m] == 0) continue

    printf "| %s | %d | %.0f | %.0f | %.0f | %.3f | %.2f | %d→%d | %d | %.0f |\n", \
      m, n[m], \
      sum_cycles[m] / n[m], \
      min_cycles[m], \
      max_cycles[m], \
      sum_time[m] / n[m], \
      sum_load[m] / n[m], \
      first_overrun[m], \
      last_overrun[m], \
      max_errors[m], \
      sum_peak[m] / n[m]
  }

  if (n["DFT"] > 0 && n["FFT"] > 0) {
    dft_avg = sum_cycles["DFT"] / n["DFT"]
    fft_avg = sum_cycles["FFT"] / n["FFT"]
    print ""
    printf "Speedup DFT/FFT: %.1fx\n", dft_avg / fft_avg
  }

  print ""
  print "Interpretation:"
  print ""
  print "- The naive DFT is expected to exceed the 32 ms real-time budget and to show increasing overrun."
  print "- The CMSIS-DSP FFT is expected to stay well below the 32 ms budget and keep overrun at zero or near zero."
}
' "$REPORT_DIR"/*_Release.log | tee "$OUT"

echo
echo "summary saved to:"
echo "$OUT"
