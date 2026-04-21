#pragma once

#include <Wire.h>
#include <Arduino.h>

static constexpr uint8_t ATTINY1_HIGH_ADDR = 0x78;
static constexpr uint8_t ATTINY2_LOW_ADDR  = 0x77;
static constexpr int THRESHOLD = 50;
static constexpr float FULL_LITERS = 37.8541f; // top of sensor

// --- Header-only non-blocking water-level state machine (call from main loop) ---

// Use these in your main loop:
//   water_level_init(timeout_ms);      // optional, default 20ms
//   water_level_poll();                // call every loop, non-blocking
//   if (water_level_has_update()) {    // new sample ready
//       float liters = water_level_get_volume();
//       /* handle liters */
//   }

static uint8_t wl_low_buf[8];
static uint8_t wl_high_buf[12];

enum class wl_stage_t : uint8_t { IDLE, READ_LOW, READ_HIGH, DONE };

static wl_stage_t wl_stage = wl_stage_t::IDLE;
static unsigned long wl_timeout_ms = 20;
static unsigned long wl_last_request_ts = 0;
static int wl_received = 0;
static size_t wl_idx = 0;
static float wl_cached_volume = 0.0f;
static bool wl_updated = false;
static const size_t WL_LOW_LEN = 8;
static const size_t WL_HIGH_LEN = 12;

static inline void water_level_init(unsigned long timeout_ms = 20) {
    wl_timeout_ms = timeout_ms;
    wl_stage = wl_stage_t::IDLE;
    wl_updated = false;
}

// internal helper: start request and set received count/index/timestamp
static inline bool wl_start_request(uint8_t addr, size_t expected) {
    wl_received = Wire.requestFrom(addr, (uint8_t)expected);
    wl_last_request_ts = millis();
    wl_idx = 0;
    return (wl_received > 0);
}

static inline void water_level_poll() {
    switch (wl_stage) {
        case wl_stage_t::IDLE:
            // start low read
            wl_start_request(ATTINY2_LOW_ADDR, WL_LOW_LEN);
            wl_stage = wl_stage_t::READ_LOW;
            return;

        case wl_stage_t::READ_LOW: {
            // read as many bytes as available (non-blocking)
            while (Wire.available() && wl_idx < (size_t)wl_received && wl_idx < WL_LOW_LEN) {
                wl_low_buf[wl_idx++] = Wire.read();
            }
            if (wl_idx >= WL_LOW_LEN) {
                // full low read complete -> start high read
                wl_start_request(ATTINY1_HIGH_ADDR, WL_HIGH_LEN);
                wl_stage = wl_stage_t::READ_HIGH;
                return;
            }
            if (millis() - wl_last_request_ts >= wl_timeout_ms) {
                // timeout -> pad remainder and proceed to high
                for (; wl_idx < WL_LOW_LEN; ++wl_idx) wl_low_buf[wl_idx] = 0;
                wl_start_request(ATTINY1_HIGH_ADDR, WL_HIGH_LEN);
                wl_stage = wl_stage_t::READ_HIGH;
            }
            return;
        }

        case wl_stage_t::READ_HIGH: {
            while (Wire.available() && wl_idx < (size_t)wl_received && wl_idx < WL_HIGH_LEN) {
                wl_high_buf[wl_idx++] = Wire.read();
            }
            if (wl_idx >= WL_HIGH_LEN) {
                wl_stage = wl_stage_t::DONE;
            } else if (millis() - wl_last_request_ts >= wl_timeout_ms) {
                for (; wl_idx < WL_HIGH_LEN; ++wl_idx) wl_high_buf[wl_idx] = 0;
                wl_stage = wl_stage_t::DONE;
            }
            return;
        }

        case wl_stage_t::DONE: {
            // compute percent and cached volume
            uint8_t count = 0;
            for (size_t i = 0; i < WL_LOW_LEN; ++i) if (wl_low_buf[i] > THRESHOLD) ++count;
            for (size_t i = 0; i < WL_HIGH_LEN; ++i) if (wl_high_buf[i] > THRESHOLD) ++count;
            float percent = count * (100.0f / float(WL_LOW_LEN + WL_HIGH_LEN));
            if (percent > 100.0f) percent = 100.0f;
            wl_cached_volume = (percent / 100.0f) * FULL_LITERS;
            wl_updated = true;
            // prepare for next cycle
            wl_stage = wl_stage_t::IDLE;
            return;
        }

        default:
            wl_stage = wl_stage_t::IDLE;
            return;
    }
}

static inline bool water_level_has_update() {
    return wl_updated;
}

static inline float get_volume() {
    wl_updated = false;
    return wl_cached_volume;
}

static inline void water_level_log(Stream &port) {
    port.print("Water: ");
    port.print(wl_cached_volume, 2);
    port.println(" L");
}






