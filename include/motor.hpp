#pragma once

#include <Arduino.h>
// #include <TMCStepper.h>
#include <AccelStepper.h>


class motor {
    public:
        motor(uint8_t DIR, uint8_t STEP);

        void stop();

        void test();

        void dose(uint16_t volume);









    private:
    uint8_t DIR_PIN;
    uint8_t STEP_PIN;

    AccelStepper stepper;


};



motor::motor(uint8_t DIR, uint8_t STEP) : DIR_PIN(DIR), STEP_PIN(STEP), stepper(AccelStepper::DRIVER, STEP, DIR) {

    stepper.setMaxSpeed(1000);
    stepper.setAcceleration(500);

}



