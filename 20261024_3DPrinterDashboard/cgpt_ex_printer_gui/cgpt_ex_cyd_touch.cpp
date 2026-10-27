#include "cgpt_ex_cyd_touch.h"
#include "cgpt_ex_config.h"
#include "XPT2046_TouchscreenSOFTSPI.h"

namespace {
SoftSPI cgpt_ex_touchSpi(cgpt_ex_touchMosi, cgpt_ex_touchMiso, cgpt_ex_touchSck);
// 255 disables interrupt wake-up. Polling continues even if an IRQ edge is missed.
XPT2046_TouchscreenSOFTSPI cgpt_ex_touch(cgpt_ex_touchCs, 255);
lv_indev_t *cgpt_ex_touchInput = nullptr;
lv_point_t cgpt_ex_touchLast = {};
bool cgpt_ex_touchWasPressed = false;
#if cgpt_ex_touchDebug
uint32_t cgpt_ex_touchLastLogMs = 0;
bool cgpt_ex_touchReaderLogged = false;
#endif

int32_t cgpt_ex_touchScale(int16_t cgpt_ex_raw, int16_t cgpt_ex_min,
    int16_t cgpt_ex_max, int32_t cgpt_ex_size) {
    const int32_t cgpt_ex_value = (int32_t(cgpt_ex_raw) - cgpt_ex_min) *
        (cgpt_ex_size - 1) / (int32_t(cgpt_ex_max) - cgpt_ex_min);
    return cgpt_ex_value < 0 ? 0 : (cgpt_ex_value >= cgpt_ex_size ? cgpt_ex_size - 1 : cgpt_ex_value);
}

void cgpt_ex_touchRead(lv_indev_t *cgpt_ex_input, lv_indev_data_t *cgpt_ex_data) {
#if cgpt_ex_touchDebug
    if (!cgpt_ex_touchReaderLogged) {
        Serial.println("cgpt_ex_: LVGL touch reader running");
        cgpt_ex_touchReaderLogged = true;
    }
#endif
    lv_display_t *cgpt_ex_display = lv_indev_get_display(cgpt_ex_input);
    int32_t cgpt_ex_width = lv_display_get_horizontal_resolution(cgpt_ex_display);
    int32_t cgpt_ex_height = lv_display_get_vertical_resolution(cgpt_ex_display);
    const lv_display_rotation_t cgpt_ex_rotation = lv_display_get_rotation(cgpt_ex_display);
    if (cgpt_ex_rotation == LV_DISPLAY_ROTATION_90 || cgpt_ex_rotation == LV_DISPLAY_ROTATION_270) {
        const int32_t cgpt_ex_swap = cgpt_ex_width;
        cgpt_ex_width = cgpt_ex_height; cgpt_ex_height = cgpt_ex_swap;
    }
    cgpt_ex_data->state = LV_INDEV_STATE_RELEASED;
    cgpt_ex_data->continue_reading = false;
    if (cgpt_ex_touch.touched(&cgpt_ex_touchSpi)) {
        const cgpt_ex_touchPoint cgpt_ex_point = cgpt_ex_touch.getPoint(&cgpt_ex_touchSpi);
        // Unrotated coordinates: LVGL applies the display rotation once.
        cgpt_ex_touchLast.x = cgpt_ex_touchScale(cgpt_ex_point.x,
            cgpt_ex_touchRawMinX, cgpt_ex_touchRawMaxX, cgpt_ex_width);
        cgpt_ex_touchLast.y = cgpt_ex_touchScale(cgpt_ex_point.y,
            cgpt_ex_touchRawMinY, cgpt_ex_touchRawMaxY, cgpt_ex_height);
        cgpt_ex_data->state = LV_INDEV_STATE_PRESSED;
#if cgpt_ex_touchDebug
        if (!cgpt_ex_touchWasPressed || uint32_t(millis() - cgpt_ex_touchLastLogMs) >= 250) {
            lv_point_t cgpt_ex_screenPoint = cgpt_ex_touchLast;
            lv_display_rotate_point(cgpt_ex_display, &cgpt_ex_screenPoint);
            Serial.printf("cgpt_ex_: touch raw=%d,%d screen=%ld,%ld pressure=%d\n",
                int(cgpt_ex_point.x), int(cgpt_ex_point.y),
                (long)cgpt_ex_screenPoint.x, (long)cgpt_ex_screenPoint.y, int(cgpt_ex_point.z));
            cgpt_ex_touchLastLogMs = millis();
        }
#endif
    }
#if cgpt_ex_touchDebug
    else if (cgpt_ex_touchWasPressed) Serial.println("cgpt_ex_: touch released");
#endif
    cgpt_ex_touchWasPressed = cgpt_ex_data->state == LV_INDEV_STATE_PRESSED;
    cgpt_ex_data->point = cgpt_ex_touchLast;
}
}

bool cgpt_ex_cydTouchBegin() {
    if (cgpt_ex_touchInput) return true;
    if (!lv_display_get_default()) return false;
    cgpt_ex_touchInput = lv_indev_create();
    if (!cgpt_ex_touchInput) return false;
    lv_indev_set_type(cgpt_ex_touchInput, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(cgpt_ex_touchInput, lv_display_get_default());
    cgpt_ex_touch.begin(&cgpt_ex_touchSpi);
    cgpt_ex_touch.setRotation(2); // Base 240x320 portrait coordinate space.
    lv_indev_set_read_cb(cgpt_ex_touchInput, cgpt_ex_touchRead);
    Serial.println("cgpt_ex_: touch ready (software SPI, polling; no IRQ required)");
    return true;
}
