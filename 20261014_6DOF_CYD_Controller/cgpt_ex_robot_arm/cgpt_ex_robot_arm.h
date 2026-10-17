#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include "cgpt_ex_config.h"

#if LVGL_VERSION_MAJOR != 8 && LVGL_VERSION_MAJOR != 9
#error "This example requires LVGL 8 or 9"
#endif

struct cgpt_ex_pose {
    uint16_t cgpt_ex_position[cgpt_ex_jointCount];
    uint32_t cgpt_ex_moveMs; // Travel TO this pose, including pose zero.
    uint32_t cgpt_ex_holdMs; // Dwell after reaching this pose.
};

// Startup snapshot. Values are read from the chip; firmware is not hard-coded.
struct cgpt_ex_chipInfo {
    uint8_t cgpt_ex_address;
    bool cgpt_ex_connected;
    bool cgpt_ex_initialized;
    bool cgpt_ex_inBoot;
    char cgpt_ex_model[5]; // Full four-character product/model, e.g. S08C.
    char cgpt_ex_firmware[4];
    uint16_t cgpt_ex_supplyMv;
    bool cgpt_ex_systemInfoValid;
    uint32_t cgpt_ex_frames;
    uint16_t cgpt_ex_overflowFrames;
    char cgpt_ex_uniqueId[48]; // Groups of four bytes, one group per line.
    bool cgpt_ex_deviceInfoValid;
    uint16_t cgpt_ex_deviceId;
    uint16_t cgpt_ex_deviceRevision;
    int16_t cgpt_ex_errorCode;
};

// Call after Wire.begin(), lv_init(), and your display/touch registration.
// Initializes BOTH chips' example pins and shows a startup information screen.
// Touch OK to enter the 320x240 control screen.
// Call again to recover a latched I/O error; preserves the pose list.
bool cgpt_ex_begin();
bool cgpt_ex_isStartupVisible();
void cgpt_ex_dismissStartup(); // Same action as the startup OK button.

// Call frequently in the same context/thread as lv_timer_handler().
// Polls inputs, advances playback, and sends changed outputs at 50 Hz.
void cgpt_ex_service();

// Public input poll. Reads enabled analog channels and all four buttons, updates
// positions, and handles capture/edit commands. cgpt_ex_service calls this.
// If used directly, call at ~20 ms intervals; do not also poll it independently.
bool cgpt_ex_readJoysticks();

// Editing commands are driven by physical buttons only; no extra screen buttons.
// Store inserts BEFORE the selected point; the inserted point becomes current.
// Empty lists accept Store at index zero. Delete first selects the new first;
// deleting the last remaining point holds the current arm pose.
bool cgpt_ex_previousPoint();
bool cgpt_ex_nextPoint();
bool cgpt_ex_deletePoint();
bool cgpt_ex_isEditing();
uint32_t cgpt_ex_getEditIndex(); // Zero-based; meaningful while editing with count > 0.
uint32_t cgpt_ex_getEditCount();

bool cgpt_ex_storePose();
bool cgpt_ex_startPlayback();
void cgpt_ex_stopPlayback(); // Holds last commanded pose; keeps servo pulses on.
bool cgpt_ex_undoPose();
bool cgpt_ex_clearPoses();
void cgpt_ex_setLoop(bool cgpt_ex_enabled);
void cgpt_ex_setRecordTiming(uint32_t cgpt_ex_moveMs, uint32_t cgpt_ex_holdMs);
bool cgpt_ex_setPosition(uint8_t cgpt_ex_joint, uint16_t cgpt_ex_position);
uint16_t cgpt_ex_getPosition(uint8_t cgpt_ex_joint);
uint16_t cgpt_ex_getPoseCount();
bool cgpt_ex_isPlaying();
const cgpt_ex_pose *cgpt_ex_getPose(uint16_t cgpt_ex_index);

// Manual SD capture: New writes the CSV header only. Each Capture/physical
// Store press appends ONE point (or inserts in editing mode), default 1500 ms.
bool cgpt_ex_newCsvCapture();
bool cgpt_ex_capturePoint(); // Creates a new file automatically if none is open.
bool cgpt_ex_finishCsvCapture(); // Flush/close only; never captures a final pose.
bool cgpt_ex_startCsvPlayback(const char *cgpt_ex_rootFileName);
void cgpt_ex_openFileSelector();
void cgpt_ex_closeFileSelector();
bool cgpt_ex_isFileSelectorVisible();

// Hardware adapter implemented in cgpt_ex_wombat.cpp. Adapt these functions
// if you want to reuse chip objects configured elsewhere.
bool cgpt_ex_hardwareBegin();
bool cgpt_ex_hardwareWrite(uint8_t cgpt_ex_joint, uint16_t cgpt_ex_position);
bool cgpt_ex_hardwareRead(uint16_t cgpt_ex_samples[cgpt_ex_jointCount],
                          uint16_t &cgpt_ex_presses);
bool cgpt_ex_hardwareReadEditButtons(uint16_t &cgpt_ex_previous,
                                     uint16_t &cgpt_ex_next, uint16_t &cgpt_ex_delete);
const cgpt_ex_chipInfo *cgpt_ex_getChipInfo(uint8_t cgpt_ex_chipIndex); // 0=servo, 1=input.
