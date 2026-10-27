# CYD printer monitor — LVGL 9.6

Monitor any mix of up to three Elegoo Neptune 4 and Centauri Carbon 2 printers.
Cards show state, actual/target nozzle and bed temperatures in Celsius, and
remaining print time. Tap a card to configure it. The front-page diagnostic
counter has been removed.

## Installation

Extract the complete folder and open `cgpt_ex_printer_gui.ino`. Replace the
previous sketch folder rather than mixing versions of its source files.
Keep the TFT_eSPI setup and `lv_conf.h` that already work with your CYD.

**Board: Tools -> Board -> esp32 -> ESP32 Dev Module.** Do not select
ESP32-WROOM-DA Module: its WiFi antenna switching uses GPIO2 and GPIO25,
conflicting with the CYD display DC and touch clock. The sketch retains the
compile-time guard against that board selection.

Required libraries:

- LVGL **9.6**.
- TFT_eSPI (your working 2.5.43 setup).
- SerialWombat.
- ArduinoJson **6.21.2 or 7.x**; both APIs are supported.
- **PubSubClient by Nick O'Leary, version 2.8 or later** — new for CC2 MQTT.

WiFi, HTTPClient, Preferences and FreeRTOS come with the ESP32 core. Enable
`LV_USE_TFT_ESPI`, `LV_USE_BUTTON`, `LV_USE_DROPDOWN`, `LV_USE_TEXTAREA` and
`LV_USE_KEYBOARD` in `lv_conf.h`. The supplied layout uses Montserrat 14.
If needed on this 4 MB CYD, select **Huge APP (3MB No OTA/1MB SPIFFS)**.
This update has no WebSocket dependency.

Display/touch initialization is unchanged: native 240x320 TFT rotated 270
for a 320x240 GUI; bundled software-SPI touch on MOSI32, MISO39, CLK25, CS33,
without an IRQ requirement. Serial Wombat connections remain on SDA22/SCL27,
addresses **0x63** and **0x64**. The footer shows chip connection status. The two Serial Wombat 6C chips
are assumed to have firmware supporting the TM1637 pin mode.

## WiFi

Tap **WiFi**, choose a scanned SSID or enter it, enter the password, dismiss
the keyboard with its checkmark, then tap **Connect**. Successful credentials
are saved locally and used automatically on power-up. Failed attempts retain
the last working network; **Forget** removes it. Use a reachable 2.4 GHz personal
or open network. Enterprise/captive-portal login is not implemented.

## Printer setup

Tap a printer card. The dropdown beside the name selects **Neptune 4** or
**Carbon 2**. Selecting a different model fills its default port. Tap any
field to open its keyboard; the caption identifies the field. Dismiss the
keyboard before pressing Save.

### Neptune 4

1. Select **Neptune 4** and enter the name and IPv4 address.
2. Leave the port at **80**, the new default for Elegoo. You can still enter
   7125 or another port if that is what your firmware uses.
3. Leave the API key blank if the printer's Moonraker LAN policy authorizes
   the CYD. If it reports **Auth required**, supply the Moonraker API key.
4. Tap **Save**, then **Back**.

Existing saved printer names, addresses, API keys and explicitly configured
ports are preserved when upgrading from the previous Neptune-only sketch.
Previously empty slots default to port 80. The expanded storage format is
written on the next Save. WiFi settings use a separate namespace.

### Centauri Carbon 2 / Carbon 2 Combo

1. Enable local/LAN access on the printer as required by its firmware. Put
   the CYD on a network that can reach it.
2. Select **Carbon 2**. Enter the printer's name and IPv4 address.
3. Leave the port at **1883**. This is the CC2 MQTT service, separate from
   the Neptune HTTP port.
4. Enter the **printer serial number (SN)** in the next field. Copy its exact
   case; this is not the IP address, display name or CC1 mainboard ID.
5. Enter the printer's **LAN access code** in the last, masked field. This
   is the printer access code, separate from your WiFi password.
6. Tap **Save**, then **Back**. The card progresses through Connecting /
   Registering to the printer state when telemetry arrives.

IP, model, port, serial number and access code survive power cycling.
Serial numbers accept letters, digits, hyphens and underscores; access codes
accept up to 64 characters. Secrets are masked in the GUI and are not logged.
Settings use ordinary ESP32 Preferences storage, without application-level
encryption. Serial-number auto-discovery is not implemented.

**Clear**, followed by **Save**, disables a slot and clears its credentials.
**Back** closes the page without saving edits. A blank IP disables all network
requests for that slot. Changing a slot's configuration discards old replies
and releases its previous MQTT connection.

## Monitoring and troubleshooting

