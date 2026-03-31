#include "sensors/temp_sensor.hpp"

temp_sensor::temp_sensor(uint8_t pin) : read_pin(pin) {}

float temp_sensor::read_val() {
    // Placeholder implementation until DS18B20 support is added.
    // Return a safe default so project compiles.
    return 0.0f;
}
