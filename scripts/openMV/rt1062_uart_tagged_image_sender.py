import binascii
import csi
import gc
import os
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
try:
    SD_ERROR_LED = LED("LED_RED")
except Exception:
    SD_ERROR_LED = RUN_LED
RUN_LED_BLINK_PERIOD_MS = 1000
RUN_LED_ON_TIME_MS = 50
SD_ERROR_BLINK_PERIOD_MS = 1200
SD_ERROR_BLINK_ON_MS = 100
SD_WARNING_INTERVAL_MS = 2000
SD_ROOT = "/sd"
SD_IMAGE_DIR = "/sd/openmv_images"
SD_IMAGE_PREFIX = "frame_"
SD_IMAGE_SUFFIX = ".jpg"

sd_available = False
sd_image_index = 0
last_sd_warning_ms = 0


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


def compressed_jpeg_bytes(img):
    try:
        compressed = img.compress(quality=JPEG_QUALITY)
    except TypeError:
        compressed = img.compress()

    return compressed.bytearray()


def mount_sd_if_needed():
    try:
        os.stat(SD_ROOT)
        return True
    except Exception:
        pass

    try:
        os.mount(machine.SDCard(), SD_ROOT)
        return True
    except Exception as exc:
        print("WARN_OPENMV_SD_UNWRITABLE mount failed:", exc)
        return False


def ensure_sd_image_dir():
    try:
        os.stat(SD_IMAGE_DIR)
    except Exception:
        os.mkdir(SD_IMAGE_DIR)


def confirm_sd_writable():
    test_path = SD_IMAGE_DIR + "/write_test.tmp"
    test_file = open(test_path, "w")
    try:
        test_file.write("ok")
    finally:
        test_file.close()
    os.remove(test_path)


def next_sd_image_index():
    next_index = 0

    try:
        for filename in os.listdir(SD_IMAGE_DIR):
            if not filename.startswith(SD_IMAGE_PREFIX) or not filename.endswith(SD_IMAGE_SUFFIX):
                continue

            index_text = filename[len(SD_IMAGE_PREFIX):-len(SD_IMAGE_SUFFIX)]
            index = int(index_text)
            if index >= next_index:
                next_index = index + 1
    except Exception:
        return 0

    return next_index


def setup_sd_storage():
    global sd_available, sd_image_index

    try:
        if not mount_sd_if_needed():
            sd_available = False
            return

        ensure_sd_image_dir()
        confirm_sd_writable()
        sd_image_index = next_sd_image_index()
        sd_available = True
        print("DBG_OPENMV_SD_READY dir=%s next_index=%d" % (SD_IMAGE_DIR, sd_image_index))
    except Exception as exc:
        sd_available = False
        print("WARN_OPENMV_SD_UNWRITABLE setup failed:", exc)


def print_sd_warning_throttled(message):
    global last_sd_warning_ms

    now = time.ticks_ms()
    if time.ticks_diff(now, last_sd_warning_ms) < SD_WARNING_INTERVAL_MS:
        return

    print(message)
    last_sd_warning_ms = now


def save_image_to_sd(img):
    global sd_available, sd_image_index

    if not sd_available:
        print_sd_warning_throttled("WARN_OPENMV_SD_UNWRITABLE image not saved")
        return False

    image_path = "%s/%s%06d%s" % (SD_IMAGE_DIR, SD_IMAGE_PREFIX, sd_image_index, SD_IMAGE_SUFFIX)

    try:
        try:
            img.save(image_path, quality=JPEG_QUALITY)
        except TypeError:
            img.save(image_path)

        try:
            os.sync()
        except Exception:
            pass

        sd_image_index += 1
        return True
    except Exception as exc:
        sd_available = False
        print("WARN_OPENMV_SD_UNWRITABLE save failed:", exc)
        return False


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


def blink_to_show_sd_error():
    phase = time.ticks_ms() % SD_ERROR_BLINK_PERIOD_MS
    led_on = (
        phase < SD_ERROR_BLINK_ON_MS or
        (200 <= phase < 200 + SD_ERROR_BLINK_ON_MS) or
        (400 <= phase < 400 + SD_ERROR_BLINK_ON_MS)
    )

    RUN_LED.off()

    if led_on:
        SD_ERROR_LED.on()
    else:
        SD_ERROR_LED.off()


def update_status_leds():
    if sd_available:
        SD_ERROR_LED.off()
        blink_to_show_running()
    else:
        blink_to_show_sd_error()


camera = setup_camera()
setup_sd_storage()
uart = UART(UART_BUS, baudrate=BAUDRATE)

while True:

    update_status_leds()

    try:
        img = camera.snapshot()
        save_image_to_sd(img)
        jpeg_bytes = compressed_jpeg_bytes(img)
        write_base64_lines(uart, jpeg_bytes)
        gc.collect()
    except Exception as exc:
        print("UART image send error:", exc)

    time.sleep_ms(FRAME_INTERVAL_MS)
