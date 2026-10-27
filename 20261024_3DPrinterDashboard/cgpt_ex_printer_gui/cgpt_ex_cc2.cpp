#include "cgpt_ex_cc2.h"
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <esp_system.h>
#include <new>
#include <string.h>
#include <stdio.h>
#include <limits.h>

namespace {
// Full replies can include Canvas information that is not used by this GUI.
// MQTT must receive the packet before ArduinoJson can filter it.
constexpr uint16_t cgpt_ex_cc2PacketBytes = 12288;

void cgpt_ex_cc2Filter(JsonVariant cgpt_ex_filter) {
    cgpt_ex_filter["id"] = true;
    cgpt_ex_filter["method"] = true;
    cgpt_ex_filter["client_id"] = true;
    cgpt_ex_filter["error"] = true;
    cgpt_ex_filter["result"]["error_code"] = true;
    cgpt_ex_filter["result"]["extruder"]["temperature"] = true;
    cgpt_ex_filter["result"]["extruder"]["target"] = true;
    cgpt_ex_filter["result"]["heater_bed"]["temperature"] = true;
    cgpt_ex_filter["result"]["heater_bed"]["target"] = true;
    cgpt_ex_filter["result"]["machine_status"]["status"] = true;
    cgpt_ex_filter["result"]["machine_status"]["sub_status"] = true;
    cgpt_ex_filter["result"]["print_status"]["remaining_time_sec"] = true;
}

bool cgpt_ex_cc2Temperature(JsonVariantConst cgpt_ex_value, float &cgpt_ex_result, float cgpt_ex_minimum) {
    if (!cgpt_ex_value.is<float>() && !cgpt_ex_value.is<int>()) return false;
    cgpt_ex_result = cgpt_ex_value.as<float>();
    return isfinite(cgpt_ex_result) && cgpt_ex_result >= cgpt_ex_minimum && cgpt_ex_result <= 500;
}

bool cgpt_ex_cc2ParseDocument(JsonVariantConst cgpt_ex_document, cgpt_ex_NeptuneStatus &cgpt_ex_status) {
    cgpt_ex_NeptuneStatus cgpt_ex_next;
    const JsonVariantConst cgpt_ex_data = cgpt_ex_document["result"];
    if ((cgpt_ex_document["method"] | 0) != 1002 ||
        (!cgpt_ex_data["error_code"].isNull() && (cgpt_ex_data["error_code"] | -1) != 0) ||
        !cgpt_ex_data["machine_status"]["status"].is<int>() ||
        !cgpt_ex_cc2Temperature(cgpt_ex_data["extruder"]["temperature"], cgpt_ex_next.cgpt_ex_nozzle, -100) ||
        !cgpt_ex_cc2Temperature(cgpt_ex_data["extruder"]["target"], cgpt_ex_next.cgpt_ex_nozzleTarget, 0) ||
        !cgpt_ex_cc2Temperature(cgpt_ex_data["heater_bed"]["temperature"], cgpt_ex_next.cgpt_ex_bed, -100) ||
        !cgpt_ex_cc2Temperature(cgpt_ex_data["heater_bed"]["target"], cgpt_ex_next.cgpt_ex_bedTarget, 0)) {
        cgpt_ex_status = cgpt_ex_NeptuneStatus();
        snprintf(cgpt_ex_status.cgpt_ex_state, 24, "No telemetry");
        snprintf(cgpt_ex_status.cgpt_ex_message, 96, "CC2 full status missing/invalid; check LAN access and firmware");
        return false;
    }
    const int cgpt_ex_machine = cgpt_ex_data["machine_status"]["status"];
    const int cgpt_ex_sub = cgpt_ex_data["machine_status"]["sub_status"] | 0;
    const char *cgpt_ex_state = "Online";
    switch (cgpt_ex_machine) {
        case 0: cgpt_ex_state = "Initializing"; break;
        case 1: cgpt_ex_state = "Ready"; break;
        case 2: cgpt_ex_state = "Printing"; break;
        case 3: case 4: cgpt_ex_state = "Filament"; break;
        case 5: cgpt_ex_state = "Leveling"; break;
        case 6: cgpt_ex_state = "PID calibration"; break;
        case 7: cgpt_ex_state = "Resonance test"; break;
        case 8: cgpt_ex_state = "Self check"; break;
        case 9: cgpt_ex_state = "Updating"; break;
        case 10: cgpt_ex_state = "Homing"; break;
        case 11: cgpt_ex_state = "Transferring"; break;
        case 12: cgpt_ex_state = "Video"; break;
        case 13: cgpt_ex_state = "Extruder"; break;
        case 14: cgpt_ex_state = "Emergency stop"; break;
        case 15: cgpt_ex_state = "Power recovery"; break;
    }
    if (cgpt_ex_machine == 2) {
        const bool cgpt_ex_paused = cgpt_ex_sub == 2501 || cgpt_ex_sub == 2502 || cgpt_ex_sub == 2505;
        const bool cgpt_ex_complete = cgpt_ex_sub == 2077;
        const bool cgpt_ex_stopped = cgpt_ex_sub == 2503 || cgpt_ex_sub == 2504;
        const bool cgpt_ex_downloading = cgpt_ex_sub == 1081 || cgpt_ex_sub == 1082 || cgpt_ex_sub == 1086;
        cgpt_ex_state = cgpt_ex_paused ? "Paused" : cgpt_ex_complete ? "Complete" :
            cgpt_ex_stopped ? "Stopped" : cgpt_ex_downloading ? "Downloading" :
            (cgpt_ex_sub == 1045 || cgpt_ex_sub == 1096 || cgpt_ex_sub == 1405 || cgpt_ex_sub == 1906) ? "Heating" :
            (cgpt_ex_sub == 2801 || cgpt_ex_sub == 2802) ? "Homing" :
            (cgpt_ex_sub == 2901 || cgpt_ex_sub == 2902) ? "Leveling" : "Printing";
        cgpt_ex_next.cgpt_ex_printing = !cgpt_ex_paused && !cgpt_ex_complete && !cgpt_ex_stopped && !cgpt_ex_downloading;
        const JsonVariantConst cgpt_ex_time = cgpt_ex_data["print_status"]["remaining_time_sec"];
        if (!cgpt_ex_downloading && !cgpt_ex_stopped && cgpt_ex_time.is<int32_t>() && cgpt_ex_time.as<int32_t>() >= 0)
            cgpt_ex_next.cgpt_ex_remaining = cgpt_ex_time.as<int32_t>();
        if (cgpt_ex_complete) cgpt_ex_next.cgpt_ex_remaining = 0;
    }
    snprintf(cgpt_ex_next.cgpt_ex_state, 24, "%s", cgpt_ex_state);
    snprintf(cgpt_ex_next.cgpt_ex_message, 96, "CC2 MQTT online; remaining time supplied by printer");
    cgpt_ex_next.cgpt_ex_online = true;
    cgpt_ex_next.cgpt_ex_hasData = true;
    cgpt_ex_next.cgpt_ex_lastGood = millis();
    cgpt_ex_status = cgpt_ex_next;
    return true;
}

struct cgpt_ex_CC2Session {
    WiFiClient cgpt_ex_transport;
    PubSubClient cgpt_ex_mqtt;
    cgpt_ex_NeptuneStatus cgpt_ex_status;
    uint32_t cgpt_ex_revision, cgpt_ex_lastAttempt = 0, cgpt_ex_registeredAt = 0;
    uint32_t cgpt_ex_lastQuery = 0, cgpt_ex_lastPing = 0, cgpt_ex_queryId = 0;
    uint32_t cgpt_ex_pendingId = 0;
    bool cgpt_ex_attempted = false, cgpt_ex_registered = false, cgpt_ex_dirty = true;
    bool cgpt_ex_bufferOK = false, cgpt_ex_failed = false;
    char cgpt_ex_clientId[32], cgpt_ex_requestTopic[176], cgpt_ex_responseTopic[176], cgpt_ex_ackTopic[176];

