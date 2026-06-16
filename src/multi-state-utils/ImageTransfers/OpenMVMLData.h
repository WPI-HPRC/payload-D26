#pragma once

#include <Arduino.h>

static constexpr uint8_t OPENMV_ML_MAX_BLOBS = 16;

struct OpenMVMLBlob {
    int16_t cx = 0;
    int16_t cy = 0;
    uint16_t pixels = 0;
    bool hasEllipse = false;
    int16_t ellipseCx = 0;
    int16_t ellipseCy = 0;
    int16_t ellipseRx = 0;
    int16_t ellipseRy = 0;
    int16_t ellipseRotation = 0;
};

struct OpenMVMLData {
    uint32_t frameId = 0;
    bool horizonValid = false;
    int16_t horizonYPx = 0;
    int16_t horizonX1 = 0;
    int16_t horizonY1 = 0;
    int16_t horizonX2 = 0;
    int16_t horizonY2 = 0;
    uint8_t blobCount = 0;
    uint8_t expectedBlobCount = 0;
    bool droppedBlobs = false;
    OpenMVMLBlob blobs[OPENMV_ML_MAX_BLOBS];
};
