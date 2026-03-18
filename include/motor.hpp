#pragma once

#include <Arduino.h>
#include <TMCStepper.h>
#include <AccelStepper.h>

#define R_SENSE 0.11
#define DRIVER_ADDRESS 0b00

class motor {
    public:
        motor(uint8_t DIR, uint8_t STEP, HardwareSerial &SERIAL_PORT);

        void stop();

        void test();

        void dose(float volume);

    private:
        uint8_t DIR_PIN;
        uint8_t STEP_PIN;

        AccelStepper stepper;
        TMC2209Stepper driver;

};

motor::motor(uint8_t DIR, uint8_t STEP, HardwareSerial &SERIAL_PORT) : DIR_PIN(DIR), STEP_PIN(STEP), stepper(AccelStepper::DRIVER, STEP, DIR), driver(&SERIAL_PORT, R_SENSE, DRIVER_ADDRESS) {
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

void motor::stop() {
    stepper.stop();
}

void motor::test() {
    // stepper.runSpeed();
    stepper.move(1000);
}

void motor::dose(float volume) {
    // Implementation for dosing
}











