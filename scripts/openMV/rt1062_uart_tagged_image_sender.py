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

SD_FRAME_SIZE = csi.QVGA
SD_PIXFORMAT = csi.RGB565
SD_JPEG_QUALITY = 85
SD_JPEG_FALLBACK_QUALITY = 60

UART_FRAME_SIZE = csi.QQQVGA
UART_PIXFORMAT = getattr(csi, "GRAYSCALE", csi.RGB565)
UART_JPEG_QUALITY = 50

CAMERA_SETTLE_SNAPSHOT_MS = 1000
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
UART_RATE_REPORT_INTERVAL_MS = 1000
SD_ROOT = "/sd"
SD_IMAGE_PARENT_NAME = "openmv_images"
SD_IMAGE_PREFIX = "frame_"
SD_IMAGE_SUFFIX = ".jpg"
SD_RUN_DIR_CREATE_ATTEMPTS = 20
SD_MIN_BYTES = 8 * 1024 * 1024

sd_available = False
sd_image_index = 0
last_sd_warning_ms = 0
active_camera_mode = None
sd_root = SD_ROOT
sd_image_parent_dir = SD_ROOT + "/" + SD_IMAGE_PARENT_NAME
sd_image_dir = sd_image_parent_dir
uart_frames_sent = 0
uart_frames_sent_at_last_report = 0
last_uart_rate_report_ms = 0
sd_images_saved = 0
sd_save_failures = 0


def apply_camera_orientation(cam):
    cam.hmirror(False)
    cam.vflip(True)
    cam.transpose(True)


def configure_camera(cam, mode_name, pixformat, frame_size, jpeg_quality, settle=False):
    global active_camera_mode

    if active_camera_mode == mode_name:
        return

    try:
        cam.pixformat(pixformat)
        cam.framesize(frame_size)
        apply_camera_orientation(cam)

        if hasattr(cam, "quality"):
            cam.quality(jpeg_quality)

        active_camera_mode = mode_name

        if settle:
            cam.snapshot(time=CAMERA_SETTLE_SNAPSHOT_MS)
    except Exception as exc:
        active_camera_mode = None
        print("WARN_OPENMV_CAMERA_CONFIG_FAILED mode=%s error=%s" % (mode_name, exc))
        raise


def configure_camera_for_sd(cam, settle=False):
    configure_camera(cam, "sd", SD_PIXFORMAT, SD_FRAME_SIZE, SD_JPEG_QUALITY, settle)


def configure_camera_for_uart(cam, settle=False):
    configure_camera(cam, "uart", UART_PIXFORMAT, UART_FRAME_SIZE, UART_JPEG_QUALITY, settle)


def setup_camera():
    cam = csi.CSI()
    cam.reset()
    configure_camera_for_sd(cam, settle=True)
    return cam


def compressed_jpeg_bytes(img):
    try:
        compressed = img.compress(quality=UART_JPEG_QUALITY)
    except TypeError:
        compressed = img.compress()

    return compressed.bytearray()


def join_path(root, child):
    if root == "" or root == "/":
        return "/" + child

    return root + "/" + child


def format_exception(exc):
    try:
        return "%s:%s" % (type(exc).__name__, exc)
    except Exception:
        return str(exc)


def root_has_sd_capacity(root):
    try:
        stat = os.statvfs(root if root != "" else "/")
        block_size = stat[0] if stat[0] > 0 else stat[1]
        total_bytes = block_size * stat[2]
        return total_bytes >= SD_MIN_BYTES
    except Exception as exc:
        print("WARN_OPENMV_SD_STATVFS_FAILED root=%s error=%s" % (root if root != "" else "/", format_exception(exc)))
        return False


def path_exists(path):
    try:
        os.stat(path)
        return True
    except Exception:
        return False


def mount_sd_root():
    global sd_root, sd_image_parent_dir, sd_image_dir

    if path_exists(SD_ROOT) and root_has_sd_capacity(SD_ROOT):
        sd_root = SD_ROOT
        sd_image_parent_dir = join_path(sd_root, SD_IMAGE_PARENT_NAME)
        sd_image_dir = sd_image_parent_dir
        print("DBG_OPENMV_SD_ROOT_READY root=%s" % SD_ROOT)
        return True

    try:
        if not path_exists(SD_ROOT):
            os.mkdir(SD_ROOT)
    except Exception as exc:
        print("WARN_OPENMV_SD_ROOT_DIR_FAILED root=%s error=%s" % (SD_ROOT, format_exception(exc)))

    try:
        os.mount(machine.SDCard(), SD_ROOT)
    except OSError as exc:
        # Errno 16 is "device/resource busy" on many MicroPython ports, which
        # usually means the card was already mounted by the IDE/firmware.
        try:
            errno = exc.args[0]
        except Exception:
            errno = None

        if errno != 16:
            print("WARN_OPENMV_SD_MOUNT_FAILED root=%s error=%s" % (SD_ROOT, format_exception(exc)))
        else:
            print("DBG_OPENMV_SD_ALREADY_MOUNTED root=%s" % SD_ROOT)
    except Exception as exc:
        print("WARN_OPENMV_SD_MOUNT_FAILED root=%s error=%s" % (SD_ROOT, format_exception(exc)))

    if root_has_sd_capacity(SD_ROOT):
        sd_root = SD_ROOT
        sd_image_parent_dir = join_path(sd_root, SD_IMAGE_PARENT_NAME)
        sd_image_dir = sd_image_parent_dir
        print("DBG_OPENMV_SD_ROOT_MOUNTED root=%s" % SD_ROOT)
        return True

    print("WARN_OPENMV_SD_ROOT_NOT_USABLE root=%s" % SD_ROOT)

    if root_has_sd_capacity("/"):
        sd_root = ""
        sd_image_parent_dir = join_path(sd_root, SD_IMAGE_PARENT_NAME)
        sd_image_dir = sd_image_parent_dir
        print("DBG_OPENMV_SD_ROOT_READY root=/")
        return True

    return False


