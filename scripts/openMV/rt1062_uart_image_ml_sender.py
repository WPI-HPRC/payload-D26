import binascii
import csi
import gc
import image
import os
import time
import machine
from collections import namedtuple
from machine import UART
from machine import LED


UART_BUS = 1
BAUDRATE = 115200
CHUNK_SIZE = 64
INTER_LINE_DELAY_MS = 0
FRAME_INTERVAL_MS = 0

ENABLE_IMAGE_TRANSMIT = True
ENABLE_ML_DATA_TRANSMIT = True
ENABLE_DEBUGS = True

SD_FRAME_SIZE = csi.VGA
SD_WINDOW = (90, 120)
SD_PIXFORMAT = csi.RGB565
SD_JPEG_QUALITY = 95
SD_JPEG_FALLBACK_QUALITY = 60

UART_FRAME_SIZE = csi.QQVGA
UART_PIXFORMAT = image.GRAYSCALE
UART_JPEG_QUALITY = 50
UART_SCALE_DIVISOR = 4
USE_DERIVED_UART_FRAME = True
SAVE_UART_DEBUG_IMAGES = False
CLEAR_SD_IMAGE_PARENT_ON_START = False

CAMERA_HEIGHT_CM = 7
CAMERA_FOV_DEGREES = 70
CAMERA_PITCH_DEGREES = 0

Bush_LAB_thresholds = [(0, 30), (-128, 10), (-30, -10)]
HORIZON_MARGIN_PX = 10
MAX_BUSH_PIXELS = 3000
HORIZON_LINE_THRESHOLD = 1000
HORIZON_THETA_DEGREES = 90
HORIZON_THETA_MARGIN_DEGREES = 20

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
SD_SETUP_RETRY_INTERVAL_MS = 3000
UART_RATE_REPORT_INTERVAL_MS = 1000
SD_ROOT = "/sd"
SD_IMAGE_PARENT_NAME = "openmv_images"
SD_IMAGE_PREFIX = "frame_"
UART_DEBUG_IMAGE_PREFIX = "uart_frame_"
SD_IMAGE_SUFFIX = ".jpg"
SD_RUN_DIR_CREATE_ATTEMPTS = 20
SD_MIN_BYTES = 8 * 1024 * 1024

BushDetection = namedtuple("BushDetection", "blobs sizes centers")
HorizonDetection = namedtuple("HorizonDetection", "line coords y_px candidates")

sd_available = False
sd_image_index = 0
uart_debug_image_index = 0
last_sd_warning_ms = 0
last_sd_setup_attempt_ms = 0
sd_image_parent_cleared = False
active_camera_mode = None
sd_root = SD_ROOT
sd_image_parent_dir = SD_ROOT + "/" + SD_IMAGE_PARENT_NAME
sd_image_dir = sd_image_parent_dir
uart_frames_sent = 0
uart_frames_sent_at_last_report = 0
last_uart_rate_report_ms = 0
sd_images_saved = 0
sd_save_failures = 0
uart_debug_images_saved = 0
uart_debug_save_failures = 0
timing_sd_config_ms = 0
timing_sd_snapshot_ms = 0
timing_ml_ms = 0
timing_debug_draw_ms = 0
timing_sd_save_ms = 0
timing_uart_config_ms = 0
timing_uart_snapshot_ms = 0
timing_uart_derive_ms = 0
timing_ml_send_ms = 0
timing_compress_ms = 0
timing_uart_debug_save_ms = 0
timing_uart_send_ms = 0
timing_gc_ms = 0
timing_frame_total_ms = 0
command_line_buffer = ""
frame_id = 0


def bool_to_int(value):
    if value:
        return 1

    return 0


def apply_camera_orientation(cam):
    cam.hmirror(False)
    cam.vflip(True)
    cam.transpose(True)


def configure_camera(cam, mode_name, pixformat, frame_size, jpeg_quality, settle=False, window=None):
    global active_camera_mode

    if active_camera_mode == mode_name:
        return

    try:
        cam.pixformat(pixformat)
        cam.framesize(frame_size)

        if window is not None:
            cam.window(window)

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
    configure_camera(cam, "sd", SD_PIXFORMAT, SD_FRAME_SIZE, SD_JPEG_QUALITY, settle, SD_WINDOW)


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


def pixel_to_grayscale(pixel):
    if isinstance(pixel, tuple):
        return (pixel[0] * 38 + pixel[1] * 75 + pixel[2] * 15) >> 7

    return pixel


