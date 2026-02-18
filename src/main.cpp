#include "tds_sensor.hpp"
#include "ph_sensor.hpp"
#include "motor.hpp"
#include <numeric>
#include <array>

// TDS Sensors
#define TDS1_PIN A1
#define TDS2_PIN A2 
#define TDS3_PIN A3
#define TDS4_PIN A4

// pH Sensors
#define pH1_PIN A5
#define pH2_PIN A6

// pH Up Bottle Pump
#define pH_UP_DIR_PIN 1
#define pH_UP_STEP_PIN 2

// pH Down Bottle Pump
#define pH_DOWN_DIR_PIN 3
#define pH_DOWN_STEP_PIN 4

// Green Bottle Pump
#define GRO_DIR_PIN 5
#define GRO_STEP_PIN 6

// Purple Bottle Pump
#define MICRO_DIR_PIN 7
#define MICRO_STEP_PIN 8

// Pink Bottle Pump
#define BLOOM_DIR_PIN 9
#define BLOOM_STEP_PIN 10


//! Temporary define, will add better way later probably
#define true_tds_val 660

tds_sensor tds1(TDS1_PIN);
tds_sensor tds2(TDS2_PIN);
tds_sensor tds3(TDS3_PIN);
tds_sensor tds4(TDS4_PIN);

ph_sensor ph1(pH1_PIN);
ph_sensor ph2(pH2_PIN);
 
motor ph_up(pH_UP_DIR_PIN, pH_UP_STEP_PIN);
motor ph_down(pH_DOWN_DIR_PIN, pH_DOWN_STEP_PIN);
motor gro(GRO_DIR_PIN, GRO_STEP_PIN); // Green
motor micro(MICRO_DIR_PIN, MICRO_STEP_PIN); // Purple
motor bloom(BLOOM_DIR_PIN, BLOOM_STEP_PIN); // Pink

bool calibrated = false;

void calibrate() {
  Serial.println("Calibrating Sensors...");
  tds1.calibrate(true_tds_val);
  tds2.calibrate(true_tds_val);
  tds3.calibrate(true_tds_val);
  tds4.calibrate(true_tds_val);

  ph1.calibrate();
  ph2.calibrate();
}

void setup() {
  Serial.begin(9600);
  analogReadResolution(12);
}


void loop() {

  std::array<float, 4> tds_values = {tds1.read_val(), tds2.read_val(), tds3.read_val(), tds4.read_val()};
  std::array<float, 2> ph_values = {ph1.read_val(), ph2.read_val()};

  float tds_mean = std::accumulate(tds_values.begin(), tds_values.end(), 0.0);
  float ph_mean = std::accumulate(ph_values.begin(), ph_values.end(), 0.0);

  Serial.print("TDS 1 Value: ");
  Serial.println(tds_values[0]);

  Serial.print("TDS 2 Value: ");
  Serial.println(tds_values[1]); 

  Serial.print("TDS Average: ");
  Serial.println(tds_mean);

  Serial.print("TDS 1 Value: ");
  Serial.println(ph_values[0]);

  Serial.print("TDS 2 Value: ");
  Serial.println(ph_values[1]); 

  Serial.print("TDS Average: ");
  Serial.println(ph_mean);


  ph_up.test();
  ph_down.test();

  // if (!calibrated) {
    // calibrate();
    // calibrated = true;
  // }

  delay(1000);
}




