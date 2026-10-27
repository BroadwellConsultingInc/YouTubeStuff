#pragma once
#include <stdint.h>

// Call once after cgpt_ex_wombatBegin(); service from the Arduino loop task.
void cgpt_ex_gaugesBegin();
void cgpt_ex_gaugesService();

// Calibration/interpolation helpers are independent of networking and hardware.
float cgpt_ex_gaugeMillivolts(float cgpt_ex_temperatureC, bool cgpt_ex_nozzle);
uint16_t cgpt_ex_gaugeDuty(float cgpt_ex_temperatureC, bool cgpt_ex_nozzle,
    uint16_t cgpt_ex_supplyMillivolts);
