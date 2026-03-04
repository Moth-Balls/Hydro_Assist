#include "motor.hpp"
#include <TMCStepper.h>
#include <AccelStepper.h>


#define STEP_PIN 13
#define DIR_PIN 12

#define STEP2_PIN 11
#define DIR2_PIN 10

#define SERIAL_PORT Serial1  // Use Serial1 for UART communication
#define DRIVER_ADDRESS 0b00  // TMC2209 driver address (set via MS1/MS2 pins)

// TMC2209 driver instance
TMC2209Stepper driver(&SERIAL_PORT, 0.11, DRIVER_ADDRESS);
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
AccelStepper stepper2(AccelStepper::DRIVER, STEP2_PIN, DIR2_PIN);

void setup() {
  // Start USB serial for debugging
  Serial.begin(115200);
  while (!Serial);  // Wait for USB serial connection

  // Start Serial1 for TMC2209 communication
  SERIAL_PORT.begin(115200);

  // Initialize the TMC2209 driver
  driver.begin();
  driver.toff(5);  // Enable driver with a default off time
  driver.rms_current(600);  // Set motor current to 1A
  driver.microsteps(16);  // Set microstepping to 1/16
  driver.pdn_disable(true);  // Use UART instead of PDN pin
  driver.I_scale_analog(false);  // Use internal current scaling
  driver.en_spreadCycle(false);  // Enable StealthChop for quiet operation

  // Configure AccelStepper
  stepper.setMaxSpeed(4000);  // Set maximum speed (steps per second)
  stepper.setAcceleration(500);  // Set acceleration (steps per second^2)
  stepper.setSpeed(3000);  // Set initial speed (steps per second)

  stepper2.setMaxSpeed(4000);
  stepper2.setAcceleration(500);
  stepper2.setSpeed(-1000);


}

void loop() {
  stepper.runSpeed();
  stepper2.runSpeed();

  Serial.println("Still working");
}