# Physical editor validation

Checked 2026-10-14. These are native simulations and syntax checks, not a full
ESP32 firmware build or a physical CYD/robot-arm test.

- Eight editor configurations passed with real LVGL 8.3.11 / 9.2.2, SD on/off,
  and joysticks on/off. Tests cover Previous/Next bounds, current-point insertion,
  deletion and predecessor selection, first/last/empty points, unchanged point
  timing, direct pose changes with no timed advancement, manual adjustment,
  playback interruption/restart, Stop/reentry cursor retention, and RAM capacity.
- CSV editing passed with 250 points. File selection while editing stays in
  editing mode and loads its first point directly. No editor screen buttons
  were added; the existing main-screen child count is unchanged.
- Actual Serial Wombat Arduino library headers plus packet simulations verified
  all four Debounced Input pins/addresses, independent button transition counts,
  between-poll pulses, held buttons without repeat, release handling, startup-held
  suppression, and input I/O errors. Both joystick options passed; disabling
  joysticks still excludes ADC setup/polling and keeps every button working.
- Streaming storage tests passed for BOM/CRLF/quoted numeric CSV, existing
  timing preservation, header-only lists, invalid rows, invalid indices and
  durations, root-path restrictions, read/write failures, temporary-file
  collisions, failed backup rename, failed replacement with rollback, failed
  rollback retaining original backup, and retained backup after cleanup failure.
- Additional LVGL 8/9 checks passed for an initially empty editor, first Store
  creating the first CSV, and visible status when a saved edit leaves a backup.
- Existing SD UI regressions passed across both LVGL versions and joystick
  options: New/Capture/physical Store/Stop/Files/CSV playback. Existing storage
  tests passed for numbering, manual-only capture, playback, paging, long files,
  millis rollover, and I/O failures.
- The working software touch regression passed against both LVGL versions,
  all four display rotations, existing/new pointer devices, missing-card
  initialization, and SD remount/capture between touch reads.
- C++11 syntax checks passed against both LVGL versions, SD enabled/disabled,
  using actual ESP32 SD/FS headers with platform/SPI stubs. The Serial Wombat
  adapter compiled against the actual Arduino host library headers.

See PHYSICAL_EDITING_GUIDE.md for wiring, button order, CSV persistence,
recovery files, and the boundary behavior tested here.

## Playback step status update

Native checks against LVGL 8.3.11 and 9.2.2 verified the bottom text changes
from `Check ARM00001.csv` to `Play ARM00001.csv 1/2`, advances to `2/2`, and
returns to `1/2` when Loop restarts. SD-disabled C++11 syntax checks passed
for both versions. Existing CSV/storage checks also passed, including a
250-point file reporting its full total and final index. No hardware test
was performed for this display-only change.
