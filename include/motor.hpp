#pragma once

#include <Arduino.h>
#include <TMCStepper.h>
#include <AccelStepper.h>

#define R_SENSE 0.11
#define DRIVER_ADDRESS 0b00

class Motor {
    public:
        Motor(uint8_t DIR, uint8_t STEP, HardwareSerial &SERIAL_PORT);
        
        void init();
        
        void stop();
        void test(int steps);
        void prime();
        void dose(float volume);

    private:
        uint8_t DIR_PIN;
        uint8_t STEP_PIN;

        AccelStepper stepper;
        TMC2209Stepper driver;
};


Motor::Motor(uint8_t DIR, uint8_t STEP, HardwareSerial &SERIAL_PORT) 
    : DIR_PIN(DIR), STEP_PIN(STEP), stepper(AccelStepper::DRIVER, STEP, DIR), driver(&SERIAL_PORT, R_SENSE, DRIVER_ADDRESS) {
}

void Motor::init() {
    driver.begin();

    driver.toff(5);
    driver.rms_current(2000); // Set max current to 2A
    driver.microsteps(16);
    driver.pdn_disable(true);
    driver.I_scale_analog(false);
    driver.en_spreadCycle(false);

    stepper.setMaxSpeed(6000);
    stepper.setAcceleration(500);
    stepper.setSpeed(4000);

}

void Motor::stop() {
    stepper.stop();
}

void Motor::test(int steps) {

    stepper.move(steps);
    stepper.runToPosition();
}

void Motor::prime() {
    
    stepper.move(25000);
    stepper.runToPosition();
}

void Motor::dose(float volume) {
    if (volume <= 0.0) return; 

    // Experimental value
    static const int steps_per_ml = 488;

    // Multiply exact float volume by steps_per_ml, then round to the nearest whole step
    int steps_to_move = std::round(volume * static_cast<float>(steps_per_ml));
    
    stepper.move(steps_to_move);
    stepper.runToPosition(); // Blocks until the motor reaches the target position
}


bool dose_nutrients(Motor &nutrient_1_motor, float nutrient_1_amount, Motor &nutrient_2_motor, float nutrient_2_amount, Motor &nutrient_3_motor, float nutrient_3_amount) {

    nutrient_1_motor.dose(nutrient_1_amount);
    nutrient_2_motor.dose(nutrient_2_amount);
    nutrient_3_motor.dose(nutrient_3_amount);

    
}

bool dose_ph(Motor &ph_up_motor, float &ph_up_amount, Motor &ph_down_motor, float &ph_down_amount) {

    ph_up_motor.dose(ph_up_amount);
    ph_down_motor.dose(ph_down_amount);
}

void mix_resevoir(uint8_t IN1, uint8_t IN2, uint8_t ENA) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);

    analogWrite(ENA, 127); 
    delay(7000); // Mix for 7 seconds
    analogWrite(ENA, 0);
}










