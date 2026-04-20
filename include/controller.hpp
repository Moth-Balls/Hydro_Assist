#pragma once

#include <Arduino.h>

// Adds nutrients if EC is below the minimum threshold
float nutrient_calc(const float &TARGET_EC, const float &MIN_EC, const float &MAX_EC, const float &volume, const float &curr_ec) {

  if (curr_ec >= MIN_EC) {
    return 0.0;
  }

  static const float EC_INPUT_MAX = 200.0; // in mL //TODO NEED TO CHANGE
  static const float EC_INPUT_MIN = 0.0; // in mL
  static const float EC_GAIN = 1.0; //TODO NEED TO CHANGE 

  // Input before clamping
  float pre_input = ((TARGET_EC - curr_ec) * volume) / EC_GAIN;

  // pre_input is contrainted to be between EC_INPUT_MIN and EC_INPUT_MAX
  float input = constrain(pre_input, EC_INPUT_MIN, EC_INPUT_MAX);

  return input;
}


// Adds pH UP if pH drops below the minimum threshold
float ph_up_calc(const float &TARGET_PH, const float &MIN_PH, const float &MAX_PH, const float &volume, const float &curr_ph) {
  // If pH is within range or higher than we want, we don't add pH UP
  if (curr_ph >= MIN_PH) {
    return 0.0;
  }
    
  static const float PH_UP_INPUT_MAX = 1.0; // in mL //TODO NEED TO CHANGE
  static const float PH_UP_INPUT_MIN = 0.0; // in mL
  static const float PH_GAIN = 1.0; //TODO NEED TO CHANGE 

  // Input before clamping
  float pre_input = ((TARGET_PH - curr_ph) * volume) / PH_GAIN;

  // pre_input is contrainted to be between PH_UP_INPUT_MIN and PH_UP_INPUT_MAX
  float input = constrain(pre_input, PH_UP_INPUT_MIN, PH_UP_INPUT_MAX);

  return input;
}


// Adds pH DOWN if pH rises above the maximum threshold
float ph_down_calc(const float &TARGET_PH, const float &MIN_PH, const float &MAX_PH, const float &volume, const float &curr_ph) {
  // If pH is within range or lower than we want, we don't add pH DOWN
  if (curr_ph <= MAX_PH) {
    return 0.0;
  }

  static const float PH_DOWN_INPUT_MAX = 50.0; // in mL
  static const float PH_DOWN_INPUT_MIN = 0.0; // in mL
  static const float PH_GAIN = 1.0; //TODO NEED TO CHANGE

  // Input before clamping (Note: to avoid negative values since curr_ph > TARGET_PH, 
  // you might want to flip the subtraction depending on how your gain expects it)
  float pre_input = ((curr_ph - TARGET_PH) * volume) / PH_GAIN;

  // pre_input is contrainted to be between PH_DOWN_INPUT_MIN and PH_DOWN_INPUT_MAX
  float input = constrain(pre_input, PH_DOWN_INPUT_MIN, PH_DOWN_INPUT_MAX);

  return input;
}

// Expects a EC value that needs to be proportionally divided based on plant profile
std::array<float, 3> proportion_nutrient(const float &nutrient_calc_val, const uint8_t &nutrient_1_amount, const uint8_t &nutrient_2_amount, const uint8_t &nutrient_3_amount) {
  
  
  float nutrient_1_dose = nutrient_calc_val / nutrient_1_amount;
  float nutrient_2_dose = nutrient_calc_val / nutrient_2_amount;
  float nutrient_3_dose = nutrient_calc_val / nutrient_3_amount;
  
  std::array<float, 3> dose_amounts = {nutrient_1_dose, nutrient_2_dose, nutrient_3_dose};

  return dose_amounts; //in mL
}
