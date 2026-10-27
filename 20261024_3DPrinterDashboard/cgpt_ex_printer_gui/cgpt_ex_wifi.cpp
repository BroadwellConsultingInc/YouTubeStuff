#include "cgpt_ex_wifi.h"
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <stdio.h>
#include <string.h>

namespace {
struct cgpt_ex_Credentials {
    char cgpt_ex_ssid[33];
    char cgpt_ex_password[65];
};
struct cgpt_ex_Network {
    char cgpt_ex_ssid[33];
    int32_t cgpt_ex_rssi;
};
cgpt_ex_Credentials cgpt_ex_saved = {};
cgpt_ex_Credentials cgpt_ex_pending = {};
cgpt_ex_Network cgpt_ex_networks[16] = {};
uint8_t cgpt_ex_networkCount = 0;
uint32_t cgpt_ex_scanRevision = 0;
uint32_t cgpt_ex_attemptMs = 0, cgpt_ex_retryMs = 0;
bool cgpt_ex_scanning = false, cgpt_ex_connecting = false;
bool cgpt_ex_savePending = false, cgpt_ex_wasConnected = false;
bool cgpt_ex_radioOK = false;
char cgpt_ex_message[144] = "Wi-Fi starting";

void cgpt_ex_startAttempt(bool cgpt_ex_save) {
    cgpt_ex_savePending = cgpt_ex_save;
    cgpt_ex_connecting = true;
    cgpt_ex_wasConnected = false;
    cgpt_ex_attemptMs = millis();
    WiFi.disconnect(false, false);
    WiFi.begin(cgpt_ex_pending.cgpt_ex_ssid, cgpt_ex_pending.cgpt_ex_password);
    snprintf(cgpt_ex_message, sizeof(cgpt_ex_message), "Connecting: %s", cgpt_ex_pending.cgpt_ex_ssid);
}

bool cgpt_ex_store() {
    Preferences cgpt_ex_preferences;
    if (!cgpt_ex_preferences.begin("cgpt_ex_wifi", false)) return false;
    // One NVS blob keeps the SSID/password pair together.
    const bool cgpt_ex_ok = cgpt_ex_preferences.putBytes("cgpt_ex_creds",
        &cgpt_ex_pending, sizeof(cgpt_ex_pending)) == sizeof(cgpt_ex_pending);
    cgpt_ex_preferences.end();
    if (cgpt_ex_ok) cgpt_ex_saved = cgpt_ex_pending;
    return cgpt_ex_ok;
}

void cgpt_ex_finishScan(int16_t cgpt_ex_count) {
    cgpt_ex_networkCount = 0;
    for (int16_t cgpt_ex_index = 0; cgpt_ex_index < cgpt_ex_count; ++cgpt_ex_index) {
        const String cgpt_ex_ssid = WiFi.SSID(cgpt_ex_index);
        if (!cgpt_ex_ssid.length() || cgpt_ex_ssid.length() > 32) continue;
        uint8_t cgpt_ex_slot = 0;
        while (cgpt_ex_slot < cgpt_ex_networkCount &&
            strcmp(cgpt_ex_networks[cgpt_ex_slot].cgpt_ex_ssid, cgpt_ex_ssid.c_str())) ++cgpt_ex_slot;
        const int32_t cgpt_ex_rssi = WiFi.RSSI(cgpt_ex_index);
        if (cgpt_ex_slot < cgpt_ex_networkCount) {
            if (cgpt_ex_rssi > cgpt_ex_networks[cgpt_ex_slot].cgpt_ex_rssi)
                cgpt_ex_networks[cgpt_ex_slot].cgpt_ex_rssi = cgpt_ex_rssi;
        } else if (cgpt_ex_networkCount < 16) {
            snprintf(cgpt_ex_networks[cgpt_ex_slot].cgpt_ex_ssid, 33, "%s", cgpt_ex_ssid.c_str());
            cgpt_ex_networks[cgpt_ex_slot].cgpt_ex_rssi = cgpt_ex_rssi;
            ++cgpt_ex_networkCount;
        }
    }
    WiFi.scanDelete();
    cgpt_ex_scanning = false;
    ++cgpt_ex_scanRevision;
    if (cgpt_ex_count < 0) snprintf(cgpt_ex_message, sizeof(cgpt_ex_message), "Scan failed. Tap Scan to retry.");
    else if (!cgpt_ex_networkCount) snprintf(cgpt_ex_message, sizeof(cgpt_ex_message), "No networks found. Type an SSID.");
    else snprintf(cgpt_ex_message, sizeof(cgpt_ex_message), "Select a network, then enter password.");
}
}

