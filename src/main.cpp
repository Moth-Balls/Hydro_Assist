#include "ec_sensor.hpp"
#include "ph_sensor.hpp"
#include "motor.hpp"
#include <numeric>
#include <array>
#include "kalman.hpp"

// ec Sensors
#define EC1_PIN A3
#define EC2_PIN A4 
#define EC3_PIN A5
#define EC4_PIN A6

// pH Sensors
#define pH1_PIN A1
#define pH2_PIN A2

// pH Up Bottle Pump
#define pH_UP_STEP_PIN 13
#define pH_UP_DIR_PIN 12

// pH Down Bottle Pump
#define pH_DOWN_STEP_PIN 11
#define pH_DOWN_DIR_PIN 10

// Green Bottle Pump
#define GRO_STEP_PIN 9
#define GRO_DIR_PIN 6

// Purple Bottle Pump
#define MICRO_STEP_PIN 5
#define MICRO_DIR_PIN 22

// Pink Bottle Pump
#define BLOOM_STEP_PIN 21
#define BLOOM_DIR_PIN 25

//! Temporary define, will add better way later probably
#define true_ec_val 1177

// ec Sensors
ec_sensor ec1(EC1_PIN, 78.08493545);
ec_sensor ec2(EC2_PIN, 75.48113613);
ec_sensor ec3(EC3_PIN, 113.20943457);
ec_sensor ec4(EC4_PIN, 66.01233875);

// pH Sensors
ph_sensor ph1(pH1_PIN);
ph_sensor ph2(pH2_PIN);

// Motors with TMC2209 configuration
motor ph_up(pH_UP_DIR_PIN, pH_UP_STEP_PIN);
motor ph_down(pH_DOWN_DIR_PIN, pH_DOWN_STEP_PIN);
motor gro(GRO_DIR_PIN, GRO_STEP_PIN); // Green
motor micro(MICRO_DIR_PIN, MICRO_STEP_PIN); // Purple
motor bloom(BLOOM_DIR_PIN, BLOOM_STEP_PIN); // Pink

KalmanFilter ec_kalman;
KalmanFilter pH_kalman;

bool calibrated = false;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  // Serial1.begin(115200);

  // Init filter vals
  ec_kalman.x = true_ec_val;
  ec_kalman.p = 0.3;

  pH_kalman.x = 7.5;
  pH_kalman.p = 0.1;

}

void loop() {

  std::array<float, 4> ec_values = {ec1.read_val(), ec2.read_val(), ec3.read_val(), ec4.read_val()};
  // std::array<float, 2> ph_values = {ph1.read_val(), ph2.read_val()};

  // Print sensor values
  Serial.print("ec 1 Value: ");
  Serial.println(ec_values[0]);

  Serial.print("ec 2 Value: ");
  Serial.println(ec_values[1]); 

  Serial.print("ec 3 Value: ");
  Serial.println(ec_values[2]); 

  Serial.print("ec 4 Value: ");
  Serial.println(ec_values[3]); 

  // Serial.print("pH 1 Value: ");
  // Serial.println(ph_values[0]);

  // Serial.print("pH 2 Value: ");
  // Serial.println(ph_values[1]); 

  float ec_filtered = ec_filter(ec_values, ec_kalman, 0.3);
  // float pH_filtered = ph_filter(ph_values, pH_kalman, 0.1);

  Serial.print("EC Filtered Value:");
  Serial.println(ec_filtered);

  // Serial.print("pH Filtered Value:");
  // Serial.println(pH_filtered);

  // Test motor movements
  // ph_up.test();
  // ph_down.test();
  // gro.test();
  // micro.test();
  // bloom.test();

  delay(1000); // Delay for 1 second
}




