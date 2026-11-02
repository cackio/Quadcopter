"""Capture private diagnostics, or replay a saved binary recording (no hardware)."""
import argparse
import csv
import json
import time
from pathlib import Path
from protocol import Parser, SCHEMA, decode_diagnostic


def consume(chunks, output, raw_file=None):
    parser = Parser()
    rows = 0
    writer = None
    last_sequence = None
    duplicate_samples = 0
    with Path(output).open("w", newline="", encoding="utf-8") as csv_file:
        for chunk in chunks:
            if raw_file is not None:
                raw_file.write(chunk)
            for frame_id, payload in parser.feed(chunk):
                if frame_id != SCHEMA["DIAG_FRAME_ID"]:
                    continue
                row = decode_diagnostic(payload)
                if writer is None:
                    writer = csv.DictWriter(csv_file, fieldnames=list(row))
                    writer.writeheader()
                writer.writerow(row)
                rows += 1
                duplicate_samples += row["sequence"] == last_sequence
                last_sequence = row["sequence"]
    return {"diagnostic_frames": rows, "bad_checksums": parser.bad_checksums,
            "discarded_bytes": parser.discarded_bytes, "pending_bytes": len(parser.buffer),
            "duplicate_samples": duplicate_samples}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list", action="store_true", help="List available serial ports")
    parser.add_argument("--port", help="e.g. COM12; firmware must be in DIAG mode")
    parser.add_argument("--baud", type=int, default=9600)
    parser.add_argument("--seconds", type=float, default=120)
    parser.add_argument("--output", type=Path, default=Path("captures/diagnostic.csv"))
    parser.add_argument("--replay", type=Path, help="Saved .bin, instead of a live serial port")
    args = parser.parse_args()
    if args.list:
        from serial.tools import list_ports
        for port in list_ports.comports():
            print(port.device, port.description)
        return
    if args.seconds <= 0:
        parser.error("--seconds must be positive")
    if not args.replay and not args.port:
        parser.error("Supply --port, --replay or --list")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if args.replay:
        if args.replay.resolve() == args.output.resolve():
            parser.error("Input and output paths must differ")
        with args.replay.open("rb") as source:
            summary = consume(iter(lambda: source.read(4096), b""), args.output)
    else:
        import serial
        def live_chunks(port):
            deadline = time.monotonic() + args.seconds
            try:
                while time.monotonic() < deadline:
                    chunk = port.read(port.in_waiting or 1)
                    if chunk:
                        yield chunk
            except KeyboardInterrupt:
                return
        binary = args.output.with_suffix(".bin")
        if binary.resolve() == args.output.resolve():
            parser.error("Live output must have a .csv suffix, not .bin")
        with serial.Serial(args.port, args.baud, timeout=0.25) as port, binary.open("wb") as raw:
            summary = consume(live_chunks(port), args.output, raw)
        summary["binary"] = str(binary)
    summary["csv"] = str(args.output)
    print(json.dumps(summary, ensure_ascii=False, indent=2))
    if not summary["diagnostic_frames"]:
        raise SystemExit("No DIAG frames: check firmware mode, module UART baud and outgoing COM port")


if __name__ == "__main__":
    main()
