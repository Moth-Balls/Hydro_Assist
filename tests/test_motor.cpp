#include "motor.hpp"
#include <TMCStepper.h>
#include <AccelStepper.h>


#define STEP_PIN1 5
#define DIR_PIN1 6

#define STEP_PIN2 9
#define DIR_PIN2 10


#define SERIAL_PORT Serial1  // Use Serial1 for UART communication
#define DRIVER_ADDRESS 0b00  // TMC2209 driver address (set via MS1/MS2 pins)

// TMC2209 driver instance
// TMC2209Stepper driver(&SERIAL_PORT, 0.11, DRIVER_ADDRESS);
// AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

Motor motor1(DIR_PIN1, STEP_PIN1, SERIAL_PORT);
Motor motor2(DIR_PIN2, STEP_PIN2, SERIAL_PORT);


void setup() {
  // Start USB serial for debugging
  Serial.begin(115200);
  // while (!Serial);  // Wait for USB serial connection

  // Start Serial1 for TMC2209 communication
  SERIAL_PORT.begin(115200);

  // // Initialize the TMC2209 driver
  // driver.begin();
  // driver.toff(5);  // Enable driver with a default off time
  // driver.rms_current(2000);  // Set motor current to 1A
  // driver.microsteps(16);  // Set microstepping to 1/16
  // driver.pdn_disable(true);  // Use UART instead of PDN pin
  // driver.I_scale_analog(false);  // Use internal current scaling
  // driver.en_spreadCycle(false);  // Enable StealthChop for quiet operation

  // // Configure AccelStepper
  // stepper.setMaxSpeed(8000);  // Set maximum speed (steps per second)
  // stepper.setAcceleration(500);  // Set acceleration (steps per second^2)
  // stepper.setSpeed(3000);  // Set initial speed (steps per second)

  motor1.init();
  motor2.init();

}

void loop() {
  // stepper.runSpeed();


  motor1.test(); 
  motor2.test(); 



  


  

  Serial.println("Still working");
}