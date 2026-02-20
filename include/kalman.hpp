#pragma once

#include "arm_math.h"
#include <array>
#include <vector>

struct KalmanFilter
{
    float x;
    float p;
};

float tds_filter(std::array<float, 4> curr_vals, KalmanFilter& state, float process_noise) {
    // Predict
    float x_pred = state.x;
    float P_pred = state.p + process_noise;

    static float h_val[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    // H (4x1)
    arm_matrix_instance_f32 H;
    arm_mat_init_f32(&H, 4, 1, h_val);

    // H^T (1x4)
    arm_matrix_instance_f32 Ht;
    arm_mat_init_f32(&Ht, 1, 4, h_val);

    // R (4x4)
    const static float noise = 1111.0f;
    static float R_data[16] = {noise, 0.0f, 0.0f, 0.0f,
                               0.0f, noise, 0.0f, 0.0f,
                               0.0f, 0.0f, noise, 0.0f,
                               0.0f, 0.0f, 0.0f, noise};
    arm_matrix_instance_f32 R;
    arm_mat_init_f32(&R, 4, 4, R_data);

    // tmp buffers
    float H_scaled_data[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // H * P_pred
    arm_matrix_instance_f32 H_scaled;
    arm_mat_init_f32(&H_scaled, 4, 1, H_scaled_data);

    float S_data[16] = {0.0f}; // innovation covariance
    arm_matrix_instance_f32 S;
    arm_mat_init_f32(&S, 4, 4, S_data);

    // Compute H_scaled = H * P_pred
    for (uint8_t i = 0; i < 4; ++i) {
        H_scaled_data[i] = h_val[i] * P_pred;
    }

    // S = H_scaled * H^T + R
    arm_status st = arm_mat_mult_f32(&H_scaled, &Ht, &S);
    if (st != ARM_MATH_SUCCESS) {
        return x_pred;
    }
    arm_mat_add_f32(&S, &R, &S);

    // Invert S -> S_inv (4x4)
    float S_inv_data[16] = {0.0f};
    arm_matrix_instance_f32 S_inv;
    arm_mat_init_f32(&S_inv, 4, 4, S_inv_data);
    st = arm_mat_inverse_f32(&S, &S_inv);
    if (st != ARM_MATH_SUCCESS) {
        return x_pred;
    }

    // Compute PH^T (1x4) = P_pred * H^T
    float PHt_data[4];
    for (uint8_t j = 0; j < 4; ++j) {
        PHt_data[j] = P_pred * h_val[j];
    }
    arm_matrix_instance_f32 PHt;
    arm_mat_init_f32(&PHt, 1, 4, PHt_data);

    // Compute K = PH^T * S_inv
    float K_data[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    arm_matrix_instance_f32 K;
    arm_mat_init_f32(&K, 1, 4, K_data);
    st = arm_mat_mult_f32(&PHt, &S_inv, &K);
    if (st != ARM_MATH_SUCCESS) {
        return x_pred;
    }

    // Measurement vector z
    float z_data[4] = {curr_vals[0], curr_vals[1], curr_vals[2], curr_vals[3]};
    arm_matrix_instance_f32 z;
    arm_mat_init_f32(&z, 4, 1, z_data);

    // Compute residual y = z - H * x_pred
    float Hx_data[4];
    for (uint8_t i = 0; i < 4; ++i) Hx_data[i] = h_val[i] * x_pred;
    arm_matrix_instance_f32 Hx;
    arm_mat_init_f32(&Hx, 4, 1, Hx_data);

    float y_data[4];
    arm_matrix_instance_f32 y;
    arm_mat_init_f32(&y, 4, 1, y_data);
    arm_mat_sub_f32(&z, &Hx, &y);

    // Compute K * y
    float Ky_data[1] = {0.0f};
    arm_matrix_instance_f32 Ky;
    arm_mat_init_f32(&Ky, 1, 1, Ky_data);
    st = arm_mat_mult_f32(&K, &y, &Ky);
    if (st != ARM_MATH_SUCCESS) return x_pred;

    float x_post = x_pred + Ky_data[0];

    // Update P: P_post = (1 - K*H) * P_pred  where K*H is scalar 
    float KH_data[1] = {0.0f};
    arm_matrix_instance_f32 KH;
    arm_mat_init_f32(&KH, 1, 1, KH_data);
    st = arm_mat_mult_f32(&K, &H, &KH);
    float KH_scalar = (st == ARM_MATH_SUCCESS) ? KH_data[0] : 0.0f;
    float P_post = (1.0f - KH_scalar) * P_pred;

    state.x = x_post;
    state.p = P_post;

    return x_post;
}


// pH Filter
float ph_filter(std::array<float, 2> curr_vals, KalmanFilter& state, float process_noise) {
    // Predict
    float x_pred = state.x;
    float P_pred = state.p + process_noise;

    static float h_val[2] = {1.0f, 1.0f};

    // H
    arm_matrix_instance_f32 H;
    arm_mat_init_f32(&H, 2, 1, h_val);

    // H^T
    arm_matrix_instance_f32 Ht;
    arm_mat_init_f32(&Ht, 1, 2, h_val);

    // R
    const static float noise = 0.0011f;
    static float R_data[4] = {noise, 0.0f,
                               0.0f, noise};
    arm_matrix_instance_f32 R;
    arm_mat_init_f32(&R, 2, 2, R_data);

    // tmp buffers
    float H_scaled_data[2] = {0.0f, 0.0f}; // H * P_pred
    arm_matrix_instance_f32 H_scaled;
    arm_mat_init_f32(&H_scaled, 2, 1, H_scaled_data);

    float S_data[4] = {0.0f}; // innovation covariance
    arm_matrix_instance_f32 S;
    arm_mat_init_f32(&S, 2, 2, S_data);

    // Compute H_scaled = H * P_pred
    for (uint8_t i = 0; i < 2; ++i) {
        H_scaled_data[i] = h_val[i] * P_pred;
    }

    // S = H_scaled * H^T + R
    arm_status st = arm_mat_mult_f32(&H_scaled, &Ht, &S);
    if (st != ARM_MATH_SUCCESS) {
        return x_pred;
    }
    arm_mat_add_f32(&S, &R, &S);

    // Invert S -> S_inv (2x2)
    float S_inv_data[4] = {0.0f};
    arm_matrix_instance_f32 S_inv;
    arm_mat_init_f32(&S_inv, 2, 2, S_inv_data);
    st = arm_mat_inverse_f32(&S, &S_inv);
    if (st != ARM_MATH_SUCCESS) {
        return x_pred;
    }

    // Compute PH^T (1x2) = P_pred * H^T
    float PHt_data[2];
    for (uint8_t j = 0; j < 2; ++j) {
        PHt_data[j] = P_pred * h_val[j];
    }
    arm_matrix_instance_f32 PHt;
    arm_mat_init_f32(&PHt, 1, 2, PHt_data);

    // Compute K = PH^T * S_inv
    float K_data[2] = {0.0f, 0.0f};
    arm_matrix_instance_f32 K;
    arm_mat_init_f32(&K, 1, 2, K_data);
    st = arm_mat_mult_f32(&PHt, &S_inv, &K);
    if (st != ARM_MATH_SUCCESS) {
        return x_pred;
    }

    // Measurement vector z
    float z_data[2] = {curr_vals[0], curr_vals[1]};
    arm_matrix_instance_f32 z;
    arm_mat_init_f32(&z, 2, 1, z_data);

    // Compute residual y = z - H * x_pred
    float Hx_data[2];
    for (uint8_t i = 0; i < 2; ++i) Hx_data[i] = h_val[i] * x_pred;
    arm_matrix_instance_f32 Hx;
    arm_mat_init_f32(&Hx, 2, 1, Hx_data);

    float y_data[2];
    arm_matrix_instance_f32 y;
    arm_mat_init_f32(&y, 2, 1, y_data);
    arm_mat_sub_f32(&z, &Hx, &y);

    // Compute K * y
    float Ky_data[1] = {0.0f};
    arm_matrix_instance_f32 Ky;
    arm_mat_init_f32(&Ky, 1, 1, Ky_data);
    st = arm_mat_mult_f32(&K, &y, &Ky);
    if (st != ARM_MATH_SUCCESS) return x_pred;

    float x_post = x_pred + Ky_data[0];

    // Update P: P_post = (1 - K*H) * P_pred  where K*H is scalar 
    float KH_data[1] = {0.0f};
    arm_matrix_instance_f32 KH;
    arm_mat_init_f32(&KH, 1, 1, KH_data);
    st = arm_mat_mult_f32(&K, &H, &KH);
    float KH_scalar = (st == ARM_MATH_SUCCESS) ? KH_data[0] : 0.0f;
    float P_post = (1.0f - KH_scalar) * P_pred;

    state.x = x_post;
    state.p = P_post;

    return x_post;
}
