#pragma once

#include <Arduino.h>

static constexpr uint8_t MAX_ROVER_VISION_BLOBS = 12;

struct RoverVisionHorizon {
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
};

struct RoverVisionBlob {
    String id = "";
    int cx = 0;
    int cy = 0;
    int a = 0;
    int b = 0;
    int rotation = 0;
    float confidence = 0.0f;
    int pixels = 0;
};

struct RoverVisionFrame {
    int frameWidth = 0;
    int frameHeight = 0;
    uint32_t timestamp = 0;
    RoverVisionHorizon horizon;
    RoverVisionBlob blobs[MAX_ROVER_VISION_BLOBS];
    uint8_t blobCount = 0;
};