    explicit cgpt_ex_CC2Session(uint32_t cgpt_ex_configRevision) :
        cgpt_ex_mqtt(cgpt_ex_transport), cgpt_ex_revision(cgpt_ex_configRevision) {
        snprintf(cgpt_ex_clientId, sizeof(cgpt_ex_clientId), "1_PC_%lu", (unsigned long)esp_random());
        cgpt_ex_mqtt.setKeepAlive(30);
        cgpt_ex_mqtt.setSocketTimeout(1);
        cgpt_ex_bufferOK = cgpt_ex_mqtt.setBufferSize(cgpt_ex_cc2PacketBytes);
        cgpt_ex_mqtt.setCallback([this](char *cgpt_ex_topic, uint8_t *cgpt_ex_payload, unsigned int cgpt_ex_length) {
            cgpt_ex_message(cgpt_ex_topic, cgpt_ex_payload, cgpt_ex_length);
        });
        snprintf(cgpt_ex_status.cgpt_ex_state, 24, "Connecting");
    }
    ~cgpt_ex_CC2Session() { cgpt_ex_mqtt.disconnect(); cgpt_ex_transport.stop(); }

    void cgpt_ex_fail(const char *cgpt_ex_state, const char *cgpt_ex_messageText) {
        cgpt_ex_failed = true;
        cgpt_ex_mqtt.disconnect();
        cgpt_ex_transport.stop();
        cgpt_ex_registered = false;
        cgpt_ex_pendingId = 0;
        cgpt_ex_lastAttempt = millis();
        cgpt_ex_attempted = true;
        cgpt_ex_status = cgpt_ex_NeptuneStatus();
        snprintf(cgpt_ex_status.cgpt_ex_state, 24, "%s", cgpt_ex_state);
        snprintf(cgpt_ex_status.cgpt_ex_message, 96, "%s", cgpt_ex_messageText);
        cgpt_ex_dirty = true;
    }

