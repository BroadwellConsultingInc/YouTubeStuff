#include "cgpt_ex_gui.h"
#include <lvgl.h>
#include <stdio.h>
#include "cgpt_ex_wifi_gui.h"
#include "cgpt_ex_neptune_gui.h"

namespace {
lv_obj_t *cgpt_ex_names[3] = {};
lv_obj_t *cgpt_ex_states[3] = {};
lv_obj_t *cgpt_ex_nozzles[3] = {};
lv_obj_t *cgpt_ex_beds[3] = {};
lv_obj_t *cgpt_ex_times[3] = {};
lv_obj_t *cgpt_ex_chips[2] = {};

lv_obj_t *cgpt_ex_label(lv_obj_t *cgpt_ex_parent, int32_t cgpt_ex_x,
    int32_t cgpt_ex_y, int32_t cgpt_ex_width, const char *cgpt_ex_text) {
    lv_obj_t *cgpt_ex_result = lv_label_create(cgpt_ex_parent);
    lv_obj_set_pos(cgpt_ex_result, cgpt_ex_x, cgpt_ex_y);
    lv_obj_set_width(cgpt_ex_result, cgpt_ex_width);
    lv_label_set_long_mode(cgpt_ex_result, LV_LABEL_LONG_MODE_CLIP);
    lv_label_set_text(cgpt_ex_result, cgpt_ex_text);
    return cgpt_ex_result;
}

void cgpt_ex_temperature(float cgpt_ex_value, char *cgpt_ex_text,
    size_t cgpt_ex_capacity) {
    if (isfinite(cgpt_ex_value) && cgpt_ex_value >= -99 && cgpt_ex_value <= 999) {
        snprintf(cgpt_ex_text, cgpt_ex_capacity, "%.0f", double(cgpt_ex_value));
    } else {
        snprintf(cgpt_ex_text, cgpt_ex_capacity, "--");
    }
}

void cgpt_ex_heaterLabel(lv_obj_t *cgpt_ex_obj, const char *cgpt_ex_prefix,
    float cgpt_ex_actual, float cgpt_ex_target) {
    char cgpt_ex_actualText[12], cgpt_ex_targetText[12], cgpt_ex_line[40];
    cgpt_ex_temperature(cgpt_ex_actual, cgpt_ex_actualText, sizeof(cgpt_ex_actualText));
    cgpt_ex_temperature(cgpt_ex_target, cgpt_ex_targetText, sizeof(cgpt_ex_targetText));
    snprintf(cgpt_ex_line, sizeof(cgpt_ex_line), "%s %s/%s",
        cgpt_ex_prefix, cgpt_ex_actualText, cgpt_ex_targetText);
    lv_label_set_text(cgpt_ex_obj, cgpt_ex_line);
}
} // namespace

