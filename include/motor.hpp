#pragma once

#include <Arduino.h>
#include <TMCStepper.h>
#include <AccelStepper.h>

#define R_SENSE 0.11

class motor {
    public:
        motor(uint8_t DIR, uint8_t STEP);

        void stop();

        void test();

        void dose(float volume);

        void move(float speed, bool dir);

    private:
        uint8_t DIR_PIN;
        uint8_t STEP_PIN;

        AccelStepper stepper;
        // TMC2209Stepper driver;
};

motor::motor(uint8_t DIR, uint8_t STEP) : DIR_PIN(DIR), STEP_PIN(STEP), stepper(AccelStepper::DRIVER, STEP, DIR) {
    stepper.setMaxSpeed(1000);
    stepper.setAcceleration(5000);
    stepper.setSpeed(500);

    // driver.begin();
    // driver.rms_current(800);
    // driver.microsteps(16);
    // driver.en_spreadCycle(false);
}

void motor::stop() {
    stepper.stop();
}

void motor::test() {
    stepper.moveTo(1000);

    while (stepper.distanceToGo() != 0) {
        stepper.run();
    }

    stepper.stop();

    stepper.moveTo(-1000);

    while (stepper.distanceToGo() != 0) {
        stepper.run();
    }

    stepper.stop();
}

void motor::dose(float volume) {
    // Implementation for dosing
}