Requests run in a background task; that task never calls LVGL. Each configured
printer is normally queried about every three seconds. Unreachable printers
can delay subsequent polls. Reopen a card to see the latest detailed error;
long messages scroll horizontally.

- **Reconnecting**: a connection/request failed, but the last valid sample is still within the 20-second grace period.
- **No WiFi**: CYD disconnected and no unexpired sample is available.
- **Offline / MQTT offline**: check IP, port, LAN reachability and printer LAN mode.
- **Auth required**: check the Neptune API key or CC2 LAN access code.
- **Register timeout**: check the CC2 serial number and connected-client limit.
- **Register failed**: CC2 rejected registration; see the detailed message.
- **Status timeout**: CC2 has not returned a full status reply within 20 seconds of its request.
- **No telemetry**: a reply lacks valid heater readings or reports an API error.
- **Stale**: no successful sample for 20 seconds. Screen readings show `--`.

The printer no-data timeout is **20 seconds from the last valid telemetry**,
independently for each slot. A failed HTTP/MQTT request or a WiFi dropout keeps
the last actual/target temperatures and the last confirmed print state. During
this grace period the screen and analog gauges retain those temperatures, and
a previously running print keeps counting down from its original sample time.
A previously paused printer keeps PAUSE and its remaining time stays fixed.

At **20 seconds without valid data**, the TM1637 changes to one dash in the
minutes ones position (other digits/points off), the gauges request zero duty,
and the GUI hides the expired readings. These changes happen on the next
normal display/gauge service tick (up to about 500 ms later). Failed retries,
MQTT reconnects, registration acknowledgements and heartbeat replies do not
restart this timer. Only a new valid temperature/status sample renews it.

The shared constant is `cgpt_ex_printerTimeoutMs` in `cgpt_ex_neptune.h`.
This is the no-data grace period; individual HTTP connection/read timeouts
remain short (900/1200 ms) so one unreachable printer does not monopolize the
worker. CC2 registration and full-status response waits are now 20 seconds.
A slot with no prior valid sample shows unknown/minimum immediately. Saving
a new printer configuration clears its retained sample; disabling it still
shows STOP. Existing saved WiFi and printer settings are retained.

CC2 reconnects after a ten-second delay following a failed attempt. It uses
MQTT username `elegoo`, the supplied access code, per-client registration and
method **1002** full-status queries. It subscribes only to its own registration
and response topics. Request IDs are checked; partial status broadcasts are
ignored. An Elegoo application PING is sent every ten seconds in addition to
MQTT transport keepalive. Each configured CC2 has a separate connection and
12 KB receive buffer, allocated only when needed. Replies exceeding that
limit time out; firmware with a larger status packet may need the
`cgpt_ex_cc2PacketBytes` constant increased if heap permits.

Neptune time is estimated from elapsed print duration and progress, marked
**Left~HH:MM**. It appears after at least 30 seconds and 1% progress. CC2 time
comes directly from `print_status.remaining_time_sec`, shown as **Left HH:MM**.
Both count down between polls while printing and hold while paused. Times
round up to whole minutes. Unknown times show `--:--`; over 99:59 shows `>99h`.
These values can differ from the final completion time as print conditions change.

## Six-digit TM1637 displays

Three TM1637 displays now mirror the remaining time for printer cards 1–3.
Default wiring uses Serial Wombat pin numbers, not ESP32 GPIO numbers:

| Printer card | Serial Wombat I2C address | TM1637 CLK pin | TM1637 DIO pin |
| --- | --- | --- | --- |
| 1 | 0x63 | 0 | 1 |
| 2 | 0x63 | 2 | 3 |
| 3 | 0x64 | 0 | 1 |

Connect each module's power and ground as appropriate for your existing
hardware. All three displays are initialized for **six digits**, character-array
mode and **minimum brightness, 0** (range 0–7). Each receives
`writeDigitOrder(2, 1, 0, 5, 4, 3)`; the Arduino library's method is named
`writeDigitOrder`, rather than `writeDigitalOrder`.

Edit the three rows of **`cgpt_ex_tmWiring` in `cgpt_ex_tm1637.cpp`** to change
chip/CLK/DIO assignments. The display objects automatically bind to the chip
selected in that table. Keep all six CLK/DIO pins distinct within each chip.
These pin modes are initialized at CYD startup without resetting either chip.
The remaining pins used below now drive the six temperature gauges.

| Printer status | Six-digit display |
| --- | --- |
| Printing / active job with known remaining time | `HH.MM.SS`, e.g. `01.23.45` = 1 h 23 m 45 s |
| Paused | `PAUSE ` |
| Idle / ready / complete / cancelled / stopped / error | ` STOP ` |
| Unconfigured / disabled slot | ` STOP ` |
| No prior valid data, or last valid data at least 20 seconds old | Single dash in the minutes ones position (fourth digit), other digits blank |
| Running, but remaining time unknown or above 99:59:59 | `------` |

