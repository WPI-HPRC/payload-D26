import base64
import time
from pathlib import Path
from datetime import datetime
from collections import deque

import serial

try:
    import cv2
    import numpy as np
except ImportError as exc:
    raise SystemExit(
        "This script needs opencv-python and numpy to write MP4 video. "
        "Install them with: pip install opencv-python numpy"
    ) from exc


# ---------------- CONFIG ----------------

PORT = "COM22"
BAUD = 115200

project_root = Path(__file__).parent.parent
OUTPUT_FOLDER = project_root / "images" / "received" / "video"
FRAME_FOLDER = OUTPUT_FOLDER / "frames"

VIDEO_FPS = 2.0
VIDEO_CODEC = "mp4v"
VIDEO_FILENAME = "received_feed.mp4"

SERIAL_TIMEOUT_SECONDS = 0.05

DEBUG_HISTORY_LINES = 120
PARTIAL_DUMP_CHARS = 2000

READ_SIZE = 1024

# ----------------------------------------


def make_frame_path(frame_folder: Path, frame_number: int) -> Path:
    return frame_folder / f"frame_{frame_number:06d}.jpg"


def make_video_path(output_folder: Path) -> Path:
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    stem = Path(VIDEO_FILENAME).stem
    suffix = Path(VIDEO_FILENAME).suffix or ".mp4"
    return output_folder / f"{stem}_{timestamp}{suffix}"


class VideoRecorder:
    def __init__(self, output_folder: Path, frame_folder: Path):
        self.output_folder = output_folder
        self.frame_folder = frame_folder
        self.video_path = make_video_path(output_folder)
        self.writer = None
        self.frame_size = None
        self.frame_count = 0

        self.output_folder.mkdir(parents=True, exist_ok=True)
        self.frame_folder.mkdir(parents=True, exist_ok=True)

    def add_jpeg_frame(self, image_bytes: bytes) -> bool:
        frame_array = np.frombuffer(image_bytes, dtype=np.uint8)
        frame = cv2.imdecode(frame_array, cv2.IMREAD_COLOR)

        if frame is None:
            print("[ERROR] OpenCV could not decode JPEG frame")
            return False

        height, width = frame.shape[:2]

        if self.writer is None:
            self.frame_size = (width, height)
            fourcc = cv2.VideoWriter_fourcc(*VIDEO_CODEC)
            self.writer = cv2.VideoWriter(
                str(self.video_path),
                fourcc,
                VIDEO_FPS,
                self.frame_size,
            )

            if not self.writer.isOpened():
                raise RuntimeError(f"Failed to open video writer: {self.video_path}")

            print(f"[INFO] Video output: {self.video_path}")
            print(f"[INFO] Video frame size: {width}x{height}, fps={VIDEO_FPS}")

        if (width, height) != self.frame_size:
            frame = cv2.resize(frame, self.frame_size, interpolation=cv2.INTER_AREA)
            print(
                "[WARN] Resized frame from "
                f"{width}x{height} to {self.frame_size[0]}x{self.frame_size[1]}"
            )

        self.frame_count += 1
        frame_path = make_frame_path(self.frame_folder, self.frame_count)

        with open(frame_path, "wb") as f:
            f.write(image_bytes)

        self.writer.write(frame)
        print(f"[INFO] Stored frame {self.frame_count}: {frame_path}")
        return True

    def close(self):
        if self.writer is not None:
            self.writer.release()
            print(f"[INFO] Finalized video: {self.video_path}")
            print(f"[INFO] Total video frames: {self.frame_count}")


def decode_frame(base64_text: str, expected_bytes: int):
    expected_base64_chars = 4 * ((expected_bytes + 2) // 3) if expected_bytes >= 0 else None

    if expected_base64_chars is not None:
        print(f"[INFO] Expected base64 chars: {expected_base64_chars}")
        print(f"[INFO] Actual base64 chars:   {len(base64_text)}")
        print(f"[INFO] Base64 char delta:    {len(base64_text) - expected_base64_chars}")

    try:
        image_bytes = base64.b64decode(base64_text, validate=True)
    except Exception as exc:
        print(f"[ERROR] Failed to decode base64 frame: {exc}")
        return None

    print(f"[INFO] Decoded bytes: {len(image_bytes)}")
    print(f"[INFO] Expected bytes: {expected_bytes}")

    if expected_bytes >= 0:
        print(f"[INFO] Decoded byte delta: {len(image_bytes) - expected_bytes}")

    if expected_bytes >= 0 and len(image_bytes) != expected_bytes:
        print("[WARN] Decoded byte count does not match expected size")

    return image_bytes


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


def handle_line(line, state, recorder: VideoRecorder):
    receiving = state["receiving"]
    expected_bytes = state["expected_bytes"]
    base64_chunks = state["base64_chunks"]
    decoded_history = state["decoded_history"]

    line = line.strip("\r\n")
    decoded_history.append(line)

    if not line:
        return

    if line.startswith("DBG_"):
        print(f"[BOARD] {line}")
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
        print("[INFO] Receiving base64 frame...")
        return

    if line == "IMG_END":
        if not receiving:
            print("[WARN] IMG_END received while not receiving")
            return

        print("[INFO] IMG_END received")

        base64_text = "".join(base64_chunks)
        print(f"[INFO] Received chunks: {len(base64_chunks)}")
        print(f"[INFO] Total base64 chars: {len(base64_text)}")

        image_bytes = decode_frame(base64_text, expected_bytes)
        if image_bytes is not None:
            recorder.add_jpeg_frame(image_bytes)

        state["receiving"] = False
        state["expected_bytes"] = -1
        state["base64_chunks"] = []
        print("[INFO] Waiting for next IMG_BEGIN...")
        return

    if receiving:
        base64_chunks.append(line)

        now = time.time()
        if now - state["last_progress_time"] > 1.0:
            chars = sum(len(chunk) for chunk in base64_chunks)
            print(f"[INFO] Receiving... chunks={len(base64_chunks)}, chars={chars}")
            state["last_progress_time"] = now
    else:
        print(f"[DEBUG] Ignoring outside transfer: {repr(line)}")


def main():
    print("[INFO] Starting continuous image-to-video receiver")
    print(f"[INFO] Port: {PORT}")
    print(f"[INFO] Baud: {BAUD}")
    print(f"[INFO] Output folder: {OUTPUT_FOLDER.resolve()}")
    print("[INFO] Waiting for IMG_BEGIN...")

    raw_history = deque(maxlen=DEBUG_HISTORY_LINES)
    decoded_history = deque(maxlen=DEBUG_HISTORY_LINES)
    recorder = VideoRecorder(OUTPUT_FOLDER, FRAME_FOLDER)

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
                    except Exception as exc:
                        print(f"[WARN] Decode error: {exc}")
                        continue

                    handle_line(line, state, recorder)

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

    except serial.SerialException as exc:
        print(f"[ERROR] Serial error: {exc}")

    finally:
        recorder.close()


if __name__ == "__main__":
    main()
