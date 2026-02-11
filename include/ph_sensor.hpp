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
    Serial.println("Calibrating pH Sensor...");

    static const float TRUE_VAL_7 = 7.0;
    static const float TRUE_VAL_4 = 4.0;

    // Measure voltage at pH 7.0
    Serial.println("Place the sensor in pH 7.0 solution...");
    delay(10000);
    float voltage_7 = static_cast<float>(analogRead(this->analog_pin)) * (5.0 / 1023.0);

    // Measure voltage at pH 4.0
    Serial.println("Place the sensor in pH 4.0 solution...");
    delay(10000);
    float voltage_4 = static_cast<float>(analogRead(this->analog_pin)) * (5.0 / 1023.0);

    // Calculate SCALE and OFFSET
    this->SCALE = (TRUE_VAL_4 - TRUE_VAL_7) / (voltage_4 - voltage_7);
    this->OFFSET = TRUE_VAL_7 - (this->SCALE * voltage_7);

    Serial.println("Calibration Complete!");
    Serial.print("SCALE: ");
    Serial.println(this->SCALE);
    Serial.print("OFFSET: ");
    Serial.println(this->OFFSET);
}


// Read Sensor
float ph_sensor::read_val() {

    uint16_t sensor_val = analogRead(this->analog_pin);
    float voltage = static_cast<float>(sensor_val) * (5.0 / 1023.0);

    float ph = SCALE * voltage + OFFSET;

    return ph;
}
