#pragma once

#include <Arduino.h>

/**
 * Shared vision detection data structures.
 *
 * These mirror the over-the-air FlatBuffer schema in
 * telemetry-2026/PayloadTelemetryPacket.fbs (struct BlobCoord and the horiz_*
 * fields) so a detection frame maps one-to-one onto the telemetry packet:
 *
 *   struct BlobCoord {
 *     index: byte; x: byte; y: byte; width: byte; height: byte;
 *     ellipse_a: byte; ellipse_b: byte; rotation: int16; confidence: float;
 *   }
 *   blob_data: [BlobCoord];
 *   horiz_x1/x2/y1/y2: byte;  horiz_valid: bool;
 *
 * The OpenMV camera computes blobs/horizon on-device and streams them to the
 * flight computer as JSON lines over the camera UART (one flat JSON object per
 * line, routed by its "type" field):
 *
 *   {"type":"det_begin","count":<n>}
 *   {"type":"blob","index":..,"x":..,"y":..,"w":..,"h":..,"a":..,"b":..,"r":..,"c":..}
 *   {"type":"horizon","x1":..,"y1":..,"x2":..,"y2":..}
 *   {"type":"det_end"}
 *
 * OpenMVReceiver parses these into VisionDetections; the flight computer then
 * encodes them into the PayloadTelemetryPacket FlatBuffer for the ground link.
 */

// Maximum number of blobs carried in a single detection frame. Blobs beyond
// this count in one frame are dropped on the receiver side.
static const uint8_t MAX_VISION_BLOBS = 12;

// Mirrors hprc::BlobCoord. Pixel fields are bytes to match the schema, so
// image coordinates must fit in 0-255 (QQVGA is 160x120).
struct VisionBlob {
    uint8_t index = 0;        // blob index within the frame
    uint8_t x = 0;            // ellipse center x in image pixels
    uint8_t y = 0;            // ellipse center y in image pixels
    uint8_t width = 0;        // bounding width in pixels
    uint8_t height = 0;       // bounding height in pixels
    uint8_t ellipseA = 0;     // ellipse semi-axis a  (schema: ellipse_a)
    uint8_t ellipseB = 0;     // ellipse semi-axis b  (schema: ellipse_b)
    int16_t rotation = 0;     // ellipse rotation in degrees
    float confidence = 0.0f;  // fill density, 0.0 - 1.0
};

// Mirrors the horiz_* fields: start = (x1, y1), end = (x2, y2).
struct VisionHorizon {
    uint8_t x1 = 0;
    uint8_t y1 = 0;
    uint8_t x2 = 0;
    uint8_t y2 = 0;
    bool valid = false;       // schema: horiz_valid
};

struct VisionDetections {
    VisionBlob blobs[MAX_VISION_BLOBS];
    uint8_t blobCount = 0;
    VisionHorizon horizon;
};
