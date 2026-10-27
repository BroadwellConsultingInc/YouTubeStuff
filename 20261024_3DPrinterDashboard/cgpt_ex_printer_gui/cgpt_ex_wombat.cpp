#include "cgpt_ex_wombat.h"

SerialWombatChip cgpt_ex_wombat63;
SerialWombatChip cgpt_ex_wombat64;

namespace {
bool cgpt_ex_ready[2] = {false, false};
const char *cgpt_ex_status[2] = {"Not checked", "Not checked"};
const uint8_t cgpt_ex_addresses[2] = {0x63, 0x64};
SerialWombatChip *cgpt_ex_devices[2] = {&cgpt_ex_wombat63, &cgpt_ex_wombat64};
}

void cgpt_ex_wombatBegin() {
    Serial.printf("cgpt_ex_: I2C SDA=%d SCL=%d\n", cgpt_ex_i2cSda, cgpt_ex_i2cScl);
    // Global Wire also matches the initial address probe inside the SW library.
    const bool cgpt_ex_busOK = Wire.begin(cgpt_ex_i2cSda, cgpt_ex_i2cScl, 100000);
    Wire.setTimeOut(50);
    for (uint8_t cgpt_ex_chip = 0; cgpt_ex_chip < 2; ++cgpt_ex_chip) {
        cgpt_ex_ready[cgpt_ex_chip] = false;
        if (!cgpt_ex_busOK) {
            cgpt_ex_status[cgpt_ex_chip] = "Bus error";
            continue;
        }
        Serial.printf("cgpt_ex_: checking 0x%02X\n", cgpt_ex_addresses[cgpt_ex_chip]);
        Wire.beginTransmission(cgpt_ex_addresses[cgpt_ex_chip]);
        const uint8_t cgpt_ex_probe = Wire.endTransmission();
        if (cgpt_ex_probe != 0) {
            cgpt_ex_status[cgpt_ex_chip] = "Missing";
            Serial.printf("cgpt_ex_: 0x%02X no ACK, I2C code %u\n",
                cgpt_ex_addresses[cgpt_ex_chip], unsigned(cgpt_ex_probe));
            continue;
        }
        // false avoids resetting the chip and leaves existing pin modes intact.
        const int16_t cgpt_ex_result = cgpt_ex_devices[cgpt_ex_chip]->begin(
            Wire, cgpt_ex_addresses[cgpt_ex_chip], false);
        cgpt_ex_ready[cgpt_ex_chip] = (cgpt_ex_result == 0);
        cgpt_ex_status[cgpt_ex_chip] = cgpt_ex_ready[cgpt_ex_chip] ? "Ready" : "Init error";
        Serial.printf("cgpt_ex_: 0x%02X %s, begin result %d\n",
            cgpt_ex_addresses[cgpt_ex_chip], cgpt_ex_status[cgpt_ex_chip], int(cgpt_ex_result));
    }
}

bool cgpt_ex_wombatReady(uint8_t cgpt_ex_chip) {
    return cgpt_ex_chip < 2 && cgpt_ex_ready[cgpt_ex_chip];
}

const char *cgpt_ex_wombatStatusText(uint8_t cgpt_ex_chip) {
    return cgpt_ex_chip < 2 ? cgpt_ex_status[cgpt_ex_chip] : "Invalid chip";
}
