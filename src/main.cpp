#include "sensors/ec_sensor.hpp"
#include "sensors/ph_sensor.hpp"
#include "sensors/temp_sensor.hpp"
#include "sensors/water_level_sensor.hpp"

#include "serial_comm.hpp"
#include "controller.hpp"
#include "kalman.hpp"
#include "motor.hpp"
#include "plant_profile.hpp"

#include "wiring_private.h"
#include <numeric>
#include <array>

#define DEBUG_PORT Serial
#define COMM_PORT Serial1
#define TMC2209_PORT TMC2209_Serial

//!#############################*/
//!######## Pin Defines ########*/
//!#############################*/

// EC Sensors
#define EC1_PIN A5
#define EC2_PIN A9
#define EC3_PIN A13
#define EC4_PIN A1

// pH Sensors
#define pH1_PIN A3
#define pH2_PIN A15

// Temp Sensors
#define TEMP1_PIN A2 
#define TEMP2_PIN A6
#define TEMP3_PIN A10
#define TEMP4_PIN A14


// Mixing Motor
#define MIX_PIN_IN1 37
#define MIX_PIN_IN2 39
#define MIX_PIN_ENA 35

// pH Up Bottle Pump
#define pH_UP_STEP_PIN 50
#define pH_UP_DIR_PIN 52

// pH Down Bottle Pump
#define pH_DOWN_STEP_PIN 48
#define pH_DOWN_DIR_PIN 46

// Green Bottle Pump
#define GRO_STEP_PIN 36
#define GRO_DIR_PIN 34

// Purple Bottle Pump
#define MICRO_STEP_PIN 38
#define MICRO_DIR_PIN 40

// Pink Bottle Pump
#define BLOOM_STEP_PIN 42
#define BLOOM_DIR_PIN 44

//!#######################################*/
//!######## Serial 2 Define ##############*/
//!#######################################*/

Uart TMC2209_Serial(&sercom1, 17, 16, SERCOM_RX_PAD_1, UART_TX_PAD_0);

// Handlers for SERCOM1
void SERCOM1_0_Handler() { TMC2209_Serial.IrqHandler(); }
void SERCOM1_1_Handler() { TMC2209_Serial.IrqHandler(); }
void SERCOM1_2_Handler() { TMC2209_Serial.IrqHandler(); }
void SERCOM1_3_Handler() { TMC2209_Serial.IrqHandler(); }

#define TMC2209_PORT TMC2209_Serial


//!################################################*/
//!######## Sensor & Motor Object Creation ########*/
//!################################################*/

// EC Sensors
ec_sensor ec1(EC1_PIN, 660.37735849f);
ec_sensor ec2(EC2_PIN, 2456.14035088f);
ec_sensor ec3(EC3_PIN, 633.4841629f);
ec_sensor ec4(EC4_PIN, 583.33333333f);


// pH Sensors
ph_sensor ph1(pH1_PIN);
ph_sensor ph2(pH2_PIN);


// Temp Sensors
temp_sensor temp1(TEMP1_PIN);
temp_sensor temp2(TEMP2_PIN);
temp_sensor temp3(TEMP3_PIN);
temp_sensor temp4(TEMP4_PIN);


// Motors with TMC2209 configuration
Motor ph_up(pH_UP_DIR_PIN, pH_UP_STEP_PIN, TMC2209_PORT);
Motor ph_down(pH_DOWN_DIR_PIN, pH_DOWN_STEP_PIN, TMC2209_PORT);
Motor gro(GRO_DIR_PIN, GRO_STEP_PIN, TMC2209_PORT); // Green
Motor micro(MICRO_DIR_PIN, MICRO_STEP_PIN, TMC2209_PORT); // Purple
Motor bloom(BLOOM_DIR_PIN, BLOOM_STEP_PIN, TMC2209_PORT); // Pink


// Kalman filter objects
KalmanFilter ec_kalman;
KalmanFilter pH_kalman;
KalmanFilter temp_kalman;

