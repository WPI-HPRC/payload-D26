# Open MV Camera Script for Reference

import sensor
import time, image, struct
import pyb
from pyb import I2C

#I2C
peripheral = 0x12

sensor.reset()
sensor.set_pixformat(sensor.GRAYSCALE)
sensor.set_framesize(sensor.QQVGA)

#sensor.skip_frames(time=2000)
sensor.set_quality() #0-100

i2c = I2C(2, I2C.PERIHERAL, addr=peripheral)

# Computer Vision
debug = True
thresholds = (0, 70) # Dark objects [min grey, max grey]

print("Initializing I2C")


while (True):
    try:
        # take image an send of over I2C
        if i2c.is_ready(peripheral):
            img = sensor.snapshot()
            jpeg = img.compress(quality=50)
            size = len(jpeg)

            #send the size first
            i2c.send(struct.pack("<H", size), timeout=1000)

            #send image in chunks
            chunk_size = 32
            for i in range(0, size, chunk_size):
                i2c.send(jpeg[i:i+chunk_size], timeout=1000)

        # check for blobs, also send of I2C
        blobs = img.find_blobs([thresholds], pixels_threshold=200, area_threshold=200)

        # debug to display blobs on camera
        if blobs and debug:
              for blob in blobs:
                  # Draw a box around the blob
                  img.draw_rectangle(blob.rect())
                  # Draw the centroid (cx, cy)
                  img.draw_cross(blob.cx(), blob.cy())

                  # Print centroid to console
                  print("Centroid X:", blob.cx(), "Y:", blob.cy())

    except OSError:
        print("Error, trying again")
        pass