void cgpt_ex_wifiBegin() {
    cgpt_ex_saved = {};
    Preferences cgpt_ex_preferences;
    if (cgpt_ex_preferences.begin("cgpt_ex_wifi", true)) {
        if (cgpt_ex_preferences.getBytesLength("cgpt_ex_creds") == sizeof(cgpt_ex_saved)) {
            cgpt_ex_preferences.getBytes("cgpt_ex_creds", &cgpt_ex_saved, sizeof(cgpt_ex_saved));
            if (!memchr(cgpt_ex_saved.cgpt_ex_ssid, 0, 33) ||
                !memchr(cgpt_ex_saved.cgpt_ex_password, 0, 65)) cgpt_ex_saved = {};
        }
        cgpt_ex_preferences.end();
    }
    WiFi.persistent(false); // This module owns saving; failed attempts are not saved by WiFi.
    cgpt_ex_radioOK = WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false); // Retries below stay out of active scans.
    cgpt_ex_scanning = cgpt_ex_connecting = cgpt_ex_savePending = cgpt_ex_wasConnected = false;
    cgpt_ex_retryMs = millis();
    if (!cgpt_ex_radioOK) snprintf(cgpt_ex_message, sizeof(cgpt_ex_message), "Wi-Fi initialization failed.");
    else if (cgpt_ex_saved.cgpt_ex_ssid[0]) {
        cgpt_ex_pending = cgpt_ex_saved;
        cgpt_ex_startAttempt(false);
    } else snprintf(cgpt_ex_message, sizeof(cgpt_ex_message), "Select or type a network name.");
}

bool cgpt_ex_wifiScan() {
    if (!cgpt_ex_radioOK || cgpt_ex_connecting || cgpt_ex_scanning) return false;
    WiFi.scanDelete();
    cgpt_ex_scanning = true;
    snprintf(cgpt_ex_message, sizeof(cgpt_ex_message), "Scanning 2.4 GHz networks...");
    const int16_t cgpt_ex_result = WiFi.scanNetworks(true);
    if (cgpt_ex_result != WIFI_SCAN_RUNNING) cgpt_ex_finishScan(cgpt_ex_result);
    return true;
}

bool cgpt_ex_wifiConnect(const char *cgpt_ex_ssid, const char *cgpt_ex_password) {
    if (!cgpt_ex_radioOK || cgpt_ex_scanning || cgpt_ex_connecting) return false;
    const size_t cgpt_ex_ssidLength = cgpt_ex_ssid ? strlen(cgpt_ex_ssid) : 0;
    const size_t cgpt_ex_passwordLength = cgpt_ex_password ? strlen(cgpt_ex_password) : 0;
    if (!cgpt_ex_ssidLength || cgpt_ex_ssidLength > 32 || cgpt_ex_passwordLength > 64) {
        snprintf(cgpt_ex_message, sizeof(cgpt_ex_message), "SSID: 1-32 bytes; password: 0-64.");
        return false;
    }
    cgpt_ex_pending = {};
    snprintf(cgpt_ex_pending.cgpt_ex_ssid, 33, "%s", cgpt_ex_ssid);
    snprintf(cgpt_ex_pending.cgpt_ex_password, 65, "%s", cgpt_ex_password ? cgpt_ex_password : "");
    cgpt_ex_startAttempt(true);
    return true;
}