Spaces pad the messages to exactly six characters so previous digits are
cleared. Letters use the firmware's seven-segment character font. Decimal
points after digits 2 and 4 separate hours/minutes/seconds (`writeDecimalBitmap(0x0A)`).
Points turn off for STOP, PAUSE and dashes. Blinking remains disabled.
A job reaching zero shows `000000` until the printer reports completion,
when it changes to STOP. A fresh paused state takes priority over its time.

The main loop checks the displays twice per second and sends data only when
characters change. Running time counts down between telemetry polls with the
same elapsed-time calculation used by the GUI. Neptune time remains an
estimate; CC2 uses its reported remaining seconds. A paused display keeps
showing PAUSE until the next state update. I2C writes run only from the Arduino
loop task, separate from the printer network worker.

Initialization/write failures are logged at **115200 baud**. A missing chip or
failed display initialization does not stop the other displays, GUI or WiFi.
Failed character writes retry on the next service call, with repeated identical
error logs suppressed. Initialization is attempted at startup; after fixing a
missing chip or unsupported firmware, restart the CYD to initialize it again.

## Six PWM temperature gauges

Six non-inverted PWM outputs now drive one bed gauge and one nozzle gauge per
printer. These use actual temperatures, not heater targets. Paused, idle and
completed printers continue to show their fresh temperatures while cooling.
The existing display wiring remains unchanged.

| Printer card | Serial Wombat address | Bed PWM pin | Nozzle PWM pin |
| --- | --- | --- | --- |
| 1 | 0x63 | 4 | 5 |
| 2 | 0x64 | 2 | 3 |
| 3 | 0x64 | 4 | 5 |

Pin numbers are Serial Wombat logical pins. Connect each PWM output to the
corresponding gauge's voltage input through its PWM-averaging/input circuit;
connect gauge ground to the shared CYD/Serial Wombat ground. PWM is a sequence
of pulses, not a true DAC voltage. Use a gauge/input that already averages PWM
or an external low-pass filter (and a buffer if the gauge loads it appreciably).
The compensation assumes a high-impedance averaged input, a PWM high level near
chip VDD, and no additional gain or voltage drop in the signal path.

Your calibration is implemented exactly as these two temperature-to-voltage
curves, with linear interpolation inside each segment:

| Bed temperature | Nozzle temperature | Desired output |
| --- | --- | --- |
| 20 °C | 20 °C | 0.000 V |
| 40 °C | 180 °C | 0.724 V |
| 50 °C | 200 °C | 1.360 V |
| 60 °C | 220 °C | 2.030 V |
| 80 °C | 240 °C | 2.640 V |
| 125 °C | 280 °C | 3.280 V |

For example, a bed reading of 55 °C or a nozzle reading of 210 °C requests
1.695 V, midway between 1.360 V and 2.030 V. Temperatures below 20 °C clamp to
0 V. Temperatures above 125 °C bed / 280 °C nozzle clamp to 3.280 V rather than
extrapolating past the calibrated scale.

Each chip is measured separately using its public
`SerialWombatChip::readSupplyVoltage_mV()` function. Supply readings refresh
once per second. Duty is recomputed from that chip's latest measurement:

```text
PWM duty (0..65535) = round(desired millivolts / measured VDD millivolts * 65535)
```

For the 1.695 V example, a 3.300 V source needs about 51.36% duty; a 3.600 V
source needs about 47.08%. Values saturate at 100% duty if the requested voltage
exceeds the measured source: PWM cannot generate a voltage above its high level.
Actual accuracy also depends on the chip's VDD measurement, PWM resolution,
filter and gauge loading.

All six outputs initialize at zero duty. A connection loss keeps each gauge
at its last valid temperature for the remainder of the 20-second telemetry
window, with VDD compensation still active. After 20 seconds without data,
with no previous sample, for a nonfinite temperature, or for a disabled slot,
zero duty parks the needle at the calibrated 20 °C position. This is a
missing-data behavior, not a measurement of room temperature. The screen and
TM1637 status distinguish connection loss from a valid temperature reading.

A failed supply-voltage read (including an increment in the chip's public
`errorCount`) or an implausible result outside 2000–5500 mV requests zero duty
on that chip until a valid reading returns. These are sanity bounds for the
measurement, not instructions to change the board supply. State changes are
logged at 115200 baud. A missing chip or failed channel initialization leaves
other channels operating. Restart the CYD after correcting an initialization
failure.

