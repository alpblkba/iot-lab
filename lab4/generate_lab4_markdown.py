#!/usr/bin/env python3
from __future__ import annotations

import re
import statistics as stats
import sys
from pathlib import Path
from datetime import datetime

if len(sys.argv) != 2:
    raise SystemExit("usage: generate_lab4_markdown.py <run_dir>")

run_dir = Path(sys.argv[1]).resolve()
if not run_dir.is_dir():
    raise SystemExit(f"not a directory: {run_dir}")

out = run_dir / "lab4_benchmark_report.md"

metadata = {}
meta_file = run_dir / "metadata.env"
if meta_file.exists():
    for line in meta_file.read_text(errors="replace").splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            metadata[k] = v

ansi_re = re.compile(r"\x1b\[[0-9;?]*[ -/]*[@-~]")

def clean_text(s: str) -> str:
    s = ansi_re.sub("", s)
    # Keep printable ASCII-ish + UTF-8 terminal chars; remove common control clutter.
    s = s.replace("\x00", "")
    return s


def read_log(name: str) -> str:
    p = run_dir / name
    if not p.exists():
        return ""
    return clean_text(p.read_text(errors="replace"))

# ---------------- Task 1 parsing ----------------

task1_text = read_log("task1_uart.log")
# Example:
# AXPY base      : PASS cycles=136067
# AXPY unrolled  : PASS cycles=122008 speedup=1.11 x
# CONV SIMD      : PASS cycles=3112587 speedup=1.82 x
# SAD SIMD bonus : PASS cycles=40236 speedup=3.48 x value=348396
task1_re = re.compile(
    r"^(?P<kernel>AXPY|CONV|SAD)\s+(?P<variant>.+?)\s*:\s*"
    r"(?P<status>PASS|FAIL)"
    r"(?:\s+cycles=(?P<cycles>\d+))?"
    r"(?:\s+speedup=(?P<speedup>[0-9.]+)\s*x)?"
    r"(?:\s+value=(?P<value>\d+))?",
    re.MULTILINE,
)

task1_rows = []
for m in task1_re.finditer(task1_text):
    task1_rows.append(
        {
            "kernel": m.group("kernel"),
            "variant": " ".join(m.group("variant").split()),
            "status": m.group("status"),
            "cycles": int(m.group("cycles")) if m.group("cycles") else None,
            "speedup": float(m.group("speedup")) if m.group("speedup") else None,
            "value": int(m.group("value")) if m.group("value") else None,
        }
    )

# ---------------- Task 2 parsing ----------------

# Example:
# mode=FFT block=0 half=1 cycles=142700 time=0.892 ms load=2.78% overrun=0 errors=0 peak=31Hz
task2_re = re.compile(
    r"mode=(?P<mode>DFT|FFT)\s+"
    r"block=(?P<block>\d+)\s+"
    r"half=(?P<half>\d+)\s+"
    r"cycles=(?P<cycles>\d+)\s+"
    r"time=(?P<time_ms>[0-9.]+)\s+ms\s+"
    r"load=(?P<load_pct>[0-9.]+)%\s+"
    r"overrun=(?P<overrun>\d+)\s+"
    r"errors=(?P<errors>\d+)\s+"
    r"peak=(?P<peak_hz>\d+)Hz"
)

def parse_task2_log(filename: str, expected_mode: str, max_rows: int) -> list[dict]:
    rows = []
    for m in task2_re.finditer(read_log(filename)):
        if m.group("mode") != expected_mode:
            continue
        rows.append(
            {
                "mode": m.group("mode"),
                "block": int(m.group("block")),
                "half": int(m.group("half")),
                "cycles": int(m.group("cycles")),
                "time_ms": float(m.group("time_ms")),
                "load_pct": float(m.group("load_pct")),
                "overrun": int(m.group("overrun")),
                "errors": int(m.group("errors")),
                "peak_hz": int(m.group("peak_hz")),
            }
        )
    return rows[:max_rows]


dft_rows = parse_task2_log("task2_dft_uart.log", "DFT", 5)
fft_rows = parse_task2_log("task2_fft_uart.log", "FFT", 20)

# ---------------- helpers ----------------

