#pragma once
#include <stdint.h>

#ifndef cgpt_ex_enableSd
#if defined(ARDUINO_ARCH_ESP32)
#define cgpt_ex_enableSd 1
#else
#define cgpt_ex_enableSd 0
#endif
#endif
// CYD workaround: TFT on HSPI, SD on VSPI, touch on GPIO software SPI.
#ifndef cgpt_ex_enableCydSoftTouch
#if defined(ARDUINO_ARCH_ESP32)
#define cgpt_ex_enableCydSoftTouch 1
#else
#define cgpt_ex_enableCydSoftTouch 0
#endif
#endif
#if cgpt_ex_enableCydSoftTouch != 0 && cgpt_ex_enableCydSoftTouch != 1
#error "cgpt_ex_enableCydSoftTouch must be 0 or 1"
#endif
static constexpr uint8_t cgpt_ex_touchMosi = 32;
static constexpr uint8_t cgpt_ex_touchMiso = 39;
static constexpr uint8_t cgpt_ex_touchSck = 25;
static constexpr uint8_t cgpt_ex_touchCs = 33;
static constexpr uint8_t cgpt_ex_touchIrq = 36;
// -1: use rotation 2 for a portrait base display, 1 for a landscape base.
// For the tutorial's 240x320 display + LVGL rotation 270 this selects 2.
// Otherwise select 0..3 to suit your panel; LVGL handles its own rotation.
static constexpr int8_t cgpt_ex_touchRotation = -1;
static constexpr int16_t cgpt_ex_touchRawMinX = 200;
static constexpr int16_t cgpt_ex_touchRawMaxX = 3700;
static constexpr int16_t cgpt_ex_touchRawMinY = 240;
static constexpr int16_t cgpt_ex_touchRawMaxY = 3800;
static_assert(cgpt_ex_touchRotation >= -1 && cgpt_ex_touchRotation <= 3, "Invalid touch rotation");
static_assert(cgpt_ex_touchRawMinX != cgpt_ex_touchRawMaxX && cgpt_ex_touchRawMinY != cgpt_ex_touchRawMaxY, "Invalid touch calibration");

// Set 0 if your sketch mounts the global SD object before cgpt_ex_begin().
#ifndef cgpt_ex_mountSdInternally
#define cgpt_ex_mountSdInternally 1
#endif
// ESP32-2432S028R onboard microSD pins; default mount uses dedicated VSPI.
static constexpr uint8_t cgpt_ex_sdCs = 5;
static constexpr uint8_t cgpt_ex_sdSck = 18;
static constexpr uint8_t cgpt_ex_sdMiso = 19;
static constexpr uint8_t cgpt_ex_sdMosi = 23;
static constexpr uint32_t cgpt_ex_sdFrequency = 4000000;
static constexpr uint32_t cgpt_ex_csvMaxMoveMs = 600000; // Up to 10 min/segment.
static constexpr uint8_t cgpt_ex_csvRowsPerPass = 16;
static constexpr uint8_t cgpt_ex_filesPerPage = 4;
static_assert(cgpt_ex_filesPerPage == 4, "The 320x240 file selector has four slots");
static constexpr uint16_t cgpt_ex_fileNameSize = 96;


// Set to 0 when joystick potentiometers are not attached. This disables ADC
// configuration, ADC polling, and jogging; chip-side Store remains enabled.
// A build-wide -Dcgpt_ex_enableJoysticks=0 can override this default.
#ifndef cgpt_ex_enableJoysticks
#define cgpt_ex_enableJoysticks 1
#endif
#if cgpt_ex_enableJoysticks != 0 && cgpt_ex_enableJoysticks != 1
#error "cgpt_ex_enableJoysticks must be 0 or 1"
#endif

// Order: base yaw, shoulder pitch, elbow pitch, wrist pitch, wrist roll, grip.
static constexpr uint8_t cgpt_ex_jointCount = 6;
static constexpr uint16_t cgpt_ex_poseCapacity = 100;
static constexpr uint32_t cgpt_ex_updatePeriodMs = 20;
static constexpr uint32_t cgpt_ex_defaultMoveMs = 1500;
static constexpr uint32_t cgpt_ex_defaultHoldMs = 500;
static constexpr uint8_t cgpt_ex_servoAddress = 0x63;
static constexpr uint8_t cgpt_ex_inputAddress = 0x64;
static constexpr uint8_t cgpt_ex_storeButtonPin = 7;
// Four active-low buttons, each between its pin and GND. All pins use
// chip-side Debounced Input with inversion and weak pull-up enabled.
// 0=input chip (0x64), 1=servo chip (0x63).
struct cgpt_ex_buttonConfig {
    uint8_t cgpt_ex_chip; // 0=input, 1=servo
    uint8_t cgpt_ex_pin;
};
static constexpr uint8_t cgpt_ex_buttonCount = 4;
static constexpr cgpt_ex_buttonConfig cgpt_ex_buttons[cgpt_ex_buttonCount] = {
    {0, cgpt_ex_storeButtonPin}, // Store
    {0, 6},                    // Previous
    {1, 6},                    // Next
    {1, 7}                     // Delete
};
static constexpr uint16_t cgpt_ex_buttonDebounceMs = 30;

struct cgpt_ex_jointConfig {
    const char *cgpt_ex_name;
    uint8_t cgpt_ex_servoPin;
    uint8_t cgpt_ex_analogPin;
    uint16_t cgpt_ex_minPulseUs;
    uint16_t cgpt_ex_maxPulseUs;
    bool cgpt_ex_reverseServo;
    uint16_t cgpt_ex_minPosition;
    uint16_t cgpt_ex_maxPosition;
    uint16_t cgpt_ex_initialPosition;
    uint16_t cgpt_ex_joystickMin;
    uint16_t cgpt_ex_joystickCenter;
    uint16_t cgpt_ex_joystickMax;
    uint16_t cgpt_ex_deadband;
    bool cgpt_ex_reverseJoystick;
    uint32_t cgpt_ex_maxCountsPerSecond;
};

// Six channels on each chip use pins 0..5; input-chip Store uses pin 7.
// The startup screen displays the live model reported by each chip.
// Set pulse endpoints, joint limits, and startup poses for YOUR mechanism.
// Joystick values are normalized ADC counts, NOT millivolts.
static constexpr cgpt_ex_jointConfig cgpt_ex_jointSettings[cgpt_ex_jointCount] = {
    {"Base",       0,  0, 1000, 2000, true, 0, 65535, 32768, 0, 32768, 65535, 2500, true, 40000},
    {"Shoulder",   1,  1, 1000, 2000, false, 0, 65535, 32768, 0, 32768, 65535, 2500, true, 40000},
    {"Elbow",      2,  2, 1000, 2000, false, 0, 65535, 32768, 0, 32768, 65535, 2500, true, 40000},
    {"Wrist",      3,  3, 1000, 2000, false, 0, 65535, 32768, 0, 32768, 65535, 2500, true, 40000},
    {"Wrist roll", 4,  4, 1000, 2000, false, 0, 65535, 32768, 0, 32768, 65535, 2500, true, 40000},
    {"Gripper",    5,  5, 1000, 2000, false, 0, 65535, 32768, 0, 32768, 65535, 2500, true, 40000}
};

static_assert(cgpt_ex_poseCapacity > 0, "Pose capacity must be positive");
static_assert(cgpt_ex_updatePeriodMs > 0, "Update period must be positive");
