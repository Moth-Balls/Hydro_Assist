/*#################################################*/ 
/* pH Sensor class definition and member functions*/
/*#################################################*/ 
#pragma once

#include <Arduino.h>

class ph_sensor {
    public:
        ph_sensor(uint8_t pin);
        
        float read_val(float current_temp_c = 25.0);

    private:
        // float OFFSET;
        // float SCALE;
        uint8_t analog_pin;
};

// Constructor
ph_sensor::ph_sensor(uint8_t pin) : analog_pin(pin) {}

// Read Sensor with Temperature Compensation
float ph_sensor::read_val(float current_temp_c) {
    float raw = analogRead(this->analog_pin);

    float raw_ph = (-0.0225 * raw) + 24.36;

    float current_temp_k = current_temp_c + 273.15;
    float reference_temp_k = 298.15;

    float compensated_ph = 7.0 + ((raw_ph - 7.0) * (reference_temp_k / current_temp_k));

    return compensated_ph;
}
