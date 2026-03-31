#pragma once

#include <Arduino.h>

class temp_sensor {
public:
    temp_sensor(uint8_t pin);
    float read_val();

private:
    uint8_t read_pin;
};


temp_sensor::temp_sensor(uint8_t pin) : read_pin(pin){}


float temp_sensor::read_val() {

return 0.0;
}


