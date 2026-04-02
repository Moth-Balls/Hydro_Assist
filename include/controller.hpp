#pragma once

#include <Arduino.h>

float nutrient_calc(const float &TARGET_EC ,const float &volume, const float &curr_ec) {

  static const float EC_INPUT_MAX = 1.0; //TODO NEED TO CHANGE
  static const float EC_INPUT_MIN = 0.0;

  static const float EC_GAIN = 0.0; //TODO NEED TO CHANGE

  // Input before clamping
  float pre_input = ((TARGET_EC - curr_ec) * volume) / EC_GAIN;

  // pre_input is contrainted to be between EC_INPUT_MIN and EC_INPUT_MAX
  float input = constrain(pre_input, EC_INPUT_MIN, EC_INPUT_MAX);

  return input;
}


float ph_up_calc(const float &TARGET_PH, const float &volume, const float &curr_ph) {
    
  static const float PH_UP_INPUT_MAX = 1.0; //TODO NEED TO CHANGE
  static const float PH_UP_INPUT_MIN = 0.0;

  static const float PH_GAIN = 0.0; //TODO NEED TO CHANGE

  // Input before clamping
  float pre_input = ((TARGET_PH - curr_ph) * volume) / PH_GAIN;

  // pre_input is contrainted to be between PH_UP_INPUT_MIN and PH_UP_INPUT_MAX
  float input = constrain(pre_input, PH_UP_INPUT_MIN, PH_UP_INPUT_MAX);

  return input;
}


float ph_down_calc(const float &TARGET_PH, const float &volume, const float &curr_ph) {

  static const float PH_DOWN_INPUT_MAX = 1.0;
  static const float PH_DOWN_INPUT_MIN = 0.0;

  static const float PH_GAIN = 0.0; //TODO NEED TO CHANGE

  // Input before clamping
  float pre_input = ((TARGET_PH - curr_ph) * volume) / PH_GAIN;

  // pre_input is contrainted to be between PH_DOWN_INPUT_MIN and PH_DOWN_INPUT_MAX
  float input = constrain(pre_input, PH_DOWN_INPUT_MIN, PH_DOWN_INPUT_MAX);

  return input;
}
