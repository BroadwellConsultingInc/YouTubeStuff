#pragma once
#include <stdint.h>

// Software SPI matches the touch adapter from the working robot-arm sketch.
// It does not change the hardware SPI bus used by TFT_eSPI.
#define cgpt_ex_enableCydSoftTouch 1
#define cgpt_ex_touchDebug 1 // Serial touch coordinates; set to 0 after checking.
constexpr uint8_t cgpt_ex_touchMosi = 32;
constexpr uint8_t cgpt_ex_touchMiso = 39;
constexpr uint8_t cgpt_ex_touchSck = 25;
constexpr uint8_t cgpt_ex_touchCs = 33;
constexpr uint8_t cgpt_ex_touchIrq = 36;
constexpr int16_t cgpt_ex_touchRawMinX = 200;
constexpr int16_t cgpt_ex_touchRawMaxX = 3700;
constexpr int16_t cgpt_ex_touchRawMinY = 240;
constexpr int16_t cgpt_ex_touchRawMaxY = 3800;
