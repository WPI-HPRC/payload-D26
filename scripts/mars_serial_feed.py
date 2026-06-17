import json


class RoverTelemetryState:
    def __init__(self):
        self.reset_image()
        self.latest_ml_frame_id = None

    def reset_image(self):
        self.image_chunks = {}
        self.expected_chunk_count = None
        self.expected_base64_length = None
        self.expected_byte_count = -1

    def handle_line(self, line):
        try:
            packet = json.loads(line)
        except json.JSONDecodeError:
            return None

        if not isinstance(packet, dict) or packet.get("type") != "roverTelemetry":
            return None

        self._handle_ml(packet.get("ml"))
        return self._handle_image(packet.get("image"))

    def _handle_ml(self, ml_data):
        if not isinstance(ml_data, dict):
            return

        frame_id = ml_data.get("frameId")
        if frame_id == self.latest_ml_frame_id:
            return

        self.latest_ml_frame_id = frame_id
        horizon = ml_data.get("horizon") or {}
        blobs = ml_data.get("blobs") or []
        print(
            "[ML] frame=%s horizon_valid=%s horizon_y=%s blobs=%d expected=%s dropped=%s"
            % (
                frame_id,
                horizon.get("valid"),
                horizon.get("yPx"),
                len(blobs),
                ml_data.get("expectedBlobCount"),
                ml_data.get("droppedBlobs"),
            )
        )

    def _handle_image(self, image_data):
        if image_data is None:
            return None

        if not isinstance(image_data, dict):
            print("[WARN] Ignoring telemetry image field that is not an object")
            return None

        try:
            byte_count = int(image_data.get("byteCount", -1))
            base64_length = int(image_data.get("base64Length", -1))
            chunk_index = int(image_data["chunkIndex"])
            chunk_count = int(image_data["chunkCount"])
            final = bool(image_data.get("final", False))
            chunk_data = str(image_data["data"])
        except (KeyError, TypeError, ValueError) as exc:
            print(f"[WARN] Ignoring malformed telemetry image chunk: {exc}")
            return None

        if chunk_index == 0:
            self.reset_image()
            self.expected_chunk_count = chunk_count
            self.expected_base64_length = base64_length
            self.expected_byte_count = byte_count

        if self.expected_chunk_count is None:
            self.expected_chunk_count = chunk_count
            self.expected_base64_length = base64_length
            self.expected_byte_count = byte_count

        if chunk_count != self.expected_chunk_count:
            print(
                "[WARN] Telemetry image chunk count changed mid-frame: "
                f"{chunk_count} != {self.expected_chunk_count}"
            )
            self.reset_image()
            return None

        self.image_chunks[chunk_index] = chunk_data
        print(
            "[INFO] Telemetry image chunk "
            f"{chunk_index + 1}/{chunk_count}, chars={len(chunk_data)}, final={final}"
        )

        if not final and len(self.image_chunks) < self.expected_chunk_count:
            return None

        missing = [i for i in range(self.expected_chunk_count) if i not in self.image_chunks]
        if missing:
            print(f"[WARN] Telemetry image final chunk arrived with missing chunks: {missing}")
            self.reset_image()
            return None

        base64_text = "".join(self.image_chunks[i] for i in range(self.expected_chunk_count))
        expected_base64_length = self.expected_base64_length
        expected_byte_count = self.expected_byte_count
        self.reset_image()

        if expected_base64_length >= 0 and len(base64_text) != expected_base64_length:
            print(
                "[WARN] Telemetry image base64 length mismatch: "
                f"{len(base64_text)} != {expected_base64_length}"
            )

        return base64_text, expected_byte_count
