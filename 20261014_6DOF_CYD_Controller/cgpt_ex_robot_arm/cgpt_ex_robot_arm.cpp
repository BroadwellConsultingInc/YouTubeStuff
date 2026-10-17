#include "cgpt_ex_cyd_touch.h"
#include "cgpt_ex_robot_arm.h"
#include "cgpt_ex_sd.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *cgpt_ex_screen = nullptr;
static lv_obj_t *cgpt_ex_startupScreen = nullptr;
static lv_obj_t *cgpt_ex_chipLabels[2] = {};
static bool cgpt_ex_startupVisible = false;
static bool cgpt_ex_fileSelectorVisible = false;
static char cgpt_ex_selectedCsv[cgpt_ex_fileNameSize] = {};
#if cgpt_ex_enableSd
static lv_obj_t *cgpt_ex_fileScreen = nullptr;
static lv_obj_t *cgpt_ex_fileButtons[cgpt_ex_filesPerPage] = {};
static lv_obj_t *cgpt_ex_fileLabels[cgpt_ex_filesPerPage] = {};
static lv_obj_t *cgpt_ex_fileStatus = nullptr;
static lv_obj_t *cgpt_ex_prevButton = nullptr;
static lv_obj_t *cgpt_ex_nextButton = nullptr;
static char cgpt_ex_fileNames[cgpt_ex_filesPerPage][cgpt_ex_fileNameSize] = {};
static uint8_t cgpt_ex_fileIndices[cgpt_ex_filesPerPage] = {0,1,2,3};
static uint32_t cgpt_ex_filePage = 0;
static uint8_t cgpt_ex_fileCount = 0;
static bool cgpt_ex_fileMore = false;
#endif
static lv_obj_t *cgpt_ex_sliders[cgpt_ex_jointCount] = {};
static lv_obj_t *cgpt_ex_values[cgpt_ex_jointCount] = {};
static lv_obj_t *cgpt_ex_statusLabel = nullptr;
static lv_obj_t *cgpt_ex_loopLabel = nullptr;
static uint8_t cgpt_ex_jointIndices[cgpt_ex_jointCount] = {0, 1, 2, 3, 4, 5};
static uint16_t cgpt_ex_positions[cgpt_ex_jointCount] = {};
static uint16_t cgpt_ex_sentPositions[cgpt_ex_jointCount] = {};
static bool cgpt_ex_sentValid[cgpt_ex_jointCount] = {};
static int64_t cgpt_ex_jogRemainder[cgpt_ex_jointCount] = {};
static cgpt_ex_pose cgpt_ex_poses[cgpt_ex_poseCapacity] = {};
static uint16_t cgpt_ex_poseCount = 0;
static uint16_t cgpt_ex_playIndex = 0;
static uint16_t cgpt_ex_segmentStart[cgpt_ex_jointCount] = {};
static uint32_t cgpt_ex_phaseStartMs = 0;
static uint32_t cgpt_ex_lastServiceMs = 0;
static uint32_t cgpt_ex_lastJoystickMs = 0;
static uint32_t cgpt_ex_recordMoveMs = cgpt_ex_defaultMoveMs;
static uint32_t cgpt_ex_recordHoldMs = cgpt_ex_defaultHoldMs;
static bool cgpt_ex_editing = false;
static bool cgpt_ex_editCsv = false;
static uint32_t cgpt_ex_editIndex = 0;
static uint32_t cgpt_ex_editCount = 0;
static uint32_t cgpt_ex_csvCursor = 0;
static uint16_t cgpt_ex_ramCursor = 0;
static uint32_t cgpt_ex_captureCount = 0;
static bool cgpt_ex_playing = false;
static bool cgpt_ex_holding = false;
static bool cgpt_ex_loop = false;
static bool cgpt_ex_ready = false;
static bool cgpt_ex_fault = false;
static bool cgpt_ex_uiDirty = true;
static char cgpt_ex_notice[96] = {};
static uint32_t cgpt_ex_noticeStartMs = 0;

static uint16_t cgpt_ex_clampPosition(uint8_t cgpt_ex_joint, int64_t cgpt_ex_value) {
    const cgpt_ex_jointConfig &cgpt_ex_cfg = cgpt_ex_jointSettings[cgpt_ex_joint];
    if (cgpt_ex_value < cgpt_ex_cfg.cgpt_ex_minPosition) return cgpt_ex_cfg.cgpt_ex_minPosition;
    if (cgpt_ex_value > cgpt_ex_cfg.cgpt_ex_maxPosition) return cgpt_ex_cfg.cgpt_ex_maxPosition;
    return static_cast<uint16_t>(cgpt_ex_value);
}

static void cgpt_ex_showNotice(const char *cgpt_ex_text) {
    snprintf(cgpt_ex_notice, sizeof(cgpt_ex_notice), "%s", cgpt_ex_text);
    cgpt_ex_noticeStartMs = millis();
    cgpt_ex_uiDirty = true;
}

static void cgpt_ex_latchFault() {
    cgpt_ex_fault = true;
    cgpt_ex_playing = false;
    cgpt_ex_sdPlayStop();
    cgpt_ex_sdCaptureStop();
    cgpt_ex_uiDirty = true;
}

static void cgpt_ex_updateUi() {
    if (!cgpt_ex_screen || !cgpt_ex_uiDirty) return;
    cgpt_ex_uiDirty = false;
    for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < cgpt_ex_jointCount; ++cgpt_ex_i) {
        lv_slider_set_value(cgpt_ex_sliders[cgpt_ex_i], cgpt_ex_positions[cgpt_ex_i], LV_ANIM_OFF);
        lv_label_set_text_fmt(cgpt_ex_values[cgpt_ex_i], "%u", static_cast<unsigned>(cgpt_ex_positions[cgpt_ex_i]));
    }
    lv_label_set_text(cgpt_ex_loopLabel, cgpt_ex_loop ? "Loop on" : "Loop off");
    if (cgpt_ex_fault) {
        lv_label_set_text(cgpt_ex_statusLabel, "I/O error: reinitialize");
    } else if (!cgpt_ex_ready) {
        lv_label_set_text(cgpt_ex_statusLabel, "Initialization failed");
    } else if (cgpt_ex_notice[0]) {
        lv_label_set_text(cgpt_ex_statusLabel, cgpt_ex_notice);
    } else if (cgpt_ex_editing) {
        lv_label_set_text_fmt(cgpt_ex_statusLabel, "Edit %lu/%lu | %s",
            static_cast<unsigned long>(cgpt_ex_editCount ? cgpt_ex_editIndex + 1 : 0),
            static_cast<unsigned long>(cgpt_ex_editCount), cgpt_ex_editCsv ? "CSV" : "RAM");
    } else if (cgpt_ex_sdCapturing()) {
        lv_label_set_text_fmt(cgpt_ex_statusLabel,"Capture %s",cgpt_ex_sdCaptureName()+1);
    } else if (cgpt_ex_sdPlayState()!=cgpt_ex_csvIdle) {
        if (cgpt_ex_sdPlayState()==cgpt_ex_csvChecking) {
            lv_label_set_text_fmt(cgpt_ex_statusLabel,"Check %s",cgpt_ex_selectedCsv+1);
        } else {
            lv_label_set_text_fmt(cgpt_ex_statusLabel,"Play %s %lu/%lu",cgpt_ex_selectedCsv+1,
                static_cast<unsigned long>(cgpt_ex_sdPlayPointIndex()+1),
                static_cast<unsigned long>(cgpt_ex_sdPlayPointCount()));
        }
    } else if (cgpt_ex_playing) {
        lv_label_set_text_fmt(cgpt_ex_statusLabel, "Play %u/%u | %s",
            static_cast<unsigned>(cgpt_ex_playIndex + 1), static_cast<unsigned>(cgpt_ex_poseCount),
            cgpt_ex_holding ? "hold" : "move");
    } else if (cgpt_ex_selectedCsv[0]) {
        lv_label_set_text_fmt(cgpt_ex_statusLabel,"CSV ready: %s",cgpt_ex_selectedCsv+1);
    } else {
        lv_label_set_text_fmt(cgpt_ex_statusLabel, "Jog | poses %u/%u",
            static_cast<unsigned>(cgpt_ex_poseCount), static_cast<unsigned>(cgpt_ex_poseCapacity));
    }
}

bool cgpt_ex_setPosition(uint8_t cgpt_ex_joint, uint16_t cgpt_ex_position) {
    if (!cgpt_ex_ready || cgpt_ex_fault || cgpt_ex_startupVisible || cgpt_ex_fileSelectorVisible || cgpt_ex_joint >= cgpt_ex_jointCount) return false;
    cgpt_ex_stopPlayback();
    cgpt_ex_positions[cgpt_ex_joint] = cgpt_ex_clampPosition(cgpt_ex_joint, cgpt_ex_position);
    cgpt_ex_jogRemainder[cgpt_ex_joint] = 0;
    cgpt_ex_uiDirty = true;
    return true;
}

