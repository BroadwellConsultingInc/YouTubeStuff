#include "cgpt_ex_tm1637.h"
#include "cgpt_ex_wombat.h"
#include <SerialWombatTM1637.h>
#include <WiFi.h>
#include <stdio.h>
#include <string.h>

namespace {
struct cgpt_ex_TM1637Wiring {
    uint8_t cgpt_ex_chip; // 0 = 0x63; 1 = 0x64.
    uint8_t cgpt_ex_clkPin;
    uint8_t cgpt_ex_dioPin;
};
// Serial Wombat pin numbers, NOT ESP32 GPIO numbers. One row per printer.
// Change these assignments to match your display wiring.
constexpr cgpt_ex_TM1637Wiring cgpt_ex_tmWiring[3] = {
    {0, 0, 1}, // Printer 1: 0x63 CLK0 / DIO1.
    {0, 2, 3}, // Printer 2: 0x63 CLK2 / DIO3.
    {1, 0, 1}  // Printer 3: 0x64 CLK0 / DIO1.
};
constexpr uint8_t cgpt_ex_tmBrightness = 0; // 0 (dimmest) through 7.
SerialWombatTM1637 cgpt_ex_tmDisplay1(
    cgpt_ex_tmWiring[0].cgpt_ex_chip ? cgpt_ex_wombat64 : cgpt_ex_wombat63);
SerialWombatTM1637 cgpt_ex_tmDisplay2(
    cgpt_ex_tmWiring[1].cgpt_ex_chip ? cgpt_ex_wombat64 : cgpt_ex_wombat63);
SerialWombatTM1637 cgpt_ex_tmDisplay3(
    cgpt_ex_tmWiring[2].cgpt_ex_chip ? cgpt_ex_wombat64 : cgpt_ex_wombat63);
SerialWombatTM1637 *cgpt_ex_tmDisplays[3] = {
    &cgpt_ex_tmDisplay1, &cgpt_ex_tmDisplay2, &cgpt_ex_tmDisplay3
};
bool cgpt_ex_tmReady[3] = {}, cgpt_ex_tmWriteFailed[3] = {};
char cgpt_ex_tmLastText[3][7] = {};
uint8_t cgpt_ex_tmLastDecimal[3] = {};
uint32_t cgpt_ex_tmLastService = 0;

void cgpt_ex_tmWrite(uint8_t cgpt_ex_slot, const char cgpt_ex_text[7]) {
    if (!cgpt_ex_tmReady[cgpt_ex_slot] || !strcmp(cgpt_ex_tmLastText[cgpt_ex_slot], cgpt_ex_text)) return;
    uint8_t cgpt_ex_characters[6];
    memcpy(cgpt_ex_characters, cgpt_ex_text, sizeof(cgpt_ex_characters));
    // Logical digit bits 1 and 3: HH.MM.SS. Keep words and dashes free of points.
    bool cgpt_ex_numeric = true;
    for (uint8_t cgpt_ex_digit = 0; cgpt_ex_digit < 6; ++cgpt_ex_digit)
        if (cgpt_ex_text[cgpt_ex_digit] < '0' || cgpt_ex_text[cgpt_ex_digit] > '9') cgpt_ex_numeric = false;
    const uint8_t cgpt_ex_decimal = cgpt_ex_numeric ? 0x0A : 0;
    int16_t cgpt_ex_result = 0;
    if (cgpt_ex_decimal != cgpt_ex_tmLastDecimal[cgpt_ex_slot]) {
        cgpt_ex_result = cgpt_ex_tmDisplays[cgpt_ex_slot]->writeDecimalBitmap(cgpt_ex_decimal);
        if (cgpt_ex_result >= 0) cgpt_ex_tmLastDecimal[cgpt_ex_slot] = cgpt_ex_decimal;
    }
    if (cgpt_ex_result >= 0) cgpt_ex_result = cgpt_ex_tmDisplays[cgpt_ex_slot]->writeArray(cgpt_ex_characters);
    if (cgpt_ex_result < 0) {
        // Leave the cache unchanged so the next service call retries the complete array.
        if (!cgpt_ex_tmWriteFailed[cgpt_ex_slot])
            Serial.printf("cgpt_ex_: TM1637 printer %u write failed (%d)\n",
                unsigned(cgpt_ex_slot + 1), int(cgpt_ex_result));
        cgpt_ex_tmWriteFailed[cgpt_ex_slot] = true;
        return;
    }
    if (cgpt_ex_tmWriteFailed[cgpt_ex_slot])
        Serial.printf("cgpt_ex_: TM1637 printer %u write recovered\n", unsigned(cgpt_ex_slot + 1));
    cgpt_ex_tmWriteFailed[cgpt_ex_slot] = false;
    memcpy(cgpt_ex_tmLastText[cgpt_ex_slot], cgpt_ex_text, 7);
}
} // namespace