TargetPlant plant;
std::string plant_name = "Arugula";

void setup() {
  DEBUG_PORT.begin(115200);
  COMM_PORT.begin(115200); 
  TMC2209_PORT.begin(115200); 

  Wire.begin();

  pinMode(MIX_PIN_IN1, OUTPUT);
  pinMode(MIX_PIN_IN2, OUTPUT);
  pinMode(MIX_PIN_ENA, OUTPUT);


  pinPeripheral(16, PIO_SERCOM);
  pinPeripheral(17, PIO_SERCOM);

  // Initialize Motors
  ph_up.init();
  ph_down.init();
  gro.init();
  micro.init();
  bloom.init();

  analogReadResolution(12);

  // Init filter vals
  ec_kalman.x = 0.7;
  ec_kalman.p = 0.3;

  pH_kalman.x = 6.5;
  pH_kalman.p = 0.1;

  temp_kalman.x = 22.0;
  temp_kalman.p = 0.4;

  // // plant.name = "Arugula";
  plant.ec_high = 1.8f;
  plant.ec_low = 0.8f;
  plant.ec_avg = (plant.ec_high + plant.ec_low) / 2.0f;
  plant.ph_high = 6.8f;
  plant.ph_low = 6.0f;
  plant.ph_avg = (plant.ph_high + plant.ph_low) / 2.0f;

  plant.gro_amount = 1;
  plant.bloom_amount = 1;
  plant.micro_amount = 1;

}

void loop() {

  // Read sensors
  float volume = read_volume_liters();

  // float volume = 10.0f;

  // Read temp and filter before passing to ph sensor read()
  std::array<float, 4> temp_raw = {temp1.read_val(), temp2.read_val(), temp3.read_val(), temp4.read_val()};
  float temp_val = temp_filter(temp_raw, temp_kalman, 0.1);

  std::array<float, 4> ec_raw = {ec1.read_val(), ec2.read_val(), ec3.read_val(), ec4.read_val()};
  std::array<float, 2> ph_raw = {ph1.read_val(temp_val), ph2.read_val(temp_val)};

  // Filter sensor data
  float ec_val = ec_filter(ec_raw, ec_kalman, 0.1);
  float ph_val = ph_filter(ph_raw, pH_kalman, 0.1);

  // Calc nutrient and ph dosing amount
  float nutrient_dose = nutrient_calc(plant.ec_avg, plant.ec_low, plant.ec_high, volume, ec_val);
  float ph_up_dose = ph_up_calc(plant.ph_avg, plant.ph_low, plant.ph_high, volume, ph_val);
  float ph_down_dose = ph_down_calc(plant.ph_avg, plant.ph_low, plant.ph_high, volume, ph_val);

  // Split nutrients proportionally for the 3 nutrients
  std::array<float, 3> dose = proportion_nutrient(nutrient_dose, plant.gro_amount, plant.micro_amount, plant.bloom_amount);

  // Dose 
  static bool nut_dosed_check = false;
  static bool ph_dosed_check = false;

  // Dose and check if nutrient or ph was dosed
  nut_dosed_check = (gro, dose[0], bloom, dose[1], micro, dose[2]);
  ph_dosed_check = dose_ph(ph_up, ph_up_dose, ph_down, ph_down_dose);

  // Check if nutrient or ph was dosed. Mix if either was dosed, do nothing if not
  if (nut_dosed_check || ph_dosed_check) {
    mix_resevoir(MIX_PIN_IN1, MIX_PIN_IN2, MIX_PIN_ENA);
    DEBUG_PORT.println("Dosed. Starting mixing");
  } else {
    DEBUG_PORT.println("Nothing dosed. Skipping mixing");
  }

  //! This currently pauses serial comms so messes with motors !//
  // //send_data(DEBUG_PORT, ph_val, ec_val, temp_val); // Display the data in monitor
  // //send_data(COMM_PORT, ph_val, ec_val, temp_val); // Send data to ESP32

}