# OpenMV ML Interface Spec

This document describes the OpenMV bush-detection data path from the OpenMV
script, across UART, into the MARS receiver, through `AntennaConnectorInterface`,
and out through rover telemetry JSON.

Update this file in the same change whenever any `ML_*` UART line, MARS ML
storage struct, or outbound telemetry JSON shape changes.

## OpenMV Output State

The OpenMV script `rt1062_uart_image_ml_sender.py` produces ML data after the
high-quality RGB565 frame is captured and before any image downscaling or JPEG
compression.

OpenMV-side values:

- `current_frame_id`: integer frame counter, sent as `frameId`.
- `horizon_detection.line`: `None` or OpenMV line object, sent as
  `horizon.valid`.
- `horizon_detection.y_px`: integer pixel row, sent as `horizon.yPx`.
- `horizon_detection.coords`: tuple `(x1, y1, x2, y2)`, sent as horizon line
  endpoints.
- `bush_detection.blobs`: list of OpenMV blob objects.
- Per blob: `cx()`, `cy()`, `pixels()`, and optional `enclosed_ellipse()`.

Detection runs only when `ENABLE_ML_DATA_TRANSMIT` or `ENABLE_DEBUGS` is true.
Debug drawing happens after detection and before SD save.

## UART Protocol

ML data is newline-delimited ASCII. It is never sent between `IMG_BEGIN` and
`IMG_END`.

```text
ML_BEGIN <frameId> <blobCount>
ML_HORIZON <valid:0|1> <yPx> <x1> <y1> <x2> <y2>
ML_BLOB <index> <cx> <cy> <pixels> [ellipseCx ellipseCy ellipseRx ellipseRy ellipseRotation]
ML_END
```

Example ML-only frame:

```text
ML_BEGIN 42 2
ML_HORIZON 1 77 0 77 159 79
ML_BLOB 0 20 90 123 20 90 8 5 0
ML_BLOB 1 100 95 250 100 95 12 7 3
ML_END
```

Example ML followed by image transfer:

```text
ML_BEGIN 43 1
ML_HORIZON 0 80 0 80 159 80
ML_BLOB 0 62 102 180 62 102 9 6 1
ML_END
IMG_BEGIN 7210
<base64 jpeg line>
<base64 jpeg line>
IMG_END
```

The MARS receiver uses one line reader for both protocols. `ML_*`, `CFG*`, and
`DBG_*` lines are not appended to image base64 data.

## MARS Receiver State

`OpenMVReceiver` owns the OpenMV UART parser.

Image state remains unchanged:

- `receiving`: `bool`, true while inside an image transfer.
- `imageQueue[maxQueueSize]`: `String`, queued base64 image payloads.
- `imageSizes[maxQueueSize]`: `int`, decoded JPEG byte counts when provided by
  `IMG_BEGIN <byteCount>`.
- `getImage(String&, int&)`: copies the oldest queued image to the caller.

ML state:

- `mlReceiving`: `bool`, true after `ML_BEGIN` until `ML_END` or a malformed
  ML line resets the packet.
- `incomingMLResult`: `OpenMVMLData`, scratch packet being parsed.
- `pendingMLResult`: `OpenMVMLData`, latest completed packet ready for handoff.
- `mlResultAvailable`: `bool`, true when `pendingMLResult` can be read.
- `hasMLResult()`: reports whether a complete ML packet is available.
- `getMLResult(OpenMVMLData&)`: copies and clears the pending ML result.

`OpenMVReceiver::runReceiver()` still returns true only when an image completes.
ML completion is intentionally separate.

## Shared MARS ML Types

The shared data type lives in
`src/multi-state-utils/ImageTransfers/OpenMVMLData.h`.

```cpp
static constexpr uint8_t OPENMV_ML_MAX_BLOBS = 16;

struct OpenMVMLBlob {
    int16_t cx;
    int16_t cy;
    uint16_t pixels;
    bool hasEllipse;
    int16_t ellipseCx;
    int16_t ellipseCy;
    int16_t ellipseRx;
    int16_t ellipseRy;
    int16_t ellipseRotation;
};

struct OpenMVMLData {
    uint32_t frameId;
    bool horizonValid;
    int16_t horizonYPx;
    int16_t horizonX1;
    int16_t horizonY1;
    int16_t horizonX2;
    int16_t horizonY2;
    uint8_t blobCount;
    uint8_t expectedBlobCount;
    bool droppedBlobs;
    OpenMVMLBlob blobs[OPENMV_ML_MAX_BLOBS];
};
```

`int16_t` is used for pixel coordinates and ellipse values because OpenMV frame
dimensions are small. `uint16_t` is used for pixel area. `uint32_t` is used for
the frame counter.

## Antenna Connector Storage

`AntennaConnectorInterface` stores the latest complete ML result. This is not
queued or locked like image data because it is small and latest-value telemetry
is the desired behavior.

Storage variables:

- `latestOpenMVMLData`: `OpenMVMLData`, latest complete result.
- `openMVMLDataAvailable`: `bool`, true after the first ML result is stored.

Public methods:

- `intakeOpenMVMLData(const OpenMVMLData& data)`: replaces the stored result.
- `accessOpenMVMLData(OpenMVMLData& data) const`: copies the latest result.
- `hasOpenMVMLData() const`: reports whether ML data has been stored.

`PayloadROV` calls `openMVReceiver.getMLResult(...)` and immediately passes the
result to `antennaConnector.intakeOpenMVMLData(...)`.

## Telemetry JSON

`AntennaSerialTransmitter` appends a top-level `ml` field to rover telemetry.
The existing `sensors` and `image` fields keep their existing shapes.

When no ML result is available:

```json
{
  "type": "roverTelemetry",
  "connection": true,
  "sensors": {
    "leftScrewCurrent": 0.000,
    "rightScrewCurrent": 0.000,
    "batteryVoltage": 12.100
  },
  "image": null,
  "ml": null
}
```

When ML data is available:

```json
{
  "type": "roverTelemetry",
  "connection": true,
  "sensors": {
    "leftScrewCurrent": 0.000,
    "rightScrewCurrent": 0.000,
    "batteryVoltage": 12.100
  },
  "image": null,
  "ml": {
    "frameId": 42,
    "expectedBlobCount": 2,
    "droppedBlobs": false,
    "horizon": {
      "valid": true,
      "yPx": 77,
      "x1": 0,
      "y1": 77,
      "x2": 159,
      "y2": 79
    },
    "blobs": [
      {
        "cx": 20,
        "cy": 90,
        "pixels": 123,
        "ellipse": {
          "cx": 20,
          "cy": 90,
          "rx": 8,
          "ry": 5,
          "rotation": 0
        }
      },
      {
        "cx": 100,
        "cy": 95,
        "pixels": 250,
        "ellipse": {
          "cx": 100,
          "cy": 95,
          "rx": 12,
          "ry": 7,
          "rotation": 3
        }
      }
    ]
  }
}
```

If OpenMV reports more than `OPENMV_ML_MAX_BLOBS`, MARS stores the first 16
blob slots and sends `"droppedBlobs": true`.
