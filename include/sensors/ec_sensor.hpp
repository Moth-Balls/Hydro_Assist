/*#################################################*/ 
/* EC Sensor class definition and member functions*/
/*#################################################*/ 
#pragma once

#include <Arduino.h> 

class ec_sensor {
  public:
    ec_sensor(uint8_t pin, float compensation_val);
    float read_val(float temp = 25.0);
    
  private:
    float compensation_val;
    uint8_t analog_pin;
    
};

// Constructor
ec_sensor::ec_sensor(uint8_t pin, float compensation_val) : compensation_val(compensation_val), analog_pin(pin) {}

// Read Sensor
float ec_sensor::read_val(float temp) {

    uint16_t sensor_val = analogRead(this->analog_pin);
    
    float voltage = static_cast<float>(sensor_val) * (3.3f / 4095.0f); 

    float temp_coefficient = 1.0 + 0.02 * (temp - 25.0);
    float compensated_voltage = voltage / temp_coefficient;

    float ec_microS = compensated_voltage * this->compensation_val;
  
    float ec_milliS = ec_microS / 1000.0f;

    return ec_milliS;
    // return ec_microS;
}