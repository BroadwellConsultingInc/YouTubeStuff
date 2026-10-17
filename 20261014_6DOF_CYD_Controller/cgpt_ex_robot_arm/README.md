# Arduino LVGL six-joint robot arm example

This module adds a 320x240 landscape LVGL screen to your existing Arduino
display/touch application. All identifiers introduced by the module, including
types, struct fields, function parameters, and local variables, begin with
`cgpt_ex_`. Existing Arduino, Serial Wombat, LVGL, and bundled third-party library API
names keep their names; see THIRD_PARTY_NOTICES.md.

For the new **manual SD CSV recording**, file selector, PC editing format,
and CYD SPI/touch integration, read **CSV_SD_GUIDE.md** first. Captures now
store one point per button press, with a default `move_ms` of 1500.

**Physical Previous/Next/Delete editing is now included.** Read
PHYSICAL_EDITING_GUIDE.md for wiring and insertion semantics. The buttons edit
the active CSV on SD or the RAM list, without timed playback while editing.

## Files and integration

Copy all fourteen `.h`/`.cpp` files into your existing Arduino sketch folder:

- `cgpt_ex_config.h`: pin assignments, addresses, calibration, capacity, timing.
- `cgpt_ex_robot_arm.h`: public interface and pose structure.
- `cgpt_ex_robot_arm.cpp`: touchscreen, pose list, playback, joystick jogging.
- `cgpt_ex_wombat.cpp`: Serial Wombat chip initialization and checked packets.
- `cgpt_ex_csv.h` / `cgpt_ex_csv.cpp`: PC-editable pose CSV format and validation.
- `cgpt_ex_sd.h` / `cgpt_ex_sd.cpp`: numbered files, root listing, capture, streaming playback.
- `cgpt_ex_cyd_touch.h` / `cgpt_ex_cyd_touch.cpp`: CYD software touch and LVGL binding.
- `SoftSPI.h` / `SoftSPI.cpp`, `XPT2046_TouchscreenSOFTSPI.h` / `.cpp`: locally bundled software touch libraries with fixes to remove all hardware SPI calls.

The target is **ESP32-2432S028R (Cheap Yellow Display)**. The ESP32 Arduino
core supplies `SD`, `FS`, and `SPI`; no additional SD library is required.
SD support defaults on when `ARDUINO_ARCH_ESP32` is defined. To disable it,
set `cgpt_ex_enableSd=0` in the common header or for the whole build.

Updated configuration: servo chip `0x63`, input chip `0x64`, six channels on
pins 0..5, and chip-side Debounced Input for Store on input-chip pin 7. The
startup screen displays each chip's actual reported model; it does not assume
the previous SW18AB or SW8B configuration.

Install the Serial Wombat Arduino library and use LVGL 8 or 9. Enable labels,
sliders, buttons, and a suitable font in your LVGL configuration. The preview
uses the standard Montserrat 14 default font; a larger default font may require
layout adjustments. The hardware adapter includes `<SerialWombat.h>`.

At the top of your existing sketch:

```cpp
#include "cgpt_ex_robot_arm.h"
```

At the end of your existing `setup()`, after `Wire.begin()`, `lv_init()`, and
display/touch registration:

```cpp
if (!cgpt_ex_begin()) {
    Serial.println("Robot arm initialization failed");
}
```

In your existing `loop()`:

```cpp
cgpt_ex_service();
lv_timer_handler();
```

Keep your existing display driver, buffer flush callback, and LVGL tick source.
On ESP32, `cgpt_ex_begin()` now replaces the first pointer input on the default
display with the bundled software SPI touch reader, or registers one if absent.
Remove the old hardware SPI touch initialization/polling and keep TFT on HSPI
(`USE_HSPI_PORT` for TFT_eSPI). Read CYD_TOUCH_GUIDE.md for the exact integration
steps, calibration, and the option to retain your own software touch driver.
Call all module functions from the same thread/context as LVGL; never from an ISR.

`cgpt_ex_service()` polls the joystick/button function, advances playback, and
sends changed servo commands every 20 ms. Do not independently poll
`cgpt_ex_readJoysticks()` while also using `cgpt_ex_service()`.

