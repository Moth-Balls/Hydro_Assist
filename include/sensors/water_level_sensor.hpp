#pragma once

#include <Wire.h>
#include <Arduino.h>


static constexpr uint8_t ATTINY1_HIGH_ADDR = 0x78;
static constexpr uint8_t ATTINY2_LOW_ADDR  = 0x77;
static constexpr int THRESHOLD = 100;
static constexpr float FULL_LITERS = 37.8541f; // top of sensor

// Read buffers
extern uint8_t low_data[8];
extern uint8_t high_data[12];

// Safe read with timeout (ms)
inline bool requestBytes(uint8_t addr, uint8_t *buf, size_t len, unsigned long timeout = 100) {
	Wire.requestFrom(addr, (uint8_t)len);
	unsigned long start = millis();
	while (Wire.available() < (int)len) {
		if (millis() - start >= timeout) return false;
	}
	for (size_t i = 0; i < len; ++i) buf[i] = Wire.read();
	return true;
}

// Populate the external buffers; returns true if both reads succeeded
inline bool read_sections(uint8_t *low_out, uint8_t *high_out) {
	bool ok1 = requestBytes(ATTINY2_LOW_ADDR, low_out, 8);
	bool ok2 = requestBytes(ATTINY1_HIGH_ADDR, high_out, 12);
	return ok1 && ok2;
}

// Count how many sections exceed THRESHOLD
inline uint8_t count_sections(const uint8_t *low_in, const uint8_t *high_in) {
	uint8_t count = 0;
	for (int i = 0; i < 8; ++i) if (low_in[i] > THRESHOLD) ++count;
	for (int i = 0; i < 12; ++i) if (high_in[i] > THRESHOLD) ++count;
	return count;
}

// Returns percentage (0..100)
inline float read_level_percent() {
	uint8_t l[8];
	uint8_t h[12];
	if (!read_sections(l, h)) return 0.0f; // on error assume empty
	uint8_t count = count_sections(l, h);
	float percent = count * 5.0f; // 20 sections -> 5% each
	if (percent > 100.0f) percent = 100.0f;
	return percent;
}

// Returns volume in liters based on FULL_LITERS at 100%
inline float read_volume_liters() {
	float pct = read_level_percent();
	return (pct / 100.0f) * FULL_LITERS;
}

// Concise logging helper
inline void log_short(Stream &SERIAL_PORT) {
	float liters = read_volume_liters();
	SERIAL_PORT.print("Water: ");
	SERIAL_PORT.print(liters, 2);
	SERIAL_PORT.println(" L");
}


float get_volume() { }