static void cgpt_ex_sliderEvent(lv_event_t *cgpt_ex_event) {
    const uint8_t cgpt_ex_joint = *static_cast<uint8_t *>(lv_event_get_user_data(cgpt_ex_event));
    cgpt_ex_setPosition(cgpt_ex_joint,
        static_cast<uint16_t>(lv_slider_get_value(cgpt_ex_sliders[cgpt_ex_joint])));
    cgpt_ex_updateUi();
}

static void cgpt_ex_storeEvent(lv_event_t *cgpt_ex_event) {
    (void)cgpt_ex_event;
#if cgpt_ex_enableSd
    cgpt_ex_capturePoint();
#else
    cgpt_ex_storePose();
#endif
    cgpt_ex_updateUi();
}
static void cgpt_ex_playEvent(lv_event_t *cgpt_ex_event) {
    (void)cgpt_ex_event;
    if (cgpt_ex_selectedCsv[0]) cgpt_ex_startCsvPlayback(cgpt_ex_selectedCsv);
    else cgpt_ex_startPlayback();
    cgpt_ex_updateUi();
}
static void cgpt_ex_stopEvent(lv_event_t *cgpt_ex_event) {
    (void)cgpt_ex_event;
    const bool cgpt_ex_wasCapturing=cgpt_ex_sdCapturing();
    const bool cgpt_ex_wasEditing=cgpt_ex_editing;
    cgpt_ex_stopPlayback();
    cgpt_ex_editing = false;
    cgpt_ex_finishCsvCapture();
    if (!cgpt_ex_wasCapturing) cgpt_ex_showNotice(cgpt_ex_wasEditing ? "Editing stopped" : "Playback stopped");
    cgpt_ex_updateUi();
}
static void cgpt_ex_undoEvent(lv_event_t *cgpt_ex_event) {
    (void)cgpt_ex_event;
#if cgpt_ex_enableSd
    cgpt_ex_newCsvCapture();
#else
    cgpt_ex_undoPose();
#endif
    cgpt_ex_updateUi();
}
static void cgpt_ex_clearEvent(lv_event_t *cgpt_ex_event) {
    (void)cgpt_ex_event;
#if cgpt_ex_enableSd
    cgpt_ex_openFileSelector();
#else
    cgpt_ex_clearPoses();
#endif
    cgpt_ex_updateUi();
}
static void cgpt_ex_loopEvent(lv_event_t *cgpt_ex_event) {
    (void)cgpt_ex_event;
    cgpt_ex_setLoop(!cgpt_ex_loop);
    cgpt_ex_updateUi();
}

static lv_obj_t *cgpt_ex_makeButton(int32_t cgpt_ex_x, int32_t cgpt_ex_y,
                                   const char *cgpt_ex_text, lv_event_cb_t cgpt_ex_callback) {
#if LVGL_VERSION_MAJOR == 9
    lv_obj_t *cgpt_ex_button = lv_button_create(cgpt_ex_screen);
#else
    lv_obj_t *cgpt_ex_button = lv_btn_create(cgpt_ex_screen);
#endif
    lv_obj_set_pos(cgpt_ex_button, cgpt_ex_x, cgpt_ex_y);
    lv_obj_set_size(cgpt_ex_button, 100, 25);
    lv_obj_set_style_pad_all(cgpt_ex_button, 0, 0);
    lv_obj_set_style_radius(cgpt_ex_button, 5, 0);
    lv_obj_set_style_bg_color(cgpt_ex_button, lv_color_hex(0x245578), 0);
    lv_obj_t *cgpt_ex_label = lv_label_create(cgpt_ex_button);
    lv_label_set_text(cgpt_ex_label, cgpt_ex_text);
    lv_obj_center(cgpt_ex_label);
    lv_obj_add_event_cb(cgpt_ex_button, cgpt_ex_callback, LV_EVENT_CLICKED, nullptr);
    return cgpt_ex_label;
}

