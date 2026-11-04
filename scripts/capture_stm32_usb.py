#!/usr/bin/env python3
"""Save the STM32's versioned SPI payload trace from its USB CDC port.

Requires pyserial. The output is plain text containing F (frame) and S (stats)
CSV records; the original bytes from the device are preserved without decoding.
"""
import argparse
from datetime import datetime, timezone
from pathlib import Path
import sys
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list", action="store_true", help="List serial ports")
    parser.add_argument("--port", help="e.g. /dev/ttyACM0 or COM5")
    parser.add_argument("--output", type=Path, help="New trace file (never overwritten)")
    parser.add_argument("--seconds", type=float, help="Stop after this many seconds; default until Ctrl-C")
    args = parser.parse_args()
    if args.seconds is not None and args.seconds <= 0:
        parser.error("--seconds must be positive")
    try:
        import serial
        from serial.tools import list_ports
    except ImportError:
        parser.exit(1, "Install pyserial first: python3 -m pip install pyserial\n")
    if args.list:
        for port in list_ports.comports():
            print(f"{port.device}\t{port.description}")
        return 0
    if not args.port or not args.output:
        parser.error("--port and --output are required for capture")
    frames = [0, 0]
    flagged = 0
    last_stats = ""
    try:
        # Open exclusively before touching the serial port, to protect old logs.
        with args.output.open("xb") as output:
            stamp = datetime.now(timezone.utc).isoformat()
            output.write(f"#PC UTC start {stamp}\n".encode("ascii"))
            with serial.Serial(args.port, baudrate=115200, timeout=0.25) as port:
                port.dtr = True  # Enables firmware streaming; baud does not clock USB.
                start = time.monotonic()
                pending = b""
                print(f"Recording {args.port} to {args.output}; Ctrl-C to stop", file=sys.stderr)
                try:
                    while args.seconds is None or time.monotonic() - start < args.seconds:
                        data = port.read(4096)
                        if not data:
                            continue
                        output.write(data)
                        output.flush()
                        pending += data
                        while b"\n" in pending:
                            line, pending = pending.split(b"\n", 1)
                            fields = line.split(b",")
                            if len(fields) == 6 and fields[0] == b"F":
                                try:
                                    bus = int(fields[1])
                                    if bus not in (1, 2):
                                        continue
                                    frames[bus - 1] += 1
                                    flagged += int(fields[4], 16) != 0
                                except ValueError:
                                    pass
                            elif fields[0] == b"S":
                                last_stats = line.decode("ascii", errors="replace")
                        # Preserve arbitrary device data without unbounded parser memory.
                        if len(pending) > 4096:
                            pending = b""
                except KeyboardInterrupt:
                    pass
                finally:
                    port.dtr = False
    except (OSError, serial.SerialException) as exc:
        print(f"Capture failed: {exc}", file=sys.stderr)
        return 1
    print(f"Saved {frames[0]} SPI1 and {frames[1]} SPI2 frames; {flagged} flagged.", file=sys.stderr)
    if last_stats:
        print(f"Last device counters: {last_stats}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
