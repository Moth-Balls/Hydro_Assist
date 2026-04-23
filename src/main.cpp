#include "sensors/ec_sensor.hpp"
#include "sensors/ph_sensor.hpp"
#include "sensors/temp_sensor.hpp"
#include "sensors/water_level_sensor.hpp"

#include "serial_comm.hpp"
#include "controller.hpp"
#include "kalman.hpp"
#include "motor.hpp"
#include "plant_profile.hpp"
#include "noise.hpp"

#include "wiring_private.h"
#include <numeric>
#include <array>

#define DEBUG_PORT Serial
#define COMM_PORT  Serial1

//!#############################
//!######## Pin Defines ########
//!#############################

#define EC1_PIN  A5
#define EC2_PIN  A9
#define EC3_PIN  A13
#define EC4_PIN  A1

#define pH1_PIN  A3
#define pH2_PIN  A15

#define TEMP1_PIN A2
#define TEMP2_PIN A6
#define TEMP3_PIN A10
#define TEMP4_PIN A14

#define MIX_PIN_IN1 37
#define MIX_PIN_IN2 39
#define MIX_PIN_ENA 35

#define pH_UP_STEP_PIN   50
#define pH_UP_DIR_PIN    52
#define pH_DOWN_STEP_PIN 48
#define pH_DOWN_DIR_PIN  46
#define GRO_STEP_PIN     36
#define GRO_DIR_PIN      34
#define MICRO_STEP_PIN   38
#define MICRO_DIR_PIN    40
#define BLOOM_STEP_PIN   42
#define BLOOM_DIR_PIN    44

//!#######################################
//!######## Serial 2 (TMC2209) ###########
//!#######################################

Uart TMC2209_Serial(&sercom1, 17, 16, SERCOM_RX_PAD_1, UART_TX_PAD_0);
void SERCOM1_0_Handler() { TMC2209_Serial.IrqHandler(); }
void SERCOM1_1_Handler() { TMC2209_Serial.IrqHandler(); }
void SERCOM1_2_Handler() { TMC2209_Serial.IrqHandler(); }
void SERCOM1_3_Handler() { TMC2209_Serial.IrqHandler(); }
#define TMC2209_PORT TMC2209_Serial

//!################################################
//!######## Sensor & Motor Objects ################
//!################################################

ec_sensor ec1(EC1_PIN, 660.37735849f);
ec_sensor ec2(EC2_PIN, 2456.14035088f);
ec_sensor ec3(EC3_PIN, 633.4841629f);
ec_sensor ec4(EC4_PIN, 583.33333333f);

ph_sensor ph1(pH1_PIN);
ph_sensor ph2(pH2_PIN);

temp_sensor temp1(TEMP1_PIN);
temp_sensor temp2(TEMP2_PIN);
temp_sensor temp3(TEMP3_PIN);
temp_sensor temp4(TEMP4_PIN);

Motor ph_up(pH_UP_DIR_PIN,    pH_UP_STEP_PIN,   TMC2209_PORT);
Motor ph_down(pH_DOWN_DIR_PIN, pH_DOWN_STEP_PIN, TMC2209_PORT);
Motor gro(GRO_DIR_PIN,         GRO_STEP_PIN,     TMC2209_PORT);
Motor micro(MICRO_DIR_PIN,     MICRO_STEP_PIN,   TMC2209_PORT);
Motor bloom(BLOOM_DIR_PIN,     BLOOM_STEP_PIN,   TMC2209_PORT);

KalmanFilter ec_kalman;
KalmanFilter pH_kalman;
KalmanFilter temp_kalman;

TargetPlant plant;

//!##################################################
//!######## Volume / Depletion Config ###############
//!##################################################

static const int   NUM_PLANTS                  = 21;
static const float UPTAKE_ML_PER_PLANT_PER_DAY = 25.0f;
static const float INTERVALS_PER_DAY           = 96.0f;
static const float UPTAKE_PER_INTERVAL =
    (NUM_PLANTS * UPTAKE_ML_PER_PLANT_PER_DAY) / INTERVALS_PER_DAY;

static float reservoir_volume_ml   = 10000.0f;
static const float RESERVOIR_MIN_ML = 2000.0f;

//!##################################################
//!######## Latest sensor cache #####################
//! Populated every loop(), sent on M:1001 requests #
//!##################################################

static float latest_ph   = 0.0f;
static float latest_ec   = 0.0f;
static float latest_temp = 0.0f;

//!##################################################
//!######## System run state ########################
//! When false: no sensor reads, no automated dosing.
//! Serial polling and loading-dose (M:2001) still run.
//!##################################################