static void cgpt_ex_createUi() {
    cgpt_ex_screen = lv_obj_create(nullptr);
    lv_obj_set_size(cgpt_ex_screen, 320, 240);
    lv_obj_clear_flag(cgpt_ex_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(cgpt_ex_screen, 0, 0);
    lv_obj_set_style_bg_color(cgpt_ex_screen, lv_color_hex(0x101923), 0);
    lv_obj_set_style_text_color(cgpt_ex_screen, lv_color_hex(0xF2F6FA), 0);
    lv_obj_t *cgpt_ex_title = lv_label_create(cgpt_ex_screen);
    lv_label_set_text(cgpt_ex_title, "6 DOF ROBOT ARM");
    lv_obj_set_pos(cgpt_ex_title, 6, 3);
    for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < cgpt_ex_jointCount; ++cgpt_ex_i) {
        const int32_t cgpt_ex_y = 27 + cgpt_ex_i * 23;
        lv_obj_t *cgpt_ex_name = lv_label_create(cgpt_ex_screen);
        lv_label_set_text(cgpt_ex_name, cgpt_ex_jointSettings[cgpt_ex_i].cgpt_ex_name);
        lv_obj_set_pos(cgpt_ex_name, 6, cgpt_ex_y);
        cgpt_ex_sliders[cgpt_ex_i] = lv_slider_create(cgpt_ex_screen);
        lv_obj_set_pos(cgpt_ex_sliders[cgpt_ex_i], 100, cgpt_ex_y + 5);
        lv_obj_set_size(cgpt_ex_sliders[cgpt_ex_i], 150, 8);
        lv_slider_set_range(cgpt_ex_sliders[cgpt_ex_i],
            cgpt_ex_jointSettings[cgpt_ex_i].cgpt_ex_minPosition,
            cgpt_ex_jointSettings[cgpt_ex_i].cgpt_ex_maxPosition);
        lv_obj_set_style_pad_all(cgpt_ex_sliders[cgpt_ex_i], 4, LV_PART_KNOB);
        lv_obj_set_style_bg_color(cgpt_ex_sliders[cgpt_ex_i], lv_color_hex(0x36C9AA), LV_PART_INDICATOR);
        lv_obj_set_ext_click_area(cgpt_ex_sliders[cgpt_ex_i], 4);
        lv_obj_add_event_cb(cgpt_ex_sliders[cgpt_ex_i], cgpt_ex_sliderEvent,
            LV_EVENT_VALUE_CHANGED, &cgpt_ex_jointIndices[cgpt_ex_i]);
        cgpt_ex_values[cgpt_ex_i] = lv_label_create(cgpt_ex_screen);
        lv_obj_set_pos(cgpt_ex_values[cgpt_ex_i], 260, cgpt_ex_y);
        lv_obj_set_width(cgpt_ex_values[cgpt_ex_i], 54);
        lv_obj_set_style_text_align(cgpt_ex_values[cgpt_ex_i], LV_TEXT_ALIGN_RIGHT, 0);
    }
    cgpt_ex_statusLabel = lv_label_create(cgpt_ex_screen);
    lv_obj_set_pos(cgpt_ex_statusLabel, 6, 167);
    lv_obj_set_width(cgpt_ex_statusLabel, 308);
    lv_label_set_long_mode(cgpt_ex_statusLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    cgpt_ex_makeButton(5, 185, cgpt_ex_enableSd ? "Capture" : "Store", cgpt_ex_storeEvent);
    cgpt_ex_makeButton(110, 185, "Play", cgpt_ex_playEvent);
    cgpt_ex_makeButton(215, 185, "Stop", cgpt_ex_stopEvent);
    cgpt_ex_makeButton(5, 213, cgpt_ex_enableSd ? "New" : "Undo", cgpt_ex_undoEvent);
    cgpt_ex_makeButton(110, 213, cgpt_ex_enableSd ? "Files" : "Clear", cgpt_ex_clearEvent);
    cgpt_ex_loopLabel = cgpt_ex_makeButton(215, 213, "Loop off", cgpt_ex_loopEvent);
#if LVGL_VERSION_MAJOR == 9
    lv_screen_load(cgpt_ex_screen);
#else
    lv_scr_load(cgpt_ex_screen);
#endif
}

bool cgpt_ex_isStartupVisible() { return cgpt_ex_startupVisible; }

void cgpt_ex_dismissStartup() {
    if (!cgpt_ex_startupVisible) return;
    if (cgpt_ex_ready && !cgpt_ex_fault) {
        uint16_t cgpt_ex_samples[cgpt_ex_jointCount] = {};
        uint16_t cgpt_ex_ignoredPresses = 0;
        // Discard button transitions accumulated while the startup screen was
        // visible. A button still held at OK must be released/repressed to Store.
        uint16_t cgpt_ex_previous = 0, cgpt_ex_next = 0, cgpt_ex_delete = 0;
        if (!cgpt_ex_hardwareRead(cgpt_ex_samples, cgpt_ex_ignoredPresses) ||
            !cgpt_ex_hardwareReadEditButtons(cgpt_ex_previous, cgpt_ex_next, cgpt_ex_delete)) cgpt_ex_latchFault();
    }
    cgpt_ex_startupVisible = false;
    cgpt_ex_lastServiceMs = millis();
    cgpt_ex_lastJoystickMs = cgpt_ex_lastServiceMs;
#if LVGL_VERSION_MAJOR == 9
    lv_screen_load(cgpt_ex_screen);
#else
    lv_scr_load(cgpt_ex_screen);
#endif
    cgpt_ex_uiDirty = true;
    cgpt_ex_updateUi();
}

static void cgpt_ex_startupOkEvent(lv_event_t *cgpt_ex_event) {
    (void)cgpt_ex_event;
    cgpt_ex_dismissStartup();
}

static void cgpt_ex_showStartup() {
    if (!cgpt_ex_startupScreen) {
        cgpt_ex_startupScreen = lv_obj_create(nullptr);
        lv_obj_set_size(cgpt_ex_startupScreen, 320, 240);
        lv_obj_clear_flag(cgpt_ex_startupScreen, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(cgpt_ex_startupScreen, 0, 0);
        lv_obj_set_style_bg_color(cgpt_ex_startupScreen, lv_color_hex(0x101923), 0);
        lv_obj_set_style_text_color(cgpt_ex_startupScreen, lv_color_hex(0xF2F6FA), 0);
        lv_obj_t *cgpt_ex_title = lv_label_create(cgpt_ex_startupScreen);
        lv_label_set_text(cgpt_ex_title, "SERIAL WOMBAT STARTUP");
        lv_obj_set_pos(cgpt_ex_title, 6, 4);
        lv_obj_t *cgpt_ex_body = lv_obj_create(cgpt_ex_startupScreen);
        lv_obj_set_pos(cgpt_ex_body, 5, 27);
        lv_obj_set_size(cgpt_ex_body, 310, 158);
        lv_obj_set_style_pad_all(cgpt_ex_body, 0, 0);
        lv_obj_set_style_border_width(cgpt_ex_body, 0, 0);
        lv_obj_set_style_bg_color(cgpt_ex_body, lv_color_hex(0x192A38), 0);
        lv_obj_set_style_text_color(cgpt_ex_body, lv_color_hex(0xF2F6FA), 0);
        lv_obj_set_scroll_dir(cgpt_ex_body, LV_DIR_VER);
        for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < 2; ++cgpt_ex_i) {
            cgpt_ex_chipLabels[cgpt_ex_i] = lv_label_create(cgpt_ex_body);
            lv_obj_set_pos(cgpt_ex_chipLabels[cgpt_ex_i], 6 + cgpt_ex_i * 156, 4);
            lv_obj_set_width(cgpt_ex_chipLabels[cgpt_ex_i], 142);
            lv_label_set_long_mode(cgpt_ex_chipLabels[cgpt_ex_i], LV_LABEL_LONG_WRAP);
        }
        lv_obj_t *cgpt_ex_hint = lv_label_create(cgpt_ex_startupScreen);
#if cgpt_ex_enableJoysticks
        lv_label_set_text(cgpt_ex_hint, "Scroll for details | Joysticks ON");
#else
        lv_label_set_text(cgpt_ex_hint, "Scroll for details | Joysticks OFF");
#endif
        lv_obj_set_pos(cgpt_ex_hint, 6, 189);
#if LVGL_VERSION_MAJOR == 9
        lv_obj_t *cgpt_ex_ok = lv_button_create(cgpt_ex_startupScreen);
#else
        lv_obj_t *cgpt_ex_ok = lv_btn_create(cgpt_ex_startupScreen);
#endif
        lv_obj_set_pos(cgpt_ex_ok, 5, 208);
        lv_obj_set_size(cgpt_ex_ok, 310, 30);
        lv_obj_set_style_bg_color(cgpt_ex_ok, lv_color_hex(0x245578), 0);
        lv_obj_set_style_radius(cgpt_ex_ok, 5, 0);
        lv_obj_t *cgpt_ex_okLabel = lv_label_create(cgpt_ex_ok);
        lv_label_set_text(cgpt_ex_okLabel, "OK");
        lv_obj_center(cgpt_ex_okLabel);
        lv_obj_add_event_cb(cgpt_ex_ok, cgpt_ex_startupOkEvent, LV_EVENT_CLICKED, nullptr);
    }
    for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < 2; ++cgpt_ex_i) {
        const cgpt_ex_chipInfo *cgpt_ex_info = cgpt_ex_getChipInfo(cgpt_ex_i);
        if (!cgpt_ex_info) { lv_label_set_text(cgpt_ex_chipLabels[cgpt_ex_i], "Chip info unavailable"); continue; }
        char cgpt_ex_voltage[28] = "N/A";
        char cgpt_ex_device[24] = "N/A";
        char cgpt_ex_revision[24] = "N/A";
        char cgpt_ex_counters[80] = "Frames: N/A\nOverruns: N/A";
        char cgpt_ex_firmware[16] = "N/A";
        if (cgpt_ex_info->cgpt_ex_firmware[0]) {
            snprintf(cgpt_ex_firmware, sizeof(cgpt_ex_firmware), "%c.%c.%c",
                cgpt_ex_info->cgpt_ex_firmware[0], cgpt_ex_info->cgpt_ex_firmware[1], cgpt_ex_info->cgpt_ex_firmware[2]);
        }
        if (cgpt_ex_info->cgpt_ex_systemInfoValid) {
            snprintf(cgpt_ex_voltage, sizeof(cgpt_ex_voltage), "%u.%03u V",
                static_cast<unsigned>(cgpt_ex_info->cgpt_ex_supplyMv / 1000),
                static_cast<unsigned>(cgpt_ex_info->cgpt_ex_supplyMv % 1000));
            snprintf(cgpt_ex_counters, sizeof(cgpt_ex_counters), "Frames: %lu\nOverruns: %u",
                static_cast<unsigned long>(cgpt_ex_info->cgpt_ex_frames),
                static_cast<unsigned>(cgpt_ex_info->cgpt_ex_overflowFrames));
        }
        if (cgpt_ex_info->cgpt_ex_deviceInfoValid) {
            snprintf(cgpt_ex_device, sizeof(cgpt_ex_device), "0x%04X", static_cast<unsigned>(cgpt_ex_info->cgpt_ex_deviceId));
            snprintf(cgpt_ex_revision, sizeof(cgpt_ex_revision), "%u", static_cast<unsigned>(cgpt_ex_info->cgpt_ex_deviceRevision));
        }
        lv_label_set_text_fmt(cgpt_ex_chipLabels[cgpt_ex_i],
            "%s @ 0x%02X\nInit: %s\nModel: %s\nFW: %s\nVDD: %s\nMode: %s\nUID:\n%s\nDevice: %s\nRev: %s\n%s\nError: %d",
            cgpt_ex_i ? "Input" : "Servo", static_cast<unsigned>(cgpt_ex_info->cgpt_ex_address),
            cgpt_ex_info->cgpt_ex_initialized ? "OK" : "FAILED",
            cgpt_ex_info->cgpt_ex_connected ? cgpt_ex_info->cgpt_ex_model : "not found",
            cgpt_ex_firmware, cgpt_ex_voltage,
            !cgpt_ex_info->cgpt_ex_connected ? "N/A" : (cgpt_ex_info->cgpt_ex_inBoot ? "Boot" : "App"),
            cgpt_ex_info->cgpt_ex_uniqueId[0] ? cgpt_ex_info->cgpt_ex_uniqueId : "N/A",
            cgpt_ex_device, cgpt_ex_revision, cgpt_ex_counters,
            static_cast<int>(cgpt_ex_info->cgpt_ex_errorCode));
    }
    // Reinitialization also starts at the top of the diagnostics.
    lv_obj_scroll_to_y(lv_obj_get_parent(cgpt_ex_chipLabels[0]), 0, LV_ANIM_OFF);
    cgpt_ex_startupVisible = true;
#if LVGL_VERSION_MAJOR == 9
    lv_screen_load(cgpt_ex_startupScreen);
#else
    lv_scr_load(cgpt_ex_startupScreen);
#endif
}

