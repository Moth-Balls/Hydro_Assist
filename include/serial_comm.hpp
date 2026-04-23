#pragma once

#include <Arduino.h>
#include <cstdint>
#include <array>
#include <ArduinoJson.h>

static constexpr uint16_t MSG_SENSOR_REQUEST  = 1001; // ESP32 → MCU: Request sensor data
static constexpr uint16_t MSG_SENSOR_RESPONSE = 1002; // MCU → ESP32: sensor data response
static constexpr uint16_t MSG_LOAD_DOSE       = 2001; // ESP32 → MCU: loading dose command
static constexpr uint16_t MSG_SYSTEM_STATE    = 2002; // ESP32 → MCU: start/stop system
static constexpr uint16_t MSG_SET_PROFILE     = 2003; // ESP32 → MCU: plant profile 

// Send data
void send_sensor_data(Stream &port, float ph, float ec, float temp) {
    JsonDocument msg;
    msg["M"]    = MSG_SENSOR_RESPONSE;
    msg["pH"]   = ph;
    msg["ec"]   = ec;
    msg["temp"] = temp;
    serializeJson(msg, port);
    port.print('\n');
}

void send_data(Stream &port, const float &ph, const float &ec, const float &temp) {
    send_sensor_data(port, ph, ec, temp);
}

static constexpr uint8_t COMM_BUF_SIZE = 255;

struct CommReader {
    char    buf[COMM_BUF_SIZE];
    uint8_t idx = 0;

    // Returns true when a full line has arrived
    bool poll(Stream &port) {
        while (port.available() > 0) {
            char c = static_cast<char>(port.read());
            if (c == '\n') {
                buf[idx] = '\0';
                idx = 0;
                return true; // caller can now parse buf
            }
            if (idx < COMM_BUF_SIZE - 1) {
                buf[idx++] = c;
            }
            // overflow
            else { idx = 0; }
        }
        return false;
    }
};


bool parse_message(const char *line, JsonDocument &doc) {
    DeserializationError err = deserializeJson(doc, line);
    return !err;
}