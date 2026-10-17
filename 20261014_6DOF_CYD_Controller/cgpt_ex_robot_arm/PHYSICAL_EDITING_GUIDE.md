# Physical point editing

Copy the package's fourteen .h/.cpp files over the matching files in your
existing sketch. Your working CYD software touch driver, servo directions,
joystick directions/speeds, and SD configuration are retained. No additional
initialization calls or screen buttons are needed.

## Wiring

Connect one normally-open momentary button from each listed pin to common GND.
All four inputs use chip-side Debounced Input, 30 ms debounce, inversion, and
weak pull-ups. No ESP32 GPIO is used for these buttons.

| Button | Serial Wombat address | Pin |
|---|---|---:|
| Store (existing) | 0x64, input chip | 7 |
| Previous | 0x64, input chip | 6 |
| Next | 0x63, servo chip | 6 |
| Delete | 0x63, servo chip | 7 |

Both chips continue using pins 0..5 for the original six servos/analog axes.
The four buttons work when `cgpt_ex_enableJoysticks=0`. To change assignments,
edit `cgpt_ex_buttons` in `cgpt_ex_config.h`; its chip field is 0 for the input
chip and 1 for the servo chip. Keep Store at array index 0, Previous at 1,
Next at 2, Delete at 3. Initialization checks for pin conflicts.

## Operation

Pressing Previous, Next, or Delete enters editing mode and performs that action.
It stops any timed playback and closes the capture file without adding a point.
The existing status line displays `Edit 2/5 | CSV` (or `RAM`); indices on screen
start at 1.

- **Previous:** select and command the preceding saved point.
- **Next:** select and command the following saved point.
- **Delete:** remove the selected point, then select and command its predecessor.
- **Store / touchscreen Capture:** insert the current six commanded positions
  immediately **before** the selected point. The insertion becomes the selected
  point. It does not overwrite or remove the original selected point.

For example, with `[A, B, C]` and B selected, jog the arm to X and press Store:
`[A, X, B, C]` results, with X selected. Next selects B. Delete while X is
selected produces `[A, B, C]` and moves to A. Repeated Stores insert before the
currently selected insertion; use Next to advance the insertion location.

Navigation commands all six target positions directly, with no software
interpolation, dwell, or automatic advance. Physical servos still take time to
reach their positions. Joysticks and sliders remain available for adjusting
the arm before inserting a point. Jogging alone never changes a saved point.

After normal recording, the current point is the most recently captured point.
During playback, the current point is the playback target. A selected point
is remembered when leaving and reentering editing mode.

**Play** exits editing and starts normal timed playback from the first point.
**Stop** exits editing and holds the current command (center joysticks to hold).
**New** exits editing and opens a fresh sequential capture file, without motion
or an automatic point capture. Outside editing mode, Capture/Store retains its
original append-to-capture behavior.

While already editing, **Files** lets you choose another root CSV for editing.
Selecting it stays in editing mode and directly selects its first point; it
never starts timed playback. Outside editing mode, Files keeps its original
select-and-play behavior. An empty selected file holds the arm position.

## Boundaries and input handling

Previous at the first point and Next at the last point stay at that point;
there is no wrap. Deleting the first point selects the new first point, since
there is no predecessor. Deleting the only point leaves an empty list and
holds the last commanded arm position. Store in an empty list creates point 1.
For an empty editor without a file yet, its first Store creates a sequential
CSV file. A missing SD reports an error; normal manual arm control continues.

Held buttons do not repeat. Short complete presses between polls are retained
by chip-side transition counts. Startup and file-browser button events are
discarded when returning to the controls. If multiple editing button presses
arrive in one poll, they are processed Previous, Next, Delete; editing buttons
take precedence over joystick jogging and Store for that poll. Separate polls
preserve normal button press order.

## CSV persistence and PC editing

CSV insertions and deletions are saved immediately to the same root file.
Existing points retain their numeric positions and `move_ms`; a newly inserted
point uses the current recording duration (default 1500 ms). You can continue
editing those durations on a PC. Saving normalizes CSV formatting, including
quotes, blank lines, BOM, and line endings; numeric point data is preserved.

The SD editor streams the file and has no 100-point RAM limit. It validates
the full source before navigation or changing it. A rewrite writes and checks
`yourfile.csv.cgpt.tmp`, renames the original to `yourfile.csv.cgpt.bak`, then
installs the replacement and removes the backup. Normal read/write failures
leave the original in place, and a failed replacement attempts to restore it.

If interrupted between renames, inspect the card on a PC: `.cgpt.bak` contains
the original file, and `.cgpt.tmp` contains the prospective replacement. The
editor refuses to overwrite preexisting recovery files. Restore or retain the
wanted file as a root `.csv`, then remove recovery files once satisfied. File
selection only lists `.csv`, so these recovery files are hidden from it.
The FAT rename sequence is not a power-loss-atomic filesystem transaction.

SD operations are synchronous, so navigation scans and file rewrites can pause
the UI briefly on long files. Stop SD activity and power off before exchanging
the card with a PC.

With `cgpt_ex_enableSd=0`, the same controls edit the RAM list. The existing
100-pose limit applies, and the list clears on power loss.

## Optional APIs

The physical inputs call these functions from the normal service loop:

```cpp
cgpt_ex_previousPoint();
cgpt_ex_nextPoint();
cgpt_ex_deletePoint();
// While editing, existing Capture/Store APIs insert instead of appending.
cgpt_ex_capturePoint();
cgpt_ex_storePose();
```

`cgpt_ex_isEditing()`, `cgpt_ex_getEditIndex()` (zero-based), and
`cgpt_ex_getEditCount()` expose the editor state. Call all APIs in the LVGL
thread. The adapter adds `cgpt_ex_hardwareReadEditButtons(previous, next, delete)`;
custom hardware adapters should implement it and return debounced press counts.
