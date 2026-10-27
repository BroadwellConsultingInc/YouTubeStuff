#include "cgpt_ex_neptune.h"
#include "cgpt_ex_cc2.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

namespace {
cgpt_ex_NeptuneConfig cgpt_ex_neptuneConfigs[3];
cgpt_ex_NeptuneStatus cgpt_ex_neptuneStatuses[3];
SemaphoreHandle_t cgpt_ex_neptuneMutex = nullptr;
TaskHandle_t cgpt_ex_neptuneTaskHandle = nullptr;
constexpr uint32_t cgpt_ex_neptuneMagic = 0x4E503402;
bool cgpt_ex_neptuneParseDocument(JsonVariantConst cgpt_ex_document,
    cgpt_ex_NeptuneStatus &cgpt_ex_status);
struct cgpt_ex_NeptuneStorage {
    uint32_t cgpt_ex_magic = cgpt_ex_neptuneMagic;
    cgpt_ex_NeptuneConfig cgpt_ex_slots[3];
};

struct cgpt_ex_NeptuneLegacyConfig {
    char cgpt_ex_name[25];
    char cgpt_ex_ip[16];
    uint16_t cgpt_ex_port;
    char cgpt_ex_apiKey[81];
    uint32_t cgpt_ex_revision;
};
struct cgpt_ex_NeptuneLegacyStorage {
    uint32_t cgpt_ex_magic;
    cgpt_ex_NeptuneLegacyConfig cgpt_ex_slots[3];
};
static_assert(sizeof(cgpt_ex_NeptuneLegacyStorage) == 400, "Legacy NVS layout changed");

bool cgpt_ex_neptuneValidConfig(const cgpt_ex_NeptuneConfig &cgpt_ex_config) {
    if (!memchr(cgpt_ex_config.cgpt_ex_name, 0, sizeof(cgpt_ex_config.cgpt_ex_name)) ||
        !memchr(cgpt_ex_config.cgpt_ex_ip, 0, sizeof(cgpt_ex_config.cgpt_ex_ip)) ||
        !memchr(cgpt_ex_config.cgpt_ex_apiKey, 0, sizeof(cgpt_ex_config.cgpt_ex_apiKey)) ||
        !memchr(cgpt_ex_config.cgpt_ex_serial, 0, sizeof(cgpt_ex_config.cgpt_ex_serial)) ||
        !memchr(cgpt_ex_config.cgpt_ex_accessCode, 0, sizeof(cgpt_ex_config.cgpt_ex_accessCode)) ||
        cgpt_ex_config.cgpt_ex_model > 1 || !cgpt_ex_config.cgpt_ex_port) return false;
    if (cgpt_ex_config.cgpt_ex_model == 1 && cgpt_ex_config.cgpt_ex_ip[0]) {
        if (!cgpt_ex_config.cgpt_ex_serial[0] || !cgpt_ex_config.cgpt_ex_accessCode[0]) return false;
        for (const char *cgpt_ex_char = cgpt_ex_config.cgpt_ex_serial; *cgpt_ex_char; ++cgpt_ex_char)
            if (!((*cgpt_ex_char >= '0' && *cgpt_ex_char <= '9') ||
                (*cgpt_ex_char >= 'a' && *cgpt_ex_char <= 'z') ||
                (*cgpt_ex_char >= 'A' && *cgpt_ex_char <= 'Z') ||
                *cgpt_ex_char == '-' || *cgpt_ex_char == '_')) return false;
    }
    IPAddress cgpt_ex_address;
    if (cgpt_ex_config.cgpt_ex_ip[0] && !cgpt_ex_address.fromString(cgpt_ex_config.cgpt_ex_ip)) return false;
    // An API key becomes an HTTP header; reject line breaks.
    return !strchr(cgpt_ex_config.cgpt_ex_apiKey, '\r') && !strchr(cgpt_ex_config.cgpt_ex_apiKey, '\n');
}

bool cgpt_ex_neptuneNumber(JsonVariantConst cgpt_ex_value, float &cgpt_ex_result) {
    if (!cgpt_ex_value.is<float>() && !cgpt_ex_value.is<int>()) return false;
    cgpt_ex_result = cgpt_ex_value.as<float>();
    return isfinite(cgpt_ex_result);
}

void cgpt_ex_neptunePoll(const cgpt_ex_NeptuneConfig &cgpt_ex_config,
    cgpt_ex_NeptuneStatus &cgpt_ex_status) {
    WiFiClient cgpt_ex_client;
    HTTPClient cgpt_ex_http;
    const String cgpt_ex_url = String("http://") + cgpt_ex_config.cgpt_ex_ip + ":" +
        String(cgpt_ex_config.cgpt_ex_port) +
        "/printer/objects/query?extruder=temperature,target&heater_bed=temperature,target"
        "&print_stats=state,print_duration&display_status=progress&virtual_sdcard=progress";
    cgpt_ex_http.setConnectTimeout(900);
    cgpt_ex_http.setTimeout(1200);
    cgpt_ex_http.useHTTP10(true); // Moonraker returns a plain, non-chunked stream.
    cgpt_ex_http.setReuse(false);
    snprintf(cgpt_ex_status.cgpt_ex_state, 24, "Offline");
    if (!cgpt_ex_http.begin(cgpt_ex_client, cgpt_ex_url)) {
        snprintf(cgpt_ex_status.cgpt_ex_message, 96, "Could not open HTTP connection");
        return;
    }
    if (cgpt_ex_config.cgpt_ex_apiKey[0])
        cgpt_ex_http.addHeader("X-Api-Key", cgpt_ex_config.cgpt_ex_apiKey);
    const int cgpt_ex_code = cgpt_ex_http.GET();
    if (cgpt_ex_code == 200 && cgpt_ex_http.getSize() <= 8192) {
        // Only retain fields we use. No full HTTP response String is allocated.
#if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument cgpt_ex_filter;
        JsonDocument cgpt_ex_json;
#else
        DynamicJsonDocument cgpt_ex_filter(1536);
        DynamicJsonDocument cgpt_ex_json(2048);
#endif
        cgpt_ex_filter["result"]["status"]["extruder"] = true;
        cgpt_ex_filter["result"]["status"]["heater_bed"] = true;
        cgpt_ex_filter["result"]["status"]["print_stats"] = true;
        cgpt_ex_filter["result"]["status"]["display_status"] = true;
        cgpt_ex_filter["result"]["status"]["virtual_sdcard"] = true;
        const DeserializationError cgpt_ex_error = deserializeJson(cgpt_ex_json,
            cgpt_ex_http.getStream(), DeserializationOption::Filter(cgpt_ex_filter),
            DeserializationOption::NestingLimit(8));
        if (!cgpt_ex_error) {
            cgpt_ex_neptuneParseDocument(cgpt_ex_json.as<JsonVariantConst>(), cgpt_ex_status);
        } else snprintf(cgpt_ex_status.cgpt_ex_message, 96,
            "JSON read failed: %s", cgpt_ex_error.c_str());
    } else if (cgpt_ex_code == 200) {
        snprintf(cgpt_ex_status.cgpt_ex_state, 24, "Reply too large");
        snprintf(cgpt_ex_status.cgpt_ex_message, 96, "Unexpected HTTP response larger than 8192 bytes");
    } else {
        snprintf(cgpt_ex_status.cgpt_ex_message, 96, "HTTP %d from %s:%u",
            cgpt_ex_code, cgpt_ex_config.cgpt_ex_ip, unsigned(cgpt_ex_config.cgpt_ex_port));
        if (cgpt_ex_code == 401 || cgpt_ex_code == 403)
            snprintf(cgpt_ex_status.cgpt_ex_state, 24, "Auth required");
        else if (cgpt_ex_code == 404) snprintf(cgpt_ex_status.cgpt_ex_state, 24, "Check port");
        else if (cgpt_ex_code == 503) snprintf(cgpt_ex_status.cgpt_ex_state, 24, "Klipper down");
    }
    cgpt_ex_http.end();
}

void cgpt_ex_neptuneMergeSample(const cgpt_ex_NeptuneStatus &cgpt_ex_sample,
    cgpt_ex_NeptuneStatus &cgpt_ex_stored) {
    if (cgpt_ex_sample.cgpt_ex_online) {
        cgpt_ex_stored = cgpt_ex_sample;
        cgpt_ex_stored.cgpt_ex_hasData = true;
    } else if (cgpt_ex_stored.cgpt_ex_hasData || cgpt_ex_stored.cgpt_ex_online) {
        // Keep temperatures, printer state, remaining time AND its original timestamp.
        // In particular, neither a failure nor a reconnect rebases the countdown.
        cgpt_ex_stored.cgpt_ex_online = false;
        cgpt_ex_stored.cgpt_ex_hasData = true;
        snprintf(cgpt_ex_stored.cgpt_ex_linkState, 24, "%s", cgpt_ex_sample.cgpt_ex_state);
        snprintf(cgpt_ex_stored.cgpt_ex_message, 96, "%s", cgpt_ex_sample.cgpt_ex_message);
    } else cgpt_ex_stored = cgpt_ex_sample;
}

void cgpt_ex_neptuneTask(void *) {
    uint32_t cgpt_ex_lastPoll[3] = {};
    uint32_t cgpt_ex_seenRevision[3] = {};
    for (;;) {
        for (uint8_t cgpt_ex_slot = 0; cgpt_ex_slot < 3; ++cgpt_ex_slot) {
            cgpt_ex_NeptuneConfig cgpt_ex_config;
            cgpt_ex_NeptuneStatus cgpt_ex_status;
            if (!cgpt_ex_neptuneSnapshot(cgpt_ex_slot, cgpt_ex_config, cgpt_ex_status)) continue;
            bool cgpt_ex_updated = cgpt_ex_cc2Service(cgpt_ex_slot, cgpt_ex_config, cgpt_ex_status);
            if (cgpt_ex_config.cgpt_ex_model == 0 && cgpt_ex_config.cgpt_ex_ip[0] &&
                WiFi.status() == WL_CONNECTED) {
                const bool cgpt_ex_newConfig = cgpt_ex_seenRevision[cgpt_ex_slot] != cgpt_ex_config.cgpt_ex_revision;
                if (cgpt_ex_newConfig || uint32_t(millis() - cgpt_ex_lastPoll[cgpt_ex_slot]) >= 3000) {
                    cgpt_ex_seenRevision[cgpt_ex_slot] = cgpt_ex_config.cgpt_ex_revision;
                    cgpt_ex_lastPoll[cgpt_ex_slot] = millis();
                    cgpt_ex_status = cgpt_ex_NeptuneStatus();
                    cgpt_ex_neptunePoll(cgpt_ex_config, cgpt_ex_status);
                    cgpt_ex_updated = true;
                }
            }
            if (cgpt_ex_updated) {
                xSemaphoreTake(cgpt_ex_neptuneMutex, portMAX_DELAY);
                // Never publish a reply from a configuration edited during I/O.
                if (cgpt_ex_neptuneConfigs[cgpt_ex_slot].cgpt_ex_revision == cgpt_ex_config.cgpt_ex_revision)
                    cgpt_ex_neptuneMergeSample(cgpt_ex_status, cgpt_ex_neptuneStatuses[cgpt_ex_slot]);
                xSemaphoreGive(cgpt_ex_neptuneMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool cgpt_ex_neptuneParseDocument(JsonVariantConst cgpt_ex_document,
    cgpt_ex_NeptuneStatus &cgpt_ex_status) {
    cgpt_ex_NeptuneStatus cgpt_ex_next;
    const JsonVariantConst cgpt_ex_data = cgpt_ex_document["result"]["status"];
    if (!cgpt_ex_neptuneNumber(cgpt_ex_data["extruder"]["temperature"], cgpt_ex_next.cgpt_ex_nozzle) ||
        !cgpt_ex_neptuneNumber(cgpt_ex_data["extruder"]["target"], cgpt_ex_next.cgpt_ex_nozzleTarget) ||
        !cgpt_ex_neptuneNumber(cgpt_ex_data["heater_bed"]["temperature"], cgpt_ex_next.cgpt_ex_bed) ||
        !cgpt_ex_neptuneNumber(cgpt_ex_data["heater_bed"]["target"], cgpt_ex_next.cgpt_ex_bedTarget) ||
        cgpt_ex_next.cgpt_ex_nozzle < -100 || cgpt_ex_next.cgpt_ex_nozzle > 500 ||
        cgpt_ex_next.cgpt_ex_nozzleTarget < 0 || cgpt_ex_next.cgpt_ex_nozzleTarget > 500 ||
        cgpt_ex_next.cgpt_ex_bed < -100 || cgpt_ex_next.cgpt_ex_bed > 500 ||
        cgpt_ex_next.cgpt_ex_bedTarget < 0 || cgpt_ex_next.cgpt_ex_bedTarget > 500) {
        cgpt_ex_status = cgpt_ex_NeptuneStatus();
        snprintf(cgpt_ex_status.cgpt_ex_state, 24, "No telemetry");
        snprintf(cgpt_ex_status.cgpt_ex_message, 96, "Missing/invalid heater telemetry; check Moonraker/Klipper");
        return false;
    }
    const char *cgpt_ex_state = cgpt_ex_data["print_stats"]["state"] | "unknown";
    const bool cgpt_ex_paused = !strcmp(cgpt_ex_state, "paused");
    cgpt_ex_next.cgpt_ex_printing = !strcmp(cgpt_ex_state, "printing");
    const char *cgpt_ex_displayState = cgpt_ex_next.cgpt_ex_printing ? "Printing" :
        cgpt_ex_paused ? "Paused" : !strcmp(cgpt_ex_state, "complete") ? "Complete" :
        !strcmp(cgpt_ex_state, "error") ? "Error" : !strcmp(cgpt_ex_state, "cancelled") ? "Cancelled" :
        !strcmp(cgpt_ex_state, "standby") ? "Ready" : "Online";
    snprintf(cgpt_ex_next.cgpt_ex_state, 24, "%s", cgpt_ex_displayState);
    if (!strcmp(cgpt_ex_state, "complete")) cgpt_ex_next.cgpt_ex_remaining = 0;
    else if (cgpt_ex_next.cgpt_ex_printing || cgpt_ex_paused) {
        float cgpt_ex_progress = 0, cgpt_ex_duration = 0;
        cgpt_ex_neptuneNumber(cgpt_ex_data["display_status"]["progress"], cgpt_ex_progress);
        if (cgpt_ex_progress <= 0 || cgpt_ex_progress > 1)
            cgpt_ex_neptuneNumber(cgpt_ex_data["virtual_sdcard"]["progress"], cgpt_ex_progress);
        cgpt_ex_neptuneNumber(cgpt_ex_data["print_stats"]["print_duration"], cgpt_ex_duration);
        if (cgpt_ex_progress >= 0.01f && cgpt_ex_progress <= 1 && cgpt_ex_duration >= 30) {
            const double cgpt_ex_remaining = double(cgpt_ex_duration) * (1.0 - cgpt_ex_progress) / cgpt_ex_progress;
            if (isfinite(cgpt_ex_remaining) && cgpt_ex_remaining >= 0 && cgpt_ex_remaining < INT32_MAX) {
                cgpt_ex_next.cgpt_ex_remaining = int32_t(ceil(cgpt_ex_remaining));
                cgpt_ex_next.cgpt_ex_estimated = true;
            }
        }
    }
    cgpt_ex_next.cgpt_ex_online = true;
    cgpt_ex_next.cgpt_ex_hasData = true;
    cgpt_ex_next.cgpt_ex_lastGood = millis();
    snprintf(cgpt_ex_next.cgpt_ex_message, 96, "Moonraker online; remaining time marked ~ is estimated");
    cgpt_ex_status = cgpt_ex_next;
    return true;
}
} // namespace

bool cgpt_ex_neptuneParse(const char *cgpt_ex_json, cgpt_ex_NeptuneStatus &cgpt_ex_status) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument cgpt_ex_document;
#else
    DynamicJsonDocument cgpt_ex_document(3072);
#endif
    const DeserializationError cgpt_ex_error = deserializeJson(cgpt_ex_document,
        cgpt_ex_json, DeserializationOption::NestingLimit(8));
    if (cgpt_ex_error) {
        cgpt_ex_status = cgpt_ex_NeptuneStatus();
        snprintf(cgpt_ex_status.cgpt_ex_state, 24, "Bad JSON");
        return false;
    }
    return cgpt_ex_neptuneParseDocument(cgpt_ex_document.as<JsonVariantConst>(), cgpt_ex_status);
}

bool cgpt_ex_neptuneBegin() {
    if (cgpt_ex_neptuneMutex) return cgpt_ex_neptuneTaskHandle != nullptr;
    cgpt_ex_neptuneMutex = xSemaphoreCreateMutex();
    if (!cgpt_ex_neptuneMutex) return false;
    cgpt_ex_NeptuneStorage cgpt_ex_storage;
    Preferences cgpt_ex_preferences;
    bool cgpt_ex_loaded = false;
    if (cgpt_ex_preferences.begin("cgpt_ex_np4", true)) {
        cgpt_ex_loaded = cgpt_ex_preferences.getBytesLength("cgpt_ex_slots") == sizeof(cgpt_ex_storage) &&
            cgpt_ex_preferences.getBytes("cgpt_ex_slots", &cgpt_ex_storage, sizeof(cgpt_ex_storage)) == sizeof(cgpt_ex_storage) &&
            cgpt_ex_storage.cgpt_ex_magic == cgpt_ex_neptuneMagic;
        if (!cgpt_ex_loaded && cgpt_ex_preferences.getBytesLength("cgpt_ex_slots") == sizeof(cgpt_ex_NeptuneLegacyStorage)) {
            cgpt_ex_NeptuneLegacyStorage cgpt_ex_legacy;
            if (cgpt_ex_preferences.getBytes("cgpt_ex_slots", &cgpt_ex_legacy, sizeof(cgpt_ex_legacy)) == sizeof(cgpt_ex_legacy) &&
                cgpt_ex_legacy.cgpt_ex_magic == 0x4E503401) {
                for (uint8_t cgpt_ex_index = 0; cgpt_ex_index < 3; ++cgpt_ex_index) {
                    // Member copies avoid depending on padding in the expanded layout.
                    const cgpt_ex_NeptuneLegacyConfig &cgpt_ex_old = cgpt_ex_legacy.cgpt_ex_slots[cgpt_ex_index];
                    cgpt_ex_NeptuneConfig &cgpt_ex_new = cgpt_ex_storage.cgpt_ex_slots[cgpt_ex_index];
                    memcpy(cgpt_ex_new.cgpt_ex_name, cgpt_ex_old.cgpt_ex_name, sizeof(cgpt_ex_old.cgpt_ex_name));
                    memcpy(cgpt_ex_new.cgpt_ex_ip, cgpt_ex_old.cgpt_ex_ip, sizeof(cgpt_ex_old.cgpt_ex_ip));
                    memcpy(cgpt_ex_new.cgpt_ex_apiKey, cgpt_ex_old.cgpt_ex_apiKey, sizeof(cgpt_ex_old.cgpt_ex_apiKey));
                    cgpt_ex_new.cgpt_ex_port = cgpt_ex_old.cgpt_ex_ip[0] ? cgpt_ex_old.cgpt_ex_port : 80;
                    cgpt_ex_new.cgpt_ex_revision = cgpt_ex_old.cgpt_ex_revision;
                }
                cgpt_ex_loaded = true;
            }
        }
        cgpt_ex_preferences.end();
    }
    for (uint8_t cgpt_ex_slot = 0; cgpt_ex_slot < 3; ++cgpt_ex_slot) {
        if (cgpt_ex_loaded && cgpt_ex_neptuneValidConfig(cgpt_ex_storage.cgpt_ex_slots[cgpt_ex_slot]))
            cgpt_ex_neptuneConfigs[cgpt_ex_slot] = cgpt_ex_storage.cgpt_ex_slots[cgpt_ex_slot];
        if (!cgpt_ex_neptuneConfigs[cgpt_ex_slot].cgpt_ex_name[0])
            snprintf(cgpt_ex_neptuneConfigs[cgpt_ex_slot].cgpt_ex_name, 25, "Neptune 4 #%u", unsigned(cgpt_ex_slot + 1));
        if (cgpt_ex_neptuneConfigs[cgpt_ex_slot].cgpt_ex_ip[0])
            snprintf(cgpt_ex_neptuneStatuses[cgpt_ex_slot].cgpt_ex_state, 24, "Connecting");
    }
    if (xTaskCreate(cgpt_ex_neptuneTask, "cgpt_ex_np4", 6144, nullptr, 1,
        &cgpt_ex_neptuneTaskHandle) != pdPASS) {
        for (uint8_t cgpt_ex_slot = 0; cgpt_ex_slot < 3; ++cgpt_ex_slot) {
            snprintf(cgpt_ex_neptuneStatuses[cgpt_ex_slot].cgpt_ex_state, 24, "Monitor error");
            snprintf(cgpt_ex_neptuneStatuses[cgpt_ex_slot].cgpt_ex_message, 96, "Not enough memory to start monitoring");
        }
        return false;
    }
    return true;
}

bool cgpt_ex_neptuneSnapshot(uint8_t cgpt_ex_slot, cgpt_ex_NeptuneConfig &cgpt_ex_config,
    cgpt_ex_NeptuneStatus &cgpt_ex_status) {
    if (cgpt_ex_slot >= 3 || !cgpt_ex_neptuneMutex ||
        xSemaphoreTake(cgpt_ex_neptuneMutex, 0) != pdTRUE) return false;
    cgpt_ex_config = cgpt_ex_neptuneConfigs[cgpt_ex_slot];
    cgpt_ex_status = cgpt_ex_neptuneStatuses[cgpt_ex_slot];
    xSemaphoreGive(cgpt_ex_neptuneMutex);
    return true;
}

bool cgpt_ex_neptuneSave(uint8_t cgpt_ex_slot, const cgpt_ex_NeptuneConfig &cgpt_ex_config,
    char *cgpt_ex_error, size_t cgpt_ex_errorSize) {
    if (cgpt_ex_slot >= 3 || !cgpt_ex_neptuneMutex || !cgpt_ex_neptuneValidConfig(cgpt_ex_config)) {
        snprintf(cgpt_ex_error, cgpt_ex_errorSize, "Check IP/port. CC2 needs a serial number and LAN code.");
        return false;
    }
    cgpt_ex_NeptuneStorage cgpt_ex_storage;
    xSemaphoreTake(cgpt_ex_neptuneMutex, portMAX_DELAY);
    for (uint8_t cgpt_ex_index = 0; cgpt_ex_index < 3; ++cgpt_ex_index)
        cgpt_ex_storage.cgpt_ex_slots[cgpt_ex_index] = cgpt_ex_neptuneConfigs[cgpt_ex_index];
    xSemaphoreGive(cgpt_ex_neptuneMutex);
    cgpt_ex_NeptuneConfig cgpt_ex_next = cgpt_ex_config;
    if (!cgpt_ex_next.cgpt_ex_name[0])
        snprintf(cgpt_ex_next.cgpt_ex_name, 25, cgpt_ex_next.cgpt_ex_model ? "CC2 #%u" : "Neptune 4 #%u", unsigned(cgpt_ex_slot + 1));
    cgpt_ex_next.cgpt_ex_revision = cgpt_ex_storage.cgpt_ex_slots[cgpt_ex_slot].cgpt_ex_revision + 1;
    cgpt_ex_storage.cgpt_ex_slots[cgpt_ex_slot] = cgpt_ex_next;
    Preferences cgpt_ex_preferences;
    if (!cgpt_ex_preferences.begin("cgpt_ex_np4", false)) {
        snprintf(cgpt_ex_error, cgpt_ex_errorSize, "Could not open printer settings storage.");
        return false;
    }
    const bool cgpt_ex_savedOK = cgpt_ex_preferences.putBytes("cgpt_ex_slots", &cgpt_ex_storage,
        sizeof(cgpt_ex_storage)) == sizeof(cgpt_ex_storage);
    cgpt_ex_preferences.end();
    if (!cgpt_ex_savedOK) {
        snprintf(cgpt_ex_error, cgpt_ex_errorSize, "Could not save printer settings.");
        return false;
    }
    xSemaphoreTake(cgpt_ex_neptuneMutex, portMAX_DELAY);
    cgpt_ex_neptuneConfigs[cgpt_ex_slot] = cgpt_ex_next;
    cgpt_ex_neptuneStatuses[cgpt_ex_slot] = cgpt_ex_NeptuneStatus();
    if (cgpt_ex_next.cgpt_ex_ip[0]) snprintf(cgpt_ex_neptuneStatuses[cgpt_ex_slot].cgpt_ex_state, 24, "Connecting");
    xSemaphoreGive(cgpt_ex_neptuneMutex);
    snprintf(cgpt_ex_error, cgpt_ex_errorSize, "Saved. Monitoring starts automatically.");
    return true;
}
