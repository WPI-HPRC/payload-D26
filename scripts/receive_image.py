import serial
import base64
import time
from pathlib import Path
from datetime import datetime
from collections import deque

# ---------------- CONFIG ----------------

PORT = "COM22"
BAUD = 115200

project_root = Path(__file__).parent.parent
OUTPUT_FOLDER = project_root / "images" / "test-transfers" / "received"
OUTPUT_EXTENSION = ".png"

SERIAL_TIMEOUT_SECONDS = 0.05

DEBUG_HISTORY_LINES = 120
PARTIAL_DUMP_CHARS = 2000

READ_SIZE = 1024

# ----------------------------------------


def make_output_path(output_folder: Path, extension: str) -> Path:
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    return output_folder / f"received_image_{timestamp}{extension}"


def decode_and_write_image(base64_text: str, expected_bytes: int):
    print("[INFO] Decoding base64 data...")

    expected_base64_chars = 4 * ((expected_bytes + 2) // 3) if expected_bytes >= 0 else None

    if expected_base64_chars is not None:
        print(f"[INFO] Expected base64 chars: {expected_base64_chars}")
        print(f"[INFO] Actual base64 chars:   {len(base64_text)}")

    try:
        image_bytes = base64.b64decode(base64_text, validate=True)
    except Exception as e:
        print(f"[ERROR] Failed to decode base64 data: {e}")
        return

    print(f"[INFO] Decoded bytes: {len(image_bytes)}")
    print(f"[INFO] Expected bytes: {expected_bytes}")

    if expected_bytes >= 0 and len(image_bytes) != expected_bytes:
        print("[WARN] Decoded byte count does not match expected size")

    OUTPUT_FOLDER.mkdir(parents=True, exist_ok=True)
    output_path = make_output_path(OUTPUT_FOLDER, OUTPUT_EXTENSION)

    with open(output_path, "wb") as f:
        f.write(image_bytes)

    print(f"[INFO] Wrote image: {output_path}")
    print("----------------------------------------")


def dump_debug_state(raw_history, decoded_history, receiving, expected_bytes, base64_chunks, line_buffer):
    print("\n========== DEBUG DUMP ==========")
    print(f"receiving: {receiving}")
    print(f"expected_bytes: {expected_bytes}")
    print(f"chunks_received: {len(base64_chunks)}")

    base64_text = "".join(base64_chunks)
    print(f"base64_chars_received: {len(base64_text)}")

    if expected_bytes >= 0:
        expected_base64_chars = 4 * ((expected_bytes + 2) // 3)
        print(f"expected_base64_chars: {expected_base64_chars}")
        print(f"base64_char_delta: {len(base64_text) - expected_base64_chars}")

    print(f"line_buffer_len: {len(line_buffer)}")
    print(f"line_buffer_tail: {line_buffer[-300:]!r}")

    print("\n----- Recent decoded lines -----")
    for i, line in enumerate(decoded_history):
        print(f"{i:03d}: {repr(line)}")

    print("\n----- Recent raw reads -----")
    for i, raw in enumerate(raw_history):
        print(f"{i:03d}: {raw!r}")

    print("\n----- Partial base64 end -----")
    print(base64_text[-PARTIAL_DUMP_CHARS:])

    print("========== END DEBUG DUMP ==========\n")


def handle_line(line, state):
    receiving = state["receiving"]
    expected_bytes = state["expected_bytes"]
    base64_chunks = state["base64_chunks"]
    decoded_history = state["decoded_history"]

    line = line.strip("\r\n")
    decoded_history.append(line)

    if not line:
        return

    if line.startswith("IMG_BEGIN"):
        print("[INFO] IMG_BEGIN received")

        parts = line.split()
        if len(parts) >= 2:
            try:
                expected_bytes = int(parts[1])
            except ValueError:
                expected_bytes = -1
        else:
            expected_bytes = -1

        print(f"[INFO] Expected decoded image size: {expected_bytes} bytes")

        state["receiving"] = True
        state["expected_bytes"] = expected_bytes
        state["base64_chunks"] = []
        state["last_progress_time"] = time.time()
        print("[INFO] Receiving base64 data...")
        return

    if line == "IMG_END":
        if not receiving:
            print("[WARN] IMG_END received while not receiving")
            return

        print("[INFO] IMG_END received")

        base64_text = "".join(base64_chunks)
        print(f"[INFO] Received chunks: {len(base64_chunks)}")
        print(f"[INFO] Total base64 chars: {len(base64_text)}")

        decode_and_write_image(base64_text, expected_bytes)

        state["receiving"] = False
        state["expected_bytes"] = -1
        state["base64_chunks"] = []
        print("[INFO] Waiting for next IMG_BEGIN...")
        return

    if receiving:
        base64_chunks.append(line)

        # Time-based progress only, much less spammy than every 25 chunks
        now = time.time()
        if now - state["last_progress_time"] > 1.0:
            chars = sum(len(chunk) for chunk in base64_chunks)
            print(f"[INFO] Receiving... chunks={len(base64_chunks)}, chars={chars}")
            state["last_progress_time"] = now
    else:
        print(f"[DEBUG] Ignoring outside transfer: {repr(line)}")


def main():
    print("[INFO] Starting continuous image receiver")
    print(f"[INFO] Port: {PORT}")
    print(f"[INFO] Baud: {BAUD}")
    print(f"[INFO] Output folder: {OUTPUT_FOLDER.resolve()}")
    print("[INFO] Waiting for IMG_BEGIN...")

    raw_history = deque(maxlen=DEBUG_HISTORY_LINES)
    decoded_history = deque(maxlen=DEBUG_HISTORY_LINES)

    state = {
        "receiving": False,
        "expected_bytes": -1,
        "base64_chunks": [],
        "decoded_history": decoded_history,
        "last_progress_time": time.time(),
    }

    line_buffer = b""

    try:
        with serial.Serial(PORT, BAUD, timeout=SERIAL_TIMEOUT_SECONDS) as ser:
            time.sleep(2)
            ser.reset_input_buffer()

            while True:
                data = ser.read(READ_SIZE)

                if not data:
                    continue

                raw_history.append(data)
                line_buffer += data

                while b"\n" in line_buffer:
                    raw_line, line_buffer = line_buffer.split(b"\n", 1)

                    try:
                        line = raw_line.decode("ascii", errors="replace")
                    except Exception as e:
                        print(f"[WARN] Decode error: {e}")
                        continue

                    handle_line(line, state)

    except KeyboardInterrupt:
        print("\n[INFO] Receiver stopped by user")
        dump_debug_state(
            raw_history=raw_history,
            decoded_history=decoded_history,
            receiving=state["receiving"],
            expected_bytes=state["expected_bytes"],
            base64_chunks=state["base64_chunks"],
            line_buffer=line_buffer,
        )

    except serial.SerialException as e:
        print(f"[ERROR] Serial error: {e}")


if __name__ == "__main__":
    main()