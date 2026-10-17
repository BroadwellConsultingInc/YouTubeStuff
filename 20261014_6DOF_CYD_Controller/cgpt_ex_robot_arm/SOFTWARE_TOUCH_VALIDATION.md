# Software touch change validation

Checked 2026-10-14 using native gcc/g++ builds, real LVGL 8.3.11 and 9.2.2,
Arduino platform stubs, simulated XPT2046 ADC/GPIO, and a filesystem-backed SD
stub. No physical CYD/arm test or full ESP32 firmware build was performed.

- Sixteen software touch scenarios passed: two LVGL versions, four display
  rotations, and pointer creation versus reuse. No duplicate pointer devices.
- Missing-card initialization retained the chip startup screen and software
  touch. SD remount and CSV capture writes between touch reads also passed.
- Press/release, unchanged release coordinates, out-of-range edge clamping,
  correct LVGL coordinate rotation, repeat initialization, and inactive old
  read callback all passed. Touch GPIO writes were limited to 25, 32, and 33.
- All four SD UI regression scenarios (LVGL 8/9, joystick on/off) passed:
  New/Capture/physical Store/Stop/Files/CSV playback.
- Storage regression tests passed: manual-only capture, numbering, PC CSV,
  playback interpolation/validation, root paging, long files, rollover, and
  mount/read/write failures.
- C++11 syntax checks passed for the new touch driver against both LVGL
  versions and for SD/touch-disabled builds with joysticks enabled/disabled.

Check your panel's calibration/orientation on hardware after integrating.
Keep the TFT on HSPI and remove all old hardware SPI touch polling.
