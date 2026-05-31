from pathlib import Path
import serial
import base64
import time

# get the file path of the image relative to this script
project_root = Path(__file__).parent.parent
image_path = project_root / "images" / "test-transfers" / "image.png"

# Update this to your serial port
PORT = "COM22"
# PORT = "/dev/ttyUSB0"  # Linux
# PORT = "/dev/cu.usbserial-XXXX"  # macOS
BAUD = 115200

with open(image_path, "rb") as f:
    raw = f.read()

encoded = base64.b64encode(raw).decode("ascii")

ser = serial.Serial(PORT, BAUD, timeout=1)
time.sleep(2)

ser.write(f"IMG_BEGIN {len(raw)}\n".encode())

chunk_size = 128
for i in range(0, len(encoded), chunk_size):
    ser.write((encoded[i:i+chunk_size] + "\n").encode())
    time.sleep(0.01)

ser.write(b"IMG_END\n")

print("Sent image")
ser.close()