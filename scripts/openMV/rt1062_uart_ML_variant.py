import binascii
import csi
import gc
import time
from machine import UART
import image
from collections import namedtuple


UART_BUS = 1
BAUDRATE = 10600
CHUNK_SIZE = 48
INTER_LINE_DELAY_MS = 2
FRAME_INTERVAL_MS = 0

FRAME_SIZE = csi.QQVGA
JPEG_QUALITY = 50

CAMERA_HEIGHT_CM = 7
CAMERA_FOV_DEGREES = 70
CAMERA_PITCH_DEGREES = 0

#color thresholds for blob detection
Bush_LAB_thresholds = [(0, 30), (-128, 10),(-30, -10)] # Detects all blobs with an average color lighter than 180

# birds eye view parameters
HORIZON_MARGIN_PX = 10

MAX_BUSH_PIXELS = 3000
HORIZON_LINE_THRESHOLD = 1000
HORIZON_THETA_DEGREES = 90
HORIZON_THETA_MARGIN_DEGREES = 20

BushDetection = namedtuple("BushDetection", "blobs sizes centers")
HorizonDetection = namedtuple("HorizonDetection", "line coords y_px candidates")
Obstacle = namedtuple("Obstacle", "blob priority heading_degrees")


def setup_camera():
    cam = csi.CSI()
    cam.reset()

    pixformat = getattr(csi, "RGB565", csi.RGB565)
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


def find_bushes(img):

    print("Finding bushes...")

    detected_blobs = img.find_blobs(Bush_LAB_thresholds, merge = True) # Detects all blobs with an average color lighter than 180
    filtered_blobs = []
    blob_sizes = []
    blob_centers = []

    for blob in detected_blobs:
        # Large blobs are likely false positives. Tune this for the expected bush size.
        if blob.pixels() > MAX_BUSH_PIXELS:
            continue

        filtered_blobs.append(blob)
        blob_sizes.append(blob.pixels())
        blob_centers.append((blob.cx(), blob.cy()))

        print("Blob size: %d, Blob center x: %d, Blob center y: %d" % (blob.pixels(), blob.cx(), blob.cy()))

        # Draw ellipse and crosshairs for each blob.
        img.draw_ellipse(blob.enclosed_ellipse(), color = (255, 255, 255))
        img.draw_cross(blob.cx(), blob.cy(), color = (0, 0, 0))

    if len(filtered_blobs) == 0:
        print("No blobs found.")

    return BushDetection(filtered_blobs, blob_sizes, blob_centers)

def find_horizon(img):

    lines = img.find_lines(threshold = HORIZON_LINE_THRESHOLD, theta_margin = 10, rho_margin = 10, roi = (0, int(img.height()*0.25), img.width(), int(img.height()*0.75)))
    horizontal_lines = []

    for line in lines:
        error = abs(line.theta() - HORIZON_THETA_DEGREES)
        # remove lines that are not approximately horizontal
        if error <= HORIZON_THETA_MARGIN_DEGREES:
            horizontal_lines.append(line)

    if len(horizontal_lines) > 1:
        horizontal_lines.sort(key = lambda line: (line.x1()+line.x2())/2, reverse = True)
        horizon = horizontal_lines[0]
        print("Horizon line rho: %d, Horizon line theta: %d" % (horizon.rho(), horizon.theta()))

    elif len(horizontal_lines) == 1:
        horizon = horizontal_lines[0]
        print("Horizon line rho: %d, Horizon line theta: %d" % (horizon.rho(), horizon.theta()))

    else:
        print("No lines found.")
        horizon_y_px = img.height() // 2
        return HorizonDetection(None, (0, horizon_y_px, img.width() - 1, horizon_y_px), horizon_y_px, horizontal_lines)

    coords = horizon.line()
    horizon_y_px = (horizon.y1() + horizon.y2()) // 2
    return HorizonDetection(horizon, coords, horizon_y_px, horizontal_lines)




### not too important since the plan is to handle reconstruction on the GS side
# def blobs_to_birds_eye(bush_detection, horizon_height_px, camera_fov_degrees, camera_height_cm, image_width_px):
#     # This function would contain the logic to project the blob coordinates from the image space to the real world space, based on the camera's intrinsic parameters and the known geometry of the scene. This is a complex task that typically involves camera calibration and may require additional information about the scene, such as the distance to the objects or the height of the camera. For simplicity, this function is left as a placeholder.
#     obstacles = []

#     for blob in bush_detection.blobs:

#         # if the blob is above the horizon line, it is likely a false positive and is ignored. This threshold can be adjusted based on the expected position of the bushes in the image.
#         if blob.cy() < horizon_height_px + HORIZON_MARGIN_PX:
#             continue
        
#         px_from_horizon = (blob.cy() - horizon_height_px)*(blob.cy() - horizon_height_px)


#         obstacle_priority = blob.pixels()*(px_from_horizon) # This is a simple heuristic for determining the priority of the obstacle, based on its size and distance from the camera. The larger and closer the blob is, the higher its priority.        
#         obstacle_heading = (blob.cx() - image_width_px // 2) * (camera_fov_degrees / image_width_px) # This is a simple heuristic for determining the heading of the obstacle, based on its horizontal position in the image. The further to the left or right the blob is, the greater its heading angle from the center of the camera's field of view.
#         print("Obstacle priority: %d, Obstacle heading: %d degrees" % (obstacle_priority, obstacle_heading))
#         obstacles.append(Obstacle(blob, obstacle_priority, obstacle_heading))

#     return obstacles


camera = setup_camera()
uart = UART(UART_BUS, baudrate=BAUDRATE)

while True:
    # try:
    #     jpeg_bytes = compressed_jpeg_bytes(camera)
    #     write_base64_lines(uart, jpeg_bytes)
    #     gc.collect()
    # except Exception as exc:
    #     print("UART image send error:", exc)

    # time.sleep_ms(FRAME_INTERVAL_MS)

    try:
        img = camera.snapshot()
    except Exception as exc:
        print("Camera error:", exc)
        continue

    try:
        horizon_detection = find_horizon(img)
    except Exception as exc:
        print("Horizon detection error:", exc)
        fallback_horizon_y_px = img.height() // 2
        horizon_detection = HorizonDetection(None, (0, fallback_horizon_y_px, img.width() - 1, fallback_horizon_y_px), fallback_horizon_y_px, [])

    try:
        bush_detection = find_bushes(img)
    except Exception as exc:
        print("Blob detection error:", exc)
        bush_detection = BushDetection([], [], [])

    try:
        img.draw_line(horizon_detection.coords, color = (255, 0, 0))
    except Exception as exc:
        print("Image drawing error:", exc)
   


