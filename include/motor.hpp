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
        void test();
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
    stepper.setMaxSpeed(4000);
    stepper.setAcceleration(500);
    stepper.setSpeed(4000);

    driver.begin();
    driver.toff();
    driver.rms_current(800);
    driver.microsteps(16);
    driver.pdn_disable(true);
    driver.I_scale_analog(false);
    driver.en_spreadCycle(false);
}

void Motor::stop() {
    stepper.stop();
}

void Motor::test() {
    stepper.runSpeed();
}

void Motor::dose(float volume) {
    // Implementation for dosing
}


void test_all_motors(Motor &motor1, Motor &motor2, Motor &motor3, Motor &motor4, Motor &motor5) {

motor1.test();
motor2.test();
motor3.test();
motor4.test();
motor5.test();
}