## Default connections

The defaults use your **two C-model Serial Wombat chips**, on the existing
global `Wire` bus. Chip A is at `0x63`, chip B at `0x64`. Set their actual
hardware addresses to match. The loaded firmware supports Servo, Analog Input,
and Debounced Input modes. All four physical buttons are debounced on-chip.

| Index | Joint | Chip A servo pin | Chip B joystick analog pin |
|---|---|---|---|
| 0 | Entire arm / base rotation | 0 | 0 |
| 1 | Shoulder raise/lower | 1 | 1 |
| 2 | Elbow raise/lower | 2 | 2 |
| 3 | Wrist raise/lower | 3 | 3 |
| 4 | Wrist / gripper rotation | 4 | 4 |
| 5 | Gripper open/close | 5 | 5 |

Connect each physical button between its pin and GND:

| Button | Address | Pin |
|---|---|---:|
| Store | 0x64 (chip B) | 7 |
| Previous | 0x64 (chip B) | 6 |
| Next | 0x63 (chip A) | 6 |
| Delete | 0x63 (chip A) | 7 |

All use 30 ms debounce, inversion, and chip weak pull-ups. Six analog axes
can be supplied by three two-axis joysticks. Connect each potentiometer wiper
to its analog input; power the potentiometers from the input chip's VDD/GND.
Use a servo supply appropriate for the arm and share ground with the control
electronics.

## Startup information screen

`cgpt_ex_begin()` initializes both chips, commands the configured initial servo
positions, and displays an information screen. Each chip has a column showing
its address/role, initialization result, full four-character model identifier,
firmware version, measured logic supply voltage, application/boot state,
unique identifier, MCU device ID/revision where available, executed 1 ms frames,
overflow-frame count, and initialization error code. Scroll the central panel
to see the remaining details. The wide **OK** button enters the arm controls.

Data comes from the chips, using the same model/version, supply-voltage, and
identity information as the Arduino `_SerialWombatChipFinder` example, with
additional system counters. Fields unavailable for a given model are shown
as **N/A**. The Serial Wombat library does not populate MCU device ID/revision
for its CH32-based models. This screen is a startup snapshot, not a live
monitor. VDD is the chip's logic supply; it is not the servo power voltage.

Joystick movement, recording, playback, and further servo command updates are
paused until OK. Servos already hold their configured initialization commands.
Physical button events accumulated while the information screen is open are
discarded when OK is pressed. A button still held then must be released and
pressed again to record. Both chips are queried even if one fails so the screen
can identify which one is absent, in boot mode, or failed initialization.
After a failed initialization, OK opens the controls with movement inhibited.

`cgpt_ex_isStartupVisible()` reports this state and
`cgpt_ex_dismissStartup()` performs the same action as OK. A successful or
failed reinitialization shows the startup screen again and preserves the pose
list. `cgpt_ex_getChipInfo(0)` returns the servo snapshot;
`cgpt_ex_getChipInfo(1)` returns the input snapshot.

The included `cgpt_ex_startup_preview.png` and
`cgpt_ex_startup_details_preview.png` show the top and scrolled views, using
**simulated example data**, not measurements from your hardware. Actual model,
version, identity, and voltage will be read at startup.

## Compile without joysticks

In **`cgpt_ex_config.h`**, change the default line to:

```cpp
#define cgpt_ex_enableJoysticks 0
```

Set it to `1` when joysticks are connected. Alternatively, pass
`-Dcgpt_ex_enableJoysticks=0` to the entire build. A define placed only in the
`.ino` will not affect the separately compiled `.cpp` files; edit the common
configuration header or use a build-wide compiler definition.

With `0`, analog pin initialization, analog polling, and incremental jogging
are compiled out. Floating ADC inputs cannot move the servos. The second chip,
its startup information, and all four chip-side physical buttons remain active;
touchscreen controls, pose recording, and playback remain available. System
voltage/diagnostic reads still run once during initialization. Joysticks are
enabled by default to preserve the original example's behavior.

## Hardware defaults

The joint settings now match the configuration supplied for your hardware:

