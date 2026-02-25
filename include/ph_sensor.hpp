/*#################################################*/ 
/* pH Sensor class definition and member functions*/
/*#################################################*/ 
#pragma once

#include <Arduino.h>

class ph_sensor {
    public:
        ph_sensor(uint8_t pin);
        float read_val();


    private:
        // float OFFSET;
        // float SCALE;
        uint8_t analog_pin;
        
};


// Constructor
ph_sensor::ph_sensor(uint8_t pin){}

// Read Sensor
float ph_sensor::read_val() {
    float raw = analogRead(this->analog_pin);
    float ph = (-0.0225 * raw) + 24.36;

    return ph;
}
