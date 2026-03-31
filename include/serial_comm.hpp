#pragma once

#include <Arduino.h>
#include <cstdint>
#include <array>
// #include "SerialLogger.h"
#include <ArduinoJson.h>


void send_data(HardwareSerial &comm_port, const float &ph_val, const float &ec_val, const float &temp_val) {

    StaticJsonDocument<200> msg;
    msg["pH"] = ph_val;
    msg["ec"] = ec_val;
    msg["temp"] = temp_val;

    serializeJson(msg, comm_port);
}