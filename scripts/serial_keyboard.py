import argparse
import sys
import threading
import time

import serial


DEFAULT_PORT = "COM22"
DEFAULT_BAUD = 115200


def read_board_output(ser, stop_event):
    while not stop_event.is_set():
        try:
            data = ser.read(ser.in_waiting or 1)
        except serial.SerialException as exc:
            print(f"\n[SERIAL ERROR] {exc}")
            stop_event.set()
            return

        if data:
            text = data.decode(errors="replace")
            print(text, end="", flush=True)


def write_line_input(ser, stop_event):
    print("[INFO] Line mode. Type a line and press Enter to send it. Ctrl+C exits.")
    while not stop_event.is_set():
        try:
            line = input()
        except (EOFError, KeyboardInterrupt):
            stop_event.set()
            return

        ser.write((line + "\n").encode())
        ser.flush()


def get_key_windows():
    import msvcrt

    while True:
        if msvcrt.kbhit():
            key = msvcrt.getwch()
            if key in ("\x00", "\xe0"):
                key += msvcrt.getwch()
            return key
        time.sleep(0.01)


def key_to_bytes(key):
    if key == "\r":
        return b"\n"
    if key == "\x08":
        return b"\b"
    if key in ("\x03", "\x1a"):
        raise KeyboardInterrupt
    return key.encode(errors="replace")


def write_key_input_windows(ser, stop_event):
    print("[INFO] Key mode. Each key is sent immediately. Enter sends newline. Ctrl+C exits.")
    while not stop_event.is_set():
        try:
            data = key_to_bytes(get_key_windows())
        except KeyboardInterrupt:
            stop_event.set()
            return

        ser.write(data)
        ser.flush()


def write_key_input_posix(ser, stop_event):
    import termios
    import tty

    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    print("[INFO] Key mode. Each key is sent immediately. Enter sends newline. Ctrl+C exits.")

    try:
        tty.setraw(fd)
        while not stop_event.is_set():
            try:
                key = sys.stdin.read(1)
                data = key_to_bytes(key)
            except KeyboardInterrupt:
                stop_event.set()
                return

            ser.write(data)
            ser.flush()
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)


def write_key_input(ser, stop_event):
    if sys.platform.startswith("win"):
        write_key_input_windows(ser, stop_event)
    else:
        write_key_input_posix(ser, stop_event)


def parse_args():
    parser = argparse.ArgumentParser(description="Send keyboard input to the payload board over serial.")
    parser.add_argument("--port", default=DEFAULT_PORT, help=f"Serial port. Default: {DEFAULT_PORT}")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help=f"Baud rate. Default: {DEFAULT_BAUD}")
    parser.add_argument("--line", action="store_true", help="Send one full line at a time instead of raw keypresses.")
    return parser.parse_args()


def main():
    args = parse_args()
    stop_event = threading.Event()

    try:
        with serial.Serial(args.port, args.baud, timeout=0.05) as ser:
            time.sleep(2)
            print(f"[INFO] Opened {args.port} at {args.baud}.")

            reader = threading.Thread(target=read_board_output, args=(ser, stop_event), daemon=True)
            reader.start()

            if args.line:
                write_line_input(ser, stop_event)
            else:
                write_key_input(ser, stop_event)
    except serial.SerialException as exc:
        print(f"[ERROR] Could not open serial port: {exc}")
        return 1
    except KeyboardInterrupt:
        stop_event.set()

    print("\n[INFO] Closed serial connection.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
