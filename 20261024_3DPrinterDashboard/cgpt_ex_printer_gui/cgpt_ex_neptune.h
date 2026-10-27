#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>

struct cgpt_ex_NeptuneConfig {
    char cgpt_ex_name[25] = {};
    char cgpt_ex_ip[16] = {}; // Empty disables this slot.
    uint16_t cgpt_ex_port = 80;
    char cgpt_ex_apiKey[81] = {};
    uint32_t cgpt_ex_revision = 1;
    uint8_t cgpt_ex_model = 0; // 0 = Neptune 4 (HTTP), 1 = Centauri Carbon 2 (MQTT).
    char cgpt_ex_serial[65] = {};
    char cgpt_ex_accessCode[65] = {};
};

struct cgpt_ex_NeptuneStatus {
    bool cgpt_ex_online = false;
    bool cgpt_ex_hasData = false; // Last valid telemetry is retained across transport failures.
    bool cgpt_ex_printing = false;
    bool cgpt_ex_estimated = false;
    float cgpt_ex_nozzle = NAN, cgpt_ex_nozzleTarget = NAN;
    float cgpt_ex_bed = NAN, cgpt_ex_bedTarget = NAN;
    int32_t cgpt_ex_remaining = -1;
    uint32_t cgpt_ex_lastGood = 0;
    char cgpt_ex_state[24] = "Tap to set up";
    char cgpt_ex_message[96] = {};
    char cgpt_ex_linkState[24] = {}; // Latest failed request, separate from the last printer state.
};

// One no-data timeout for the GUI, time displays and PWM gauges. Failed requests
// do not change lastGood. Data expires at the 20-second boundary.
constexpr uint32_t cgpt_ex_printerTimeoutMs = 20000;
inline bool cgpt_ex_printerDataFresh(const cgpt_ex_NeptuneStatus &cgpt_ex_status, uint32_t cgpt_ex_now) {
    return (cgpt_ex_status.cgpt_ex_hasData || cgpt_ex_status.cgpt_ex_online) &&
        uint32_t(cgpt_ex_now - cgpt_ex_status.cgpt_ex_lastGood) < cgpt_ex_printerTimeoutMs;
}
inline int32_t cgpt_ex_printerRemaining(const cgpt_ex_NeptuneStatus &cgpt_ex_status, uint32_t cgpt_ex_now) {
    if (!cgpt_ex_status.cgpt_ex_printing || cgpt_ex_status.cgpt_ex_remaining < 0)
        return cgpt_ex_status.cgpt_ex_remaining;
    const uint32_t cgpt_ex_elapsed = uint32_t(cgpt_ex_now - cgpt_ex_status.cgpt_ex_lastGood) / 1000;
    return cgpt_ex_elapsed < uint32_t(cgpt_ex_status.cgpt_ex_remaining) ?
        cgpt_ex_status.cgpt_ex_remaining - cgpt_ex_elapsed : 0;
}

bool cgpt_ex_neptuneBegin();
bool cgpt_ex_neptuneSnapshot(uint8_t cgpt_ex_slot,
    cgpt_ex_NeptuneConfig &cgpt_ex_config, cgpt_ex_NeptuneStatus &cgpt_ex_status);
bool cgpt_ex_neptuneSave(uint8_t cgpt_ex_slot, const cgpt_ex_NeptuneConfig &cgpt_ex_config,
    char *cgpt_ex_error, size_t cgpt_ex_errorSize);

// Parser is independent of LVGL and networking for fixture testing.
bool cgpt_ex_neptuneParse(const char *cgpt_ex_json, cgpt_ex_NeptuneStatus &cgpt_ex_status);
