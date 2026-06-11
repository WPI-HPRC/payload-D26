import binascii
import csi
import gc
import time
from machine import UART


UART_BUS = 1
BAUDRATE = 10600
CHUNK_SIZE = 48
INTER_LINE_DELAY_MS = 2
FRAME_INTERVAL_MS = 0

FRAME_SIZE = csi.QQQVGA
JPEG_QUALITY = 50


def setup_camera():
    cam = csi.CSI()
    cam.reset()

    pixformat = getattr(csi, "GRAYSCALE", csi.RGB565)
    cam.pixformat(pixformat)
    cam.framesize(FRAME_SIZE)

    if hasattr(cam, "quality"):
        cam.quality(JPEG_QUALITY)

    cam.snapshot(time=1000)
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
    chunks = (len(encoded) + CHUNK_SIZE - 1) // CHUNK_SIZE

    uart.write("IMG_BEGIN %d\n" % len(image_bytes))
    time.sleep_ms(INTER_LINE_DELAY_MS)

    for offset in range(0, len(encoded), CHUNK_SIZE):
        uart.write(encoded[offset:offset + CHUNK_SIZE])
        uart.write("\n")
        time.sleep_ms(INTER_LINE_DELAY_MS)

    uart.write("IMG_END\n")
    time.sleep_ms(INTER_LINE_DELAY_MS)
    # uart.write(
    #     "DBG_OPENMV_END jpeg_bytes=%d base64_chars=%d chunks=%d chunk_size=%d baud=%d\n"
    #     % (len(image_bytes), len(encoded), chunks, CHUNK_SIZE, BAUDRATE)
    # )


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
