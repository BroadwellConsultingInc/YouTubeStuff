#include "cgpt_ex_gauges.h"
#include "cgpt_ex_wombat.h"
#include "cgpt_ex_neptune.h"
#include <SerialWombatPWM.h>
#include <math.h>

namespace {
struct cgpt_ex_GaugeWiring {
    uint8_t cgpt_ex_chip; // 0 = 0x63; 1 = 0x64.
    uint8_t cgpt_ex_pin;  // Serial Wombat logical pin, NOT ESP32 GPIO.
    uint8_t cgpt_ex_printer;
    bool cgpt_ex_nozzle;
};
// Existing TM1637 pins remain 0x63:0/1,2/3 and 0x64:0/1.
// Edit this table if gauge wiring differs; PWM objects follow the selected chip.
constexpr cgpt_ex_GaugeWiring cgpt_ex_gaugeWiring[6] = {
    {0, 4, 0, false}, {0, 5, 0, true}, // Printer 1 bed / nozzle.
    {1, 2, 1, false}, {1, 3, 1, true}, // Printer 2 bed / nozzle.
    {1, 4, 2, false}, {1, 5, 2, true}  // Printer 3 bed / nozzle.
};
constexpr float cgpt_ex_gaugeBedC[6] = {20, 40, 50, 60, 80, 125};
constexpr float cgpt_ex_gaugeNozzleC[6] = {20, 180, 200, 220, 240, 280};
constexpr float cgpt_ex_gaugeCalibration_mV[6] = {0, 724, 1360, 2030, 2640, 3280};
constexpr uint32_t cgpt_ex_gaugeUpdateMs = 500;
constexpr uint32_t cgpt_ex_gaugeSupplyUpdateMs = 1000;
// Reject zero/implausible measurements instead of calculating from a guessed VDD.
constexpr uint16_t cgpt_ex_gaugeMinSupply_mV = 2000;
constexpr uint16_t cgpt_ex_gaugeMaxSupply_mV = 5500;
SerialWombatChip *cgpt_ex_gaugeChips[2] = {&cgpt_ex_wombat63, &cgpt_ex_wombat64};
SerialWombatPWM cgpt_ex_gaugePWM1(*cgpt_ex_gaugeChips[cgpt_ex_gaugeWiring[0].cgpt_ex_chip]);
SerialWombatPWM cgpt_ex_gaugePWM2(*cgpt_ex_gaugeChips[cgpt_ex_gaugeWiring[1].cgpt_ex_chip]);
SerialWombatPWM cgpt_ex_gaugePWM3(*cgpt_ex_gaugeChips[cgpt_ex_gaugeWiring[2].cgpt_ex_chip]);
SerialWombatPWM cgpt_ex_gaugePWM4(*cgpt_ex_gaugeChips[cgpt_ex_gaugeWiring[3].cgpt_ex_chip]);
SerialWombatPWM cgpt_ex_gaugePWM5(*cgpt_ex_gaugeChips[cgpt_ex_gaugeWiring[4].cgpt_ex_chip]);
SerialWombatPWM cgpt_ex_gaugePWM6(*cgpt_ex_gaugeChips[cgpt_ex_gaugeWiring[5].cgpt_ex_chip]);
SerialWombatPWM *cgpt_ex_gaugePWMs[6] = {&cgpt_ex_gaugePWM1, &cgpt_ex_gaugePWM2,
    &cgpt_ex_gaugePWM3, &cgpt_ex_gaugePWM4, &cgpt_ex_gaugePWM5, &cgpt_ex_gaugePWM6};
bool cgpt_ex_gaugeReady[6] = {};
uint16_t cgpt_ex_gaugeSupply_mV[2] = {};
bool cgpt_ex_gaugeSupplyWasInvalid[2] = {};
uint32_t cgpt_ex_gaugeLastUpdate = 0, cgpt_ex_gaugeLastSupplyUpdate = 0;

void cgpt_ex_gaugeReadSupplies() {
    for (uint8_t cgpt_ex_chip = 0; cgpt_ex_chip < 2; ++cgpt_ex_chip) {
        cgpt_ex_gaugeSupply_mV[cgpt_ex_chip] = 0;
        if (!cgpt_ex_wombatReady(cgpt_ex_chip)) continue;
        SerialWombatChip &cgpt_ex_device = *cgpt_ex_gaugeChips[cgpt_ex_chip];
        const uint16_t cgpt_ex_errorsBefore = cgpt_ex_device.errorCount;
        const uint16_t cgpt_ex_supply = cgpt_ex_device.readSupplyVoltage_mV();
        const bool cgpt_ex_valid = cgpt_ex_device.errorCount == cgpt_ex_errorsBefore &&
            cgpt_ex_supply >= cgpt_ex_gaugeMinSupply_mV && cgpt_ex_supply <= cgpt_ex_gaugeMaxSupply_mV;
        if (cgpt_ex_valid) {
            cgpt_ex_gaugeSupply_mV[cgpt_ex_chip] = cgpt_ex_supply;
            if (cgpt_ex_gaugeSupplyWasInvalid[cgpt_ex_chip])
                Serial.printf("cgpt_ex_: gauge VDD 0x%02X recovered: %u mV\n",
                    unsigned(cgpt_ex_chip ? 0x64 : 0x63), unsigned(cgpt_ex_supply));
        } else if (!cgpt_ex_gaugeSupplyWasInvalid[cgpt_ex_chip]) {
            Serial.printf("cgpt_ex_: gauge VDD 0x%02X invalid/read error (%u mV); requesting zero duty\n",
                unsigned(cgpt_ex_chip ? 0x64 : 0x63), unsigned(cgpt_ex_supply));
        }
        cgpt_ex_gaugeSupplyWasInvalid[cgpt_ex_chip] = !cgpt_ex_valid;
    }
}
} // namespace

