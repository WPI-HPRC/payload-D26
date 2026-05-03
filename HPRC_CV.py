# Image Transfer - Ast the Remote Device
# Example code: https://www.youtube.com/watch?v=WRHrqlKBZ3s

import image, newtwork, omv, rpc, sensor, struct

sensor.reset()
sensor.set_pixformat(sensor.GRAYSCALE)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time = 2000)

omv.disable_fb(True)

interface = rpc.rpc_usb_vcp_slave()

def jpeg_image_snapshot(data):
    pixformat, framesize = bytes(data).decode().split(",")
    sensor.set_pixformat(eval(pixformat))
    sensor.set_framesize(eval(framesize))
    img = sensor.snapshot().compress(quality=90)
    return struct.pack("<I", img.size())

def jpeg_image_read_cb():
    interface.put_bytes(sensor.get_fb().bytearray(), -5000)

def jpeg_image_read(data):
    if not len(data):
        interface.schedule_callback(jpeg_image_read_cb())
        return bytes()
    else:
        offset, size = struct.unpack("<II", data)
        return memoryview(sensor.get_fb().bytearray())[offset:offset+size]

interface.register_callback(jpeg_image_snapshot)
interface.register_callback(jpeg_image_read)

interface.loop()































# # Open MV Camera Script for Reference

# import sensor
# import time, image, struct
# import pyb
# from pyb import I2C
# from pyb import SPI

# #I2C
# peripheral = 0x12

# sensor.reset()
# sensor.set_pixformat(sensor.GRAYSCALE)
# sensor.set_framesize(sensor.QQVGA)

# #sensor.skip_frames(time=2000)
# sensor.set_quality() #0-100

# i2c = I2C(2, I2C.PERIHERAL, addr=peripheral)
# spi = SPI(2, SPI.PERIHERAL, baudrate=600000, polarity=1, phase=0, crc=0x7)

# # Computer Vision
# debug = True
# thresholds = (0, 70) # Dark objects [min grey, max grey]

# print("Initializing SPI")


# while (True):
#     try:
#         # take image an send of over I2C
#         if spi.is_ready(peripheral):
#             img = sensor.snapshot()
#             jpeg = img.compress(quality=50)
#             size = len(jpeg)

#             #send the size first
#             spi.send(struct.pack("<H", size), timeout=1000)

#             #send image in chunks
#             chunk_size = 32
#             for i in range(0, size, chunk_size):
#                 spi.send(jpeg[i:i+chunk_size], timeout=1000)

#         # check for blobs, also send of I2C
#         blobs = img.find_blobs([thresholds], pixels_threshold=200, area_threshold=200)

#         # debug to display blobs on camera
#         if blobs and debug:
#               for blob in blobs:
#                   # Draw a box around the blob
#                   img.draw_rectangle(blob.rect())
#                   # Draw the centroid (cx, cy)
#                   img.draw_cross(blob.cx(), blob.cy())

#                   # Print centroid to console
#                   print("Centroid X:", blob.cx(), "Y:", blob.cy())

#     except OSError:
#         print("Error, trying again")
#         pass
