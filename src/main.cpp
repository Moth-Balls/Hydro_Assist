#include "tds_sensor.hpp"
#include "ph_sensor.hpp"
#include <array>
#include <numeric>
#include "arm_math.h"

#define TDS1_PIN A1
#define TDS2_PIN A2 
#define TDS3_PIN A3
#define TDS4_PIN A4

#define pH1_PIN A5
#define pH2_PIN A6

//! Temporary define, will add better way later probably
#define true_tds_val 660

tds_sensor tds1(TDS1_PIN);
tds_sensor tds2(TDS2_PIN);
tds_sensor tds3(TDS3_PIN);
tds_sensor tds4(TDS4_PIN);
bool calibrated = false;


void calibrate() {
  Serial.println("Calibrating Sensors...");
  tds1.calibrate(true_tds_val);
  tds2.calibrate(true_tds_val);
}


void setup() {
  Serial.begin(9600);
  analogReadResolution(12);
}

void loop() {
  // Read TDS values
  float tds1_val = tds1.read_val();
  float tds2_val = tds2.read_val();
  std::array<float, 4> tds_values = {tds1.read_val(), tds2.read_val(), tds3.read_val(), tds4.read_val()};

  float tds_mean = std::accumulate(tds_values.begin(), tds_values.end(), 0.0);

  Serial.print("TDS 1 Value: ");
  Serial.println(tds1_val);

  Serial.print("TDS 2 Value: ");
  Serial.println(tds2_val); 

  Serial.print("TDS Average: ");
  Serial.println(tds_mean);
  

  if (!calibrated) {
    calibrate();
    calibrated = true;
  }

  delay(1000); // Delay for 1 second
}



