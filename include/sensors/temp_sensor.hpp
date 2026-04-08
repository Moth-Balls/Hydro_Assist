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
    float voltage = raw * (this->v_ref / 4095.0f);
    if (voltage <= 0.0f || voltage >= this->v_ref) return NAN;

    float sensor_resistance = this->fixed_resistance * (voltage / (this->v_ref - voltage));

    float steinhart = sensor_resistance / this->nominal_resistance;
    steinhart = logf(steinhart);
    steinhart /= this->b_coefficient;
    steinhart += 1.0f / (this->nominal_temp + 273.15f);
    steinhart = 1.0f / steinhart;
    steinhart -= 273.15f;
    return steinhart;
}


