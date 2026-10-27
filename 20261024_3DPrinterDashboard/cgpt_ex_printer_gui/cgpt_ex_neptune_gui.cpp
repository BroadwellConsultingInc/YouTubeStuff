#include "cgpt_ex_neptune_gui.h"
#include "cgpt_ex_neptune.h"
#include "cgpt_ex_gui.h"
#include <Arduino.h>
#include <WiFi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace {
lv_obj_t *cgpt_ex_npPage = nullptr, *cgpt_ex_npTitle = nullptr;
lv_obj_t *cgpt_ex_npFields[5] = {};
lv_obj_t *cgpt_ex_npKeyboard = nullptr, *cgpt_ex_npCaption = nullptr;
lv_obj_t *cgpt_ex_npSaveButton = nullptr, *cgpt_ex_npClearButton = nullptr;
lv_obj_t *cgpt_ex_npMessage = nullptr, *cgpt_ex_npNote = nullptr;
lv_obj_t *cgpt_ex_npModel = nullptr;
uint8_t cgpt_ex_npSelectedModel = 0;
char cgpt_ex_npApiKey[81] = {}, cgpt_ex_npAccessCode[65] = {};
uint8_t cgpt_ex_npSlot = 0;
uint32_t cgpt_ex_npLastRefresh = 0;
constexpr int32_t cgpt_ex_npX[5] = {8, 8, 222, 8, 8};
constexpr int32_t cgpt_ex_npY[5] = {40, 76, 76, 148, 112};
constexpr int32_t cgpt_ex_npWidth[5] = {150, 206, 90, 304, 304};
const char *cgpt_ex_npCaptions[5] = {"Printer name", "Printer IPv4 address", "Printer port", "API key (optional)", "CC2 serial number (SN)"};

lv_obj_t *cgpt_ex_npLabel(lv_obj_t *cgpt_ex_parent, int32_t cgpt_ex_x,
    int32_t cgpt_ex_y, int32_t cgpt_ex_width, const char *cgpt_ex_text) {
    lv_obj_t *cgpt_ex_obj = lv_label_create(cgpt_ex_parent);
    lv_obj_set_pos(cgpt_ex_obj, cgpt_ex_x, cgpt_ex_y);
    lv_obj_set_width(cgpt_ex_obj, cgpt_ex_width);
    lv_label_set_long_mode(cgpt_ex_obj, LV_LABEL_LONG_MODE_CLIP);
    lv_label_set_text(cgpt_ex_obj, cgpt_ex_text);
    return cgpt_ex_obj;
}

