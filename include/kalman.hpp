#pragma once

#include <array>

struct KalmanFilter
{
    float x;
    float p;
};


float ec_filter(std::array<float, 4> curr_vals, KalmanFilter& state, float process_noise) {

    static const float R = 1111.0f;

    float pred_state = state.x;
    float uncertainty_new = state.p + process_noise;

    static const float sensor_noise = R / 4.0f;

    float k_gain = uncertainty_new / (uncertainty_new + sensor_noise);

    float z_avg = std::accumulate(curr_vals.begin(), curr_vals.end(), 0.0f) / curr_vals.size();
    
    pred_state = pred_state + k_gain * (z_avg - pred_state);

    uncertainty_new = (1 - k_gain) * uncertainty_new;

    state.x = pred_state;
    state.p = uncertainty_new;

    return pred_state;
}


// pH Filter
float ph_filter(std::array<float, 2> curr_vals, KalmanFilter& state, float process_noise) {


    static const float R = 0.0011f;
    
    float pred_state = state.x;
    float uncertainty_new = state.p + process_noise;

    static const float sensor_noise = R / 4.0f;

    float k_gain = uncertainty_new / (uncertainty_new + sensor_noise);

    float z_avg = std::accumulate(curr_vals.begin(), curr_vals.end(), 0.0f) / curr_vals.size();
    
    pred_state = pred_state + k_gain * (z_avg - pred_state);

    uncertainty_new = (1 - k_gain) * uncertainty_new;

    state.x = pred_state;
    state.p = uncertainty_new;

    return pred_state;

}
