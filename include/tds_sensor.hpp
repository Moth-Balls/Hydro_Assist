/*#################################################*/ 
/* TDS Sensor class definition and member functions*/
/*#################################################*/ 
#pragma once

#include <Arduino.h> 

class tds_sensor {
  public:
    tds_sensor(uint8_t pin);
    void calibrate(float true_val);
    float read_val();
    

  private:
    float compensation_val;
    float uncomp_val;
    uint8_t analog_pin;
    
};

// Constructor
tds_sensor::tds_sensor(uint8_t pin) : compensation_val(300.0), analog_pin(pin), uncomp_val(0.0) {}


// Sensor Calibration
void tds_sensor::calibrate(float true_val) {
    this->compensation_val = true_val / this->uncomp_val;
    Serial.println("TDS Sensor Calibrated");
}

// Sensor read
float tds_sensor::read_val() {

    uint16_t read_val = analogRead(this->analog_pin);
    float voltage = static_cast<float>(read_val) * (5.0 / 1023.0); 
    this->uncomp_val = voltage;

    float val_adjusted = voltage * this->compensation_val;
    
    return val_adjusted;
}