static bool is_system_running = false;

//!##################################################
//!######## Dosing timer ############################
//!##################################################

static unsigned long lastDoseMillis = 0;
static const unsigned long doseInterval = 15UL * 60UL * 1000UL;

// Rate-limit the USB debug mirror so it doesn't flood Serial and stall the loop.
static unsigned long lastDebugMillis = 0;
static const unsigned long debugInterval = 2000UL;

// Non-blocking reader for incoming COMM_PORT messages
static CommReader commReader;


// ─────────────────────────────────────────────────────────────────────────────
// handle_comm_message
// Dispatches on the "M" field of a fully-received JSON line from the ESP32.
// ─────────────────────────────────────────────────────────────────────────────
void handle_comm_message(const char *line) {
    JsonDocument doc;
    if (!parse_message(line, doc)) {
        DEBUG_PORT.print("COMM parse err: ");
        DEBUG_PORT.println(line);
        return;
    }

    uint16_t msgType = doc["M"] | 0;

    switch (msgType) {

        // ── ESP32 polling for sensor data ──────────────────────────────
        case MSG_SENSOR_REQUEST:   // 1001
            send_sensor_data(COMM_PORT, latest_ph, latest_ec, latest_temp);
            break;

        // ── Web-triggered loading dose ─────────────────────────────────
        // BYPASSES is_system_running by design: user must be able to
        // pre-load the reservoir while the system is idle.
        case MSG_LOAD_DOSE: {      // 2001
            float g  = doc["gro"]   | 0.0f;
            float m  = doc["micro"] | 0.0f;
            float b  = doc["bloom"] | 0.0f;
            float pu = doc["ph_up"] | 0.0f;
            float pd = doc["ph_dn"] | 0.0f;

            DEBUG_PORT.println("=== Web loading dose ===");
            if (g  > 0.0f) gro.dose(g);
            if (m  > 0.0f) micro.dose(m);
            if (b  > 0.0f) bloom.dose(b);
            if (pu > 0.0f) ph_up.dose(pu);
            if (pd > 0.0f) ph_down.dose(pd);

            if (g > 0.0f || m > 0.0f || b > 0.0f || pu > 0.0f || pd > 0.0f) {
                mix_resevoir(MIX_PIN_IN1, MIX_PIN_IN2, MIX_PIN_ENA);
                DEBUG_PORT.println("Web loading dose complete. Mixed.");
            }
            break;
        }

        // ── Web-triggered start/stop ───────────────────────────────────
        case MSG_SYSTEM_STATE: {   // 2002
            bool run = doc["run"] | false;

            // On transition idle → running, reset the dose timer so the
            // Kalman filters get a full interval to converge on fresh
            // sensor readings before the first automated dose fires.
            if (run && !is_system_running) {
                lastDoseMillis = millis();
            }

            is_system_running = run;
            DEBUG_PORT.print("System state -> ");
            DEBUG_PORT.println(run ? "RUNNING" : "STANDBY");
            break;
        }

        // ── Web-triggered plant profile update ─────────────────────────
        // Payload (all mS/cm for EC):
        //   { "M":2003, "ec_min":0.8, "ec_max":1.2, "ec_avg":1.0,
        //               "ph_min":6.0, "ph_max":7.0, "ph_avg":6.5 }
        // Does NOT gate on is_system_running — profile is configuration.
        case MSG_SET_PROFILE: {    // 2003
            plant.ec_low  = doc["ec_min"] | plant.ec_low;
            plant.ec_high = doc["ec_max"] | plant.ec_high;
            plant.ec_avg  = doc["ec_avg"] | plant.ec_avg;
            plant.ph_low  = doc["ph_min"] | plant.ph_low;
            plant.ph_high = doc["ph_max"] | plant.ph_high;
            plant.ph_avg  = doc["ph_avg"] | plant.ph_avg;

            DEBUG_PORT.print("Profile updated -> EC [");
            DEBUG_PORT.print(plant.ec_low, 2);  DEBUG_PORT.print(", ");
            DEBUG_PORT.print(plant.ec_high, 2); DEBUG_PORT.print("] avg ");
            DEBUG_PORT.print(plant.ec_avg, 2);  DEBUG_PORT.print(" mS/cm | pH [");
            DEBUG_PORT.print(plant.ph_low, 2);  DEBUG_PORT.print(", ");
            DEBUG_PORT.print(plant.ph_high, 2); DEBUG_PORT.print("] avg ");
            DEBUG_PORT.println(plant.ph_avg, 2);
            break;
        }

        default:
            DEBUG_PORT.print("Unknown M: ");
            DEBUG_PORT.println(msgType);
            break;
    }
}


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

    ph_up.init(); ph_down.init();
    gro.init();   micro.init();  bloom.init();

    analogReadResolution(12);

    ec_kalman.x = 0.7f;    ec_kalman.p = 0.3f;
    pH_kalman.x = 6.5f;    pH_kalman.p = 0.1f;
    temp_kalman.x = 22.0f; temp_kalman.p = 0.4f;

    plant.ec_high = 1.8f; plant.ec_low = 0.8f;
    plant.ec_avg  = (plant.ec_high + plant.ec_low) / 2.0f;
    plant.ph_high = 6.8f; plant.ph_low = 6.0f;
    plant.ph_avg  = (plant.ph_high + plant.ph_low) / 2.0f;
    plant.gro_amount = 1; plant.bloom_amount = 1; plant.micro_amount = 1;

    // System boots in STANDBY. User must press START from the web UI
    // to begin sensor reads and automated dosing.
    is_system_running = false;
    lastDoseMillis    = millis();

    DEBUG_PORT.println("Setup complete. System STANDBY — waiting for start command.");
}


