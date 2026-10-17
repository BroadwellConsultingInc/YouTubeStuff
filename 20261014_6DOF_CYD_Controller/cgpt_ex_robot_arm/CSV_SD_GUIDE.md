# Manual CSV capture and SD playback on ESP32-2432S028R

Copy **all fourteen .h/.cpp files** into your existing sketch. Keep your display,
flush callback, Wire initialization, and LVGL tick source. Keep the working
software touch arrangement described in CYD_TOUCH_GUIDE.md.
Call `cgpt_ex_begin()` after those are initialized and `cgpt_ex_service()` in
the same thread as `lv_timer_handler()`. Chip addresses remain 0x63/0x64;
servo and joystick channels are pins 0..5, with chip-side Store on input pin 7.
Previous is on input-chip pin 6; Next and Delete are on servo-chip pins 6 and 7.
The joystick compile option continues to work; all four physical buttons remain active.

See **PHYSICAL_EDITING_GUIDE.md**: physical Previous/Next/Delete enter editing,
stop timed playback, and navigate/delete the active CSV. Capture/Store inserts
before the selected point while editing, and all edits save to that same file.

## Capture a sequence

The SD-enabled screen has Capture, Play, Stop, New, Files, and Loop buttons.

1. **New** creates the next sequential root file, starting at `ARM00001.csv`.
   It closes any previous capture and writes the header only.
2. Position the arm using joysticks or sliders.
3. During normal recording, **Capture** appends exactly one six-joint pose. The physical debounced
   Store button does the same. If no capture is open, the first Capture press
   creates the next file and writes one point.
4. Repeat positioning/Capture as desired.
5. **Stop** flushes and closes the file, without capturing a final point.

Jogging, waiting, New, Stop, and normal loop service never add pose rows.
The chip's debounce logic prevents a held Store button from adding more rows.
If multiple complete presses occur between polls, each captures the command
pose sampled by that poll, rather than reconstructing historical positions.

Each captured row is flushed to the card. Use Stop before removing the card;
power off the CYD before exchanging it with the PC. Files/Refresh remounts
after replacement when the module owns the SD mount.

During editing, existing CSV files are modified only by Store/Capture or Delete.
Previous/Next do not write points. New capture file numbering is unchanged.

The root scan uses one above the highest existing `ARMnnnnn.csv` number,
up to `ARM99999.csv`. Existing files are not overwritten. New without any
Capture presses leaves a header-only file; trying to play it reports that
there are no positions.

## CSV format and PC editing

```csv
move_ms,base,shoulder,elbow,wrist,wrist_roll,gripper
1500,32768,32768,32768,32768,32768,32768
2000,40000,32000,40000,32768,32768,40000
```

The header names/order are fixed. `move_ms` is the time to move **to this
row's pose from the preceding command pose**, in milliseconds. The first row
moves from the arm's current commands. Each Capture uses **1500 ms by
default**, regardless of how much real time you spent positioning the arm or
waiting between button presses. Change one row's duration on the PC without
adjusting later rows; these are durations, not absolute timestamps.

To change the default for subsequent captures in Arduino code:

```cpp
cgpt_ex_setRecordTiming(2000, 0);
```

The first argument is used as CSV move duration. The second is used only by
the legacy RAM pose list; CSV has no hold column. To add a dwell on the PC,
repeat the preceding row's six positions and set `move_ms` to the dwell time.

All positions must be integer commands 0..65535 and within the configured
joint limits. `move_ms` must be an integer **20..600000 ms**. Command values
are not feedback measurements or Cartesian coordinates.

Edit with a text editor, Excel, or LibreOffice. Save **comma-separated CSV**,
preserving the header. Quoted numeric fields/headers, spaces around fields,
Windows CRLF, UTF-8 BOM, blank rows, and a final row without a newline are
supported. Semicolon/tab separators, decimal/exponential notation, comments,
extra columns, and unevaluated formulas are not. At most 63 consecutive blank
lines are accepted. Invalid data is rejected, never wrapped or partially used.

The included `example_motion.csv` is editable sample data. Its positions are
illustrative; adapt them to your calibrated arm. Put it in the SD **root**,
under any filename ending in `.csv` or `.CSV`.

## Choose and play a file

Press Stop to close capture, then **Files**. Four ordinary root `.csv` files
are shown per page, with Previous/Next/Back/Refresh. The selector does not
enter folders or scan subdirectories. It uses filesystem enumeration order,
not alphabetic sorting. Names longer than 94 bytes are skipped; spaces and
UTF-8 names otherwise work. Playback also rejects subdirectory/traversal paths.

Tap a file to start. The entire file is validated in batches of 16 rows per
service pass **before moving**, with Check shown in the status. Larger files
take longer to check. Playback then streams one row at a time and interpolates
all joints together; SD sequences are not limited to 100 poses. RAM use does
not grow with file length. Loop reopens the file after its last pose and moves
back to the first using that first row's duration.

