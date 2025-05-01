#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <math.h>

#define NAME_LEN 30
#define NUM_BEACONS 8


// iBeacon positions in meters
float ibeacon_coords[8][2] = {
    {0.0, 0.0},  // iBeacon 1 (A)
    {0.0, 1.5},  // iBeacon 2 (B)
    {0.0, 3.0},  // iBeacon 3 (C)
    {2.0, 3.0},  // iBeacon 4 (D)
    {4.0, 3.0},  // iBeacon 5 (E)
    {4.0, 1.5},  // iBeacon 6 (F)
    {4.0, 0.0},  // iBeacon 7 (G)
    {2.0, 0.0},  // iBeacon 8 (H)
};


// RSSI to distance conversion (simple log-distance model)
float rssi_to_distance(float rssi, float tx_power, float path_loss_exp) {
    return powf(10.0f, (tx_power - rssi) / (10.0f * path_loss_exp));
}

// Solve 2D position using least squares multilateration
void estimate_position(float *distances, float *x_est, float *y_est) {
    float A[NUM_BEACONS - 1][2];
    float b[NUM_BEACONS - 1];

    float x1 = ibeacon_coords[0][0];
    float y1 = ibeacon_coords[0][1];
    float r1 = distances[0];

    // Construct A and b
    for (int i = 1; i < NUM_BEACONS; ++i) {
        float xi = ibeacon_coords[i][0];
        float yi = ibeacon_coords[i][1];
        float ri = distances[i];

        A[i - 1][0] = 2 * (xi - x1);
        A[i - 1][1] = 2 * (yi - y1);

        b[i - 1] = pow(ri, 2) - pow(r1, 2)
                 - pow(xi, 2) + pow(x1, 2)
                 - pow(yi, 2) + pow(y1, 2);
    }

    // Calculate (A^T * A)
    float AtA[2][2] = {{0}};
    for (int i = 0; i < NUM_BEACONS - 1; ++i) {
        AtA[0][0] += A[i][0] * A[i][0];
        AtA[0][1] += A[i][0] * A[i][1];
        AtA[1][0] += A[i][1] * A[i][0];
        AtA[1][1] += A[i][1] * A[i][1];
    }

    // Calculate (A^T * b)
    float Atb[2] = {0};
    for (int i = 0; i < NUM_BEACONS - 1; ++i) {
        Atb[0] += A[i][0] * b[i];
        Atb[1] += A[i][1] * b[i];
    }

    // Invert 2x2 matrix AtA
    float det = AtA[0][0]*AtA[1][1] - AtA[0][1]*AtA[1][0];
    if (fabs(det) < 1e-6) {
        *x_est = 0;
        *y_est = 0;
        return;
    }

    float invAtA[2][2] = {
        { AtA[1][1] / det, -AtA[0][1] / det },
        { -AtA[1][0] / det, AtA[0][0] / det }
    };

    // Multiply inverse with Atb: (AtA)^-1 * Atb
    *x_est = invAtA[0][0] * Atb[0] + invAtA[0][1] * Atb[1];
    *y_est = invAtA[1][0] * Atb[0] + invAtA[1][1] * Atb[1];
}

