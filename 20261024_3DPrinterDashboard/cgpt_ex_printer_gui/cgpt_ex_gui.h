#pragma once
#include <stdint.h>
#include <math.h>

struct cgpt_ex_PrinterStatus {
    const char *cgpt_ex_name = "Printer";
    const char *cgpt_ex_state = "No data";
    bool cgpt_ex_online = false;
    float cgpt_ex_nozzleC = NAN;
    float cgpt_ex_nozzleTargetC = NAN;
    float cgpt_ex_bedC = NAN;
    float cgpt_ex_bedTargetC = NAN;
    int32_t cgpt_ex_remainingSeconds = -1; // -1 means unknown.
    bool cgpt_ex_remainingEstimated = false;
};

// Call after lv_init() and display creation. Call once.
void cgpt_ex_guiBegin();
// Zero-based slots: 0, 1, 2. Call from the LVGL/Arduino loop task only.
// Labels copy text, so the supplied strings need only live during this call.
bool cgpt_ex_guiSetPrinterStatus(uint8_t cgpt_ex_slot,
    const cgpt_ex_PrinterStatus &cgpt_ex_status);
void cgpt_ex_guiSetChipStatus(uint8_t cgpt_ex_chip,
    const char *cgpt_ex_text, bool cgpt_ex_ready);