static bool cgpt_ex_configValid() {
    if (cgpt_ex_servoAddress == cgpt_ex_inputAddress || cgpt_ex_storeButtonPin > 7) return false;
    for (uint8_t cgpt_ex_b = 0; cgpt_ex_b < cgpt_ex_buttonCount; ++cgpt_ex_b) {
        const cgpt_ex_buttonConfig &cgpt_ex_button = cgpt_ex_buttons[cgpt_ex_b];
        if (cgpt_ex_button.cgpt_ex_chip > 1 || cgpt_ex_button.cgpt_ex_pin > 7) return false;
        for (uint8_t cgpt_ex_k = 0; cgpt_ex_k < cgpt_ex_b; ++cgpt_ex_k)
            if (cgpt_ex_button.cgpt_ex_chip == cgpt_ex_buttons[cgpt_ex_k].cgpt_ex_chip &&
                cgpt_ex_button.cgpt_ex_pin == cgpt_ex_buttons[cgpt_ex_k].cgpt_ex_pin) return false;
        for (uint8_t cgpt_ex_j = 0; cgpt_ex_j < cgpt_ex_jointCount; ++cgpt_ex_j) {
            if (cgpt_ex_button.cgpt_ex_chip && cgpt_ex_button.cgpt_ex_pin == cgpt_ex_jointSettings[cgpt_ex_j].cgpt_ex_servoPin) return false;
#if cgpt_ex_enableJoysticks
            if (!cgpt_ex_button.cgpt_ex_chip && cgpt_ex_button.cgpt_ex_pin == cgpt_ex_jointSettings[cgpt_ex_j].cgpt_ex_analogPin) return false;
#endif
        }
    }
    for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < cgpt_ex_jointCount; ++cgpt_ex_i) {
        const cgpt_ex_jointConfig &cgpt_ex_cfg = cgpt_ex_jointSettings[cgpt_ex_i];
        if (cgpt_ex_cfg.cgpt_ex_minPosition >= cgpt_ex_cfg.cgpt_ex_maxPosition ||
            cgpt_ex_cfg.cgpt_ex_initialPosition < cgpt_ex_cfg.cgpt_ex_minPosition ||
            cgpt_ex_cfg.cgpt_ex_initialPosition > cgpt_ex_cfg.cgpt_ex_maxPosition ||
            cgpt_ex_cfg.cgpt_ex_minPulseUs < 100 ||
            cgpt_ex_cfg.cgpt_ex_maxPulseUs <= cgpt_ex_cfg.cgpt_ex_minPulseUs ||
#if cgpt_ex_enableJoysticks
            cgpt_ex_cfg.cgpt_ex_analogPin > 7 ||
            static_cast<int32_t>(cgpt_ex_cfg.cgpt_ex_joystickCenter) - cgpt_ex_cfg.cgpt_ex_deadband <= cgpt_ex_cfg.cgpt_ex_joystickMin ||
            static_cast<uint32_t>(cgpt_ex_cfg.cgpt_ex_joystickCenter) + cgpt_ex_cfg.cgpt_ex_deadband >= cgpt_ex_cfg.cgpt_ex_joystickMax ||
            cgpt_ex_cfg.cgpt_ex_analogPin == cgpt_ex_storeButtonPin ||
#endif
            cgpt_ex_cfg.cgpt_ex_servoPin > 7) return false;
        for (uint8_t cgpt_ex_j = 0; cgpt_ex_j < cgpt_ex_i; ++cgpt_ex_j) {
            if (cgpt_ex_cfg.cgpt_ex_servoPin == cgpt_ex_jointSettings[cgpt_ex_j].cgpt_ex_servoPin) return false;
#if cgpt_ex_enableJoysticks
            if (cgpt_ex_cfg.cgpt_ex_analogPin == cgpt_ex_jointSettings[cgpt_ex_j].cgpt_ex_analogPin) return false;
#endif
        }
    }
    return true;
}

bool cgpt_ex_begin() {
    cgpt_ex_sdPlayStop();
    cgpt_ex_sdCaptureStop();
    cgpt_ex_fileSelectorVisible = false;
    cgpt_ex_ready = false;
    cgpt_ex_editing = false;
    cgpt_ex_fault = false;
    cgpt_ex_playing = false;
    cgpt_ex_notice[0] = '\0';
    cgpt_ex_uiDirty = true;
    if (!cgpt_ex_configValid()) return false;
    for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < cgpt_ex_jointCount; ++cgpt_ex_i) {
        cgpt_ex_positions[cgpt_ex_i] = cgpt_ex_jointSettings[cgpt_ex_i].cgpt_ex_initialPosition;
        cgpt_ex_sentValid[cgpt_ex_i] = false;
        cgpt_ex_jogRemainder[cgpt_ex_i] = 0;
    }
    // Bind software touch BEFORE SD takes VSPI, even when no card is present.
    if (!cgpt_ex_cydTouchBegin()) return false;
    cgpt_ex_ready = cgpt_ex_hardwareBegin();
    cgpt_ex_sdBegin(); // A missing SD card does not disable chip/UI operation.
    if (!cgpt_ex_screen) cgpt_ex_createUi();
    cgpt_ex_lastServiceMs = millis();
    cgpt_ex_lastJoystickMs = cgpt_ex_lastServiceMs;
    cgpt_ex_updateUi();
    cgpt_ex_showStartup();
    return cgpt_ex_ready;
}

