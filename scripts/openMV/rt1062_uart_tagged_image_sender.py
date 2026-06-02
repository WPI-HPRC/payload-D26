import binascii
import csi
import gc
import time
from machine import UART


UART_BUS = 1
BAUDRATE = 921600
CHUNK_SIZE = 128
FRAME_INTERVAL_MS = 1000

FRAME_SIZE = csi.QQVGA
JPEG_QUALITY = 50


def setup_camera():
    cam = csi.CSI()
    cam.reset()

    pixformat = getattr(csi, "GRAYSCALE", csi.RGB565)
    cam.pixformat(pixformat)
    cam.framesize(FRAME_SIZE)

    if hasattr(cam, "quality"):
        cam.quality(JPEG_QUALITY)

    cam.snapshot(time=2000)
    return cam


def compressed_jpeg_bytes(cam):
    img = cam.snapshot()

    try:
        compressed = img.compress(quality=JPEG_QUALITY)
    except TypeError:
        compressed = img.compress()

    return compressed.bytearray()


def write_base64_lines(uart, image_bytes):
    encoded = binascii.b2a_base64(image_bytes).strip()

    uart.write("IMG_BEGIN %d\n" % len(image_bytes))

    for offset in range(0, len(encoded), CHUNK_SIZE):
        uart.write(encoded[offset:offset + CHUNK_SIZE])
        uart.write("\n")

    uart.write("IMG_END\n")


camera = setup_camera()
uart = UART(UART_BUS, baudrate=BAUDRATE)

while True:
    try:
        jpeg_bytes = compressed_jpeg_bytes(camera)
        write_base64_lines(uart, jpeg_bytes)
        gc.collect()
    except Exception as exc:
        print("UART image send error:", exc)

    time.sleep_ms(FRAME_INTERVAL_MS)
