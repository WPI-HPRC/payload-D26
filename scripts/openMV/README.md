# OpenMV RT1062 Image Transfer Scripts

These scripts run on the OpenMV RT1062 and expose a `snapshot` RPC callback
that returns a low-resolution compressed JPEG frame to the MARS board.

## Primary Path: SPI RPC

Use `rt1062_spi_rpc_camera_slave.py` first. It keeps the MARS board as the SPI
master and the OpenMV RT1062 as the RPC SPI slave.

Default OpenMV RT1062 SPI settings:

- SPI bus: `1`
- CS: `P3`
- SCLK: `P2`
- MOSI: `P0`
- MISO: `P1`
- Clock polarity: `1`
- Clock phase: `0`

Wire the bus by signal direction, not by pin name alone:

- MARS camera SCK to OpenMV `P2` / SPI1 SCLK
- MARS camera CS to OpenMV `P3` / SPI1 SS
- MARS camera MOSI to OpenMV `P0` / SPI1 MOSI
- MARS camera MISO to OpenMV `P1` / SPI1 MISO
- MARS GND to OpenMV GND

Both boards use 3.3 V logic. Do not connect the OpenMV RT1062 I/O pins to 5 V.

## UART Fallback

Use `rt1062_uart_tagged_image_sender.py` if SPI RPC fails on the RT1062
firmware with `ImportError: no module named 'pyb'`.

That error means the installed OpenMV `rpc` module is still using the
STM32-only `pyb` module. The RT1062 is not STM32-based, so that RPC path cannot
be fixed in user script code. The tagged UART sender avoids `rpc` entirely and
uses the same text framing that the MARS receiver tests already exercise:

- `IMG_BEGIN <jpeg byte count>`
- base64 image chunks, one line per chunk
- `IMG_END`

The default baud rate is `921600`; lower it to `115200` if the link is
unreliable.

UART wiring:

- MARS TX to OpenMV UART RX for the selected UART port
- MARS RX to OpenMV UART TX for the selected UART port
- Shared GND

## Raw SPI Template Caveat

The original `machine.SPI.write(...)` template makes the OpenMV act as the SPI
master. That does not match the current MARS camera bus design, where MARS is
intended to be the SPI master.

Raw `machine.SPI` slave mode is not the documented RT1062 path in OpenMV
MicroPython. The documented target/slave path is `rpc.rpc_spi_slave(...)`.
If RPC cannot run on the installed OpenMV firmware because it imports `pyb`, use
the non-RPC UART tagged sender before redesigning MARS as an SPI slave.

## Bench Validation Checklist

- The OpenMV script boots without import errors.
- The MARS board can call the OpenMV `snapshot` callback.
- The returned JPEG byte count is nonzero.
- Repeated captures do not grow memory without bound.
- The UART fallback sends `IMG_BEGIN` / base64 chunks / `IMG_END` if SPI RPC
  fails.