During playback the bottom status shows `Play filename current/total`, for
example `Play ARM00001.csv 3/12`. The total is obtained from the existing full
validation pass; it is not the RAM pose count. Steps start at 1 and return to
1 on each loop. While validation is running the status remains `Check filename`.

Stop closes playback and holds the latest commands with centered joysticks.
A slider takes manual control. Joystick/Store presses are ignored during
checking/playback. Previous/Next/Delete interrupt playback to enter editing.
In the file selector jogging is paused, and physical button events are discarded
when returning. Outside editing mode, selecting a file starts playback. While
editing, selection retains editing and selects its first point directly.
Play repeats the selected CSV and exits editing. Invalid rows,
headers, limits, and read errors stop playback with a status message;
`cgpt_ex_sdError()` provides its full text.

The RAM interfaces still exist for use elsewhere in the sketch. In the
SD-enabled UI, Store/Undo/Clear are replaced by Capture/New/Files.

Public APIs:

```cpp
cgpt_ex_newCsvCapture();              // Header only.
cgpt_ex_capturePoint();               // One point, default 1500 ms.
cgpt_ex_finishCsvCapture();           // Close only.
cgpt_ex_startCsvPlayback("example_motion.csv");
cgpt_ex_openFileSelector();
cgpt_ex_closeFileSelector();
```

## CYD SPI integration

The onboard SD slot uses CS GPIO5, SCK GPIO18, MISO GPIO19, and MOSI GPIO23.
The default adapter creates `SPIClass(VSPI)` and mounts at 4 MHz. The CYD
display, touch, and SD use three different pin sets, while ESP32 has two usable
hardware SPI controllers. Reserve **HSPI for TFT**, **VSPI for SD**, and use
**software SPI for touch**. A touch driver already using VSPI on GPIO25/39/32
must be adapted before this default SD mount can coexist with it. Another
VSPI object does not create an additional controller.

The software touch workaround is now included and enabled by default on ESP32,
using the tutorial's `XPT2046_TouchscreenSOFTSPI` and `SoftSPI` libraries, with
remaining hardware SPI dependencies removed. Follow **CYD_TOUCH_GUIDE.md**:
remove old hardware touch initialization/polling, keep TFT on HSPI, and call
`cgpt_ex_begin()` after LVGL display registration. The call binds software touch
before mounting SD. The adapter supports LVGL 8 and 9 and reuses the existing
pointer input when present.

If you already have a compatible software touch reader, set
`cgpt_ex_enableCydSoftTouch=0`. If your sketch mounts global `SD` itself, set
`cgpt_ex_mountSdInternally=0` and mount it before `cgpt_ex_begin()`; your sketch
then owns remounts. External SD initialization must not abort setup when the
card is absent.

A missing SD does not prevent chip initialization, startup OK, or normal
servo/jog control. SD actions report the mount error. SD support defaults on
for ESP32; `cgpt_ex_enableSd=0` restores the legacy RAM buttons while software
touch remains available. Options must be changed in the common config or
whole-build compiler definitions, not only in the .ino file.

Arduino SD operations are synchronous. Manual recording writes only on a
button press. Validation is divided into short batches. A delayed playback
loop reaches each endpoint instead of skipping poses. Physical card timing
and coexistence with your specific display/touch driver need hardware testing.

## Verification

The added software touch path passed native GPIO/ADC simulations against real
LVGL 8.3.11 and 9.2.2, all four LVGL rotations, with an existing pointer and
without one. Checks covered missing-card initialization with startup still
visible, pointer reuse/idempotence, press/release, retained release coordinates,
edge clamping, SD remount/capture interleaved with touch reads, and no hardware
SPI dependency in the bundled touch driver.
These checks simulate the touch controller; panel calibration needs board testing.


Native tests with actual LVGL 8.3.11 and 9.2.2 passed with joysticks enabled
and disabled, SD enabled and disabled. Filesystem simulations covered header-
only New/Stop, exactly one row per Capture/Store, no rows during jogging/waits,
sequential naming/no overwrite, PC-edited CSV/BOM/CRLF, integer overflow,
full-file validation before motion, interpolation/loop/stop, root-only paging,
250-point files, millis rollover, mount/read/write failures, and UI selection.
The SD adapter passed C++11 syntax checking against the current ESP32 core's
actual SD/FS headers with platform/SPI stubs. This is not a physical-board
or full ESP32 firmware build. The source still needs integration with your
existing display/touch sketch and testing on the arm.

Sources:

- https://docs.espressif.com/projects/arduino-esp32/en/latest/api/spi.html
- https://github.com/espressif/arduino-esp32/tree/master/libraries/SD
- https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/TROUBLESHOOTING.md
- https://randomnerdtutorials.com/esp32-cyd-display-touchscreen-microsd-card/
- https://github.com/RuiSantosdotme/ESP32-TFT-Touchscreen/tree/main/examples/ESP32_CYD_Micro_SD_Card_Display_Touchscreen