static bool cgpt_ex_editorAllowed() {
    return cgpt_ex_ready && !cgpt_ex_fault && !cgpt_ex_startupVisible && !cgpt_ex_fileSelectorVisible;
}
bool cgpt_ex_isEditing() { return cgpt_ex_editing; }
uint32_t cgpt_ex_getEditIndex() { return cgpt_ex_editIndex; }
uint32_t cgpt_ex_getEditCount() { return cgpt_ex_editCount; }
static bool cgpt_ex_selectEditPoint(uint32_t cgpt_ex_index) {
    if (cgpt_ex_editCsv) {
        if (!cgpt_ex_selectedCsv[0]) { cgpt_ex_editCount = 0; cgpt_ex_editIndex = 0; return true; }
        cgpt_ex_csvPoint cgpt_ex_point = {};
        uint32_t cgpt_ex_count = 0;
        if (!cgpt_ex_sdEditRead(cgpt_ex_selectedCsv, cgpt_ex_index, cgpt_ex_point, cgpt_ex_count)) {
            cgpt_ex_showNotice(cgpt_ex_sdError()); return false;
        }
        if (cgpt_ex_count && cgpt_ex_index >= cgpt_ex_count) {
            cgpt_ex_index = cgpt_ex_count - 1;
            if (!cgpt_ex_sdEditRead(cgpt_ex_selectedCsv, cgpt_ex_index, cgpt_ex_point, cgpt_ex_count)) {
                cgpt_ex_showNotice(cgpt_ex_sdError()); return false;
            }
        }
        cgpt_ex_editCount = cgpt_ex_count;
        if (cgpt_ex_count) memcpy(cgpt_ex_positions, cgpt_ex_point.cgpt_ex_positions, sizeof(cgpt_ex_positions));
        cgpt_ex_csvCursor = cgpt_ex_index;
    } else {
        cgpt_ex_editCount = cgpt_ex_poseCount;
        if (cgpt_ex_editCount) {
            if (cgpt_ex_index >= cgpt_ex_editCount) cgpt_ex_index = cgpt_ex_editCount - 1;
            memcpy(cgpt_ex_positions, cgpt_ex_poses[cgpt_ex_index].cgpt_ex_position, sizeof(cgpt_ex_positions));
            cgpt_ex_ramCursor = static_cast<uint16_t>(cgpt_ex_index);
        }
    }
    cgpt_ex_editIndex = cgpt_ex_editCount ? cgpt_ex_index : 0;
    memset(cgpt_ex_jogRemainder, 0, sizeof(cgpt_ex_jogRemainder));
    cgpt_ex_lastJoystickMs = millis();
    cgpt_ex_notice[0] = 0; cgpt_ex_uiDirty = true;
    return true;
}
static bool cgpt_ex_enterEditor() {
    if (!cgpt_ex_editorAllowed()) return false;
    if (cgpt_ex_editing) return true;
    const uint32_t cgpt_ex_ramIndex = cgpt_ex_ramCursor;
    if (cgpt_ex_sdPlayState() != cgpt_ex_csvIdle) cgpt_ex_csvCursor = cgpt_ex_sdPlayPointIndex();
    cgpt_ex_stopPlayback();
    if (!cgpt_ex_finishCsvCapture()) return false;
    cgpt_ex_editCsv = cgpt_ex_enableSd && (cgpt_ex_selectedCsv[0] || !cgpt_ex_poseCount);
    // Count/validate the file without moving to an anchor before applying the
    // requested button. One press produces only its requested target command.
    if (cgpt_ex_editCsv && cgpt_ex_selectedCsv[0]) {
        cgpt_ex_csvPoint cgpt_ex_unused = {};
        uint32_t cgpt_ex_count = 0;
        if (!cgpt_ex_sdEditRead(cgpt_ex_selectedCsv, UINT32_MAX, cgpt_ex_unused, cgpt_ex_count)) {
            cgpt_ex_showNotice(cgpt_ex_sdError()); return false;
        }
        cgpt_ex_editCount = cgpt_ex_count;
        cgpt_ex_editIndex = !cgpt_ex_count ? 0 : (cgpt_ex_csvCursor < cgpt_ex_count ? cgpt_ex_csvCursor : cgpt_ex_count - 1);
    } else {
        cgpt_ex_editCount = cgpt_ex_editCsv ? 0 : cgpt_ex_poseCount;
        cgpt_ex_editIndex = cgpt_ex_ramIndex < cgpt_ex_editCount ? cgpt_ex_ramIndex : (cgpt_ex_editCount ? cgpt_ex_editCount - 1 : 0);
    }
    cgpt_ex_editing = true;
    cgpt_ex_notice[0] = 0; cgpt_ex_uiDirty = true;
    return true;
}
bool cgpt_ex_previousPoint() {
    return cgpt_ex_enterEditor() && cgpt_ex_selectEditPoint(cgpt_ex_editIndex ? cgpt_ex_editIndex - 1 : 0);
}
bool cgpt_ex_nextPoint() {
    if (!cgpt_ex_enterEditor()) return false;
    return cgpt_ex_selectEditPoint(cgpt_ex_editCount && cgpt_ex_editIndex < cgpt_ex_editCount - 1 ? cgpt_ex_editIndex + 1 : cgpt_ex_editIndex);
}
bool cgpt_ex_deletePoint() {
    if (!cgpt_ex_enterEditor()) return false;
    if (!cgpt_ex_editCount) { cgpt_ex_showNotice("Edit: list is empty"); return false; }
    char cgpt_ex_warning[96] = {};
    if (cgpt_ex_editCsv) {
        if (!cgpt_ex_sdEditDelete(cgpt_ex_selectedCsv, cgpt_ex_editIndex)) { cgpt_ex_showNotice(cgpt_ex_sdError()); return false; }
        snprintf(cgpt_ex_warning, sizeof(cgpt_ex_warning), "%s", cgpt_ex_sdError());
    } else {
        memmove(&cgpt_ex_poses[cgpt_ex_editIndex], &cgpt_ex_poses[cgpt_ex_editIndex + 1],
            (cgpt_ex_poseCount - cgpt_ex_editIndex - 1) * sizeof(cgpt_ex_pose));
        --cgpt_ex_poseCount;
    }
    --cgpt_ex_editCount;
    // At the first point there is no predecessor: select the new first.
    // An empty list holds the current command and never invents a home pose.
    const bool cgpt_ex_ok = cgpt_ex_selectEditPoint(cgpt_ex_editIndex ? cgpt_ex_editIndex - 1 : 0);
    if (cgpt_ex_ok && cgpt_ex_warning[0]) cgpt_ex_showNotice(cgpt_ex_warning);
    return cgpt_ex_ok;
}
static bool cgpt_ex_storeEditedPoint() {
    if (!cgpt_ex_editorAllowed() || !cgpt_ex_editing) return false;
    if (cgpt_ex_editCsv) {
        if (!cgpt_ex_selectedCsv[0]) {
            if (!cgpt_ex_sdCaptureStart() || !cgpt_ex_finishCsvCapture()) { cgpt_ex_showNotice(cgpt_ex_sdError()); return false; }
        }
        cgpt_ex_csvPoint cgpt_ex_point = {};
        cgpt_ex_point.cgpt_ex_moveMs = cgpt_ex_recordMoveMs;
        memcpy(cgpt_ex_point.cgpt_ex_positions, cgpt_ex_positions, sizeof(cgpt_ex_positions));
        if (!cgpt_ex_sdEditInsert(cgpt_ex_selectedCsv, cgpt_ex_editIndex, cgpt_ex_point)) { cgpt_ex_showNotice(cgpt_ex_sdError()); return false; }
        cgpt_ex_csvCursor = cgpt_ex_editIndex;
    } else {
        if (cgpt_ex_poseCount >= cgpt_ex_poseCapacity) { cgpt_ex_showNotice("Pose list full"); return false; }
        memmove(&cgpt_ex_poses[cgpt_ex_editIndex + 1], &cgpt_ex_poses[cgpt_ex_editIndex],
            (cgpt_ex_poseCount - cgpt_ex_editIndex) * sizeof(cgpt_ex_pose));
        cgpt_ex_pose &cgpt_ex_point = cgpt_ex_poses[cgpt_ex_editIndex];
        memcpy(cgpt_ex_point.cgpt_ex_position, cgpt_ex_positions, sizeof(cgpt_ex_positions));
        cgpt_ex_point.cgpt_ex_moveMs = cgpt_ex_recordMoveMs;
        cgpt_ex_point.cgpt_ex_holdMs = cgpt_ex_recordHoldMs;
        ++cgpt_ex_poseCount;
        cgpt_ex_ramCursor = static_cast<uint16_t>(cgpt_ex_editIndex);
    }
    ++cgpt_ex_editCount;
    if (cgpt_ex_editCsv && cgpt_ex_sdError()[0]) cgpt_ex_showNotice(cgpt_ex_sdError());
    else cgpt_ex_notice[0] = 0;
    cgpt_ex_uiDirty = true;
    return true;
}

bool cgpt_ex_storePose() {
    if (cgpt_ex_editing) return cgpt_ex_storeEditedPoint();
    if (!cgpt_ex_ready || cgpt_ex_fault || cgpt_ex_startupVisible) return false;
    if (cgpt_ex_isPlaying()) { cgpt_ex_showNotice("Stop before storing"); return false; }
    if (cgpt_ex_poseCount >= cgpt_ex_poseCapacity) { cgpt_ex_showNotice("Pose list full"); return false; }
    cgpt_ex_ramCursor = cgpt_ex_poseCount;
    cgpt_ex_pose &cgpt_ex_newPose = cgpt_ex_poses[cgpt_ex_poseCount++];
    memcpy(cgpt_ex_newPose.cgpt_ex_position, cgpt_ex_positions, sizeof(cgpt_ex_positions));
    cgpt_ex_newPose.cgpt_ex_moveMs = cgpt_ex_recordMoveMs;
    cgpt_ex_newPose.cgpt_ex_holdMs = cgpt_ex_recordHoldMs;
    cgpt_ex_showNotice("Pose stored");
    return true;
}

void cgpt_ex_setRecordTiming(uint32_t cgpt_ex_moveMs, uint32_t cgpt_ex_holdMs) {
    cgpt_ex_recordMoveMs = cgpt_ex_moveMs < 20 ? 20 : (cgpt_ex_moveMs > 600000 ? 600000 : cgpt_ex_moveMs);
    cgpt_ex_recordHoldMs = cgpt_ex_holdMs > 600000 ? 600000 : cgpt_ex_holdMs;
}

bool cgpt_ex_undoPose() {
    if (cgpt_ex_isPlaying() || !cgpt_ex_poseCount) return false;
    --cgpt_ex_poseCount;
    if (cgpt_ex_ramCursor >= cgpt_ex_poseCount) cgpt_ex_ramCursor = cgpt_ex_poseCount ? cgpt_ex_poseCount - 1 : 0;
    if (cgpt_ex_editing && !cgpt_ex_editCsv) {
        cgpt_ex_editCount = cgpt_ex_poseCount;
        cgpt_ex_editIndex = cgpt_ex_editCount && cgpt_ex_editIndex >= cgpt_ex_editCount ? cgpt_ex_editCount - 1 : cgpt_ex_editIndex;
        if (!cgpt_ex_editCount) cgpt_ex_editIndex = 0;
    }
    cgpt_ex_showNotice("Last pose removed");
    return true;
}
bool cgpt_ex_clearPoses() {
    if (cgpt_ex_isPlaying()) { cgpt_ex_showNotice("Stop before clearing"); return false; }
    cgpt_ex_poseCount = 0;
    cgpt_ex_ramCursor = 0;
    if (cgpt_ex_editing && !cgpt_ex_editCsv) cgpt_ex_editCount = cgpt_ex_editIndex = 0;
    cgpt_ex_showNotice("Pose list cleared");
    return true;
}
void cgpt_ex_setLoop(bool cgpt_ex_enabled) { cgpt_ex_loop = cgpt_ex_enabled; cgpt_ex_uiDirty = true; }
uint16_t cgpt_ex_getPosition(uint8_t cgpt_ex_joint) { return cgpt_ex_joint < cgpt_ex_jointCount ? cgpt_ex_positions[cgpt_ex_joint] : 0; }
uint16_t cgpt_ex_getPoseCount() { return cgpt_ex_poseCount; }
bool cgpt_ex_isPlaying() { return cgpt_ex_playing || cgpt_ex_sdPlayState()!=cgpt_ex_csvIdle; }
const cgpt_ex_pose *cgpt_ex_getPose(uint16_t cgpt_ex_index) { return cgpt_ex_index < cgpt_ex_poseCount ? &cgpt_ex_poses[cgpt_ex_index] : nullptr; }

