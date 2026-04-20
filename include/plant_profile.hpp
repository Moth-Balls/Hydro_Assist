#pragma once

#include <String>

struct TargetPlant
{
    std::string name;

    // EC Bounds
    float ec_high;
    float ec_low;

    float ec_avg;

    // pH Bounds
    float ph_high;
    float ph_low;

    float ph_avg;

    uint8_t gro_amount;
    uint8_t bloom_amount;
    uint8_t micro_amount;

};