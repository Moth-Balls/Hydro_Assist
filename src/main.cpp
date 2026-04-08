#include "sensors/ec_sensor.hpp"
#include "sensors/ph_sensor.hpp"
#include "sensors/temp_sensor.hpp"
#include "serial_comm.hpp"
#include "controller.hpp"
#include "kalman.hpp"
#include "motor.hpp"

#include "wiring_private.h"
#include <numeric>
#include <array>

#define DEBUG_PORT Serial
#define COMM_PORT Serial1
#define TMC2209_PORT Serial2

//!#############################*/
//!######## Pin Defines ########*/
//!#############################*/

// EC Sensors
#define EC1_PIN A2
#define EC2_PIN A3
#define EC3_PIN A4
#define EC4_PIN A5

// pH Sensors
#define pH1_PIN A0
#define pH2_PIN A1

// Temp Sensors
#define TEMP1_PIN A6
#define TEMP2_PIN A7
#define TEMP3_PIN A8
#define TEMP4_PIN A9

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


//!#######################################*/
//!######## Kalman Reference Vals ########*/
//!#######################################*/


//TODO Temporary define, will add better way later probably
#define true_ec_val 1177
#define true_ph_val 6.5


//!#######################################*/
//!######## Serial 2 Define ##############*/
//!#######################################*/

// Serial2 setup for TMC2209 driver setup.
Uart Serial2(&sercom5, 2, 3, SERCOM_RX_PAD_3, UART_TX_PAD_0); // Pin 2 RX / 3 TX

void SERCOM5_0_Handler() { Serial2.IrqHandler(); }
void SERCOM5_1_Handler() { Serial2.IrqHandler(); }
void SERCOM5_2_Handler() { Serial2.IrqHandler(); }
void SERCOM5_3_Handler() { Serial2.IrqHandler(); }

// #define SERIAL_PORT Serial2

//!################################################*/
//!######## Sensor & Motor Object Creation ########*/
//!################################################*/

// EC Sensors
ec_sensor ec1(EC1_PIN, 78.08493545);
ec_sensor ec2(EC2_PIN, 75.48113613);
ec_sensor ec3(EC3_PIN, 113.20943457);
ec_sensor ec4(EC4_PIN, 66.01233875);


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


void setup() {
  DEBUG_PORT.begin(115200);
  COMM_PORT.begin(9600); 
  TMC2209_PORT.begin(115200); 

  // Reroute Pins to be serial
  pinPeripheral(2, PIO_SERCOM_ALT);
  pinPeripheral(3, PIO_SERCOM_ALT);

  // Initialize Motors
  ph_up.init();
  ph_down.init();
  gro.init();
  micro.init();
  bloom.init();

  analogReadResolution(12);

  // Init filter vals
  ec_kalman.x = true_ec_val;
  ec_kalman.p = 0.3;

  pH_kalman.x = 6.5;
  pH_kalman.p = 0.1;

  temp_kalman.x = 25.0;
  temp_kalman.p = 0.4;
}

void loop() {

  static float volume; //TODO needs to be figured out

  std::array<float, 4> ec_raw = {ec1.read_val(), ec2.read_val(), ec3.read_val(), ec4.read_val()};
  std::array<float, 2> ph_raw = {ph1.read_val(), ph2.read_val()};
  std::array<float, 4> temp_raw = {temp1.read_val(), temp2.read_val(), temp3.read_val(), temp4.read_val()};

  float ph_val = ph_filter(ph_raw, pH_kalman, 0.1);
  float ec_val = ec_filter(ec_raw, ec_kalman, 0.1);
  float temp_val = temp_filter(temp_raw, temp_kalman, 0.1);

  send_data(DEBUG_PORT, ph_val, ec_val, temp_val); // Display the data in monitor

  send_data(COMM_PORT, ph_val, ec_val, temp_val); // Send data to ESP32
  
  // test_all_motors(ph_up, ph_down, gro, micro, bloom);


  delay(500);
}