static void cgpt_ex_beginSegment(uint32_t cgpt_ex_nowMs) {
    cgpt_ex_ramCursor = cgpt_ex_playIndex;
    memcpy(cgpt_ex_segmentStart, cgpt_ex_positions, sizeof(cgpt_ex_positions));
    cgpt_ex_phaseStartMs = cgpt_ex_nowMs;
    cgpt_ex_holding = false;
    cgpt_ex_uiDirty = true;
}

bool cgpt_ex_newCsvCapture() {
    if (!cgpt_ex_ready || cgpt_ex_fault || cgpt_ex_startupVisible || cgpt_ex_fileSelectorVisible) return false;
    cgpt_ex_stopPlayback();
    if (!cgpt_ex_sdCaptureStop() || !cgpt_ex_sdRemount() || !cgpt_ex_sdCaptureStart()) {
        cgpt_ex_showNotice(cgpt_ex_sdError());return false;
    }
    cgpt_ex_editing = false;
    cgpt_ex_captureCount = cgpt_ex_csvCursor = 0;
    cgpt_ex_selectedCsv[0]='\0';
    cgpt_ex_showNotice("New file: press Capture for points");
    return true;
}
bool cgpt_ex_capturePoint() {
    if (cgpt_ex_editing) return cgpt_ex_storeEditedPoint();
    if (!cgpt_ex_ready || cgpt_ex_fault || cgpt_ex_startupVisible || cgpt_ex_fileSelectorVisible) return false;
    if (cgpt_ex_isPlaying()) {cgpt_ex_showNotice("Stop playback before Capture");return false;}
    if (!cgpt_ex_sdCapturing() && !cgpt_ex_newCsvCapture()) return false;
    const bool cgpt_ex_ok=cgpt_ex_sdCapturePoint(cgpt_ex_positions,cgpt_ex_recordMoveMs);
    if (cgpt_ex_ok) cgpt_ex_csvCursor = cgpt_ex_captureCount++;
    cgpt_ex_showNotice(cgpt_ex_ok ? "Point saved to CSV" : cgpt_ex_sdError());
    return cgpt_ex_ok;
}
bool cgpt_ex_finishCsvCapture() {
    if (!cgpt_ex_sdCapturing()) return true;
    const bool cgpt_ex_ok=cgpt_ex_sdCaptureStop();
    if (cgpt_ex_ok) snprintf(cgpt_ex_selectedCsv,sizeof(cgpt_ex_selectedCsv),"%s",cgpt_ex_sdCaptureName());
    cgpt_ex_showNotice(cgpt_ex_ok ? "CSV saved and closed" : cgpt_ex_sdError());
    return cgpt_ex_ok;
}
bool cgpt_ex_startCsvPlayback(const char *cgpt_ex_rootFileName) {
    if (!cgpt_ex_ready || cgpt_ex_fault || cgpt_ex_startupVisible || cgpt_ex_fileSelectorVisible) return false;
    if (cgpt_ex_sdCapturing()) {cgpt_ex_showNotice("Stop to close capture first");return false;}
    // A separate path buffer also handles replay of cgpt_ex_selectedCsv itself.
    char cgpt_ex_path[cgpt_ex_fileNameSize];
    if (!cgpt_ex_csvRootPath(cgpt_ex_rootFileName,cgpt_ex_path,sizeof(cgpt_ex_path))) {
        cgpt_ex_showNotice("Choose a root CSV file");return false;
    }
    cgpt_ex_stopPlayback();
    if (!cgpt_ex_sdPlayStart(cgpt_ex_path,millis(),cgpt_ex_positions)) {cgpt_ex_showNotice(cgpt_ex_sdError());return false;}
    cgpt_ex_editing = false; cgpt_ex_csvCursor = 0;
    snprintf(cgpt_ex_selectedCsv,sizeof(cgpt_ex_selectedCsv),"%s",cgpt_ex_path);
    cgpt_ex_notice[0]='\0';cgpt_ex_uiDirty=true;
    memset(cgpt_ex_jogRemainder,0,sizeof(cgpt_ex_jogRemainder));
    return true;
}
bool cgpt_ex_isFileSelectorVisible() {return cgpt_ex_fileSelectorVisible;}
void cgpt_ex_closeFileSelector() {
    if (!cgpt_ex_fileSelectorVisible) return;
    cgpt_ex_fileSelectorVisible=false;
    if (cgpt_ex_ready && !cgpt_ex_fault) {
        uint16_t cgpt_ex_samples[cgpt_ex_jointCount]={};uint16_t cgpt_ex_ignored=0;
        uint16_t cgpt_ex_previous = 0, cgpt_ex_next = 0, cgpt_ex_delete = 0;
        if (!cgpt_ex_hardwareRead(cgpt_ex_samples,cgpt_ex_ignored) ||
            !cgpt_ex_hardwareReadEditButtons(cgpt_ex_previous, cgpt_ex_next, cgpt_ex_delete)) cgpt_ex_latchFault();
    }
    cgpt_ex_lastServiceMs=millis();cgpt_ex_lastJoystickMs=cgpt_ex_lastServiceMs;
#if LVGL_VERSION_MAJOR == 9
    lv_screen_load(cgpt_ex_screen);
#else
    lv_scr_load(cgpt_ex_screen);
#endif
    cgpt_ex_uiDirty=true;cgpt_ex_updateUi();
}
#if cgpt_ex_enableSd
static void cgpt_ex_refreshFiles() {
    const bool cgpt_ex_ok=cgpt_ex_sdListPage(cgpt_ex_filePage,cgpt_ex_fileNames,cgpt_ex_fileCount,cgpt_ex_fileMore);
    for (uint8_t cgpt_ex_i=0;cgpt_ex_i<cgpt_ex_filesPerPage;++cgpt_ex_i) {
        if (cgpt_ex_i<cgpt_ex_fileCount) {
            lv_label_set_text(cgpt_ex_fileLabels[cgpt_ex_i],cgpt_ex_fileNames[cgpt_ex_i]);
            lv_obj_clear_flag(cgpt_ex_fileButtons[cgpt_ex_i],LV_OBJ_FLAG_HIDDEN);
        } else lv_obj_add_flag(cgpt_ex_fileButtons[cgpt_ex_i],LV_OBJ_FLAG_HIDDEN);
    }
    if (!cgpt_ex_ok) lv_label_set_text(cgpt_ex_fileStatus,cgpt_ex_sdError());
    else if (!cgpt_ex_fileCount) lv_label_set_text(cgpt_ex_fileStatus,"No root CSV files");
    else lv_label_set_text_fmt(cgpt_ex_fileStatus,"Page %lu: tap a CSV to %s",static_cast<unsigned long>(cgpt_ex_filePage+1),cgpt_ex_editing ? "edit" : "play");
    if (cgpt_ex_filePage) lv_obj_clear_state(cgpt_ex_prevButton,LV_STATE_DISABLED);
    else lv_obj_add_state(cgpt_ex_prevButton,LV_STATE_DISABLED);
    if (cgpt_ex_fileMore) lv_obj_clear_state(cgpt_ex_nextButton,LV_STATE_DISABLED);
    else lv_obj_add_state(cgpt_ex_nextButton,LV_STATE_DISABLED);
}
static void cgpt_ex_fileBackEvent(lv_event_t *cgpt_ex_event) {(void)cgpt_ex_event;cgpt_ex_closeFileSelector();}
static void cgpt_ex_filePrevEvent(lv_event_t *cgpt_ex_event) {(void)cgpt_ex_event;if (cgpt_ex_filePage) --cgpt_ex_filePage;cgpt_ex_refreshFiles();}
static void cgpt_ex_fileNextEvent(lv_event_t *cgpt_ex_event) {(void)cgpt_ex_event;if (cgpt_ex_fileMore) ++cgpt_ex_filePage;cgpt_ex_refreshFiles();}
static void cgpt_ex_fileRefreshEvent(lv_event_t *cgpt_ex_event) {
    (void)cgpt_ex_event;cgpt_ex_filePage=0;
    cgpt_ex_sdRemount();cgpt_ex_refreshFiles();
}
static void cgpt_ex_filePickEvent(lv_event_t *cgpt_ex_event) {
    const uint8_t cgpt_ex_index=*static_cast<uint8_t *>(lv_event_get_user_data(cgpt_ex_event));
    if (cgpt_ex_index>=cgpt_ex_fileCount) return;
    char cgpt_ex_name[cgpt_ex_fileNameSize];
    snprintf(cgpt_ex_name,sizeof(cgpt_ex_name),"%s",cgpt_ex_fileNames[cgpt_ex_index]);
    cgpt_ex_closeFileSelector();
    if (cgpt_ex_editing) {
        char cgpt_ex_path[cgpt_ex_fileNameSize];
        cgpt_ex_csvPoint cgpt_ex_point = {}; uint32_t cgpt_ex_count = 0;
        if (!cgpt_ex_csvRootPath(cgpt_ex_name,cgpt_ex_path,sizeof(cgpt_ex_path)) ||
            !cgpt_ex_sdEditRead(cgpt_ex_path,0,cgpt_ex_point,cgpt_ex_count)) {
            cgpt_ex_showNotice(cgpt_ex_sdError());
        } else {
            snprintf(cgpt_ex_selectedCsv,sizeof(cgpt_ex_selectedCsv),"%s",cgpt_ex_path);
            cgpt_ex_editCsv = true;
            cgpt_ex_selectEditPoint(0); // File selection retains editing mode.
        }
    } else cgpt_ex_startCsvPlayback(cgpt_ex_name);
    cgpt_ex_updateUi();
}
static lv_obj_t *cgpt_ex_browserButton(int32_t cgpt_ex_x,int32_t cgpt_ex_y,int32_t cgpt_ex_width,int32_t cgpt_ex_height,
                                        const char *cgpt_ex_text,lv_event_cb_t cgpt_ex_callback,void *cgpt_ex_data=nullptr) {
#if LVGL_VERSION_MAJOR == 9
    lv_obj_t *cgpt_ex_button=lv_button_create(cgpt_ex_fileScreen);
#else
    lv_obj_t *cgpt_ex_button=lv_btn_create(cgpt_ex_fileScreen);
#endif
    lv_obj_set_pos(cgpt_ex_button,cgpt_ex_x,cgpt_ex_y);lv_obj_set_size(cgpt_ex_button,cgpt_ex_width,cgpt_ex_height);
    lv_obj_set_style_pad_all(cgpt_ex_button,0,0);lv_obj_set_style_radius(cgpt_ex_button,5,0);
    lv_obj_set_style_bg_color(cgpt_ex_button,lv_color_hex(0x245578),0);
    lv_obj_t *cgpt_ex_label=lv_label_create(cgpt_ex_button);lv_label_set_text(cgpt_ex_label,cgpt_ex_text);
    lv_obj_set_width(cgpt_ex_label,cgpt_ex_width-12);lv_label_set_long_mode(cgpt_ex_label,LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(cgpt_ex_label,LV_TEXT_ALIGN_CENTER,0);lv_obj_center(cgpt_ex_label);
    lv_obj_add_event_cb(cgpt_ex_button,cgpt_ex_callback,LV_EVENT_CLICKED,cgpt_ex_data);
    return cgpt_ex_button;
}
#endif
void cgpt_ex_openFileSelector() {
#if cgpt_ex_enableSd
    if (!cgpt_ex_ready || cgpt_ex_fault || cgpt_ex_startupVisible) return;
    if (cgpt_ex_sdCapturing()) {cgpt_ex_showNotice("Stop to close capture first");return;}
    cgpt_ex_stopPlayback();cgpt_ex_filePage=0;
    if (!cgpt_ex_fileScreen) {
        cgpt_ex_fileScreen=lv_obj_create(nullptr);lv_obj_set_size(cgpt_ex_fileScreen,320,240);
        lv_obj_clear_flag(cgpt_ex_fileScreen,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_style_pad_all(cgpt_ex_fileScreen,0,0);
        lv_obj_set_style_bg_color(cgpt_ex_fileScreen,lv_color_hex(0x101923),0);
        lv_obj_set_style_text_color(cgpt_ex_fileScreen,lv_color_hex(0xF2F6FA),0);
        lv_obj_t *cgpt_ex_title=lv_label_create(cgpt_ex_fileScreen);lv_label_set_text(cgpt_ex_title,"SD ROOT CSV FILES");lv_obj_set_pos(cgpt_ex_title,6,4);
        cgpt_ex_browserButton(229,2,86,22,"Refresh",cgpt_ex_fileRefreshEvent);
        cgpt_ex_fileStatus=lv_label_create(cgpt_ex_fileScreen);lv_obj_set_pos(cgpt_ex_fileStatus,6,27);
        lv_obj_set_width(cgpt_ex_fileStatus,308);lv_label_set_long_mode(cgpt_ex_fileStatus,LV_LABEL_LONG_CLIP);
        for (uint8_t cgpt_ex_i=0;cgpt_ex_i<cgpt_ex_filesPerPage;++cgpt_ex_i) {
            cgpt_ex_fileButtons[cgpt_ex_i]=cgpt_ex_browserButton(5,49+cgpt_ex_i*35,310,31,"",cgpt_ex_filePickEvent,&cgpt_ex_fileIndices[cgpt_ex_i]);
            cgpt_ex_fileLabels[cgpt_ex_i]=lv_obj_get_child(cgpt_ex_fileButtons[cgpt_ex_i],0);
        }
        cgpt_ex_browserButton(5,203,100,34,"Back",cgpt_ex_fileBackEvent);
        cgpt_ex_prevButton=cgpt_ex_browserButton(110,203,100,34,"Previous",cgpt_ex_filePrevEvent);
        cgpt_ex_nextButton=cgpt_ex_browserButton(215,203,100,34,"Next",cgpt_ex_fileNextEvent);
    }
    cgpt_ex_sdRemount();cgpt_ex_refreshFiles();cgpt_ex_fileSelectorVisible=true;
#if LVGL_VERSION_MAJOR == 9
    lv_screen_load(cgpt_ex_fileScreen);
#else
    lv_scr_load(cgpt_ex_fileScreen);
#endif
#else
    cgpt_ex_showNotice("SD support disabled");
#endif
}
bool cgpt_ex_startPlayback() {
    if (!cgpt_ex_ready || cgpt_ex_fault || cgpt_ex_startupVisible || cgpt_ex_fileSelectorVisible) return false;
    if (cgpt_ex_sdCapturing()) {cgpt_ex_showNotice("Stop to close capture first");return false;}
    cgpt_ex_sdPlayStop();
    cgpt_ex_selectedCsv[0]='\0';
    if (!cgpt_ex_poseCount) { cgpt_ex_showNotice("Store a pose first"); return false; }
    cgpt_ex_editing = false;
    cgpt_ex_notice[0] = '\0';
    cgpt_ex_playIndex = 0;
    cgpt_ex_playing = true;
    memset(cgpt_ex_jogRemainder, 0, sizeof(cgpt_ex_jogRemainder));
    cgpt_ex_beginSegment(millis());
    return true;
}
void cgpt_ex_stopPlayback() {
    if (!cgpt_ex_isPlaying()) return;
    cgpt_ex_sdPlayStop();
    cgpt_ex_playing = false;
    cgpt_ex_lastJoystickMs = millis();
    memset(cgpt_ex_jogRemainder, 0, sizeof(cgpt_ex_jogRemainder));
    cgpt_ex_uiDirty = true;
}

static void cgpt_ex_advancePlayback(uint32_t cgpt_ex_nowMs) {
    if (!cgpt_ex_playing || cgpt_ex_editing) return;
    const cgpt_ex_pose &cgpt_ex_target = cgpt_ex_poses[cgpt_ex_playIndex];
    const uint32_t cgpt_ex_elapsed = cgpt_ex_nowMs - cgpt_ex_phaseStartMs;
    if (!cgpt_ex_holding) {
        const uint32_t cgpt_ex_t = cgpt_ex_elapsed > cgpt_ex_target.cgpt_ex_moveMs ? cgpt_ex_target.cgpt_ex_moveMs : cgpt_ex_elapsed;
        for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < cgpt_ex_jointCount; ++cgpt_ex_i) {
            const int32_t cgpt_ex_delta = static_cast<int32_t>(cgpt_ex_target.cgpt_ex_position[cgpt_ex_i]) - cgpt_ex_segmentStart[cgpt_ex_i];
            const int64_t cgpt_ex_value = cgpt_ex_segmentStart[cgpt_ex_i] +
                static_cast<int64_t>(cgpt_ex_delta) * cgpt_ex_t / cgpt_ex_target.cgpt_ex_moveMs;
            cgpt_ex_positions[cgpt_ex_i] = cgpt_ex_clampPosition(cgpt_ex_i, cgpt_ex_value);
        }
        cgpt_ex_uiDirty = true;
        if (cgpt_ex_elapsed >= cgpt_ex_target.cgpt_ex_moveMs) {
            cgpt_ex_holding = true;
            // Do not skip the endpoint or its dwell after a delayed loop.
            cgpt_ex_phaseStartMs = cgpt_ex_nowMs;
        }
    } else if (cgpt_ex_elapsed >= cgpt_ex_target.cgpt_ex_holdMs) {
        ++cgpt_ex_playIndex;
        if (cgpt_ex_playIndex >= cgpt_ex_poseCount) {
            if (cgpt_ex_loop) cgpt_ex_playIndex = 0;
            else { cgpt_ex_stopPlayback(); cgpt_ex_showNotice("Playback complete"); return; }
        }
        cgpt_ex_beginSegment(cgpt_ex_nowMs);
    }
}

bool cgpt_ex_readJoysticks() {
    if (!cgpt_ex_ready || cgpt_ex_fault || cgpt_ex_startupVisible || cgpt_ex_fileSelectorVisible) return false;
    const uint32_t cgpt_ex_nowMs = millis();
    uint32_t cgpt_ex_elapsedMs = cgpt_ex_nowMs - cgpt_ex_lastJoystickMs;
    cgpt_ex_lastJoystickMs = cgpt_ex_nowMs;
    if (cgpt_ex_elapsedMs > 50) cgpt_ex_elapsedMs = 50; // Avoid a large jog after a stall.
    uint16_t cgpt_ex_samples[cgpt_ex_jointCount] = {};
    uint16_t cgpt_ex_presses = 0;
    uint16_t cgpt_ex_previous = 0, cgpt_ex_next = 0, cgpt_ex_delete = 0;
    if (!cgpt_ex_hardwareRead(cgpt_ex_samples, cgpt_ex_presses) ||
        !cgpt_ex_hardwareReadEditButtons(cgpt_ex_previous, cgpt_ex_next, cgpt_ex_delete)) { cgpt_ex_latchFault(); return false; }
    if (cgpt_ex_previous || cgpt_ex_next || cgpt_ex_delete) {
        // Editing controls take precedence over joystick/Store for this poll.
        while (cgpt_ex_previous) { --cgpt_ex_previous; if (!cgpt_ex_previousPoint()) break; }
        while (cgpt_ex_next) { --cgpt_ex_next; if (!cgpt_ex_nextPoint()) break; }
        while (cgpt_ex_delete) { --cgpt_ex_delete; if (!cgpt_ex_deletePoint()) break; }
        return true;
    }
    if (!cgpt_ex_isPlaying()) {
#if cgpt_ex_enableJoysticks
        for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < cgpt_ex_jointCount; ++cgpt_ex_i) {
            if (cgpt_ex_sliders[cgpt_ex_i] && lv_slider_is_dragged(cgpt_ex_sliders[cgpt_ex_i])) continue;
            const cgpt_ex_jointConfig &cgpt_ex_cfg = cgpt_ex_jointSettings[cgpt_ex_i];
            const int32_t cgpt_ex_offset = static_cast<int32_t>(cgpt_ex_samples[cgpt_ex_i]) - cgpt_ex_cfg.cgpt_ex_joystickCenter;
            const int32_t cgpt_ex_magnitude = cgpt_ex_offset < 0 ? -cgpt_ex_offset : cgpt_ex_offset;
            if (cgpt_ex_magnitude <= cgpt_ex_cfg.cgpt_ex_deadband) { cgpt_ex_jogRemainder[cgpt_ex_i] = 0; continue; }
            const int32_t cgpt_ex_span = cgpt_ex_offset > 0 ?
                cgpt_ex_cfg.cgpt_ex_joystickMax - cgpt_ex_cfg.cgpt_ex_joystickCenter - cgpt_ex_cfg.cgpt_ex_deadband :
                cgpt_ex_cfg.cgpt_ex_joystickCenter - cgpt_ex_cfg.cgpt_ex_joystickMin - cgpt_ex_cfg.cgpt_ex_deadband;
            int32_t cgpt_ex_deflection = cgpt_ex_magnitude - cgpt_ex_cfg.cgpt_ex_deadband;
            if (cgpt_ex_deflection > cgpt_ex_span) cgpt_ex_deflection = cgpt_ex_span;
            int64_t cgpt_ex_velocity = static_cast<int64_t>(cgpt_ex_deflection) * cgpt_ex_cfg.cgpt_ex_maxCountsPerSecond / cgpt_ex_span;
            if ((cgpt_ex_offset < 0) != cgpt_ex_cfg.cgpt_ex_reverseJoystick) cgpt_ex_velocity = -cgpt_ex_velocity;
            const int64_t cgpt_ex_scaled = cgpt_ex_velocity * cgpt_ex_elapsedMs + cgpt_ex_jogRemainder[cgpt_ex_i];
            const int64_t cgpt_ex_delta = cgpt_ex_scaled / 1000;
            cgpt_ex_jogRemainder[cgpt_ex_i] = cgpt_ex_scaled % 1000;
            const int64_t cgpt_ex_requested = cgpt_ex_positions[cgpt_ex_i] + cgpt_ex_delta;
            const uint16_t cgpt_ex_newPosition = cgpt_ex_clampPosition(cgpt_ex_i, cgpt_ex_requested);
            if (cgpt_ex_newPosition != cgpt_ex_requested) cgpt_ex_jogRemainder[cgpt_ex_i] = 0;
            if (cgpt_ex_newPosition != cgpt_ex_positions[cgpt_ex_i]) {
                cgpt_ex_positions[cgpt_ex_i] = cgpt_ex_newPosition;
                cgpt_ex_uiDirty = true;
            }
        }
#endif
        // Transition counts preserve a press/release that happened between polls.
        // Multiple presses between polls store the sampled current pose each time.
        while (cgpt_ex_presses && (cgpt_ex_enableSd || cgpt_ex_poseCount < cgpt_ex_poseCapacity)) {
            --cgpt_ex_presses;
#if cgpt_ex_enableSd
            if (!cgpt_ex_capturePoint()) break;
#else
            cgpt_ex_storePose();
#endif
        }
#if !cgpt_ex_enableSd
        if (cgpt_ex_presses) cgpt_ex_showNotice("Pose list full");
#endif
    }
    // Button counts are consumed during playback; they are never queued for later.
    return true;
}