def make_uart_frame_from_sd_image(sd_img):
    uart_width = sd_img.width() // UART_SCALE_DIVISOR
    uart_height = sd_img.height() // UART_SCALE_DIVISOR

    if uart_width <= 0:
        uart_width = 1

    if uart_height <= 0:
        uart_height = 1

    uart_frame = image.Image(uart_width, uart_height, UART_PIXFORMAT)
    sd_width = sd_img.width()
    sd_height = sd_img.height()

    for y in range(uart_height):
        src_y = ((y * sd_height) + (uart_height // 2)) // uart_height
        if src_y >= sd_height:
            src_y = sd_height - 1

        for x in range(uart_width):
            src_x = ((x * sd_width) + (uart_width // 2)) // uart_width
            if src_x >= sd_width:
                src_x = sd_width - 1

            pixel = sd_img.get_pixel(src_x, src_y)

            if UART_PIXFORMAT == image.GRAYSCALE:
                pixel = pixel_to_grayscale(pixel)

            uart_frame.set_pixel(x, y, pixel)

    return uart_frame


def join_path(root, child):
    if root == "" or root == "/":
        return "/" + child

    return root + "/" + child


def format_exception(exc):
    try:
        return "%s:%s" % (type(exc).__name__, exc)
    except Exception:
        return str(exc)


def safe_print(message):
    try:
        print(message)
    except Exception:
        pass


def debug_print(message):
    if ENABLE_DEBUGS:
        safe_print(message)


def root_has_sd_capacity(root):
    try:
        stat = os.statvfs(root if root != "" else "/")
        block_size = stat[0] if stat[0] > 0 else stat[1]
        total_bytes = block_size * stat[2]
        return total_bytes >= SD_MIN_BYTES
    except Exception as exc:
        safe_print("WARN_OPENMV_SD_STATVFS_FAILED root=%s error=%s" % (root if root != "" else "/", format_exception(exc)))
        return False


def path_exists(path):
    try:
        os.stat(path)
        return True
    except Exception:
        return False


def remove_path_recursive(path):
    try:
        os.remove(path)
        return
    except OSError:
        pass

    try:
        for child in os.listdir(path):
            remove_path_recursive(join_path(path, child))
        os.rmdir(path)
    except Exception as exc:
        safe_print("WARN_OPENMV_SD_CLEAR_FAILED path=%s error=%s" % (path, format_exception(exc)))
        raise


def clear_sd_image_parent_if_enabled():
    global sd_image_parent_cleared

    if sd_image_parent_cleared:
        return

    if not CLEAR_SD_IMAGE_PARENT_ON_START:
        sd_image_parent_cleared = True
        return

    if not sd_image_parent_dir.endswith("/" + SD_IMAGE_PARENT_NAME):
        safe_print("WARN_OPENMV_SD_CLEAR_SKIPPED unexpected_parent=%s" % sd_image_parent_dir)
        sd_image_parent_cleared = True
        return

    if path_exists(sd_image_parent_dir):
        safe_print("DBG_OPENMV_SD_CLEAR_START dir=%s" % sd_image_parent_dir)
        for child in os.listdir(sd_image_parent_dir):
            remove_path_recursive(join_path(sd_image_parent_dir, child))
        safe_print("DBG_OPENMV_SD_CLEAR_DONE dir=%s" % sd_image_parent_dir)

    sd_image_parent_cleared = True


def mount_sd_root():
    global sd_root, sd_image_parent_dir, sd_image_dir

    if path_exists(SD_ROOT) and root_has_sd_capacity(SD_ROOT):
        sd_root = SD_ROOT
        sd_image_parent_dir = join_path(sd_root, SD_IMAGE_PARENT_NAME)
        sd_image_dir = sd_image_parent_dir
        safe_print("DBG_OPENMV_SD_ROOT_READY root=%s" % SD_ROOT)
        return True

    try:
        if not path_exists(SD_ROOT):
            os.mkdir(SD_ROOT)
    except Exception as exc:
        safe_print("WARN_OPENMV_SD_ROOT_DIR_FAILED root=%s error=%s" % (SD_ROOT, format_exception(exc)))

    try:
        os.mount(machine.SDCard(), SD_ROOT)
    except OSError as exc:
        try:
            errno = exc.args[0]
        except Exception:
            errno = None

        if errno != 16:
            safe_print("WARN_OPENMV_SD_MOUNT_FAILED root=%s error=%s" % (SD_ROOT, format_exception(exc)))
        else:
            safe_print("DBG_OPENMV_SD_ALREADY_MOUNTED root=%s" % SD_ROOT)
    except Exception as exc:
        safe_print("WARN_OPENMV_SD_MOUNT_FAILED root=%s error=%s" % (SD_ROOT, format_exception(exc)))

    if root_has_sd_capacity(SD_ROOT):
        sd_root = SD_ROOT
        sd_image_parent_dir = join_path(sd_root, SD_IMAGE_PARENT_NAME)
        sd_image_dir = sd_image_parent_dir
        safe_print("DBG_OPENMV_SD_ROOT_MOUNTED root=%s" % SD_ROOT)
        return True

    safe_print("WARN_OPENMV_SD_ROOT_NOT_USABLE root=%s" % SD_ROOT)

    if root_has_sd_capacity("/"):
        sd_root = ""
        sd_image_parent_dir = join_path(sd_root, SD_IMAGE_PARENT_NAME)
        sd_image_dir = sd_image_parent_dir
        safe_print("DBG_OPENMV_SD_ROOT_READY root=/")
        return True

    return False


def mount_sd_if_needed():
    try:
        return mount_sd_root()
    except Exception as exc:
        safe_print("WARN_OPENMV_SD_UNWRITABLE mount setup failed: %s" % format_exception(exc))
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
            safe_print("WARN_OPENMV_SD_RUN_DIR_FAILED path=%s error=%s" % (candidate_dir, exc))

    raise OSError("could not create unique SD run directory")


def confirm_sd_writable():
    test_path = sd_image_dir + "/write_test.tmp"
    test_file = open(test_path, "w")
    try:
        test_file.write("ok")
    finally:
        test_file.close()
    os.remove(test_path)


def next_sd_image_index(prefix):
    next_index = 0

    try:
        for filename in os.listdir(sd_image_dir):
            if not filename.startswith(prefix) or not filename.endswith(SD_IMAGE_SUFFIX):
                continue

            index_text = filename[len(prefix):-len(SD_IMAGE_SUFFIX)]
            index = int(index_text)
            if index >= next_index:
                next_index = index + 1
    except Exception:
        return 0

    return next_index


def setup_sd_storage():
    global sd_available, sd_image_index, uart_debug_image_index, last_sd_setup_attempt_ms

    last_sd_setup_attempt_ms = time.ticks_ms()

    try:
        if not mount_sd_if_needed():
            sd_available = False
            return

        clear_sd_image_parent_if_enabled()
        ensure_sd_image_dir()
        confirm_sd_writable()
        sd_image_index = next_sd_image_index(SD_IMAGE_PREFIX)
        uart_debug_image_index = next_sd_image_index(UART_DEBUG_IMAGE_PREFIX)
        sd_available = True
        safe_print("DBG_OPENMV_SD_READY dir=%s next_index=%d uart_debug_next_index=%d" % (
            sd_image_dir,
            sd_image_index,
            uart_debug_image_index
        ))
    except Exception as exc:
        sd_available = False
        safe_print("WARN_OPENMV_SD_UNWRITABLE setup failed: %s" % format_exception(exc))


def retry_sd_storage_if_needed():
    if sd_available:
        return

    now = time.ticks_ms()
    if time.ticks_diff(now, last_sd_setup_attempt_ms) < SD_SETUP_RETRY_INTERVAL_MS:
        return

    safe_print("DBG_OPENMV_SD_RETRY")
    setup_sd_storage()


def print_sd_warning_throttled(message):
    global last_sd_warning_ms

    now = time.ticks_ms()
    if time.ticks_diff(now, last_sd_warning_ms) < SD_WARNING_INTERVAL_MS:
        return

    safe_print(message)
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
            safe_print("WARN_OPENMV_SD_SAVE_PRIMARY_FAILED path=%s quality=%d error=%s" % (
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
        safe_print("WARN_OPENMV_SD_UNWRITABLE save failed: %s" % format_exception(exc))
        return False


def save_uart_debug_jpeg_to_sd(jpeg_bytes):
    global sd_available, uart_debug_image_index
    global uart_debug_images_saved, uart_debug_save_failures

    if not SAVE_UART_DEBUG_IMAGES:
        return False

    if not sd_available:
        uart_debug_save_failures += 1
        print_sd_warning_throttled("WARN_OPENMV_SD_UNWRITABLE uart debug image not saved")
        return False

    image_path = "%s/%s%06d%s" % (
        sd_image_dir,
        UART_DEBUG_IMAGE_PREFIX,
        uart_debug_image_index,
        SD_IMAGE_SUFFIX
    )

    try:
        debug_file = open(image_path, "wb")
        try:
            debug_file.write(jpeg_bytes)
        finally:
            debug_file.close()

        try:
            os.sync()
        except Exception:
            pass

        uart_debug_image_index += 1
        uart_debug_images_saved += 1
        return True
    except Exception as exc:
        uart_debug_save_failures += 1
        safe_print("WARN_OPENMV_UART_DEBUG_SAVE_FAILED path=%s error=%s" % (
            image_path,
            format_exception(exc)
        ))
        return False


def find_bushes(img):
    detected_blobs = img.find_blobs(Bush_LAB_thresholds, merge=True)
    filtered_blobs = []
    blob_sizes = []
    blob_centers = []

    for blob in detected_blobs:
        if blob.pixels() > MAX_BUSH_PIXELS:
            continue

        filtered_blobs.append(blob)
        blob_sizes.append(blob.pixels())
        blob_centers.append((blob.cx(), blob.cy()))

    return BushDetection(filtered_blobs, blob_sizes, blob_centers)


def find_horizon(img):
    roi = (0, int(img.height() * 0.25), img.width(), int(img.height() * 0.75))
    lines = img.find_lines(
        threshold=HORIZON_LINE_THRESHOLD,
        theta_margin=10,
        rho_margin=10,
        roi=roi
    )
    horizontal_lines = []

    for line in lines:
        error = abs(line.theta() - HORIZON_THETA_DEGREES)
        if error <= HORIZON_THETA_MARGIN_DEGREES:
            horizontal_lines.append(line)

    if len(horizontal_lines) > 1:
        horizontal_lines.sort(key=lambda line: (line.x1() + line.x2()) // 2, reverse=True)
        horizon = horizontal_lines[0]
    elif len(horizontal_lines) == 1:
        horizon = horizontal_lines[0]
    else:
        horizon_y_px = img.height() // 2
        return HorizonDetection(None, (0, horizon_y_px, img.width() - 1, horizon_y_px), horizon_y_px, horizontal_lines)

    coords = horizon.line()
    horizon_y_px = (horizon.y1() + horizon.y2()) // 2
    return HorizonDetection(horizon, coords, horizon_y_px, horizontal_lines)


def fallback_horizon(img):
    fallback_horizon_y_px = img.height() // 2
    return HorizonDetection(
        None,
        (0, fallback_horizon_y_px, img.width() - 1, fallback_horizon_y_px),
        fallback_horizon_y_px,
        []
    )


def draw_debug_overlay(img, horizon_detection, bush_detection):
    try:
        img.draw_line(horizon_detection.coords, color=(255, 0, 0))
    except Exception as exc:
        debug_print("DBG_OPENMV_DRAW_HORIZON_FAILED error=%s" % format_exception(exc))

    for blob in bush_detection.blobs:
        try:
            img.draw_ellipse(blob.enclosed_ellipse(), color=(255, 255, 255))
            img.draw_cross(blob.cx(), blob.cy(), color=(0, 0, 0))
        except Exception as exc:
            debug_print("DBG_OPENMV_DRAW_BLOB_FAILED error=%s" % format_exception(exc))


def write_base64_lines(uart, image_bytes):
    encoded = binascii.b2a_base64(image_bytes).strip()

    uart.write("IMG_BEGIN %d\n" % len(image_bytes))
    time.sleep_ms(INTER_LINE_DELAY_MS)

    for offset in range(0, len(encoded), CHUNK_SIZE):
        uart.write(encoded[offset:offset + CHUNK_SIZE])
        uart.write("\n")
        time.sleep_ms(INTER_LINE_DELAY_MS)

    uart.write("IMG_END\n")
    time.sleep_ms(INTER_LINE_DELAY_MS)


def get_ellipse_values(blob):
    try:
        ellipse = blob.enclosed_ellipse()
    except Exception:
        return []

    values = []
    try:
        for value in ellipse:
            values.append(int(value))
    except Exception:
        return []

    return values


def write_ml_results(uart, current_frame_id, horizon_detection, bush_detection):
    uart.write("ML_BEGIN %d %d\n" % (current_frame_id, len(bush_detection.blobs)))

    horizon_valid = 0
    if horizon_detection.line is not None:
        horizon_valid = 1

    coords = horizon_detection.coords
    uart.write(
        "ML_HORIZON %d %d %d %d %d %d\n" %
        (horizon_valid, horizon_detection.y_px, coords[0], coords[1], coords[2], coords[3])
    )

    index = 0
    for blob in bush_detection.blobs:
        line = "ML_BLOB %d %d %d %d" % (index, blob.cx(), blob.cy(), blob.pixels())
        ellipse_values = get_ellipse_values(blob)
        for value in ellipse_values:
            line += " %d" % value
        uart.write(line + "\n")
        index += 1

    uart.write("ML_END\n")


def write_config(uart):
    uart.write("CFG IMG=%d ML=%d DBG=%d\n" % (
        bool_to_int(ENABLE_IMAGE_TRANSMIT),
        bool_to_int(ENABLE_ML_DATA_TRANSMIT),
        bool_to_int(ENABLE_DEBUGS)
    ))


def parse_bool_token(token):
    if token == "1" or token == "ON" or token == "TRUE":
        return True

    if token == "0" or token == "OFF" or token == "FALSE":
        return False

    return None


def apply_set_command(uart, parts):
    global ENABLE_IMAGE_TRANSMIT, ENABLE_ML_DATA_TRANSMIT, ENABLE_DEBUGS

    if len(parts) < 3:
        uart.write("CFG_ERR reason=bad_set\n")
        return

    target = parts[1]

    if target == "ALL":
        if len(parts) != 5:
            uart.write("CFG_ERR reason=bad_set_all\n")
            return

        img_value = parse_bool_token(parts[2])
        ml_value = parse_bool_token(parts[3])
        dbg_value = parse_bool_token(parts[4])

        if img_value is None or ml_value is None or dbg_value is None:
            uart.write("CFG_ERR reason=bad_bool\n")
            return

        ENABLE_IMAGE_TRANSMIT = img_value
        ENABLE_ML_DATA_TRANSMIT = ml_value
        ENABLE_DEBUGS = dbg_value
        write_config(uart)
        return

    if len(parts) != 3:
        uart.write("CFG_ERR reason=bad_set_single\n")
        return

    value = parse_bool_token(parts[2])
    if value is None:
        uart.write("CFG_ERR reason=bad_bool\n")
        return

    if target == "IMG":
        ENABLE_IMAGE_TRANSMIT = value
    elif target == "ML":
        ENABLE_ML_DATA_TRANSMIT = value
    elif target == "DBG":
        ENABLE_DEBUGS = value
    else:
        uart.write("CFG_ERR reason=unknown_target\n")
        return

    write_config(uart)


def handle_uart_command(uart, line):
    line = line.strip().upper()

    if len(line) == 0:
        return

    parts = line.split()

    if len(parts) == 2 and parts[0] == "GET" and parts[1] == "CFG":
        write_config(uart)
        return

    if parts[0] == "SET":
        apply_set_command(uart, parts)
        return

    uart.write("CFG_ERR reason=unknown_command\n")


def poll_uart_commands(uart):
    global command_line_buffer

    try:
        available = uart.any()
    except Exception:
        available = 0

    while available > 0:
        data = uart.read(1)
        if data is None:
            return

        if isinstance(data, int):
            next_char = chr(data)
        else:
            try:
                next_char = chr(data[0])
            except Exception:
                next_char = data[0]

        if next_char == "\r":
            pass
        elif next_char == "\n":
            handle_uart_command(uart, command_line_buffer)
            command_line_buffer = ""
        else:
            command_line_buffer += next_char
            if len(command_line_buffer) > 80:
                command_line_buffer = ""
                uart.write("CFG_ERR reason=line_overflow\n")

        try:
            available = uart.any()
        except Exception:
            available = 0


def report_uart_frame_rate(
    image_byte_count,
    sd_config_ms,
    sd_snapshot_ms,
    ml_ms,
    debug_draw_ms,
    sd_save_ms,
    uart_config_ms,
    uart_snapshot_ms,
    uart_derive_ms,
    ml_send_ms,
    compress_ms,
    uart_debug_save_ms,
    uart_send_ms,
    gc_ms,
    frame_total_ms
):
    global uart_frames_sent
    global uart_frames_sent_at_last_report
    global last_uart_rate_report_ms
    global timing_sd_config_ms
    global timing_sd_snapshot_ms
    global timing_ml_ms
    global timing_debug_draw_ms
    global timing_sd_save_ms
    global timing_uart_config_ms
    global timing_uart_snapshot_ms
    global timing_uart_derive_ms
    global timing_ml_send_ms
    global timing_compress_ms
    global timing_uart_debug_save_ms
    global timing_uart_send_ms
    global timing_gc_ms
    global timing_frame_total_ms

    uart_frames_sent += 1
    timing_sd_config_ms += sd_config_ms
    timing_sd_snapshot_ms += sd_snapshot_ms
    timing_ml_ms += ml_ms
    timing_debug_draw_ms += debug_draw_ms
    timing_sd_save_ms += sd_save_ms
    timing_uart_config_ms += uart_config_ms
    timing_uart_snapshot_ms += uart_snapshot_ms
    timing_uart_derive_ms += uart_derive_ms
    timing_ml_send_ms += ml_send_ms
    timing_compress_ms += compress_ms
    timing_uart_debug_save_ms += uart_debug_save_ms
    timing_uart_send_ms += uart_send_ms
    timing_gc_ms += gc_ms
    timing_frame_total_ms += frame_total_ms

    if not ENABLE_DEBUGS:
        return

    now = time.ticks_ms()

    if time.ticks_diff(now, last_uart_rate_report_ms) < UART_RATE_REPORT_INTERVAL_MS:
        return

    elapsed_ms = time.ticks_diff(now, last_uart_rate_report_ms)
    frames = uart_frames_sent - uart_frames_sent_at_last_report
    fps_x100 = (frames * 100000) // elapsed_ms

    if frames <= 0:
        frames = 1

    print(
        "DBG_OPENMV_RATE fps=%d.%02d frames=%d jpeg_bytes=%d total_ms=%d sd_cfg_ms=%d sd_snap_ms=%d ml_ms=%d draw_ms=%d sd_save_ms=%d uart_cfg_ms=%d uart_snap_ms=%d derive_uart_ms=%d ml_send_ms=%d compress_ms=%d uart_debug_save_ms=%d uart_send_ms=%d gc_ms=%d sd_saved=%d sd_failed=%d uart_dbg_saved=%d uart_dbg_failed=%d cfg_img=%d cfg_ml=%d cfg_dbg=%d sd_dir=%s" %
        (
            fps_x100 // 100,
            fps_x100 % 100,
            frames,
            image_byte_count,
            timing_frame_total_ms // frames,
            timing_sd_config_ms // frames,
            timing_sd_snapshot_ms // frames,
            timing_ml_ms // frames,
            timing_debug_draw_ms // frames,
            timing_sd_save_ms // frames,
            timing_uart_config_ms // frames,
            timing_uart_snapshot_ms // frames,
            timing_uart_derive_ms // frames,
            timing_ml_send_ms // frames,
            timing_compress_ms // frames,
            timing_uart_debug_save_ms // frames,
            timing_uart_send_ms // frames,
            timing_gc_ms // frames,
            sd_images_saved,
            sd_save_failures,
            uart_debug_images_saved,
            uart_debug_save_failures,
            bool_to_int(ENABLE_IMAGE_TRANSMIT),
            bool_to_int(ENABLE_ML_DATA_TRANSMIT),
            bool_to_int(ENABLE_DEBUGS),
            sd_image_dir
        )
    )

    uart_frames_sent_at_last_report = uart_frames_sent
    last_uart_rate_report_ms = now
    timing_sd_config_ms = 0
    timing_sd_snapshot_ms = 0
    timing_ml_ms = 0
    timing_debug_draw_ms = 0
    timing_sd_save_ms = 0
    timing_uart_config_ms = 0
    timing_uart_snapshot_ms = 0
    timing_uart_derive_ms = 0
    timing_ml_send_ms = 0
    timing_compress_ms = 0
    timing_uart_debug_save_ms = 0
    timing_uart_send_ms = 0
    timing_gc_ms = 0
    timing_frame_total_ms = 0


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
write_config(uart)

while True:
    update_status_leds()
    retry_sd_storage_if_needed()
    poll_uart_commands(uart)

    try:
        frame_start_ms = time.ticks_ms()
        current_frame_id = frame_id
        frame_id += 1

        step_start_ms = time.ticks_ms()
        configure_camera_for_sd(camera)
        sd_config_ms = time.ticks_diff(time.ticks_ms(), step_start_ms)

        step_start_ms = time.ticks_ms()
        sd_img = camera.snapshot()
        sd_snapshot_ms = time.ticks_diff(time.ticks_ms(), step_start_ms)

        should_run_ml_pipeline = ENABLE_ML_DATA_TRANSMIT or ENABLE_DEBUGS
        horizon_detection = None
        bush_detection = BushDetection([], [], [])

        if should_run_ml_pipeline:
            step_start_ms = time.ticks_ms()
            try:
                horizon_detection = find_horizon(sd_img)
            except Exception as exc:
                debug_print("DBG_OPENMV_HORIZON_FAILED error=%s" % format_exception(exc))
                horizon_detection = fallback_horizon(sd_img)

            try:
                bush_detection = find_bushes(sd_img)
            except Exception as exc:
                debug_print("DBG_OPENMV_BLOBS_FAILED error=%s" % format_exception(exc))
                bush_detection = BushDetection([], [], [])
            ml_ms = time.ticks_diff(time.ticks_ms(), step_start_ms)
        else:
            ml_ms = 0

        if ENABLE_DEBUGS and horizon_detection is not None:
            step_start_ms = time.ticks_ms()
            draw_debug_overlay(sd_img, horizon_detection, bush_detection)
            debug_draw_ms = time.ticks_diff(time.ticks_ms(), step_start_ms)
        else:
            debug_draw_ms = 0

        step_start_ms = time.ticks_ms()
        save_image_to_sd(sd_img)
        sd_save_ms = time.ticks_diff(time.ticks_ms(), step_start_ms)

        if ENABLE_ML_DATA_TRANSMIT and horizon_detection is not None:
            step_start_ms = time.ticks_ms()
            write_ml_results(uart, current_frame_id, horizon_detection, bush_detection)
            ml_send_ms = time.ticks_diff(time.ticks_ms(), step_start_ms)
        else:
            ml_send_ms = 0

        image_byte_count = 0
        uart_config_ms = 0
        uart_snapshot_ms = 0
        uart_derive_ms = 0
        compress_ms = 0
        uart_debug_save_ms = 0
        uart_send_ms = 0

        if ENABLE_IMAGE_TRANSMIT:
            if USE_DERIVED_UART_FRAME:
                step_start_ms = time.ticks_ms()
                uart_img = make_uart_frame_from_sd_image(sd_img)
                uart_derive_ms = time.ticks_diff(time.ticks_ms(), step_start_ms)
            else:
                step_start_ms = time.ticks_ms()
                configure_camera_for_uart(camera)
                uart_config_ms = time.ticks_diff(time.ticks_ms(), step_start_ms)

                step_start_ms = time.ticks_ms()
                uart_img = camera.snapshot()
                uart_snapshot_ms = time.ticks_diff(time.ticks_ms(), step_start_ms)

            step_start_ms = time.ticks_ms()
            jpeg_bytes = compressed_jpeg_bytes(uart_img)
            compress_ms = time.ticks_diff(time.ticks_ms(), step_start_ms)
            image_byte_count = len(jpeg_bytes)

            step_start_ms = time.ticks_ms()
            save_uart_debug_jpeg_to_sd(jpeg_bytes)
            uart_debug_save_ms = time.ticks_diff(time.ticks_ms(), step_start_ms)

            step_start_ms = time.ticks_ms()
            write_base64_lines(uart, jpeg_bytes)
            uart_send_ms = time.ticks_diff(time.ticks_ms(), step_start_ms)

        step_start_ms = time.ticks_ms()
        gc.collect()
        gc_ms = time.ticks_diff(time.ticks_ms(), step_start_ms)

        frame_total_ms = time.ticks_diff(time.ticks_ms(), frame_start_ms)
        report_uart_frame_rate(
            image_byte_count,
            sd_config_ms,
            sd_snapshot_ms,
            ml_ms,
            debug_draw_ms,
            sd_save_ms,
            uart_config_ms,
            uart_snapshot_ms,
            uart_derive_ms,
            ml_send_ms,
            compress_ms,
            uart_debug_save_ms,
            uart_send_ms,
            gc_ms,
            frame_total_ms
        )
    except Exception as exc:
        safe_print("WARN_OPENMV_LOOP_ERROR error=%s" % format_exception(exc))

    time.sleep_ms(FRAME_INTERVAL_MS)
