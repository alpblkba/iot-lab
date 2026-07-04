#!/usr/bin/env python3
"""Capture PCM audio from the B-U585I board and save it as a WAV file."""

import sys
import time
import wave
import serial


PORT = "/dev/cu.usbmodem21403"
BAUD_RATE = 115200
OUTPUT_PATH = "capture.wav"
CAPTURE_TIMEOUT_S = 20.0
SERIAL_TIMEOUT_S = 0.2
BOARD_READY_DELAY_S = 0.5
DRAIN_QUIET_S = 0.1

BEGIN_MARKER = b"BEGIN_PCM "
END_MARKER = b"END_PCM"


def read_until(ser, marker, timeout_s):
    deadline = time.monotonic() + timeout_s
    data = bytearray()

    while marker not in data:
        if time.monotonic() > deadline:
            message = f"timed out waiting for {marker!r}"
            text = data.decode("utf-8", errors="replace").replace("\r\n", "\n").strip()
            if text:
                message = f"{message}; received: {text}"
            raise TimeoutError(message)

        chunk = ser.read(1)
        if chunk:
            data.extend(chunk)

    return bytes(data)


def read_exact(ser, size, timeout_s):
    deadline = time.monotonic() + timeout_s
    data = bytearray()

    while len(data) < size:
        if time.monotonic() > deadline:
            raise TimeoutError(f"received {len(data)} of {size} bytes")

        chunk = ser.read(size - len(data))
        if chunk:
            data.extend(chunk)

    return bytes(data)


def read_line(ser, timeout_s):
    return read_until(ser, b"\n", timeout_s).strip()


def read_text_line(ser, timeout_s):
    return read_line(ser, timeout_s).decode("utf-8", errors="replace").strip()


def drain_input(ser, quiet_s=DRAIN_QUIET_S):
    deadline = time.monotonic() + quiet_s

    while time.monotonic() < deadline:
        waiting = ser.in_waiting
        if waiting:
            ser.read(waiting)
            deadline = time.monotonic() + quiet_s
        else:
            time.sleep(0.01)


def print_board_text(data):
    text = data.decode("utf-8", errors="replace").replace("\r\n", "\n").strip()
    if text:
        print(text)


def print_menu():
    print()
    print("B-U585I UART Audio Tool")
    print(f"Serial: {PORT} @ {BAUD_RATE} baud")
    print("Commands:")
    print("  r, record  record 2 seconds and save a WAV")
    print("  i, info    show capture format reported by the board")
    print("  h, help    show this menu")
    print("  q, quit    close the serial port")
    print()


def request_info(ser, timeout_s):
    ser.reset_input_buffer()
    ser.write(b"i")
    ser.flush()
    print(read_text_line(ser, timeout_s))


def parse_begin_pcm_line(line):
    tokens = line.decode("ascii", errors="replace").strip().split()
    if not tokens:
        raise ValueError("missing PCM size")

    pcm_size = int(tokens[0])
    metadata = {}
    for token in tokens[1:]:
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        metadata[key] = int(value)

    required = ("sample_rate", "bits", "channels")
    missing = [key for key in required if key not in metadata]
    if missing:
        raise ValueError(f"missing PCM metadata: {', '.join(missing)}")

    return pcm_size, metadata


def write_wav(output_path, pcm_data, sample_rate, bits_per_sample, channels):
    if bits_per_sample % 8 != 0:
        raise ValueError(f"unsupported sample width: {bits_per_sample} bits")

    sample_width = bits_per_sample // 8
    frame_size = sample_width * channels
    if frame_size == 0 or len(pcm_data) % frame_size != 0:
        raise ValueError(
            f"PCM payload size {len(pcm_data)} is not aligned to {frame_size} byte frames"
        )

    with wave.open(output_path, "wb") as output_file:
        output_file.setnchannels(channels)
        output_file.setsampwidth(sample_width)
        output_file.setframerate(sample_rate)
        output_file.writeframes(pcm_data)


def capture_wav(ser, output_path, timeout_s):
    ser.reset_input_buffer()
    ser.write(b"r")
    ser.flush()

    prefix = read_until(ser, BEGIN_MARKER, timeout_s)
    print_board_text(prefix[: -len(BEGIN_MARKER)])

    begin_line = read_line(ser, timeout_s)
    pcm_size, metadata = parse_begin_pcm_line(begin_line)

    pcm_data = read_exact(ser, pcm_size, timeout_s)

    suffix = read_until(ser, END_MARKER, timeout_s)
    print_board_text(suffix[: -len(END_MARKER)])
    drain_input(ser)

    write_wav(
        output_path,
        pcm_data,
        metadata["sample_rate"],
        metadata["bits"],
        metadata["channels"],
    )

    print(
        f"wrote {output_path} "
        f"({pcm_size} PCM bytes, {metadata['sample_rate']} Hz, "
        f"{metadata['bits']}-bit, {metadata['channels']} ch)"
    )


def interactive_cli(ser, output_path, timeout_s):
    print_menu()

    while True:
        try:
            cmd = input("audio> ").strip().lower()
        except EOFError:
            print()
            return

        if cmd in ("q", "quit", "exit"):
            return
        if cmd in ("h", "help", "?"):
            print_menu()
            continue
        if cmd in ("i", "info"):
            try:
                request_info(ser, timeout_s)
            except (TimeoutError, ValueError, OSError) as exc:
                print(f"info failed: {exc}")
                drain_input(ser)
            continue
        if cmd in ("r", "record"):
            try:
                capture_wav(ser, output_path, timeout_s)
            except (TimeoutError, ValueError, OSError) as exc:
                print(f"capture failed: {exc}")
                drain_input(ser)
            continue
        if cmd == "":
            continue

        print("unknown command; enter h for help")


def main():
    try:
        with serial.Serial(PORT, BAUD_RATE, timeout=SERIAL_TIMEOUT_S) as ser:
            time.sleep(BOARD_READY_DELAY_S)
            drain_input(ser)
            interactive_cli(ser, OUTPUT_PATH, CAPTURE_TIMEOUT_S)
    except serial.SerialException as exc:
        print(
            f"failed to open serial port {PORT} at {BAUD_RATE} baud: {exc}",
            file=sys.stderr,
        )
        return 1
    except (TimeoutError, ValueError, OSError) as exc:
        print(f"capture failed: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
