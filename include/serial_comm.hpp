#pragma once

#include <Arduino.h>
#include <cstdint>
#include <array>
#include <ArduinoJson.h>

// #include "some logger library for errors"

void send_data(Stream &comm_port, const float &ph_val, const float &ec_val, const float &temp_val) {

    JsonDocument msg;
    msg["pH"] = ph_val;
    msg["ec"] = ec_val;
    msg["temp"] = temp_val;

    serializeJson(msg, comm_port);
    comm_port.print('\n');
}

std::array<float, 4> read_data(Stream &comm_port) {
    constexpr uint8_t BUFFER_SIZE = 200;
    char buffer[BUFFER_SIZE];

    size_t length = comm_port.readBytesUntil('\n', buffer, BUFFER_SIZE - 1);
    buffer[length] = '\0';
    

    JsonDocument msg;
    DeserializationError error = deserializeJson(msg, buffer);

    if (error) {
        Serial.print("JSON error: ");
        Serial.println(error.c_str());
    }

    float ph = msg["pH"];
    float ec = msg["ec"];
    float temp = msg["temp"];

    std::array<float, 4> data = {ph, ec, temp};

return data;
}