    void cgpt_ex_message(char *cgpt_ex_topic, uint8_t *cgpt_ex_payload, unsigned int cgpt_ex_length) {
        const bool cgpt_ex_ack = !strcmp(cgpt_ex_topic, cgpt_ex_ackTopic);
        if (!cgpt_ex_ack && strcmp(cgpt_ex_topic, cgpt_ex_responseTopic)) return;
#if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument cgpt_ex_filter, cgpt_ex_json;
#else
        DynamicJsonDocument cgpt_ex_filter(1536), cgpt_ex_json(2048);
#endif
        cgpt_ex_cc2Filter(cgpt_ex_filter.to<JsonVariant>());
        const DeserializationError cgpt_ex_error = deserializeJson(cgpt_ex_json,
            static_cast<const uint8_t *>(cgpt_ex_payload), cgpt_ex_length,
            DeserializationOption::Filter(cgpt_ex_filter), DeserializationOption::NestingLimit(10));
        if (cgpt_ex_error) {
            cgpt_ex_fail("Bad JSON", "CC2 JSON invalid/too large; check firmware response");
            return;
        }
        if (cgpt_ex_ack) {
            if (cgpt_ex_registered) return;
            if (strcmp(cgpt_ex_json["client_id"] | "", cgpt_ex_clientId)) return;
            if (strcmp(cgpt_ex_json["error"] | "", "ok")) {
                cgpt_ex_fail("Register failed", "CC2 registration rejected; check SN and other connected clients");
                return;
            }
            cgpt_ex_registered = true;
            cgpt_ex_lastPing = millis();
            cgpt_ex_lastQuery = millis() - 3000;
            return;
        }
        // Match our exact topic AND outstanding ID; ignore PONGs, deltas and other clients.
        if (!cgpt_ex_registered || !cgpt_ex_pendingId ||
            (cgpt_ex_json["method"] | 0) != 1002 ||
            (cgpt_ex_json["id"] | uint32_t(0)) != cgpt_ex_pendingId) return;
        cgpt_ex_pendingId = 0;
        cgpt_ex_cc2ParseDocument(cgpt_ex_json.as<JsonVariantConst>(), cgpt_ex_status);
        cgpt_ex_dirty = true;
    }