def md_table(headers: list[str], rows: list[list[object]]) -> str:
    if not rows:
        return "_No rows captured._\n"
    lines = []
    lines.append("| " + " | ".join(headers) + " |")
    lines.append("|" + "|".join(["---" for _ in headers]) + "|")
    for row in rows:
        lines.append("| " + " | ".join(str(x) if x is not None else "" for x in row) + " |")
    return "\n".join(lines) + "\n"


def task2_summary(rows: list[dict], mode: str) -> dict | None:
    if not rows:
        return None
    cycles = [r["cycles"] for r in rows]
    times = [r["time_ms"] for r in rows]
    loads = [r["load_pct"] for r in rows]
    peaks = [r["peak_hz"] for r in rows]
    return {
        "mode": mode,
        "n": len(rows),
        "avg_cycles": stats.mean(cycles),
        "min_cycles": min(cycles),
        "max_cycles": max(cycles),
        "avg_time_ms": stats.mean(times),
        "avg_load_pct": stats.mean(loads),
        "overrun_first": rows[0]["overrun"],
        "overrun_last": rows[-1]["overrun"],
        "max_errors": max(r["errors"] for r in rows),
        "avg_peak_hz": stats.mean(peaks),
    }


def fmt_int(x: float | int | None) -> str:
    if x is None:
        return ""
    return f"{x:,.0f}"


def fmt_float(x: float | None, digits: int = 2) -> str:
    if x is None:
        return ""
    return f"{x:.{digits}f}"

# ---------------- interpretation ----------------

# Extract likely task1 speedups by variant name.
speedup_notes = []
for row in task1_rows:
    if row["speedup"] is not None:
        speedup_notes.append(
            f"- {row['kernel']} {row['variant']} measured a speedup of **{row['speedup']:.2f}x** over the corresponding baseline."
        )

summary_dft = task2_summary(dft_rows, "DFT")
summary_fft = task2_summary(fft_rows, "FFT")
speedup_text = None
if summary_dft and summary_fft and summary_fft["avg_cycles"]:
    speedup_text = summary_dft["avg_cycles"] / summary_fft["avg_cycles"]

# ---------------- markdown ----------------

