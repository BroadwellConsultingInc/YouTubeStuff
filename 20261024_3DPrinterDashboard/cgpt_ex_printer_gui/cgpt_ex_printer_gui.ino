#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "cgpt_ex_gui.h"
#include "cgpt_ex_wombat.h"
#include "cgpt_ex_tm1637.h"
#include "cgpt_ex_gauges.h"
#include "cgpt_ex_cyd_touch.h"
#include "cgpt_ex_wifi.h"
#include "cgpt_ex_wifi_gui.h"
#include "cgpt_ex_neptune.h"
#include "cgpt_ex_neptune_gui.h"

#if defined(BOARD_HAS_DUAL_ANTENNA) || defined(ARDUINO_ESP32_WROOM_DA)
#error "Select ESP32 Dev Module. WROOM-DA WiFi uses GPIO2/GPIO25, conflicting with CYD display/touch."
#endif

#if LVGL_VERSION_MAJOR != 9 || LVGL_VERSION_MINOR != 6
#error "This example targets LVGL 9.6."
#endif
#if !LV_USE_TFT_ESPI
#error "Enable LV_USE_TFT_ESPI in your working lv_conf.h."
#endif

// Same display approach as the working CYD sketch: 240x320, rotated 270.
// Buffer size is BYTES, not the number of pixels.
alignas(64) static uint8_t cgpt_ex_drawBuffer[320 * 10 * 2];
static uint32_t cgpt_ex_lastTickMs = 0;

// Arduino requires the names setup() and loop(). All example-owned names
// other than those two entry points have the cgpt_ex_ prefix.
void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("cgpt_ex_: starting LVGL");
    lv_init();

    lv_display_t *cgpt_ex_display = lv_tft_espi_create(
        240, 320, cgpt_ex_drawBuffer, sizeof(cgpt_ex_drawBuffer));
    if (!cgpt_ex_display) {
        Serial.println("cgpt_ex_: display allocation failed");
        return;
    }
    lv_display_set_color_format(cgpt_ex_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_rotation(cgpt_ex_display, LV_DISPLAY_ROTATION_270);
    lv_display_set_buffers(cgpt_ex_display, cgpt_ex_drawBuffer, nullptr,
        sizeof(cgpt_ex_drawBuffer), LV_DISPLAY_RENDER_MODE_PARTIAL);

    cgpt_ex_guiBegin();
    lv_tick_inc(1);
    lv_refr_now(cgpt_ex_display); // First frame BEFORE any I2C operations.
    Serial.println("cgpt_ex_: GUI first frame sent");
    if (!cgpt_ex_cydTouchBegin()) Serial.println("cgpt_ex_: touch initialization failed");

    cgpt_ex_wombatBegin();
    cgpt_ex_tm1637Begin();
    cgpt_ex_gaugesBegin();
    for (uint8_t cgpt_ex_index = 0; cgpt_ex_index < 2; ++cgpt_ex_index) {
        cgpt_ex_guiSetChipStatus(cgpt_ex_index,
            cgpt_ex_wombatStatusText(cgpt_ex_index),
            cgpt_ex_wombatReady(cgpt_ex_index));
    }
    lv_refr_now(cgpt_ex_display);
    cgpt_ex_wifiBegin();
    cgpt_ex_wifiGuiService();
    if (!cgpt_ex_neptuneBegin()) Serial.println("cgpt_ex_: Printer worker allocation failed");
    cgpt_ex_lastTickMs = millis();
    Serial.println("cgpt_ex_: setup complete");
}

void loop() {
    const uint32_t cgpt_ex_nowMs = millis();
    lv_tick_inc(cgpt_ex_nowMs - cgpt_ex_lastTickMs);
    cgpt_ex_lastTickMs = cgpt_ex_nowMs;
    cgpt_ex_wifiService();
    cgpt_ex_wifiGuiService();
    cgpt_ex_neptuneGuiService();
    cgpt_ex_tm1637Service();
    cgpt_ex_gaugesService();
    lv_timer_handler();
    delay(5);
}