void cgpt_ex_tm1637Begin() {
    for (uint8_t cgpt_ex_slot = 0; cgpt_ex_slot < 3; ++cgpt_ex_slot) {
        cgpt_ex_tmReady[cgpt_ex_slot] = false;
        cgpt_ex_tmWriteFailed[cgpt_ex_slot] = false;
        cgpt_ex_tmLastText[cgpt_ex_slot][0] = 0;
        cgpt_ex_tmLastDecimal[cgpt_ex_slot] = 0;
        const cgpt_ex_TM1637Wiring &cgpt_ex_wiring = cgpt_ex_tmWiring[cgpt_ex_slot];
        if (!cgpt_ex_wombatReady(cgpt_ex_wiring.cgpt_ex_chip)) continue;
        SerialWombatTM1637 &cgpt_ex_display = *cgpt_ex_tmDisplays[cgpt_ex_slot];
        int16_t cgpt_ex_result = cgpt_ex_display.begin(cgpt_ex_wiring.cgpt_ex_clkPin,
            cgpt_ex_wiring.cgpt_ex_dioPin, 6, tm1637CharArray, 0x55, cgpt_ex_tmBrightness);
        // The Arduino library calls this writeDigitOrder (not writeDigitalOrder).
        if (cgpt_ex_result >= 0) cgpt_ex_result = cgpt_ex_display.writeDigitOrder(2, 1, 0, 5, 4, 3);
        if (cgpt_ex_result >= 0) cgpt_ex_result = cgpt_ex_display.writeDecimalBitmap(0);
        if (cgpt_ex_result >= 0) cgpt_ex_result = cgpt_ex_display.writeBlinkBitmap(0);
        cgpt_ex_tmReady[cgpt_ex_slot] = cgpt_ex_result >= 0;
        Serial.printf("cgpt_ex_: TM1637 printer %u 0x%02X CLK%u DIO%u: %s (%d)\n",
            unsigned(cgpt_ex_slot + 1), unsigned(cgpt_ex_wiring.cgpt_ex_chip ? 0x64 : 0x63),
            unsigned(cgpt_ex_wiring.cgpt_ex_clkPin), unsigned(cgpt_ex_wiring.cgpt_ex_dioPin),
            cgpt_ex_tmReady[cgpt_ex_slot] ? "Ready" : "Init error", int(cgpt_ex_result));
        cgpt_ex_tmWrite(cgpt_ex_slot, " STOP ");
    }
    // First loop service immediately replaces the startup message with current status.
    cgpt_ex_tmLastService = millis() - 500;
}

void cgpt_ex_tm1637Format(const cgpt_ex_NeptuneConfig &cgpt_ex_config,
    const cgpt_ex_NeptuneStatus &cgpt_ex_status, bool,
    uint32_t cgpt_ex_now, char cgpt_ex_text[7]) {
    memcpy(cgpt_ex_text, " STOP ", 7);
    if (!cgpt_ex_config.cgpt_ex_ip[0]) return; // Disabled printer slot.
    if (!cgpt_ex_printerDataFresh(cgpt_ex_status, cgpt_ex_now)) {
        memcpy(cgpt_ex_text, "   -  ", 7); // Connection unknown: dash in the minutes ones position.
        return;
    }
    if (!strcmp(cgpt_ex_status.cgpt_ex_state, "Paused")) {
        memcpy(cgpt_ex_text, "PAUSE ", 7);
        return;
    }
    if (!cgpt_ex_status.cgpt_ex_printing) return;
    if (cgpt_ex_status.cgpt_ex_remaining < 0) {
        memcpy(cgpt_ex_text, "------", 7); // A running printer may not yet have an estimate.
        return;
    }
    const uint32_t cgpt_ex_seconds = uint32_t(cgpt_ex_printerRemaining(cgpt_ex_status, cgpt_ex_now));
    if (cgpt_ex_seconds > 359999) { // HHMMSS can represent at most 99:59:59.
        memcpy(cgpt_ex_text, "------", 7);
        return;
    }
    snprintf(cgpt_ex_text, 7, "%02lu%02lu%02lu", (unsigned long)(cgpt_ex_seconds / 3600),
        (unsigned long)((cgpt_ex_seconds / 60) % 60), (unsigned long)(cgpt_ex_seconds % 60));
}

void cgpt_ex_tm1637Service() {
    const uint32_t cgpt_ex_now = millis();
    if (uint32_t(cgpt_ex_now - cgpt_ex_tmLastService) < 500) return;
    cgpt_ex_tmLastService = cgpt_ex_now;
    for (uint8_t cgpt_ex_slot = 0; cgpt_ex_slot < 3; ++cgpt_ex_slot) {
        if (!cgpt_ex_tmReady[cgpt_ex_slot] || !cgpt_ex_wombatReady(cgpt_ex_tmWiring[cgpt_ex_slot].cgpt_ex_chip)) continue;
        cgpt_ex_NeptuneConfig cgpt_ex_config;
        cgpt_ex_NeptuneStatus cgpt_ex_status;
        if (!cgpt_ex_neptuneSnapshot(cgpt_ex_slot, cgpt_ex_config, cgpt_ex_status)) continue;
        char cgpt_ex_text[7];
        cgpt_ex_tm1637Format(cgpt_ex_config, cgpt_ex_status, WiFi.status() == WL_CONNECTED,
            cgpt_ex_now, cgpt_ex_text);
        cgpt_ex_tmWrite(cgpt_ex_slot, cgpt_ex_text);
    }
}
