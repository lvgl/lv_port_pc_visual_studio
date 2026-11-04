# ESP32 Smart Gauge (320x240)

This Arduino sketch draws a tachometer-style smart gauge with a metallic bezel,
0-8 scale, redline, RPM needle, speed (`KPM`) and gear. It uses LVGL 9 and
TFT_eSPI; it is a separate example and does not change the Windows simulator.

## Requirements

- An ESP32 board supported by the installed Arduino-ESP32 core.
- A color TFT configured to expose a **320x240** viewport through TFT_eSPI.
- LVGL 9.x and TFT_eSPI installed in the Arduino IDE or CLI.

Open `ArduinoSmartGauge.ino` after installing the libraries and compile/upload
for the selected ESP32 board. Before uploading, configure TFT_eSPI's
`User_Setup.h` (or selected setup file) for the actual display controller,
resolution, SPI pins, control pins, and any required display options. The sketch
sets rotation to `1`; change `tft.setRotation(1)` if the panel's landscape
orientation uses another rotation. There are no assumed GPIO assignments or
panel-controller settings in this example.

The draw buffer is a 320x30 RGB565 strip rather than a full-screen framebuffer
to limit RAM use. If the panel reports a viewport other than 320x240, setup
prints the configured dimensions and waits instead of drawing a clipped gauge.

## Supplying live values

The startup display is `6000 RPM`, `75 KPM`, gear `4`. Replace the sample call
in `setup()` with your initial readings, then call this API whenever fresh
values arrive:

```cpp
smart_gauge_set_values(rpm, speedKpm, gear);
```

RPM is limited to 0-8000; the needle turns red from 6000 RPM. Speed and gear
are unsigned integer values and can be adapted to the units/data source used
by the vehicle. Call the update from the same Arduino task that runs `loop()`
and `lv_timer_handler()`; do not mutate LVGL objects directly from an
interrupt or another FreeRTOS task.

If the gauge digits appear too small, enable `LV_FONT_MONTSERRAT_24` or
`LV_FONT_MONTSERRAT_32` in the LVGL configuration used by the Arduino library.
The sketch falls back to `LV_FONT_DEFAULT` when neither is enabled.

This example provides the display flush adapter for TFT_eSPI, not vehicle
sensing or a universal board/display driver. Verify the panel setup and
electrical connections against the exact ESP32 board and display documentation.