void cgpt_ex_guiBegin() {
    lv_obj_t *cgpt_ex_screen = lv_screen_active();
    lv_obj_remove_style_all(cgpt_ex_screen);
    lv_obj_set_style_bg_color(cgpt_ex_screen, lv_color_hex(0x101923), 0);
    lv_obj_set_style_bg_opa(cgpt_ex_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(cgpt_ex_screen, lv_color_hex(0xF0F4F8), 0);
    lv_obj_set_style_text_font(cgpt_ex_screen, LV_FONT_DEFAULT, 0);
    lv_obj_set_scrollable(cgpt_ex_screen, false);
    cgpt_ex_label(cgpt_ex_screen, 8, 5, 225, "PRINTER STATUS");
    lv_obj_t *cgpt_ex_hint = cgpt_ex_label(cgpt_ex_screen, 8, 22, 118,
        "Actual/target C");
    lv_obj_set_style_text_color(cgpt_ex_hint, lv_color_hex(0xACBACB), 0);

    for (uint8_t cgpt_ex_slot = 0; cgpt_ex_slot < 3; ++cgpt_ex_slot) {
        lv_obj_t *cgpt_ex_card = lv_obj_create(cgpt_ex_screen);
        lv_obj_remove_style_all(cgpt_ex_card);
        lv_obj_set_pos(cgpt_ex_card, 8, 42 + cgpt_ex_slot * 58);
        lv_obj_set_size(cgpt_ex_card, 304, 52);
        lv_obj_set_scrollable(cgpt_ex_card, false);
        lv_obj_set_style_bg_color(cgpt_ex_card, lv_color_hex(0x203042), 0);
        lv_obj_set_style_bg_opa(cgpt_ex_card, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(cgpt_ex_card, 5, 0);
        cgpt_ex_names[cgpt_ex_slot] = cgpt_ex_label(cgpt_ex_card, 7, 4, 164, "");
        cgpt_ex_states[cgpt_ex_slot] = cgpt_ex_label(cgpt_ex_card, 174, 4, 123, "");
        lv_obj_set_style_text_align(cgpt_ex_states[cgpt_ex_slot], LV_TEXT_ALIGN_RIGHT, 0);
        cgpt_ex_nozzles[cgpt_ex_slot] = cgpt_ex_label(cgpt_ex_card, 7, 29, 112, "");
        cgpt_ex_beds[cgpt_ex_slot] = cgpt_ex_label(cgpt_ex_card, 121, 29, 91, "");
        cgpt_ex_times[cgpt_ex_slot] = cgpt_ex_label(cgpt_ex_card, 215, 29, 82, "");
        char cgpt_ex_name[20];
        snprintf(cgpt_ex_name, sizeof(cgpt_ex_name), "Printer %u", unsigned(cgpt_ex_slot + 1));
        cgpt_ex_PrinterStatus cgpt_ex_status;
        cgpt_ex_status.cgpt_ex_name = cgpt_ex_name;
        cgpt_ex_guiSetPrinterStatus(cgpt_ex_slot, cgpt_ex_status);
        cgpt_ex_neptuneGuiBind(cgpt_ex_card, cgpt_ex_slot);
    }
    cgpt_ex_chips[0] = cgpt_ex_label(cgpt_ex_screen, 8, 222, 150, "0x63: Checking");
    cgpt_ex_chips[1] = cgpt_ex_label(cgpt_ex_screen, 168, 222, 144, "0x64: Checking");
    cgpt_ex_wifiGuiBegin(cgpt_ex_screen);
}



bool cgpt_ex_guiSetPrinterStatus(uint8_t cgpt_ex_slot,
    const cgpt_ex_PrinterStatus &cgpt_ex_status) {
    if (cgpt_ex_slot >= 3 || !cgpt_ex_names[cgpt_ex_slot]) return false;
    lv_label_set_text(cgpt_ex_names[cgpt_ex_slot],
        cgpt_ex_status.cgpt_ex_name ? cgpt_ex_status.cgpt_ex_name : "Printer");
    lv_label_set_text(cgpt_ex_states[cgpt_ex_slot],
        cgpt_ex_status.cgpt_ex_state ? cgpt_ex_status.cgpt_ex_state : "No data");
    lv_obj_set_style_text_color(cgpt_ex_states[cgpt_ex_slot], lv_color_hex(
        cgpt_ex_status.cgpt_ex_online ? 0x60E0B0 : 0xACBACB), 0);
    cgpt_ex_heaterLabel(cgpt_ex_nozzles[cgpt_ex_slot], "Noz",
        cgpt_ex_status.cgpt_ex_online ? cgpt_ex_status.cgpt_ex_nozzleC : NAN,
        cgpt_ex_status.cgpt_ex_online ? cgpt_ex_status.cgpt_ex_nozzleTargetC : NAN);
    cgpt_ex_heaterLabel(cgpt_ex_beds[cgpt_ex_slot], "Bed",
        cgpt_ex_status.cgpt_ex_online ? cgpt_ex_status.cgpt_ex_bedC : NAN,
        cgpt_ex_status.cgpt_ex_online ? cgpt_ex_status.cgpt_ex_bedTargetC : NAN);
    char cgpt_ex_time[24] = "Left --:--";
    if (cgpt_ex_status.cgpt_ex_online && cgpt_ex_status.cgpt_ex_remainingSeconds >= 0) {
        const uint32_t cgpt_ex_minutes =
            (uint32_t(cgpt_ex_status.cgpt_ex_remainingSeconds) + 59U) / 60U;
        if (cgpt_ex_minutes >= 6000) snprintf(cgpt_ex_time, sizeof(cgpt_ex_time), "Left >99h");
        else snprintf(cgpt_ex_time, sizeof(cgpt_ex_time), "%s%02lu:%02lu",
            cgpt_ex_status.cgpt_ex_remainingEstimated ? "Left~" : "Left ",
            (unsigned long)(cgpt_ex_minutes / 60), (unsigned long)(cgpt_ex_minutes % 60));
    }
    lv_label_set_text(cgpt_ex_times[cgpt_ex_slot], cgpt_ex_time);
    return true;
}

void cgpt_ex_guiSetChipStatus(uint8_t cgpt_ex_chip,
    const char *cgpt_ex_text, bool cgpt_ex_ready) {
    if (cgpt_ex_chip >= 2 || !cgpt_ex_chips[cgpt_ex_chip]) return;
    char cgpt_ex_line[40];
    snprintf(cgpt_ex_line, sizeof(cgpt_ex_line), "0x%02X: %s",
        unsigned(cgpt_ex_chip == 0 ? 0x63 : 0x64), cgpt_ex_text ? cgpt_ex_text : "Unknown");
    lv_label_set_text(cgpt_ex_chips[cgpt_ex_chip], cgpt_ex_line);
    lv_obj_set_style_text_color(cgpt_ex_chips[cgpt_ex_chip],
        lv_color_hex(cgpt_ex_ready ? 0x60E0B0 : 0xFFBC66), 0);
}
