#pragma once

#include <Arduino.h>
#include <cstdint>
#include <array>
#include <ArduinoJson.h>

// ─────────────────────────────────────────────
// Message type constants  (M field values)
// ─────────────────────────────────────────────
static constexpr uint16_t MSG_SENSOR_REQUEST  = 1001; // ESP32 → Arduino: "give me sensor data"
static constexpr uint16_t MSG_SENSOR_RESPONSE = 1002; // Arduino → ESP32: sensor payload
static constexpr uint16_t MSG_LOAD_DOSE       = 2001; // ESP32 → Arduino: loading dose command
static constexpr uint16_t MSG_SYSTEM_STATE    = 2002; // ESP32 → Arduino: start/stop system
static constexpr uint16_t MSG_SET_PROFILE     = 2003; // ESP32 → Arduino: plant profile (EC/pH ranges)


// ─────────────────────────────────────────────
// Send sensor data  (Arduino → ESP32)
// ─────────────────────────────────────────────
void send_sensor_data(Stream &port, float ph, float ec, float temp) {
    JsonDocument msg;
    msg["M"]    = MSG_SENSOR_RESPONSE;
    msg["pH"]   = ph;
    msg["ec"]   = ec;
    msg["temp"] = temp;
    serializeJson(msg, port);
    port.print('\n');
}

// ── Legacy alias so existing send_data() calls in main.cpp still compile ──
void send_data(Stream &port, const float &ph, const float &ec, const float &temp) {
    send_sensor_data(port, ph, ec, temp);
}


// ─────────────────────────────────────────────
// Non-blocking line reader
//
// Call this every loop(). Returns true when a
// complete '\n'-terminated line is ready in
// `out_buf`. Does NOT block.
// ─────────────────────────────────────────────
static constexpr uint8_t COMM_BUF_SIZE = 255;

struct CommReader {
    char    buf[COMM_BUF_SIZE];
    uint8_t idx = 0;

    // Returns true when a full line has arrived.
    bool poll(Stream &port) {
        while (port.available() > 0) {
            char c = static_cast<char>(port.read());
            if (c == '\n') {
                buf[idx] = '\0';
                idx = 0;
                return true;          // caller can now parse buf
            }
            if (idx < COMM_BUF_SIZE - 1) {
                buf[idx++] = c;
            }
            // overflow: silently drop and reset
            else { idx = 0; }
        }
        return false;
    }
};


// ─────────────────────────────────────────────
// Parse a received line into a JsonDocument.
// Returns false if the JSON is malformed.
// ─────────────────────────────────────────────
bool parse_message(const char *line, JsonDocument &doc) {
    DeserializationError err = deserializeJson(doc, line);
    return !err;
}