    void cgpt_ex_service(const cgpt_ex_NeptuneConfig &cgpt_ex_config) {
        if (!cgpt_ex_bufferOK) {
            cgpt_ex_fail("Memory low", "Could not allocate CC2 MQTT receive buffer");
            return;
        }
        if (!cgpt_ex_mqtt.connected()) {
            if (cgpt_ex_attempted && uint32_t(millis() - cgpt_ex_lastAttempt) < 10000) return;
            cgpt_ex_attempted = true;
            cgpt_ex_lastAttempt = millis();
            cgpt_ex_registered = false;
            cgpt_ex_pendingId = 0;
            snprintf(cgpt_ex_requestTopic, sizeof(cgpt_ex_requestTopic), "elegoo/%s/%s/api_request", cgpt_ex_config.cgpt_ex_serial, cgpt_ex_clientId);
            snprintf(cgpt_ex_responseTopic, sizeof(cgpt_ex_responseTopic), "elegoo/%s/%s/api_response", cgpt_ex_config.cgpt_ex_serial, cgpt_ex_clientId);
            snprintf(cgpt_ex_ackTopic, sizeof(cgpt_ex_ackTopic), "elegoo/%s/%s_req/register_response", cgpt_ex_config.cgpt_ex_serial, cgpt_ex_clientId);
            IPAddress cgpt_ex_address;
            cgpt_ex_address.fromString(cgpt_ex_config.cgpt_ex_ip);
            cgpt_ex_mqtt.setServer(cgpt_ex_address, cgpt_ex_config.cgpt_ex_port);
            if (!cgpt_ex_mqtt.connect(cgpt_ex_clientId, "elegoo", cgpt_ex_config.cgpt_ex_accessCode)) {
                const int cgpt_ex_code = cgpt_ex_mqtt.state();
                cgpt_ex_fail(cgpt_ex_code == 4 || cgpt_ex_code == 5 ? "Auth required" : "MQTT offline",
                    cgpt_ex_code == 4 || cgpt_ex_code == 5 ? "Check the printer LAN access code" : "Cannot reach CC2 MQTT; enable LAN access and check IP/port");
                return;
            }
            char cgpt_ex_topic[96], cgpt_ex_body[160];
            snprintf(cgpt_ex_topic, sizeof(cgpt_ex_topic), "elegoo/%s/api_register", cgpt_ex_config.cgpt_ex_serial);
            snprintf(cgpt_ex_body, sizeof(cgpt_ex_body), "{\"client_id\":\"%s\",\"request_id\":\"%s_req\"}", cgpt_ex_clientId, cgpt_ex_clientId);
            if (!cgpt_ex_mqtt.subscribe(cgpt_ex_ackTopic) || !cgpt_ex_mqtt.subscribe(cgpt_ex_responseTopic) ||
                !cgpt_ex_mqtt.publish(cgpt_ex_topic, cgpt_ex_body)) {
                cgpt_ex_fail("MQTT error", "Could not subscribe/register with CC2");
                return;
            }
            cgpt_ex_registeredAt = millis();
            cgpt_ex_status = cgpt_ex_NeptuneStatus();
            snprintf(cgpt_ex_status.cgpt_ex_state, 24, "Registering");
            snprintf(cgpt_ex_status.cgpt_ex_message, 96, "Waiting for CC2 registration; verify serial number if this times out");
            cgpt_ex_dirty = true;
        }
        cgpt_ex_failed = false;
        const bool cgpt_ex_loopOK = cgpt_ex_mqtt.loop();
        if (cgpt_ex_failed) return; // Preserve a specific failure from the callback.
        if (!cgpt_ex_loopOK) {
            cgpt_ex_fail("MQTT offline", "CC2 disconnected; reconnecting automatically");
            return;
        }
        if (!cgpt_ex_mqtt.connected()) return; // Callback may reject registration.
        if (!cgpt_ex_registered) {
            if (uint32_t(millis() - cgpt_ex_registeredAt) >= cgpt_ex_printerTimeoutMs)
                cgpt_ex_fail("Register timeout", "Check CC2 serial number, LAN mode and connected-client limit");
            return;
        }
        if (uint32_t(millis() - cgpt_ex_lastPing) >= 10000) {
            // Elegoo application heartbeat is required in addition to MQTT PINGREQ.
            cgpt_ex_lastPing = millis();
            if (!cgpt_ex_mqtt.publish(cgpt_ex_requestTopic, "{\"type\":\"PING\"}")) {
                cgpt_ex_fail("MQTT offline", "CC2 heartbeat send failed"); return;
            }
        }
        if (cgpt_ex_pendingId && uint32_t(millis() - cgpt_ex_lastQuery) >= cgpt_ex_printerTimeoutMs) {
            cgpt_ex_fail("Status timeout", "No CC2 full status; reply may exceed 12 KB or firmware may differ");
            return;
        }
        if (!cgpt_ex_pendingId && uint32_t(millis() - cgpt_ex_lastQuery) >= 3000) {
            if (++cgpt_ex_queryId == 0) ++cgpt_ex_queryId;
            cgpt_ex_pendingId = cgpt_ex_queryId;
            cgpt_ex_lastQuery = millis();
            char cgpt_ex_body[80];
            snprintf(cgpt_ex_body, sizeof(cgpt_ex_body), "{\"id\":%lu,\"method\":1002,\"params\":{}}", (unsigned long)cgpt_ex_pendingId);
            if (!cgpt_ex_mqtt.publish(cgpt_ex_requestTopic, cgpt_ex_body))
                cgpt_ex_fail("MQTT offline", "CC2 status request send failed");
        }
    }
};
cgpt_ex_CC2Session *cgpt_ex_cc2Sessions[3] = {};
} // namespace