float cgpt_ex_gaugeMillivolts(float cgpt_ex_temperatureC, bool cgpt_ex_nozzle) {
    if (!isfinite(cgpt_ex_temperatureC)) return 0;
    const float *cgpt_ex_temperatures = cgpt_ex_nozzle ? cgpt_ex_gaugeNozzleC : cgpt_ex_gaugeBedC;
    if (cgpt_ex_temperatureC <= cgpt_ex_temperatures[0]) return 0;
    for (uint8_t cgpt_ex_point = 1; cgpt_ex_point < 6; ++cgpt_ex_point) {
        if (cgpt_ex_temperatureC <= cgpt_ex_temperatures[cgpt_ex_point]) {
            const float cgpt_ex_fraction = (cgpt_ex_temperatureC - cgpt_ex_temperatures[cgpt_ex_point - 1]) /
                (cgpt_ex_temperatures[cgpt_ex_point] - cgpt_ex_temperatures[cgpt_ex_point - 1]);
            return cgpt_ex_gaugeCalibration_mV[cgpt_ex_point - 1] + cgpt_ex_fraction *
                (cgpt_ex_gaugeCalibration_mV[cgpt_ex_point] - cgpt_ex_gaugeCalibration_mV[cgpt_ex_point - 1]);
        }
    }
    return cgpt_ex_gaugeCalibration_mV[5]; // Clamp to the final calibrated voltage.
}

uint16_t cgpt_ex_gaugeDuty(float cgpt_ex_temperatureC, bool cgpt_ex_nozzle,
    uint16_t cgpt_ex_supplyMillivolts) {
    if (cgpt_ex_supplyMillivolts < cgpt_ex_gaugeMinSupply_mV ||
        cgpt_ex_supplyMillivolts > cgpt_ex_gaugeMaxSupply_mV) return 0;
    const float cgpt_ex_millivolts = cgpt_ex_gaugeMillivolts(cgpt_ex_temperatureC, cgpt_ex_nozzle);
    if (cgpt_ex_millivolts >= cgpt_ex_supplyMillivolts) return 65535;
    return uint16_t(cgpt_ex_millivolts * 65535.0f / cgpt_ex_supplyMillivolts + 0.5f);
}

