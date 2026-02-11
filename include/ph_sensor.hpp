/*#################################################*/ 
/* pH Sensor class definition and member functions*/
/*#################################################*/ 
#pragma once

#include <Arduino.h>

class ph_sensor {
    public:
        ph_sensor(uint8_t pin);
        void calibrate();
        float read_val();


    private:
        float OFFSET;
        float SCALE;
        uint8_t analog_pin;

};


// Constructor
ph_sensor::ph_sensor(uint8_t pin) : OFFSET(0.0), SCALE(3.5){}


// Sensor Calibration
void ph_sensor::calibrate() {


}


// Read Sensor
float ph_sensor::read_val() {

    uint16_t sensor_val = analogRead(this->analog_pin);
    float voltage = static_cast<float>(sensor_val) * (5.0 / 1023.0);

    float ph = SCALE * voltage + OFFSET;

    return ph;
}