| Joint | Reverse servo | Reverse joystick | Maximum jog counts/second |
|---|---|---|---|
| Base | true | true | 40000 |
| Shoulder | false | true | 40000 |
| Elbow | false | true | 40000 |
| Wrist | false | true | 40000 |
| Wrist roll | false | true | 40000 |
| Gripper | false | true | 40000 |

All other shared configuration values match the supplied header. The newer
SD/CSV configuration block is retained so the current package remains complete.

## Calibration

Set `cgpt_ex_jointSettings` in `cgpt_ex_config.h` for your mechanism before
driving the arm. Each entry contains:

| Field | Meaning |
|---|---|
| `cgpt_ex_minPulseUs`, `cgpt_ex_maxPulseUs` | Servo pulse endpoints; example defaults 1000/2000 us. |
| `cgpt_ex_reverseServo` | Swap which pulse endpoint corresponds to logical 0. |
| `cgpt_ex_minPosition`, `cgpt_ex_maxPosition` | Joint command limits within 0..65535. |
| `cgpt_ex_initialPosition` | Startup command, default 32768. |
| `cgpt_ex_joystickMin`, `cgpt_ex_joystickCenter`, `cgpt_ex_joystickMax` | Measured normalized ADC counts, defaults 0/32768/65535. |
| `cgpt_ex_deadband` | Center tolerance in ADC counts, default 2500. |
| `cgpt_ex_reverseJoystick` | Reverse the joystick's add/subtract direction. |
| `cgpt_ex_maxCountsPerSecond` | Jog speed at full deflection. |

Use normalized ADC counts, not millivolts. The adapter configures each Analog
Input pin's public data as a 16-sample averaged reading. Measure the joystick
center and endpoints using `SerialWombatAnalogInput::readAveragedCounts()` or
the chip's public data. Center calibration is explicit so a deflected joystick
at startup is not mistaken for the center.

The command 0 corresponds to the configured minimum pulse and 65535 to the
maximum, unless servo reversal is enabled. The numbers represent commanded
positions; no actual joint position feedback is assumed. A slider's range
uses that joint's configured position limits.

Initialization commands each servo to its configured startup position. A
small subclass sets the initial value before `attach()` so the library does
not first initialize that servo to logical zero. The adapter configures its
selected pins without resetting either chip. It uses checked versions of the
library's public-data and debounce-transition packets.

## Legacy RAM screen and joystick behavior

Store/Undo/Clear controls below apply when `cgpt_ex_enableSd=0`. With SD on,
they become Capture/New/Files. The RAM APIs remain available, and
`cgpt_ex_startPlayback()` explicitly starts the RAM list.

- Six sliders display the current 16-bit command values.
- Joystick center holds position; increasing deflection increases jog speed.
  One side subtracts counts, the other adds. All joints can jog together.
- Commands saturate at configured joint limits rather than wrapping.
- Touching/dragging a slider takes over from playback. A currently dragged
  joint is not changed by its joystick until the slider is released.
- **Store** appends all six current command values to the pose list during
  normal recording. In editing mode it inserts before the selected point.
- The physical Store button calls the exact same `cgpt_ex_storePose()` function.
  Transition counts retain press/release events between polls; holding the
  button does not repeatedly record poses. A button held at initialization is
  ignored until released and pressed again. If several presses occur between
  polls, each records the current sampled pose, not a historical pose.
- **Play** starts at pose zero, interpolating from the current command values.
- **Stop** ends playback or editing. Servos continue receiving pulses; with centered
  joysticks they hold the last command. Manual jogging resumes after Stop.
- **Loop on/off** controls whether playback returns to pose zero after the
  final dwell. **Undo** removes the last pose; **Clear** empties the list.
- Joysticks and physical Store presses are ignored/consumed during playback.
  Store, Undo, and Clear cannot modify the list while playback runs. Physical
  Previous/Next/Delete stop playback and enter editing; see PHYSICAL_EDITING_GUIDE.md.

## Legacy RAM recording and playback

The list holds 100 poses in RAM. Each pose stores six `uint16_t` commands,
travel time to that pose, and dwell time after arrival. On a typical 32-bit
Arduino this is 20 bytes per pose, or 2000 bytes for the list. Recordings clear
on reset/power loss; there is no flash/EEPROM save in this example. Capacity is
configurable at compile time.