Gauge values refresh every 500 ms in the Arduino loop task, on the same I2C
bus as the TM1637 displays. Unchanged duties are also resent so a lost PWM
write is retried at the next update. Firmware uses its normal basic PWM
frequency; no chip-specific frequency commands are sent. The network worker
never operates I2C. Edit `cgpt_ex_gaugeWiring`, the calibration arrays, and the
update constants in `cgpt_ex_gauges.cpp` to adapt the example.

Heater setters and print controls are not included in this update. Network
requests still only register, keep the connection alive and read printer status.

## Source and validation

- `cgpt_ex_gui.*`: printer cards and public display update API.
- `cgpt_ex_neptune.*`: shared configuration/persistence, old-format migration,
  worker scheduling and Neptune HTTP/parser code. The existing API names are
  retained for compatibility and now manage both printer models.
- `cgpt_ex_cc2.*`: CC2 MQTT connection, registration, polling and parser.
- `cgpt_ex_neptune_gui.*`: model selector, saved printer setup and card refresh.
- `cgpt_ex_wifi.*`, `cgpt_ex_wifi_gui.*`: saved network connection and GUI.
- `cgpt_ex_wombat.*`: I2C chip checks.
- `cgpt_ex_gauges.*`: six PWM outputs, calibration interpolation and per-chip
  supply-voltage compensation.
- `cgpt_ex_tm1637.*`: display wiring, initialization, HHMMSS/STOP/PAUSE
  formatting and display service.
- `cgpt_ex_cyd_touch.*`, `cgpt_ex_config.h`, SoftSPI/XPT2046 files: working touch
  input with retained third-party license notices. `cgpt_ex_touchDebug` controls
  touch logs; Serial Monitor uses 115200 baud.

All example-owned identifiers use `cgpt_ex_`, except Arduino's required
`setup()` and `loop()`. Third-party identifiers keep their required names.

Desktop checks use actual LVGL 9.6 and ArduinoJson 6.21.2 / 7.4.2, with mocked
HTTP, MQTT transport, GPIO and Preferences. They exercise touch/card setup,
keyboards, WiFi persistence/reconnect, Neptune parsing, the exact old NVS
layout, port defaults/custom ports, CC2 saved credentials, mixed-model slots,
registration/topics/response IDs, temperature/time parsing, paused/completed
states, heartbeat, timeouts/retry/auth failures, WiFi loss and three CC2 sessions.
The CC2 source also compiles independently against the actual PubSubClient 2.8
header. Setup, keyboard and mixed-printer screens were rendered and inspected.

TM1637 checks use the actual SerialWombatTM1637 host header with a captured
packet transport. They verify both initialization/order packets, all three
I2C/pin assignments, six-character writes, countdown and STOP/PAUSE transitions,
unknown/stale/overflow states, millis rollover, unchanged-write suppression,
partial-write retries and missing-chip/per-display initialization failures.
The WiFi, GUI, printer and settings regression checks pass with both ArduinoJson
versions. Physical Serial Wombat/display testing remains to be done on your hardware.

Gauge checks use the actual SerialWombatPWM host header with a mocked chip
transport. They cover all 12 calibration points, 10 interpolated points,
per-chip VDD changes/refresh timing, duty/end-point limits, invalid VDD reads,
paused/cooling temperatures, stale/disabled/disconnected printers, lost-write
retries and independent channel/chip initialization failures. The complete
regression suite passes with ArduinoJson 6.21.2 and 7.4.2. The new gauge module
also compiles independently with warnings treated as errors.

Dropout tests drive failed HTTP polls through the actual worker and verify
retained GUI temperatures, continuing TM1637 time and gauge voltage, independent
of WiFi availability. They check repeated failures without timestamp resets,
the exact 20-second boundary, paused-state hold, recovery, configuration changes,
uptime rollover and a valid sample at timestamp zero. CC2 failure/reconnect
tests also verify that the shared last-good sample and timestamp survive.

The existing Neptune version was confirmed working by you. This update has
not been compiled with your installed ESP32 toolchain or tested on a physical
CC2 here; target flash/heap usage and firmware compatibility need on-device
verification.

Protocol references:
- https://github.com/elegooofficial/elegoo-link/blob/main/src/lan/adapters/elegoo_fdm_cc2/elegoo_fdm_cc2_protocol.cpp
- https://github.com/elegooofficial/elegoo-link/blob/main/src/lan/adapters/elegoo_fdm_cc2/elegoo_fdm_cc2_message_adapter.cpp
- https://moonraker.readthedocs.io/en/stable/external_api/printer/

TM1637 host API reference:
- https://broadwellconsultinginc.github.io/SerialWombatArdLib/class_serial_wombat_t_m1637.html

Gauge host API references:
- https://broadwellconsultinginc.github.io/SerialWombatArdLib/class_serial_wombat_chip.html
- https://broadwellconsultinginc.github.io/SerialWombatArdLib/class_serial_wombat_p_w_m.html
