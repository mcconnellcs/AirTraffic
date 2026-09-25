#!/usr/bin/env python3
"""
screenshot.py — save what's on the AirTraffic screen as a PNG picture.

    pip install pyserial pillow
    python3 tools/screenshot.py /dev/ttyUSB0 my-radar.png            (Linux)
    python3 tools/screenshot.py /dev/cu.usbserial-110 radar.png       (Mac)
    python3 tools/screenshot.py COM5 radar.png                        (Windows)

To capture the start-up animation, add --reset and how many seconds to wait:

    python3 tools/screenshot.py /dev/ttyUSB0 boot.png --reset 1.2

Close the Arduino Serial Monitor first — only one program can use the port.
"""
import argparse
import time

import serial
from PIL import Image

TIMEOUT_S = 60


def read_line(port):
    line = bytearray()
    while True:
        b = port.read(1)
        if not b:
            raise TimeoutError("no reply from the board")
        if b == b"\n":
            return line.decode("utf-8", "replace").strip()
        line += b


def reset_board(port):
    # The board's reset pin is wired to the serial port's RTS signal.
    port.dtr = False
    port.rts = True
    time.sleep(0.1)
    port.rts = False


def capture(port_name, reset_delay):
    with serial.Serial(port_name, 115200, timeout=TIMEOUT_S) as port:
        if reset_delay is not None:
            reset_board(port)
            time.sleep(reset_delay)
        else:
            time.sleep(0.2)
        port.reset_input_buffer()
        port.write(b"shot\n")
        while True:
            line = read_line(port)
            if line.startswith("SHOT_BEGIN"):
                _, w, h = line.split()
                width, height = int(w), int(h)
                break
        total = width * height
        pixels = bytearray()
        done = 0
        while done < total:
            packet = port.read(3)
            if len(packet) < 3:
                raise TimeoutError("picture was cut short")
            run, hi, lo = packet[0], packet[1], packet[2]
            color = (hi << 8) | lo
            r = ((color >> 11) & 0x1F) * 255 // 31
            g = ((color >> 5) & 0x3F) * 255 // 63
            b = (color & 0x1F) * 255 // 31
            pixels += bytes((r, g, b)) * run
            done += run
        if done != total or "SHOT_END" not in read_line(port) + read_line(port):
            raise ValueError("picture data was garbled — try again")
        return Image.frombytes("RGB", (width, height), bytes(pixels))


def main():
    parser = argparse.ArgumentParser(description="Save the AirTraffic screen as a PNG.")
    parser.add_argument("port", help="serial port, e.g. /dev/ttyUSB0 or COM5")
    parser.add_argument("output", help="file to write, e.g. radar.png")
    parser.add_argument("--reset", type=float, metavar="SECONDS",
                        help="restart the board first and wait this long before capturing")
    args = parser.parse_args()
    for attempt in range(3):
        try:
            capture(args.port, args.reset).save(args.output)
            print(f"saved {args.output}")
            return
        except (TimeoutError, ValueError) as err:
            print(f"attempt {attempt + 1}: {err}")
    raise SystemExit(2)


if __name__ == "__main__":
    main()
