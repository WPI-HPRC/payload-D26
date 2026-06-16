import binascii
import csi
import gc
import time
import machine
from machine import UART
from machine import LED


UART_BUS = 1
BAUDRATE = 115200
CHUNK_SIZE = 64
INTER_LINE_DELAY_MS = 0
FRAME_INTERVAL_MS = 0

FRAME_SIZE = csi.QQQVGA
JPEG_QUALITY = 50
RUN_LED = LED("LED_GREEN")
RUN_LED_BLINK_PERIOD_MS = 1000
RUN_LED_ON_TIME_MS = 50


def setup_camera():
    cam = csi.CSI()
    cam.reset()

    pixformat = getattr(csi, "GRAYSCALE", csi.RGB565)
    cam.pixformat(pixformat)
    cam.framesize(FRAME_SIZE)
    cam.hmirror(False)
    cam.vflip(True)
    cam.transpose(True)

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
def blink_to_show_running():
    if time.ticks_ms() % RUN_LED_BLINK_PERIOD_MS < RUN_LED_ON_TIME_MS:
        RUN_LED.on()
    else:
        RUN_LED.off()


camera = setup_camera()
uart = UART(UART_BUS, baudrate=BAUDRATE)

while True:

    blink_to_show_running()

    try:
        jpeg_bytes = compressed_jpeg_bytes(camera)
        write_base64_lines(uart, jpeg_bytes)
        gc.collect()
    except Exception as exc:
        print("UART image send error:", exc)

    time.sleep_ms(FRAME_INTERVAL_MS)