By default, travel to each pose takes 1500 ms, followed by 500 ms dwell. Set
timing before storing to assign a different timing to subsequent poses:

```cpp
cgpt_ex_setRecordTiming(2000, 750);
cgpt_ex_storePose();
cgpt_ex_setLoop(true);
cgpt_ex_startPlayback();
```

Travel time is clamped to 20..600000 ms; dwell to 0..600000 ms. Previously
stored poses retain their timing. Playback uses signed, 64-bit intermediate
arithmetic to interpolate all six commanded positions on a shared time base.
The moves are linear in joint command space; they are not Cartesian paths or
inverse kinematics. Serial Wombat transactions are synchronous and the six
commands are sent sequentially, not as an atomic hardware update.

Playback scheduling uses elapsed `millis()` and remains valid across its
32-bit rollover. A delayed loop reaches the segment endpoint and starts its
dwell rather than skipping poses. Joystick jogging caps elapsed time at 50 ms
per poll to avoid a large catch-up step after a stalled loop. Fractional jog
increments are accumulated, so low speeds still produce movement.

The public `cgpt_ex_getPoseCount()` and read-only `cgpt_ex_getPose(index)` can
be used to export the list to storage in your larger application.

## Hardware adaptation and verification

If your application already owns/configures both chip objects, replace the
adapter functions `cgpt_ex_hardwareBegin`, `cgpt_ex_hardwareRead`, and
`cgpt_ex_hardwareWrite` with versions using those objects, and implement
`cgpt_ex_getChipInfo` to supply the startup snapshots. Keep the same interfaces
and ensure enabled input public data is normalized 16-bit ADC counts.

Communication errors latch an I/O fault, stop playback, and inhibit further
commands. After repairing the connection, call `cgpt_ex_begin()` to
reinitialize pins and startup commands; the RAM pose list is preserved.

Verified with native C++11 builds using actual LVGL **8.3.11** and **9.2.2**
sources, plus Arduino platform stubs and a simulated hardware adapter. Tests
covered positive/negative interpolation, dwell, multiple-pose looping,
Stop/manual takeover, joystick deadband and speed, command saturation, touch
and physical recording, list capacity, I/O fault latching, and millis rollover.
The Serial Wombat adapter was compiled against the current library headers
with platform stubs. Packet-level simulations also verified servo startup
positions, write-packet bytes, short/held/multiple button presses, and failed
transactions. Updated tests also covered startup scrolling/OK, discarded
startup Store events, diagnostic snapshots, boot/missing-chip reports, and
joysticks both enabled and disabled. All four LVGL-version/joystick-option
combinations passed. Packet tests confirmed zero joystick ADC initialization
and polling when disabled, while chip-side Store continues to work. Both
LVGL versions rendered the screen successfully;
`cgpt_ex_preview.png` shows the LVGL 9 render. Physical board, display,
touchscreen, and servo behavior still require testing in your existing sketch.

See EDITING_VALIDATION.md for the added CSV/RAM editor and button checks.

API references used:

- https://github.com/BroadwellConsultingInc/SerialWombatArdLib
- https://github.com/BroadwellConsultingInc/SerialWombatArdLib/blob/master/examples/_SerialWombatChipFinder/_SerialWombatChipFinder.ino
- https://broadwellconsultinginc.github.io/SerialWombatArdLib/class_serial_wombat_servo.html
- https://broadwellconsultinginc.github.io/SerialWombatArdLib/class_serial_wombat_debounced_input.html
- https://github.com/lvgl/lvgl/tree/v8.3.11
- https://github.com/lvgl/lvgl/tree/v9.2.2

## Playback progress

The bottom CSV playback status includes the current point and total validated
points, for example `Play ARM00001.csv 3/12`. Numbering begins at 1 and resets
to 1 when Loop restarts. The validation phase still displays `Check filename`.

## Prompt history

PROMPTS.md records the user prompts that produced this package, in chronological
order. Future updates should retain it and append the new project requests.