void cgpt_ex_gaugesBegin() {
    for (uint8_t cgpt_ex_channel = 0; cgpt_ex_channel < 6; ++cgpt_ex_channel) {
        cgpt_ex_gaugeReady[cgpt_ex_channel] = false;
        const cgpt_ex_GaugeWiring &cgpt_ex_wiring = cgpt_ex_gaugeWiring[cgpt_ex_channel];
        if (!cgpt_ex_wombatReady(cgpt_ex_wiring.cgpt_ex_chip)) continue;
        const int16_t cgpt_ex_result = cgpt_ex_gaugePWMs[cgpt_ex_channel]->begin(cgpt_ex_wiring.cgpt_ex_pin, 0, false);
        cgpt_ex_gaugeReady[cgpt_ex_channel] = cgpt_ex_result >= 0;
        Serial.printf("cgpt_ex_: gauge printer %u %s 0x%02X pin %u: %s (%d)\n",
            unsigned(cgpt_ex_wiring.cgpt_ex_printer + 1), cgpt_ex_wiring.cgpt_ex_nozzle ? "nozzle" : "bed",
            unsigned(cgpt_ex_wiring.cgpt_ex_chip ? 0x64 : 0x63), unsigned(cgpt_ex_wiring.cgpt_ex_pin),
            cgpt_ex_gaugeReady[cgpt_ex_channel] ? "Ready" : "Init error", int(cgpt_ex_result));
    }
    cgpt_ex_gaugeSupplyWasInvalid[0] = cgpt_ex_gaugeSupplyWasInvalid[1] = false;
    cgpt_ex_gaugeReadSupplies();
    cgpt_ex_gaugeLastSupplyUpdate = millis();
    cgpt_ex_gaugeLastUpdate = millis() - cgpt_ex_gaugeUpdateMs;
}

void cgpt_ex_gaugesService() {
    const uint32_t cgpt_ex_now = millis();
    if (uint32_t(cgpt_ex_now - cgpt_ex_gaugeLastUpdate) < cgpt_ex_gaugeUpdateMs) return;
    cgpt_ex_gaugeLastUpdate = cgpt_ex_now;
    if (uint32_t(cgpt_ex_now - cgpt_ex_gaugeLastSupplyUpdate) >= cgpt_ex_gaugeSupplyUpdateMs) {
        cgpt_ex_gaugeReadSupplies();
        cgpt_ex_gaugeLastSupplyUpdate = cgpt_ex_now;
    }
    for (uint8_t cgpt_ex_printer = 0; cgpt_ex_printer < 3; ++cgpt_ex_printer) {
        cgpt_ex_NeptuneConfig cgpt_ex_config;
        cgpt_ex_NeptuneStatus cgpt_ex_status;
        // If the worker owns the snapshot mutex, try again next time; do not invent a temperature.
        if (!cgpt_ex_neptuneSnapshot(cgpt_ex_printer, cgpt_ex_config, cgpt_ex_status)) continue;
        const bool cgpt_ex_fresh = cgpt_ex_config.cgpt_ex_ip[0] &&
            cgpt_ex_printerDataFresh(cgpt_ex_status, cgpt_ex_now);
        for (uint8_t cgpt_ex_channel = 0; cgpt_ex_channel < 6; ++cgpt_ex_channel) {
            const cgpt_ex_GaugeWiring &cgpt_ex_wiring = cgpt_ex_gaugeWiring[cgpt_ex_channel];
            if (cgpt_ex_wiring.cgpt_ex_printer != cgpt_ex_printer || !cgpt_ex_gaugeReady[cgpt_ex_channel] ||
                !cgpt_ex_wombatReady(cgpt_ex_wiring.cgpt_ex_chip)) continue;
            const float cgpt_ex_temperature = cgpt_ex_wiring.cgpt_ex_nozzle ? cgpt_ex_status.cgpt_ex_nozzle : cgpt_ex_status.cgpt_ex_bed;
            const uint16_t cgpt_ex_duty = cgpt_ex_fresh ? cgpt_ex_gaugeDuty(cgpt_ex_temperature,
                cgpt_ex_wiring.cgpt_ex_nozzle, cgpt_ex_gaugeSupply_mV[cgpt_ex_wiring.cgpt_ex_chip]) : 0;
            // Refresh every 500 ms, including unchanged values: writeDutyCycle returns void,
            // so a lost packet must be retried rather than hidden by a success cache.
            cgpt_ex_gaugePWMs[cgpt_ex_channel]->writeDutyCycle(cgpt_ex_duty);
        }
    }
}
