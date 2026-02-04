#include <Arduino.h>
#include <TMCStepper.h>

// Define pins for each driver
#define EN_PIN 19 // Shared Enable pin (optional)
#define R_SENSE 0.11f // Sense resistor value

// Driver 1
#define STEP_PIN_1 5
#define DIR_PIN_1 18
HardwareSerial SERIAL_PORT_1(2); // Use UART2

// Driver 2
#define STEP_PIN_2 4
#define DIR_PIN_2 21
HardwareSerial SERIAL_PORT_2(1); // Use UART1

// Driver 3
#define STEP_PIN_3 26
#define DIR_PIN_3 27
HardwareSerial SERIAL_PORT_3(0); // Use UART0

// Driver 4
#define STEP_PIN_4 12
#define DIR_PIN_4 13
HardwareSerial SERIAL_PORT_4(2); // Reuse UART2 with different pins

// Driver 5
#define STEP_PIN_5 23
#define DIR_PIN_5 2
HardwareSerial SERIAL_PORT_5(1); // Reuse UART1 with different pins

// TMC2209 Driver Instances
TMC2209Stepper driver1(&SERIAL_PORT_1, R_SENSE, 0b00);
TMC2209Stepper driver2(&SERIAL_PORT_2, R_SENSE, 0b01);
TMC2209Stepper driver3(&SERIAL_PORT_3, R_SENSE, 0b10);
TMC2209Stepper driver4(&SERIAL_PORT_4, R_SENSE, 0b11);
TMC2209Stepper driver5(&SERIAL_PORT_5, R_SENSE, 0b00);

void setup() {
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW); // Enable all drivers

  // Initialize UART ports
  SERIAL_PORT_1.begin(115200, SERIAL_8N1, 14, 15); // TX=14, RX=15
  SERIAL_PORT_2.begin(115200, SERIAL_8N1, 16, 17); // TX=16, RX=17
  SERIAL_PORT_3.begin(115200); // Default UART0 pins
  SERIAL_PORT_4.begin(115200, SERIAL_8N1, 32, 33); // TX=32, RX=33
  SERIAL_PORT_5.begin(115200, SERIAL_8N1, 25, 26); // TX=25, RX=26

  // Initialize drivers
  driver1.begin();
  driver2.begin();
  driver3.begin();
  driver4.begin();
  driver5.begin();

  // Set driver parameters
  driver1.rms_current(600);
  driver2.rms_current(600);
  driver3.rms_current(600);
  driver4.rms_current(600);
  driver5.rms_current(600);

  driver1.microsteps(16);
  driver2.microsteps(16);
  driver3.microsteps(16);
  driver4.microsteps(16);
  driver5.microsteps(16);
}

void loop() {
  // Example: Move each motor 200 steps forward and backward
  for (int i = 0; i < 200; i++) {
    digitalWrite(STEP_PIN_1, HIGH);
    digitalWrite(STEP_PIN_2, HIGH);
    digitalWrite(STEP_PIN_3, HIGH);
    digitalWrite(STEP_PIN_4, HIGH);
    digitalWrite(STEP_PIN_5, HIGH);
    delayMicroseconds(500);
    digitalWrite(STEP_PIN_1, LOW);
    digitalWrite(STEP_PIN_2, LOW);
    digitalWrite(STEP_PIN_3, LOW);
    digitalWrite(STEP_PIN_4, LOW);
    digitalWrite(STEP_PIN_5, LOW);
    delayMicroseconds(500);
  }
  delay(1000);
}