void loop() {

    unsigned long now = millis();

    // ── 1. Always handle incoming messages from ESP32 (non-blocking) ───
    //
    //  poll() accumulates bytes one at a time and returns true only when
    //  a full '\n'-terminated line is ready — never blocks the loop.
    //
    //  This runs regardless of is_system_running so we can always
    //  receive the start command (M:2002) and loading-dose commands
    //  (M:2001) while the system is idle.
    //
    if (commReader.poll(COMM_PORT)) {
        handle_comm_message(commReader.buf);
    }

    // ── Gate: when STANDBY, skip sensor reads, automated dosing, and
    //         USB debug mirror. Serial polling above keeps running.
    if (!is_system_running) {
        return;
    }

    // ── 2. Read & filter sensors ───────────────────────────────────────
    std::array<float, 4> temp_raw = {
        temp1.read_val(), temp2.read_val(), temp3.read_val(), temp4.read_val()
    };
    latest_temp = temp_filter(temp_raw, temp_kalman, 0.1f);

    std::array<float, 4> ec_raw = {
        ec1.read_val(latest_temp), ec2.read_val(latest_temp), ec3.read_val(latest_temp), ec4.read_val(latest_temp)
    };
    std::array<float, 2> ph_raw = {
        ph1.read_val(latest_temp), ph2.read_val(latest_temp)
    };

    latest_ec = ec_filter(ec_raw, ec_kalman, 0.1f);
    latest_ph = ph_filter(ph_raw, pH_kalman, 0.1f);
    latest_ph = fakePhValue(plant.ph_avg);

    // ── 3. Scheduled dosing every 15 min ───────────────────────────────
    if (now - lastDoseMillis >= doseInterval) {

        reservoir_volume_ml -= UPTAKE_PER_INTERVAL;
        if (reservoir_volume_ml < 0.0f) reservoir_volume_ml = 0.0f;

        if (reservoir_volume_ml < RESERVOIR_MIN_ML)
            DEBUG_PORT.println("WARNING: Reservoir low!");

        float vol_L = reservoir_volume_ml / 1000.0f;

        float nutrient_dose = nutrient_calc(plant.ec_avg, plant.ec_low, plant.ec_high, vol_L, latest_ec);
        float ph_up_dose    = ph_up_calc(plant.ph_avg, plant.ph_low, plant.ph_high, vol_L, latest_ph);
        float ph_down_dose  = ph_down_calc(plant.ph_avg, plant.ph_low, plant.ph_high, vol_L, latest_ph);

        std::array<float, 3> dose = proportion_nutrient(
            nutrient_dose, plant.gro_amount, plant.micro_amount, plant.bloom_amount);

        bool nut_dosed = dose_nutrients(gro, dose[0], bloom, dose[1], micro, dose[2]);
        bool ph_dosed  = dose_ph(ph_up, ph_up_dose, ph_down, ph_down_dose);

        if (nut_dosed || ph_dosed) {
            mix_resevoir(MIX_PIN_IN1, MIX_PIN_IN2, MIX_PIN_ENA);
            DEBUG_PORT.println("Dosed and mixed.");
        } else {
            DEBUG_PORT.println("In range - nothing dosed.");
        }

        lastDoseMillis = now;
    }

    // ── 4. Mirror to USB debug serial (rate-limited to avoid stalling loop) ───
    if (now - lastDebugMillis >= debugInterval) {
        send_data(DEBUG_PORT, latest_ph, latest_ec, latest_temp);
        lastDebugMillis = now;
    }
}