def mount_sd_if_needed():
    try:
        return mount_sd_root()
    except Exception as exc:
        print("WARN_OPENMV_SD_UNWRITABLE mount setup failed:", format_exception(exc))
        return False


def ensure_sd_image_dir():
    global sd_image_dir

    if not path_exists(sd_image_parent_dir):
        os.mkdir(sd_image_parent_dir)

    run_id = time.ticks_ms()
    for run_index in range(SD_RUN_DIR_CREATE_ATTEMPTS):
        candidate_dir = "%s/run_%010d_%03d" % (sd_image_parent_dir, run_id, run_index)
        try:
            os.mkdir(candidate_dir)
            sd_image_dir = candidate_dir
            return
        except Exception as exc:
            print("WARN_OPENMV_SD_RUN_DIR_FAILED path=%s error=%s" % (candidate_dir, exc))

    raise OSError("could not create unique SD run directory")


def confirm_sd_writable():
    test_path = sd_image_dir + "/write_test.tmp"
    test_file = open(test_path, "w")
    try:
        test_file.write("ok")
    finally:
        test_file.close()
    os.remove(test_path)


def next_sd_image_index():
    next_index = 0

    try:
        for filename in os.listdir(sd_image_dir):
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
        print("DBG_OPENMV_SD_READY dir=%s next_index=%d" % (sd_image_dir, sd_image_index))
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


def save_jpeg_with_quality(img, image_path, quality):
    try:
        img.save(image_path, quality=quality)
    except TypeError:
        img.save(image_path)


def save_image_to_sd(img):
    global sd_available, sd_image_index, sd_images_saved, sd_save_failures

    if not sd_available:
        sd_save_failures += 1
        print_sd_warning_throttled("WARN_OPENMV_SD_UNWRITABLE image not saved")
        return False

    image_path = "%s/%s%06d%s" % (sd_image_dir, SD_IMAGE_PREFIX, sd_image_index, SD_IMAGE_SUFFIX)

    try:
        try:
            save_jpeg_with_quality(img, image_path, SD_JPEG_QUALITY)
        except Exception as exc:
            print("WARN_OPENMV_SD_SAVE_PRIMARY_FAILED path=%s quality=%d error=%s" % (
                image_path,
                SD_JPEG_QUALITY,
                format_exception(exc)
            ))
            save_jpeg_with_quality(img, image_path, SD_JPEG_FALLBACK_QUALITY)

        try:
            os.sync()
        except Exception:
            pass

        sd_image_index += 1
        sd_images_saved += 1
        return True
    except Exception as exc:
        sd_available = False
        sd_save_failures += 1
        print("WARN_OPENMV_SD_UNWRITABLE save failed:", format_exception(exc))
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


def report_uart_frame_rate(image_byte_count):
    global uart_frames_sent
    global uart_frames_sent_at_last_report
    global last_uart_rate_report_ms

    uart_frames_sent += 1
    now = time.ticks_ms()

    if time.ticks_diff(now, last_uart_rate_report_ms) < UART_RATE_REPORT_INTERVAL_MS:
        return

    elapsed_ms = time.ticks_diff(now, last_uart_rate_report_ms)
    frames = uart_frames_sent - uart_frames_sent_at_last_report
    fps_x100 = (frames * 100000) // elapsed_ms

    print(
        "DBG_OPENMV_UART_RATE fps=%d.%02d frames=%d jpeg_bytes=%d sd_saved=%d sd_failed=%d sd_dir=%s" %
        (
            fps_x100 // 100,
            fps_x100 % 100,
            frames,
            image_byte_count,
            sd_images_saved,
            sd_save_failures,
            sd_image_dir
        )
    )

    uart_frames_sent_at_last_report = uart_frames_sent
    last_uart_rate_report_ms = now


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
        configure_camera_for_sd(camera)
        sd_img = camera.snapshot()
        save_image_to_sd(sd_img)

        configure_camera_for_uart(camera)
        uart_img = camera.snapshot()
        jpeg_bytes = compressed_jpeg_bytes(uart_img)
        write_base64_lines(uart, jpeg_bytes)
        report_uart_frame_rate(len(jpeg_bytes))
        gc.collect()
    except Exception as exc:
        print("UART image send error:", format_exception(exc))

    time.sleep_ms(FRAME_INTERVAL_MS)