void cgpt_ex_wifiService() {
    if (!cgpt_ex_radioOK) return;
    if (cgpt_ex_scanning) {
        const int16_t cgpt_ex_result = WiFi.scanComplete();
        if (cgpt_ex_result != WIFI_SCAN_RUNNING) cgpt_ex_finishScan(cgpt_ex_result);
    }
    const bool cgpt_ex_connected = WiFi.status() == WL_CONNECTED;
    if (cgpt_ex_connecting) {
        if (cgpt_ex_connected && WiFi.SSID() == cgpt_ex_pending.cgpt_ex_ssid) {
            const bool cgpt_ex_stored = !cgpt_ex_savePending || cgpt_ex_store();
            cgpt_ex_savePending = cgpt_ex_connecting = false;
            cgpt_ex_wasConnected = true;
            snprintf(cgpt_ex_message, sizeof(cgpt_ex_message), "%s\nIP: %s",
                cgpt_ex_stored ? "Connected" : "Connected; saving failed",
                WiFi.localIP().toString().c_str());
        } else if (uint32_t(millis() - cgpt_ex_attemptMs) >= 20000) {
            WiFi.disconnect(false, false);
            cgpt_ex_savePending = cgpt_ex_connecting = false;
            cgpt_ex_retryMs = millis();
            snprintf(cgpt_ex_message, sizeof(cgpt_ex_message), "Connection failed. Check password/signal.");
        }
    } else if (cgpt_ex_connected) cgpt_ex_wasConnected = true;
    else {
        if (cgpt_ex_wasConnected) {
            cgpt_ex_wasConnected = false;
            cgpt_ex_retryMs = millis();
            snprintf(cgpt_ex_message, sizeof(cgpt_ex_message), "Connection lost; will retry saved network.");
        }
        if (!cgpt_ex_scanning && cgpt_ex_saved.cgpt_ex_ssid[0] &&
            uint32_t(millis() - cgpt_ex_retryMs) >= 30000) {
            cgpt_ex_pending = cgpt_ex_saved;
            cgpt_ex_startAttempt(false);
        }
    }
}

bool cgpt_ex_wifiForget() {
    Preferences cgpt_ex_preferences;
    if (!cgpt_ex_preferences.begin("cgpt_ex_wifi", false)) {
        snprintf(cgpt_ex_message, sizeof(cgpt_ex_message), "Could not clear saved network.");
        return false;
    }
    const bool cgpt_ex_ok = cgpt_ex_preferences.clear();
    cgpt_ex_preferences.end();
    if (!cgpt_ex_ok) {
        snprintf(cgpt_ex_message, sizeof(cgpt_ex_message), "Could not clear saved network.");
        return false;
    }
    cgpt_ex_saved = {}; cgpt_ex_pending = {};
    cgpt_ex_connecting = cgpt_ex_savePending = cgpt_ex_wasConnected = false;
    WiFi.disconnect(false, false);
    snprintf(cgpt_ex_message, sizeof(cgpt_ex_message), "Saved network forgotten.");
    return true;
}

bool cgpt_ex_wifiConnected() { return WiFi.status() == WL_CONNECTED; }
bool cgpt_ex_wifiConnecting() { return cgpt_ex_connecting; }
bool cgpt_ex_wifiScanning() { return cgpt_ex_scanning; }
const char *cgpt_ex_wifiMessage() { return cgpt_ex_message; }
const char *cgpt_ex_wifiSavedSsid() { return cgpt_ex_saved.cgpt_ex_ssid; }
const char *cgpt_ex_wifiSavedPassword() { return cgpt_ex_saved.cgpt_ex_password; }
uint32_t cgpt_ex_wifiScanRevision() { return cgpt_ex_scanRevision; }
uint8_t cgpt_ex_wifiNetworkCount() { return cgpt_ex_networkCount; }
const char *cgpt_ex_wifiNetworkName(uint8_t cgpt_ex_index) {
    return cgpt_ex_index < cgpt_ex_networkCount ? cgpt_ex_networks[cgpt_ex_index].cgpt_ex_ssid : "";
}
int32_t cgpt_ex_wifiNetworkRssi(uint8_t cgpt_ex_index) {
    return cgpt_ex_index < cgpt_ex_networkCount ? cgpt_ex_networks[cgpt_ex_index].cgpt_ex_rssi : 0;
}