lv_obj_t *cgpt_ex_npButton(int32_t cgpt_ex_x, int32_t cgpt_ex_width,
    const char *cgpt_ex_text, lv_event_cb_t cgpt_ex_callback, int32_t cgpt_ex_y = 206) {
    lv_obj_t *cgpt_ex_obj = lv_button_create(cgpt_ex_npPage);
    lv_obj_remove_style_all(cgpt_ex_obj);
    lv_obj_set_pos(cgpt_ex_obj, cgpt_ex_x, cgpt_ex_y);
    lv_obj_set_size(cgpt_ex_obj, cgpt_ex_width, 30);
    lv_obj_set_style_bg_color(cgpt_ex_obj, lv_color_hex(0x24658C), 0);
    lv_obj_set_style_bg_opa(cgpt_ex_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(cgpt_ex_obj, 4, 0);
    lv_obj_t *cgpt_ex_textObj = lv_label_create(cgpt_ex_obj);
    lv_label_set_text(cgpt_ex_textObj, cgpt_ex_text);
    lv_obj_center(cgpt_ex_textObj);
    lv_obj_add_event_cb(cgpt_ex_obj, cgpt_ex_callback, LV_EVENT_CLICKED, nullptr);
    return cgpt_ex_obj;
}

void cgpt_ex_npEdit(int8_t cgpt_ex_field) {
    const bool cgpt_ex_editing = cgpt_ex_field >= 0;
    for (uint8_t cgpt_ex_index = 0; cgpt_ex_index < 5; ++cgpt_ex_index) {
        lv_obj_set_hidden(cgpt_ex_npFields[cgpt_ex_index],
            (cgpt_ex_editing && cgpt_ex_index != cgpt_ex_field) ||
            (!cgpt_ex_editing && cgpt_ex_index == 4 && cgpt_ex_npSelectedModel == 0));
        lv_obj_set_pos(cgpt_ex_npFields[cgpt_ex_index],
            cgpt_ex_editing ? 8 : cgpt_ex_npX[cgpt_ex_index],
            cgpt_ex_editing ? 70 : (cgpt_ex_index == 3 && cgpt_ex_npSelectedModel == 0 ? 112 : cgpt_ex_npY[cgpt_ex_index]));
        lv_obj_set_width(cgpt_ex_npFields[cgpt_ex_index], cgpt_ex_editing ? 304 : cgpt_ex_npWidth[cgpt_ex_index]);
    }
    lv_obj_set_hidden(cgpt_ex_npModel, cgpt_ex_editing);
    lv_obj_set_hidden(cgpt_ex_npCaption, !cgpt_ex_editing);
    lv_obj_set_hidden(cgpt_ex_npKeyboard, !cgpt_ex_editing);
    lv_obj_set_hidden(cgpt_ex_npSaveButton, cgpt_ex_editing);
    lv_obj_set_hidden(cgpt_ex_npClearButton, cgpt_ex_editing);
    lv_obj_set_hidden(cgpt_ex_npMessage, cgpt_ex_editing);
    lv_obj_set_hidden(cgpt_ex_npNote, cgpt_ex_editing || cgpt_ex_npSelectedModel == 1);
    if (cgpt_ex_editing) lv_label_set_text(cgpt_ex_npCaption,
        cgpt_ex_field == 3 && cgpt_ex_npSelectedModel ? "CC2 LAN access code" : cgpt_ex_npCaptions[cgpt_ex_field]);
    lv_keyboard_set_mode(cgpt_ex_npKeyboard,
        cgpt_ex_field == 1 || cgpt_ex_field == 2 ? LV_KEYBOARD_MODE_NUMBER : LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(cgpt_ex_npKeyboard, cgpt_ex_editing ? cgpt_ex_npFields[cgpt_ex_field] : nullptr);
}

void cgpt_ex_npFieldEvent(lv_event_t *cgpt_ex_event) {
    const lv_obj_t *cgpt_ex_target = lv_event_get_target_obj(cgpt_ex_event);
    for (uint8_t cgpt_ex_index = 0; cgpt_ex_index < 5; ++cgpt_ex_index)
        if (cgpt_ex_npFields[cgpt_ex_index] == cgpt_ex_target) cgpt_ex_npEdit(cgpt_ex_index);
}
void cgpt_ex_npKeyboardEvent(lv_event_t *cgpt_ex_event) {
    if (lv_event_get_code(cgpt_ex_event) == LV_EVENT_READY ||
        lv_event_get_code(cgpt_ex_event) == LV_EVENT_CANCEL) cgpt_ex_npEdit(-1);
}
void cgpt_ex_npBackEvent(lv_event_t *) {
    cgpt_ex_npEdit(-1);
    lv_obj_set_hidden(cgpt_ex_npPage, true);
}
void cgpt_ex_npClearEvent(lv_event_t *) {
    // Clearing takes effect only when Save is pressed.
    lv_textarea_set_text(cgpt_ex_npFields[1], "");
    lv_textarea_set_text(cgpt_ex_npFields[3], "");
    lv_textarea_set_text(cgpt_ex_npFields[4], "");
    cgpt_ex_npApiKey[0] = cgpt_ex_npAccessCode[0] = 0;
    lv_label_set_text(cgpt_ex_npMessage, "Press Save to disable this printer slot.");
}
void cgpt_ex_npSaveEvent(lv_event_t *) {
    cgpt_ex_NeptuneConfig cgpt_ex_config;
    char cgpt_ex_error[96];
    snprintf(cgpt_ex_config.cgpt_ex_name, sizeof(cgpt_ex_config.cgpt_ex_name), "%s", lv_textarea_get_text(cgpt_ex_npFields[0]));
    snprintf(cgpt_ex_config.cgpt_ex_ip, sizeof(cgpt_ex_config.cgpt_ex_ip), "%s", lv_textarea_get_text(cgpt_ex_npFields[1]));
    cgpt_ex_config.cgpt_ex_model = cgpt_ex_npSelectedModel;
    snprintf(cgpt_ex_config.cgpt_ex_serial, sizeof(cgpt_ex_config.cgpt_ex_serial), "%s", lv_textarea_get_text(cgpt_ex_npFields[4]));
    snprintf(cgpt_ex_config.cgpt_ex_apiKey, sizeof(cgpt_ex_config.cgpt_ex_apiKey), "%s",
        cgpt_ex_npSelectedModel ? cgpt_ex_npApiKey : lv_textarea_get_text(cgpt_ex_npFields[3]));
    snprintf(cgpt_ex_config.cgpt_ex_accessCode, sizeof(cgpt_ex_config.cgpt_ex_accessCode), "%s",
        cgpt_ex_npSelectedModel ? lv_textarea_get_text(cgpt_ex_npFields[3]) : cgpt_ex_npAccessCode);
    const char *cgpt_ex_portText = lv_textarea_get_text(cgpt_ex_npFields[2]);
    char *cgpt_ex_end = nullptr;
    const unsigned long cgpt_ex_port = strtoul(cgpt_ex_portText, &cgpt_ex_end, 10);
    if (!cgpt_ex_portText[0] || *cgpt_ex_end || cgpt_ex_port < 1 || cgpt_ex_port > 65535) {
        lv_label_set_text(cgpt_ex_npMessage, "Port must be 1-65535.");
        return;
    }
    cgpt_ex_config.cgpt_ex_port = uint16_t(cgpt_ex_port);
    cgpt_ex_neptuneSave(cgpt_ex_npSlot, cgpt_ex_config, cgpt_ex_error, sizeof(cgpt_ex_error));
    lv_label_set_text(cgpt_ex_npMessage, cgpt_ex_error);
}

void cgpt_ex_npModelEvent(lv_event_t *) {
    if (cgpt_ex_npSelectedModel)
        snprintf(cgpt_ex_npAccessCode, sizeof(cgpt_ex_npAccessCode), "%s", lv_textarea_get_text(cgpt_ex_npFields[3]));
    else snprintf(cgpt_ex_npApiKey, sizeof(cgpt_ex_npApiKey), "%s", lv_textarea_get_text(cgpt_ex_npFields[3]));
    cgpt_ex_npSelectedModel = uint8_t(lv_dropdown_get_selected(cgpt_ex_npModel));
    lv_textarea_set_max_length(cgpt_ex_npFields[3], cgpt_ex_npSelectedModel ? 64 : 80);
    lv_textarea_set_text(cgpt_ex_npFields[3], cgpt_ex_npSelectedModel ? cgpt_ex_npAccessCode : cgpt_ex_npApiKey);
    lv_textarea_set_placeholder_text(cgpt_ex_npFields[3], cgpt_ex_npSelectedModel ? "LAN access code" : "API key (optional)");
    lv_textarea_set_text(cgpt_ex_npFields[2], cgpt_ex_npSelectedModel ? "1883" : "80");
    lv_label_set_text(cgpt_ex_npMessage, cgpt_ex_npSelectedModel ?
        "CC2: enter IP, serial number and LAN access code." : "Neptune: enter IP and Save. Default port 80.");
    cgpt_ex_npEdit(-1);
}

void cgpt_ex_npCreatePage() {
    cgpt_ex_npPage = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(cgpt_ex_npPage);
    lv_obj_set_pos(cgpt_ex_npPage, 0, 0);
    lv_obj_set_size(cgpt_ex_npPage, 320, 240);
    lv_obj_set_scrollable(cgpt_ex_npPage, false);
    lv_obj_set_style_bg_color(cgpt_ex_npPage, lv_color_hex(0x101923), 0);
    lv_obj_set_style_bg_opa(cgpt_ex_npPage, LV_OPA_COVER, 0);
    cgpt_ex_npTitle = cgpt_ex_npLabel(cgpt_ex_npPage, 8, 10, 238, "PRINTER SETUP");
    cgpt_ex_npButton(254, 58, "Back", cgpt_ex_npBackEvent, 4);
    cgpt_ex_npModel = lv_dropdown_create(cgpt_ex_npPage);
    lv_obj_set_pos(cgpt_ex_npModel, 166, 40);
    lv_obj_set_size(cgpt_ex_npModel, 146, 30);
    lv_obj_set_style_pad_all(cgpt_ex_npModel, 5, 0);
    lv_dropdown_set_options(cgpt_ex_npModel, "Neptune 4\nCarbon 2");
    lv_obj_add_event_cb(cgpt_ex_npModel, cgpt_ex_npModelEvent, LV_EVENT_VALUE_CHANGED, nullptr);
    const uint32_t cgpt_ex_limits[5] = {24, 15, 5, 80, 64};
    for (uint8_t cgpt_ex_index = 0; cgpt_ex_index < 5; ++cgpt_ex_index) {
        lv_obj_t *cgpt_ex_field = lv_textarea_create(cgpt_ex_npPage);
        cgpt_ex_npFields[cgpt_ex_index] = cgpt_ex_field;
        lv_obj_set_size(cgpt_ex_field, cgpt_ex_npWidth[cgpt_ex_index], 30);
        lv_textarea_set_one_line(cgpt_ex_field, true);
        lv_textarea_set_max_length(cgpt_ex_field, cgpt_ex_limits[cgpt_ex_index]);
        lv_textarea_set_placeholder_text(cgpt_ex_field, cgpt_ex_npCaptions[cgpt_ex_index]);
        lv_obj_set_style_bg_color(cgpt_ex_field, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_color(cgpt_ex_field, lv_color_hex(0x101923), 0);
        lv_obj_set_style_pad_all(cgpt_ex_field, 5, 0);
        lv_obj_add_event_cb(cgpt_ex_field, cgpt_ex_npFieldEvent, LV_EVENT_CLICKED, nullptr);
    }
    lv_textarea_set_accepted_chars(cgpt_ex_npFields[1], "0123456789.");
    lv_textarea_set_accepted_chars(cgpt_ex_npFields[2], "0123456789");
    lv_textarea_set_password_mode(cgpt_ex_npFields[3], true);
    lv_textarea_set_password_show_time(cgpt_ex_npFields[3], 0);
    cgpt_ex_npMessage = cgpt_ex_npLabel(cgpt_ex_npPage, 8, 184, 304, "");
    lv_obj_set_height(cgpt_ex_npMessage, 18);
    lv_label_set_long_mode(cgpt_ex_npMessage, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    cgpt_ex_npSaveButton = cgpt_ex_npButton(8, 96, "Save", cgpt_ex_npSaveEvent);
    cgpt_ex_npClearButton = cgpt_ex_npButton(112, 96, "Clear", cgpt_ex_npClearEvent);
    cgpt_ex_npNote = cgpt_ex_npLabel(cgpt_ex_npPage, 8, 150, 304, "Neptune port: 80. Blank IP disables slot.");
    cgpt_ex_npCaption = cgpt_ex_npLabel(cgpt_ex_npPage, 8, 44, 304, "");
    cgpt_ex_npKeyboard = lv_keyboard_create(cgpt_ex_npPage);
    lv_obj_set_align(cgpt_ex_npKeyboard, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(cgpt_ex_npKeyboard, 0, 116);
    lv_obj_set_size(cgpt_ex_npKeyboard, 320, 124);
    lv_obj_add_event_cb(cgpt_ex_npKeyboard, cgpt_ex_npKeyboardEvent, LV_EVENT_ALL, nullptr);
    cgpt_ex_npEdit(-1);
}

void cgpt_ex_npOpenEvent(lv_event_t *cgpt_ex_event) {
    cgpt_ex_npSlot = uint8_t(reinterpret_cast<uintptr_t>(lv_event_get_user_data(cgpt_ex_event)));
    cgpt_ex_NeptuneConfig cgpt_ex_config;
    cgpt_ex_NeptuneStatus cgpt_ex_status;
    if (!cgpt_ex_neptuneSnapshot(cgpt_ex_npSlot, cgpt_ex_config, cgpt_ex_status)) return;
    if (!cgpt_ex_npPage) cgpt_ex_npCreatePage();
    cgpt_ex_npSelectedModel = cgpt_ex_config.cgpt_ex_model;
    lv_dropdown_set_selected(cgpt_ex_npModel, cgpt_ex_npSelectedModel);
    cgpt_ex_npEdit(-1);
    lv_obj_set_hidden(cgpt_ex_npPage, false);
    char cgpt_ex_title[40], cgpt_ex_port[8];
    snprintf(cgpt_ex_title, sizeof(cgpt_ex_title), "PRINTER %u SETUP", unsigned(cgpt_ex_npSlot + 1));
    lv_label_set_text(cgpt_ex_npTitle, cgpt_ex_title);
    lv_textarea_set_text(cgpt_ex_npFields[0], cgpt_ex_config.cgpt_ex_name);
    lv_textarea_set_text(cgpt_ex_npFields[1], cgpt_ex_config.cgpt_ex_ip);
    snprintf(cgpt_ex_port, sizeof(cgpt_ex_port), "%u", unsigned(cgpt_ex_config.cgpt_ex_port));
    lv_textarea_set_text(cgpt_ex_npFields[2], cgpt_ex_port);
    snprintf(cgpt_ex_npApiKey, sizeof(cgpt_ex_npApiKey), "%s", cgpt_ex_config.cgpt_ex_apiKey);
    snprintf(cgpt_ex_npAccessCode, sizeof(cgpt_ex_npAccessCode), "%s", cgpt_ex_config.cgpt_ex_accessCode);
    lv_textarea_set_max_length(cgpt_ex_npFields[3], cgpt_ex_npSelectedModel ? 64 : 80);
    lv_textarea_set_text(cgpt_ex_npFields[3], cgpt_ex_npSelectedModel ? cgpt_ex_npAccessCode : cgpt_ex_npApiKey);
    lv_textarea_set_placeholder_text(cgpt_ex_npFields[3], cgpt_ex_npSelectedModel ? "LAN access code" : "API key (optional)");
    lv_textarea_set_text(cgpt_ex_npFields[4], cgpt_ex_config.cgpt_ex_serial);
    lv_label_set_text(cgpt_ex_npMessage, cgpt_ex_status.cgpt_ex_message[0] ?
        cgpt_ex_status.cgpt_ex_message : "Enter IP and Save. Changes survive restart.");
}
}

void cgpt_ex_neptuneGuiBind(lv_obj_t *cgpt_ex_card, uint8_t cgpt_ex_slot) {
    lv_obj_set_clickable(cgpt_ex_card, true);
    lv_obj_add_event_cb(cgpt_ex_card, cgpt_ex_npOpenEvent, LV_EVENT_CLICKED,
        reinterpret_cast<void *>(uintptr_t(cgpt_ex_slot)));
}

void cgpt_ex_neptuneGuiService() {
    if (uint32_t(millis() - cgpt_ex_npLastRefresh) < 500) return;
    cgpt_ex_npLastRefresh = millis();
    for (uint8_t cgpt_ex_slot = 0; cgpt_ex_slot < 3; ++cgpt_ex_slot) {
        cgpt_ex_NeptuneConfig cgpt_ex_config;
        cgpt_ex_NeptuneStatus cgpt_ex_status;
        if (!cgpt_ex_neptuneSnapshot(cgpt_ex_slot, cgpt_ex_config, cgpt_ex_status)) continue;
        cgpt_ex_PrinterStatus cgpt_ex_display;
        cgpt_ex_display.cgpt_ex_name = cgpt_ex_config.cgpt_ex_name;
        cgpt_ex_display.cgpt_ex_state = cgpt_ex_status.cgpt_ex_state;
        const bool cgpt_ex_fresh = cgpt_ex_config.cgpt_ex_ip[0] &&
            cgpt_ex_printerDataFresh(cgpt_ex_status, millis());
        const bool cgpt_ex_connected = WiFi.status() == WL_CONNECTED;
        cgpt_ex_display.cgpt_ex_online = cgpt_ex_fresh;
        if (cgpt_ex_config.cgpt_ex_ip[0]) {
            if (cgpt_ex_fresh && (!cgpt_ex_connected || !cgpt_ex_status.cgpt_ex_online))
                cgpt_ex_display.cgpt_ex_state = "Reconnecting";
            else if (!cgpt_ex_fresh && !cgpt_ex_connected)
                cgpt_ex_display.cgpt_ex_state = "No WiFi";
            else if (!cgpt_ex_fresh && (cgpt_ex_status.cgpt_ex_hasData || cgpt_ex_status.cgpt_ex_online))
                cgpt_ex_display.cgpt_ex_state = cgpt_ex_status.cgpt_ex_linkState[0] ?
                    cgpt_ex_status.cgpt_ex_linkState : "Stale";
        }
        cgpt_ex_display.cgpt_ex_nozzleC = cgpt_ex_status.cgpt_ex_nozzle;
        cgpt_ex_display.cgpt_ex_nozzleTargetC = cgpt_ex_status.cgpt_ex_nozzleTarget;
        cgpt_ex_display.cgpt_ex_bedC = cgpt_ex_status.cgpt_ex_bed;
        cgpt_ex_display.cgpt_ex_bedTargetC = cgpt_ex_status.cgpt_ex_bedTarget;
        cgpt_ex_display.cgpt_ex_remainingSeconds = cgpt_ex_printerRemaining(cgpt_ex_status, millis());
        cgpt_ex_display.cgpt_ex_remainingEstimated = cgpt_ex_status.cgpt_ex_estimated;
        cgpt_ex_guiSetPrinterStatus(cgpt_ex_slot, cgpt_ex_display);
    }
}
