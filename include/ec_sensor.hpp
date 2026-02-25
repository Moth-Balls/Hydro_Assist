/*#################################################*/ 
/* EC Sensor class definition and member functions*/
/*#################################################*/ 
#pragma once

#include <Arduino.h> 

class ec_sensor {
  public:
    ec_sensor(uint8_t pin, float compensation_val);
    // void calibrate(float true_val);
    float read_val();
    
  private:
    float compensation_val;
    uint8_t analog_pin;
    
};


float ec_to_ec(float ec) {
  static const float conversion_factor = 0.7;
  return ec / conversion_factor;
}

// Constructor
ec_sensor::ec_sensor(uint8_t pin, float compensation_val) : compensation_val(compensation_val), analog_pin(pin) {}

// Read Sensor
float ec_sensor::read_val() {

    uint16_t sensor_val = analogRead(this->analog_pin);
    float voltage = static_cast<float>(sensor_val) * (3.3f / 4095.0f); 

    float val_adjusted = voltage * this->compensation_val;

    float ec = ec_to_ec(val_adjusted);
  
    return ec;
}