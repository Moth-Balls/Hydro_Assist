#pragma once

#include <Arduino.h>

float fakePhValue(float mean = 6.5f, float sigma = 0.1f,
                  float minVal = 6.0f, float maxVal = 6.8f) {
    auto uniform = []() {
        return (random(1, 10000) / 10000.0f); // (0,1]
    };

    float u1 = uniform();
    float u2 = uniform();
    float z0 = sqrt(-2.0f * log(u1)) * cos(2.0f * PI * u2);
    float value = mean + z0 * sigma;
    return constrain(value, minVal, maxVal);
}





