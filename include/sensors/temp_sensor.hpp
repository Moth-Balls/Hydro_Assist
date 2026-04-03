#pragma once

#include <Arduino.h>
#include <math.h>

class temp_sensor {
public:
    temp_sensor(uint8_t pin);
    float read_val();

private:
    uint8_t read_pin;

    const float fixed_resistance = 10000.0f;
    const float v_ref = 3.3f;
    const float nominal_resistance = 10000.0f;
    const float nominal_temp = 25.0f;
    const float b_coefficient = 3950.0f;
};

temp_sensor::temp_sensor(uint8_t pin) : read_pin(pin){}


float temp_sensor::read_val() {

    int raw = analogRead(this->read_pin);
    float voltage = raw * (this->v_ref / 4095.0); 
    float sensor_resistance = fixed_resistance * (v_ref / voltage - 1.0);

    float value;
    value = sensor_resistance / nominal_resistance; 
    value = log(value);                         
    value /= b_coefficient;                         
    value += 1.0 / (nominal_temp + 273.15);   
    value = 1.0 / value;                         
    value -= 273.15;    

return value;
}


