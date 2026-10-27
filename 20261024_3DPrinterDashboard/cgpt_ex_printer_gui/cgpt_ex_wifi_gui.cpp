#include "cgpt_ex_wifi_gui.h"
#include "cgpt_ex_wifi.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#if !LV_USE_BUTTON || !LV_USE_DROPDOWN || !LV_USE_TEXTAREA || !LV_USE_KEYBOARD
#error "Enable LV_USE_BUTTON, LV_USE_DROPDOWN, LV_USE_TEXTAREA and LV_USE_KEYBOARD in lv_conf.h."
#endif

namespace {
lv_obj_t *cgpt_ex_wifiPage = nullptr, *cgpt_ex_wifiButtonText = nullptr;
lv_obj_t *cgpt_ex_wifiStatus = nullptr, *cgpt_ex_wifiDropdown = nullptr;
lv_obj_t *cgpt_ex_wifiSsid = nullptr, *cgpt_ex_wifiPassword = nullptr;
lv_obj_t *cgpt_ex_wifiKeyboard = nullptr, *cgpt_ex_wifiScanButton = nullptr;
lv_obj_t *cgpt_ex_wifiConnectButton = nullptr, *cgpt_ex_wifiForgetButton = nullptr;
lv_obj_t *cgpt_ex_wifiShowButton = nullptr, *cgpt_ex_wifiNote = nullptr;
uint32_t cgpt_ex_wifiListRevision = UINT32_MAX;

lv_obj_t *cgpt_ex_wifiLabel(lv_obj_t *cgpt_ex_parent, int32_t cgpt_ex_x,
    int32_t cgpt_ex_y, int32_t cgpt_ex_width, const char *cgpt_ex_text) {
    lv_obj_t *cgpt_ex_obj = lv_label_create(cgpt_ex_parent);
    lv_obj_set_pos(cgpt_ex_obj, cgpt_ex_x, cgpt_ex_y);
    lv_obj_set_width(cgpt_ex_obj, cgpt_ex_width);
    lv_label_set_long_mode(cgpt_ex_obj, LV_LABEL_LONG_MODE_CLIP);
    lv_label_set_text(cgpt_ex_obj, cgpt_ex_text);
    return cgpt_ex_obj;
}

lv_obj_t *cgpt_ex_wifiMakeButton(lv_obj_t *cgpt_ex_parent, int32_t cgpt_ex_x,
    int32_t cgpt_ex_y, int32_t cgpt_ex_width, int32_t cgpt_ex_height,
    const char *cgpt_ex_text, lv_event_cb_t cgpt_ex_callback) {
    lv_obj_t *cgpt_ex_obj = lv_button_create(cgpt_ex_parent);
    lv_obj_remove_style_all(cgpt_ex_obj);
    lv_obj_set_pos(cgpt_ex_obj, cgpt_ex_x, cgpt_ex_y);
    lv_obj_set_size(cgpt_ex_obj, cgpt_ex_width, cgpt_ex_height);
    lv_obj_set_style_bg_color(cgpt_ex_obj, lv_color_hex(0x24658C), 0);
    lv_obj_set_style_bg_opa(cgpt_ex_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(cgpt_ex_obj, 4, 0);
    lv_obj_set_style_text_color(cgpt_ex_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_opa(cgpt_ex_obj, LV_OPA_50, LV_STATE_DISABLED);
    lv_obj_t *cgpt_ex_textObj = lv_label_create(cgpt_ex_obj);
    lv_label_set_text(cgpt_ex_textObj, cgpt_ex_text);
    lv_obj_center(cgpt_ex_textObj);
    lv_obj_add_event_cb(cgpt_ex_obj, cgpt_ex_callback, LV_EVENT_CLICKED, nullptr);
    return cgpt_ex_obj;
}

void cgpt_ex_wifiEdit(bool cgpt_ex_editing, lv_obj_t *cgpt_ex_field = nullptr) {
    lv_obj_set_hidden(cgpt_ex_wifiKeyboard, !cgpt_ex_editing);
    lv_obj_set_hidden(cgpt_ex_wifiStatus, cgpt_ex_editing);
    lv_obj_set_hidden(cgpt_ex_wifiDropdown, cgpt_ex_editing);
    lv_obj_set_hidden(cgpt_ex_wifiScanButton, cgpt_ex_editing);
    lv_obj_set_hidden(cgpt_ex_wifiConnectButton, cgpt_ex_editing);
    lv_obj_set_hidden(cgpt_ex_wifiForgetButton, cgpt_ex_editing);
    lv_obj_set_hidden(cgpt_ex_wifiShowButton, cgpt_ex_editing);
    lv_obj_set_hidden(cgpt_ex_wifiNote, cgpt_ex_editing);
    lv_obj_set_y(cgpt_ex_wifiSsid, cgpt_ex_editing ? 44 : 104);
    lv_obj_set_y(cgpt_ex_wifiPassword, cgpt_ex_editing ? 80 : 138);
    lv_obj_set_width(cgpt_ex_wifiPassword, cgpt_ex_editing ? 304 : 226);
    lv_keyboard_set_textarea(cgpt_ex_wifiKeyboard, cgpt_ex_editing ? cgpt_ex_field : nullptr);
}

void cgpt_ex_wifiFieldEvent(lv_event_t *cgpt_ex_event) {
    cgpt_ex_wifiEdit(true, lv_event_get_target_obj(cgpt_ex_event));
}

void cgpt_ex_wifiKeyboardEvent(lv_event_t *cgpt_ex_event) {
    if (lv_event_get_code(cgpt_ex_event) == LV_EVENT_READY ||
        lv_event_get_code(cgpt_ex_event) == LV_EVENT_CANCEL) cgpt_ex_wifiEdit(false);
}

void cgpt_ex_wifiBackEvent(lv_event_t *) {
    cgpt_ex_wifiEdit(false);
    lv_textarea_set_password_mode(cgpt_ex_wifiPassword, true);
    lv_label_set_text(lv_obj_get_child(cgpt_ex_wifiShowButton, 0), "Show");
    lv_obj_set_hidden(cgpt_ex_wifiPage, true);
}

void cgpt_ex_wifiOpenEvent(lv_event_t *) {
    Serial.println("cgpt_ex_: WiFi button clicked");
    lv_obj_set_hidden(cgpt_ex_wifiPage, false);
    lv_textarea_set_text(cgpt_ex_wifiSsid, cgpt_ex_wifiSavedSsid());
    lv_textarea_set_text(cgpt_ex_wifiPassword, cgpt_ex_wifiSavedPassword());
    cgpt_ex_wifiScan();
    cgpt_ex_wifiGuiService();
}

void cgpt_ex_wifiScanEvent(lv_event_t *) { cgpt_ex_wifiScan(); }
void cgpt_ex_wifiConnectEvent(lv_event_t *) {
    cgpt_ex_wifiConnect(lv_textarea_get_text(cgpt_ex_wifiSsid),
        lv_textarea_get_text(cgpt_ex_wifiPassword));
    lv_textarea_set_password_mode(cgpt_ex_wifiPassword, true);
    lv_label_set_text(lv_obj_get_child(cgpt_ex_wifiShowButton, 0), "Show");
    cgpt_ex_wifiGuiService();
}
void cgpt_ex_wifiForgetEvent(lv_event_t *) {
    if (cgpt_ex_wifiForget()) {
        lv_textarea_set_text(cgpt_ex_wifiSsid, "");
        lv_textarea_set_text(cgpt_ex_wifiPassword, "");
        lv_dropdown_set_selected(cgpt_ex_wifiDropdown, 0);
    }
    cgpt_ex_wifiGuiService();
}
void cgpt_ex_wifiShowEvent(lv_event_t *) {
    const bool cgpt_ex_hide = !lv_textarea_get_password_mode(cgpt_ex_wifiPassword);
    lv_textarea_set_password_mode(cgpt_ex_wifiPassword, cgpt_ex_hide);
    lv_label_set_text(lv_obj_get_child(cgpt_ex_wifiShowButton, 0), cgpt_ex_hide ? "Show" : "Hide");
}
void cgpt_ex_wifiSelectionEvent(lv_event_t *) {
    const uint32_t cgpt_ex_selected = lv_dropdown_get_selected(cgpt_ex_wifiDropdown);
    if (!cgpt_ex_selected || cgpt_ex_selected > cgpt_ex_wifiNetworkCount()) return;
    const char *cgpt_ex_ssid = cgpt_ex_wifiNetworkName(uint8_t(cgpt_ex_selected - 1));
    lv_textarea_set_text(cgpt_ex_wifiSsid, cgpt_ex_ssid);
    lv_textarea_set_text(cgpt_ex_wifiPassword,
        !strcmp(cgpt_ex_ssid, cgpt_ex_wifiSavedSsid()) ? cgpt_ex_wifiSavedPassword() : "");
    lv_textarea_set_password_mode(cgpt_ex_wifiPassword, true);
    lv_label_set_text(lv_obj_get_child(cgpt_ex_wifiShowButton, 0), "Show");
}

lv_obj_t *cgpt_ex_wifiField(int32_t cgpt_ex_y, int32_t cgpt_ex_width,
    const char *cgpt_ex_placeholder, uint32_t cgpt_ex_limit) {
    lv_obj_t *cgpt_ex_obj = lv_textarea_create(cgpt_ex_wifiPage);
    lv_obj_set_pos(cgpt_ex_obj, 8, cgpt_ex_y);
    lv_obj_set_size(cgpt_ex_obj, cgpt_ex_width, 30);
    lv_textarea_set_one_line(cgpt_ex_obj, true);
    lv_textarea_set_max_length(cgpt_ex_obj, cgpt_ex_limit);
    lv_textarea_set_placeholder_text(cgpt_ex_obj, cgpt_ex_placeholder);
    lv_obj_set_style_pad_all(cgpt_ex_obj, 5, 0);
    lv_obj_set_style_bg_color(cgpt_ex_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_color(cgpt_ex_obj, lv_color_hex(0x101923), 0);
    lv_obj_add_event_cb(cgpt_ex_obj, cgpt_ex_wifiFieldEvent, LV_EVENT_CLICKED, nullptr);
    return cgpt_ex_obj;
}
}

void cgpt_ex_wifiGuiBegin(lv_obj_t *cgpt_ex_parent) {
    lv_obj_t *cgpt_ex_button = cgpt_ex_wifiMakeButton(cgpt_ex_parent,
        240, 3, 72, 32, "WiFi", cgpt_ex_wifiOpenEvent);
    lv_obj_set_ext_click_area(cgpt_ex_button, 4);
    cgpt_ex_wifiButtonText = lv_obj_get_child(cgpt_ex_button, 0);
    cgpt_ex_wifiPage = lv_obj_create(cgpt_ex_parent);
    lv_obj_remove_style_all(cgpt_ex_wifiPage);
    lv_obj_set_pos(cgpt_ex_wifiPage, 0, 0);
    lv_obj_set_size(cgpt_ex_wifiPage, 320, 240);
    lv_obj_set_scrollable(cgpt_ex_wifiPage, false);
    lv_obj_set_style_bg_color(cgpt_ex_wifiPage, lv_color_hex(0x101923), 0);
    lv_obj_set_style_bg_opa(cgpt_ex_wifiPage, LV_OPA_COVER, 0);
    cgpt_ex_wifiLabel(cgpt_ex_wifiPage, 8, 9, 232, "WI-FI SETUP");
    cgpt_ex_wifiMakeButton(cgpt_ex_wifiPage, 254, 4, 58, 28, "Back", cgpt_ex_wifiBackEvent);
    cgpt_ex_wifiStatus = cgpt_ex_wifiLabel(cgpt_ex_wifiPage, 8, 36, 304, "");
    lv_obj_set_height(cgpt_ex_wifiStatus, 31);
    cgpt_ex_wifiDropdown = lv_dropdown_create(cgpt_ex_wifiPage);
    lv_obj_set_pos(cgpt_ex_wifiDropdown, 8, 70);
    lv_obj_set_size(cgpt_ex_wifiDropdown, 226, 28);
    lv_obj_set_style_pad_all(cgpt_ex_wifiDropdown, 5, 0);
    lv_obj_set_style_text_color(cgpt_ex_wifiDropdown, lv_color_hex(0x101923), 0);
    lv_dropdown_set_options(cgpt_ex_wifiDropdown, "Select a network");
    lv_obj_add_event_cb(cgpt_ex_wifiDropdown, cgpt_ex_wifiSelectionEvent, LV_EVENT_VALUE_CHANGED, nullptr);
    cgpt_ex_wifiScanButton = cgpt_ex_wifiMakeButton(cgpt_ex_wifiPage, 242, 70, 70, 28, "Scan", cgpt_ex_wifiScanEvent);
    cgpt_ex_wifiSsid = cgpt_ex_wifiField(104, 304, "SSID / network name", 32);
    cgpt_ex_wifiPassword = cgpt_ex_wifiField(138, 226, "Password (empty for open)", 64);
    lv_textarea_set_password_mode(cgpt_ex_wifiPassword, true);
    lv_textarea_set_password_show_time(cgpt_ex_wifiPassword, 0);
    cgpt_ex_wifiShowButton = cgpt_ex_wifiMakeButton(cgpt_ex_wifiPage, 242, 138, 70, 30, "Show", cgpt_ex_wifiShowEvent);
    cgpt_ex_wifiConnectButton = cgpt_ex_wifiMakeButton(cgpt_ex_wifiPage, 8, 176, 96, 30, "Connect", cgpt_ex_wifiConnectEvent);
    cgpt_ex_wifiForgetButton = cgpt_ex_wifiMakeButton(cgpt_ex_wifiPage, 112, 176, 96, 30, "Forget", cgpt_ex_wifiForgetEvent);
    cgpt_ex_wifiNote = cgpt_ex_wifiLabel(cgpt_ex_wifiPage, 8, 216, 304, "Saved for automatic connection at boot.");
    cgpt_ex_wifiKeyboard = lv_keyboard_create(cgpt_ex_wifiPage);
    lv_obj_set_align(cgpt_ex_wifiKeyboard, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(cgpt_ex_wifiKeyboard, 0, 116);
    lv_obj_set_size(cgpt_ex_wifiKeyboard, 320, 124);
    lv_obj_add_event_cb(cgpt_ex_wifiKeyboard, cgpt_ex_wifiKeyboardEvent, LV_EVENT_ALL, nullptr);
    cgpt_ex_wifiEdit(false);
    lv_obj_set_hidden(cgpt_ex_wifiPage, true);
}

void cgpt_ex_wifiGuiService() {
    if (!cgpt_ex_wifiPage) return;
    const char *cgpt_ex_button = cgpt_ex_wifiConnected() ? "WiFi OK" :
        (cgpt_ex_wifiConnecting() ? "WiFi ..." : "WiFi");
    if (strcmp(lv_label_get_text(cgpt_ex_wifiButtonText), cgpt_ex_button))
        lv_label_set_text(cgpt_ex_wifiButtonText, cgpt_ex_button);
    if (strcmp(lv_label_get_text(cgpt_ex_wifiStatus), cgpt_ex_wifiMessage()))
        lv_label_set_text(cgpt_ex_wifiStatus, cgpt_ex_wifiMessage());
    const bool cgpt_ex_busy = cgpt_ex_wifiScanning() || cgpt_ex_wifiConnecting();
    lv_obj_set_state(cgpt_ex_wifiScanButton, LV_STATE_DISABLED, cgpt_ex_busy);
    lv_obj_set_state(cgpt_ex_wifiConnectButton, LV_STATE_DISABLED, cgpt_ex_busy);
    if (cgpt_ex_wifiListRevision != cgpt_ex_wifiScanRevision()) {
        cgpt_ex_wifiListRevision = cgpt_ex_wifiScanRevision();
        char cgpt_ex_options[1024] = "Select a network";
        size_t cgpt_ex_used = strlen(cgpt_ex_options);
        for (uint8_t cgpt_ex_index = 0; cgpt_ex_index < cgpt_ex_wifiNetworkCount(); ++cgpt_ex_index) {
            char cgpt_ex_name[33];
            snprintf(cgpt_ex_name, sizeof(cgpt_ex_name), "%s", cgpt_ex_wifiNetworkName(cgpt_ex_index));
            for (char *cgpt_ex_char = cgpt_ex_name; *cgpt_ex_char; ++cgpt_ex_char)
                if (uint8_t(*cgpt_ex_char) < 32 || uint8_t(*cgpt_ex_char) == 127) *cgpt_ex_char = '?';
            cgpt_ex_used += snprintf(cgpt_ex_options + cgpt_ex_used, sizeof(cgpt_ex_options) - cgpt_ex_used,
                "\n%s (%ld)", cgpt_ex_name, (long)cgpt_ex_wifiNetworkRssi(cgpt_ex_index));
        }
        lv_dropdown_set_options(cgpt_ex_wifiDropdown, cgpt_ex_options);
    }
}
