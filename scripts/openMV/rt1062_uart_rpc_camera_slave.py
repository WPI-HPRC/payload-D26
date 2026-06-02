import csi
import rpc
import time


UART_PORT = 3
BAUDRATE = 921600

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


camera = setup_camera()


def snapshot(_args):
    img = camera.snapshot()

    try:
        return img.compress(quality=JPEG_QUALITY).bytearray()
    except TypeError:
        return img.compress().bytearray()


interface = rpc.rpc_uart_slave(baudrate=BAUDRATE, uart_port=UART_PORT)
interface.register_callback(snapshot)

while True:
    try:
        interface.loop()
    except Exception as exc:
        print("RPC UART loop error:", exc)
        time.sleep_ms(100)