bool cgpt_ex_cc2Service(uint8_t cgpt_ex_slot, const cgpt_ex_NeptuneConfig &cgpt_ex_config,
    cgpt_ex_NeptuneStatus &cgpt_ex_status) {
    if (cgpt_ex_slot >= 3) return false;
    cgpt_ex_CC2Session *&cgpt_ex_session = cgpt_ex_cc2Sessions[cgpt_ex_slot];
    if (cgpt_ex_session && (cgpt_ex_session->cgpt_ex_revision != cgpt_ex_config.cgpt_ex_revision ||
        cgpt_ex_config.cgpt_ex_model != 1 || !cgpt_ex_config.cgpt_ex_ip[0] || WiFi.status() != WL_CONNECTED)) {
        delete cgpt_ex_session;
        cgpt_ex_session = nullptr;
    }
    if (cgpt_ex_config.cgpt_ex_model != 1 || !cgpt_ex_config.cgpt_ex_ip[0] || WiFi.status() != WL_CONNECTED) return false;
    if (!cgpt_ex_session) cgpt_ex_session = new (std::nothrow) cgpt_ex_CC2Session(cgpt_ex_config.cgpt_ex_revision);
    if (!cgpt_ex_session) {
        cgpt_ex_status = cgpt_ex_NeptuneStatus();
        snprintf(cgpt_ex_status.cgpt_ex_state, 24, "Memory low");
        snprintf(cgpt_ex_status.cgpt_ex_message, 96, "Not enough memory to start CC2 monitoring");
        return true;
    }
    cgpt_ex_session->cgpt_ex_service(cgpt_ex_config);
    if (!cgpt_ex_session->cgpt_ex_dirty) return false;
    cgpt_ex_status = cgpt_ex_session->cgpt_ex_status;
    cgpt_ex_session->cgpt_ex_dirty = false;
    return true;
}

bool cgpt_ex_cc2Parse(const char *cgpt_ex_payload, size_t cgpt_ex_length, cgpt_ex_NeptuneStatus &cgpt_ex_status) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument cgpt_ex_filter, cgpt_ex_json;
#else
    DynamicJsonDocument cgpt_ex_filter(1536), cgpt_ex_json(2048);
#endif
    cgpt_ex_cc2Filter(cgpt_ex_filter.to<JsonVariant>());
    if (deserializeJson(cgpt_ex_json, cgpt_ex_payload, cgpt_ex_length,
        DeserializationOption::Filter(cgpt_ex_filter), DeserializationOption::NestingLimit(10))) {
        cgpt_ex_status = cgpt_ex_NeptuneStatus();
        snprintf(cgpt_ex_status.cgpt_ex_state, 24, "Bad JSON");
        return false;
    }
    return cgpt_ex_cc2ParseDocument(cgpt_ex_json.as<JsonVariantConst>(), cgpt_ex_status);
}
