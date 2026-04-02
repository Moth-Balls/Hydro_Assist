#pragma once

#include <Arduino.h>
#include <cstdint>
#include <array>
#include <ArduinoJson.h>



void send_data(Stream &comm_port, const float &ph_val, const float &ec_val, const float &temp_val) {
    JsonDocument msg;
    msg["pH"] = ph_val;
    msg["ec"] = ec_val;
    msg["temp"] = temp_val;

    serializeJson(msg, comm_port);
    comm_port.print('\n');
    comm_port.flush();
}


std::array<float, 3> read_data(Stream &comm_port, Stream &logger_port) {
    static constexpr uint8_t size = 255;
    char buffer[size];

    size_t bytesRead = comm_port.readBytesUntil('\n', buffer, size - 1);
    buffer[bytesRead] = '\0';

    if (bytesRead < 10) {
        return {0.0f, 0.0f, 0.0f}; 
    }
    
    JsonDocument msg;
    DeserializationError error = deserializeJson(msg, buffer);

    if (error) {
        logger_port.print("JSON error: ");
        logger_port.println(error.c_str());
        return {0.0f, 0.0f, 0.0f}; 
    }

    float ph = msg["pH"];
    float ec = msg["ec"];
    float temp = msg["temp"];

    return {ph, ec, temp};
}