void cgpt_ex_service() {
    if (cgpt_ex_startupVisible || cgpt_ex_fileSelectorVisible) return;
    const uint32_t cgpt_ex_nowMs = millis();
    if (cgpt_ex_notice[0] && cgpt_ex_nowMs - cgpt_ex_noticeStartMs >= 1500) {
        cgpt_ex_notice[0] = '\0';
        cgpt_ex_uiDirty = true;
    }
    if (cgpt_ex_ready && !cgpt_ex_fault && cgpt_ex_nowMs - cgpt_ex_lastServiceMs >= cgpt_ex_updatePeriodMs) {
        cgpt_ex_lastServiceMs = cgpt_ex_nowMs;
        cgpt_ex_readJoysticks();
        if (!cgpt_ex_fault) {
            if (!cgpt_ex_editing && cgpt_ex_sdPlayState()!=cgpt_ex_csvIdle) {
                const bool cgpt_ex_changed=cgpt_ex_sdPlayService(millis(),cgpt_ex_positions,cgpt_ex_loop);
                cgpt_ex_csvCursor = cgpt_ex_sdPlayPointIndex();
                cgpt_ex_uiDirty=true;
                if (cgpt_ex_sdPlayState()==cgpt_ex_csvIdle) {
                    cgpt_ex_lastJoystickMs=millis();
                    cgpt_ex_showNotice(cgpt_ex_sdError()[0] ? cgpt_ex_sdError() : "CSV playback complete");
                }
                if (cgpt_ex_changed) memset(cgpt_ex_jogRemainder,0,sizeof(cgpt_ex_jogRemainder));
            } else cgpt_ex_advancePlayback(millis());
            for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < cgpt_ex_jointCount; ++cgpt_ex_i) {
                if (cgpt_ex_sentValid[cgpt_ex_i] && cgpt_ex_sentPositions[cgpt_ex_i] == cgpt_ex_positions[cgpt_ex_i]) continue;
                if (!cgpt_ex_hardwareWrite(cgpt_ex_i, cgpt_ex_positions[cgpt_ex_i])) { cgpt_ex_latchFault(); break; }
                cgpt_ex_sentPositions[cgpt_ex_i] = cgpt_ex_positions[cgpt_ex_i];
                cgpt_ex_sentValid[cgpt_ex_i] = true;
            }
        }
    }
    cgpt_ex_updateUi();
}
