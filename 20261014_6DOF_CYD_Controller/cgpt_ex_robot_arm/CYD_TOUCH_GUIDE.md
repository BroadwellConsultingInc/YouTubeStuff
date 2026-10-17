# CYD touchscreen and SD workaround

The ESP32-2432S028R has three SPI pin sets and two usable hardware SPI buses.
This package follows the linked Random Nerd Tutorials workaround:

| Device | Bus | MOSI | MISO | SCK | CS | IRQ |
|---|---|---:|---:|---:|---:|---:|
| TFT | HSPI | 13 | 12 | 14 | 15 | — |
| SD | VSPI | 23 | 19 | 18 | 5 | — |
| Touch | GPIO software SPI | 32 | 39 | 25 | 33 | 36 |

The four bundled touch library files contain their original license notices.
We removed SPIClass inheritance, hardware transaction calls, and the remaining
hardware dummy transfer from the tutorial's modified libraries. Software touch
never initializes or transfers through HSPI/VSPI.

## Update your existing sketch

1. Copy **all fourteen .h/.cpp files** from the package into your sketch folder.
   These include `cgpt_ex_cyd_touch.h/.cpp`, `SoftSPI.h/.cpp`, and
   `XPT2046_TouchscreenSOFTSPI.h/.cpp`.
2. Keep your existing TFT setup, flush callback, buffers, and LVGL tick source.
   With TFT_eSPI, enable `USE_HSPI_PORT` in the selected CYD setup. Keep its
   CYD TFT pins and working display orientation. SD uses VSPI at 4 MHz.
3. Remove your old `XPT2046_Touchscreen` hardware initialization, including its
   `SPIClass(VSPI)` touch object, `touchscreenSPI.begin(...)`, `touchscreen.begin(...)`,
   and any direct hardware touch polling in your loop or another task. Remove
   the old reader if it refers to those deleted objects.
4. You may omit the old LVGL pointer registration: this adapter creates one.
   Or keep its pointer device and supply a temporary callback that reports
   released until `cgpt_ex_begin()` replaces it. Do not leave a second touch
   device or an asynchronous hardware touch reader active.
5. After `Wire.begin()`, `lv_init()`, and display registration, call your
   existing `cgpt_ex_begin()`. It now installs software touch **before** mounting
   SD and showing the chip startup screen. Keep calling `cgpt_ex_service()` and
   `lv_timer_handler()` in your existing loop. No extra touch service is needed.

For a sketch with multiple pointer devices or displays, select the touchscreen
explicitly after its registration and before calling `cgpt_ex_begin()`:

```cpp
#include "cgpt_ex_cyd_touch.h"
// cgpt_ex_touchInput is your registered lv_indev_t*.
if (!cgpt_ex_cydTouchBegin(cgpt_ex_touchInput)) {
    Serial.println("Software touchscreen initialization failed");
}
```

Without an explicit device, the adapter reuses the first pointer on the default
display, or creates one if absent. Repeated calls retain that device. Keep it
alive for the lifetime of the application. Calls run in the LVGL thread.

## Rotation and calibration

The UI remains 320x240. The reader maps touch coordinates into the display's
unrotated coordinate space, and LVGL applies its display rotation once.
Defaults in `cgpt_ex_config.h` are:

```cpp
static constexpr int8_t cgpt_ex_touchRotation = -1;
static constexpr int16_t cgpt_ex_touchRawMinX = 200;
static constexpr int16_t cgpt_ex_touchRawMaxX = 3700;
static constexpr int16_t cgpt_ex_touchRawMinY = 240;
static constexpr int16_t cgpt_ex_touchRawMaxY = 3800;
```

`-1` automatically chooses touch rotation 2 for a portrait base display and
rotation 1 for a landscape base display. The tutorial's 240x320 display with
LVGL rotation 270 therefore uses touch rotation 2. A direct 320x240 display
uses rotation 1. Set 0..3 explicitly if your panel orientation differs.
Adjust raw bounds for your calibrated panel; reversing the min/max values
reverses that axis. Points are clamped to 0..width-1 and 0..height-1.
Touch calibration depends on the panel and chosen rotation: test the four
corners, startup OK, sliders, Capture, and Files after uploading.

## Missing card and options

With no SD card, initialization continues, the startup OK button works, and
touch/joystick servo control remains available. CSV capture/playback requires
a card and reports a mount error when absent. After inserting a card, the
file selector's Refresh action can remount it; stop SD activity before swapping.
The touch bus remains independent during SD reads, writes, and remounts.

Software touch defaults on for ESP32, even if SD is compiled out. To retain
your own compatible touch driver, set `cgpt_ex_enableCydSoftTouch=0` in the
common config or whole-build compiler definitions. This excludes the bundled
touch implementation. Defining a switch only in your .ino does not configure
separately compiled .cpp files.

Your servo/joystick defaults, manual one-point Capture/Store behavior, default
1500 ms CSV timing, and root-only file selector remain unchanged.

## Validation

Native checks use the real LVGL 8.3.11 and 9.2.2 code with simulated touch ADC
and GPIO, across all four display rotations and pointer creation/reuse. They
verify touch initialization despite a failed SD mount, press/release, edge
clamping, SD remount and CSV writes between touch reads, and no writes to the
hardware SPI pin sets by the touch driver.
This is not an ESP32 firmware build or a physical-board test.
