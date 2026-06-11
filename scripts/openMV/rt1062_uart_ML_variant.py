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

blob_size = []
blob_cx = []
blob_cy = []

blobs = []
blob_cx_count = 0
blob_cy_count = 0
blob_num = 0

#color thresholds for blob detection
Bush_LAB_thresholds = [(0, 30), (-128, 10),(-30, -10)] # Detects all blobs with an average color lighter than 180

# default_horizon_data = (0, int(FRAME_SIZE[1]*0.5), FRAME_SIZE[0], int(FRAME_SIZE[1]*0.5))

# horizon_height_px = FRAME_SIZE[1]/2

# birds eye view parameters
HORIZON_MARGIN_PX = 10

obstacles = []


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

    blobs = img.find_blobs(Bush_LAB_thresholds, merge = True) # Detects all blobs with an average color lighter than 180


    if len(blobs) > 0:

        for blob in blobs: # Appends the blob size of each blob to the array blob_size, in total number of pixels

            # if the blob size is greater than 3000 pixels, it is likely a false positive and is ignored. This threshold can be adjusted based on the expected size of the bushes in the image.
            if blob.pixels() > 3000:
                blobs.remove(blob)
                continue

            # Appends the x and y coordinates of the center of each blob to the arrays blob_cx and blob_cy, respectively
            blob_size.append(blob.pixels())
            blob_cx.append(blob.cx())
            blob_cy.append(blob.cy())

            print("Blob size: %d, Blob center x: %d, Blob center y: %d" % (blob.pixels(), blob.cx(), blob.cy()))

            # Draw ellipse and crosshairs for each blob
            img.draw_ellipse(blob.enclosed_ellipse(), color = (255, 255, 255))
            img.draw_cross(blob.cx(), blob.cy(), color = (0, 0, 0))
    else:
        print("No blobs found.")

    return blobs

def find_horizon(img):

    lines = img.find_lines(threshold = 1000, theta_margin = 10, rho_margin = 10, roi = (0, int(img.height()*0.25), img.width(), int(img.height()*0.75)))

    for line in lines:

        error = abs(line.theta() - 90)
        # remove lines that are not approximately horizontal
        if error > 20:
            lines.remove(line)
            continue


    if len(lines) > 1:
        lines.sort(key = lambda line: (line.x1()+line.x2())/2, reverse = True)
        horizon = lines[0]
        print("Horizon line rho: %d, Horizon line theta: %d" % (horizon.rho(), horizon.theta()))
        
    elif len(lines) == 1:
        horizon = lines[0]
        print("Horizon line rho: %d, Horizon line theta: %d" % (horizon.rho(), horizon.theta()))

    else:
        print("No lines found.")
        horizon = (0, FRAME_SIZE[1] % 2, FRAME_SIZE[0], FRAME_SIZE[1]/2)
    return horizon     





def blobs_to_birds_eye(blobs, horizon_height_px, camera_fov_degrees, camera_height_cm):
    # This function would contain the logic to project the blob coordinates from the image space to the real world space, based on the camera's intrinsic parameters and the known geometry of the scene. This is a complex task that typically involves camera calibration and may require additional information about the scene, such as the distance to the objects or the height of the camera. For simplicity, this function is left as a placeholder.
    for blob in blobs:

        # if the blob is above the horizon line, it is likely a false positive and is ignored. This threshold can be adjusted based on the expected position of the bushes in the image.
        if blob.cy() < horizon_height_px + HORIZON_MARGIN_PX:
            blobs.remove(blob)
            continue
        
        px_from_horizon = (blob.cy() - horizon_height_px)*(blob.cy() - horizon_height_px)


        obstacle_priority = blob.pixels()*(px_from_horizon) # This is a simple heuristic for determining the priority of the obstacle, based on its size and distance from the camera. The larger and closer the blob is, the higher its priority.        
        obstacle_heading = (blob.cx() - FRAME_SIZE[0] % 2) * (camera_fov_degrees / FRAME_SIZE[0]) # This is a simple heuristic for determining the heading of the obstacle, based on its horizontal position in the image. The further to the left or right the blob is, the greater its heading angle from the center of the camera's field of view.
        print("Obstacle priority: %d, Obstacle heading: %d degrees" % (obstacle_priority, obstacle_heading))
        obstacles.append((blob, obstacle_priority, obstacle_heading))


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
    try:
        horizon = find_horizon(img)
    except Exception as exc:
        print("Horizon detection error:", exc)
        horizon = (0, FRAME_SIZE[1] % 2, FRAME_SIZE[0], FRAME_SIZE[1] % 2)
    try:
        blobs = find_bushes(img)
    except Exception as exc:
        print("Blob detection error:", exc)
        blobs = []

    try:
        img.draw_line(horizon[0], horizon[1], horizon[2], horizon[3], color = (255, 0, 0))
    except Exception as exc:
        print("Image drawing error:", exc)
    
    try:
        blobs_to_birds_eye(blobs, horizon[1], CAMERA_FOV_DEGREES, CAMERA_HEIGHT_CM)
    except Exception as exc:
        print("Bird's eye view transformation error:", exc)



