#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <SerialWombat.h>

// Shared chip objects for TM1637 displays and PWM gauge outputs.
extern SerialWombatChip cgpt_ex_wombat63;
extern SerialWombatChip cgpt_ex_wombat64;

// CYD I2C wiring. Change these two constants if your wiring differs.
constexpr int cgpt_ex_i2cSda = 22;
constexpr int cgpt_ex_i2cScl = 27;
void cgpt_ex_wombatBegin();
bool cgpt_ex_wombatReady(uint8_t cgpt_ex_chip); // 0=0x63, 1=0x64.
const char *cgpt_ex_wombatStatusText(uint8_t cgpt_ex_chip);