lines: list[str] = []
lines.append("# IoT Lab 04 Benchmark Report")
lines.append("")
lines.append(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
lines.append("")
lines.append("## Run metadata")
lines.append("")
metadata_rows = [
    ["Run ID", metadata.get("RUN_ID", run_dir.name)],
    ["CMake configuration", metadata.get("CONFIG", "Release")],
    ["UART port", metadata.get("PORT", "")],
    ["Baud rate", metadata.get("BAUD", "115200")],
    ["Lab 4 root", metadata.get("LAB4_ROOT", "")],
]
lines.append(md_table(["Field", "Value"], metadata_rows))

lines.append("## Task 1 — Kernel microbenchmarks")
lines.append("")
lines.append(
    "Task 1 benchmarks the scalar and optimized implementations of the AXPY, convolution, and SAD kernels. "
    "The firmware reports DWT cycle-counter measurements over UART. Correctness is checked against golden reference outputs before interpreting the cycle counts."
)
lines.append("")
if task1_rows:
    table_rows = []
    for r in task1_rows:
        table_rows.append(
            [
                r["kernel"],
                r["variant"],
                r["status"],
                fmt_int(r["cycles"]),
                fmt_float(r["speedup"], 2) if r["speedup"] is not None else "baseline",
                r["value"] if r["value"] is not None else "",
            ]
        )
    lines.append(md_table(["Kernel", "Variant", "Status", "Cycles", "Speedup", "Value"], table_rows))
else:
    lines.append("_No Task 1 measurement lines were captured. Check `task1_uart.log`._\n")

lines.append("### Task 1 observations")
lines.append("")
if speedup_notes:
    lines.extend(speedup_notes)
else:
    lines.append("- No optimized speedup lines were captured from Task 1.")
lines.append("- The AXPY optimization mainly reduces loop overhead through unrolling.")
lines.append("- The convolution optimization uses packed 16-bit multiply-accumulate style operations, reducing the cost of the inner loop.")
lines.append("- The SAD optimization uses SIMD-style byte absolute-difference accumulation, which is well matched to the `uint8_t` data layout.")
lines.append("")

lines.append("## Task 2 — Audio spectral benchmark")
lines.append("")
lines.append(
    "The audio benchmark processes 512-sample blocks from the microphone stream at 16 kHz. "
    "This gives a block deadline of `512 / 16000 = 32 ms`. "
    "At a 160 MHz CPU clock, the corresponding budget is `160e6 × 0.032 = 5,120,000 cycles`."
)
lines.append("")

summary_rows = []
for s in [summary_dft, summary_fft]:
    if not s:
        continue
    summary_rows.append(
        [
            s["mode"],
            s["n"],
            fmt_int(s["avg_cycles"]),
            fmt_int(s["min_cycles"]),
            fmt_int(s["max_cycles"]),
            fmt_float(s["avg_time_ms"], 3),
            fmt_float(s["avg_load_pct"], 2),
            f"{s['overrun_first']}→{s['overrun_last']}",
            s["max_errors"],
            fmt_float(s["avg_peak_hz"], 0),
        ]
    )
lines.append(md_table(
    ["Mode", "N", "Avg cycles", "Min cycles", "Max cycles", "Avg time ms", "Avg load %", "Overrun first→last", "Max errors", "Avg peak Hz"],
    summary_rows,
))

if speedup_text is not None:
    lines.append(f"Measured average speedup of CMSIS-DSP FFT over naive DFT: **{speedup_text:.1f}x**.")
    lines.append("")

lines.append("### DFT raw entries")
lines.append("")
lines.append(md_table(
    ["Block", "Half", "Cycles", "Time ms", "Load %", "Overrun", "Errors", "Peak Hz"],
    [[r["block"], r["half"], fmt_int(r["cycles"]), fmt_float(r["time_ms"], 3), fmt_float(r["load_pct"], 2), r["overrun"], r["errors"], r["peak_hz"]] for r in dft_rows],
))

lines.append("### FFT raw entries")
lines.append("")
lines.append(md_table(
    ["Block", "Half", "Cycles", "Time ms", "Load %", "Overrun", "Errors", "Peak Hz"],
    [[r["block"], r["half"], fmt_int(r["cycles"]), fmt_float(r["time_ms"], 3), fmt_float(r["load_pct"], 2), r["overrun"], r["errors"], r["peak_hz"]] for r in fft_rows],
))

lines.append("### Task 2 observations")
lines.append("")
if summary_dft:
    if summary_dft["avg_cycles"] > 5_120_000:
        lines.append(
            f"- The naive DFT is far above the real-time budget: average `{fmt_int(summary_dft['avg_cycles'])}` cycles versus `5,120,000` cycles available per block."
        )
    else:
        lines.append("- The naive DFT did not exceed the nominal cycle budget in this run; verify that the intended DFT path was selected.")
    if summary_dft["overrun_last"] > summary_dft["overrun_first"]:
        lines.append("- The DFT overrun counter increases during the capture, confirming that audio blocks arrive faster than they are processed.")
if summary_fft:
    if summary_fft["avg_cycles"] < 5_120_000:
        lines.append(
            f"- The CMSIS-DSP FFT is comfortably inside the real-time budget: average `{fmt_int(summary_fft['avg_cycles'])}` cycles."
        )
    if summary_fft["overrun_last"] == summary_fft["overrun_first"]:
        lines.append("- The FFT overrun counter remained stable during the captured entries.")
lines.append("- The FFT result demonstrates why an algorithmic change from O(N²) DFT to O(N log N) FFT is more important than small loop-level optimizations for this workload.")
lines.append("")

lines.append("## Captured files")
lines.append("")
file_rows = []
for filename in [
    "metadata.env",
    "task1_build.log",
    "task1_flash.log",
    "task1_uart.log",
    "task2_dft_build.log",
    "task2_dft_flash.log",
    "task2_dft_uart.log",
    "task2_fft_build.log",
    "task2_fft_flash.log",
    "task2_fft_uart.log",
]:
    p = run_dir / filename
    if p.exists():
        file_rows.append([filename, f"{p.stat().st_size} bytes"])
lines.append(md_table(["File", "Size"], file_rows))

out.write_text("\n".join(lines) + "\n")
print(f"wrote {out}")
