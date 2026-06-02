import csi
import rpc
import time


SPI_BUS = 1
CS_PIN = "P3"
CLK_POLARITY = 1
CLK_PHASE = 0

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


interface = rpc.rpc_spi_slave(
    cs_pin=CS_PIN,
    clk_polarity=CLK_POLARITY,
    clk_phase=CLK_PHASE,
    spi_bus=SPI_BUS,
)
interface.register_callback(snapshot)

while True:
    try:
        interface.loop()
    except Exception as exc:
        print("RPC SPI loop error:", exc)
        time.sleep_ms(100)
