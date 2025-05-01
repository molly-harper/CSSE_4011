#include <stdlib.h>


#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

LOG_MODULE_REGISTER(test_kal_module, LOG_LEVEL_DBG);


// Kalman filter variables
float kalmanGain = 0.0;   // estimated value
float estError = 1.0;   // estimation error
float measErr = 0.1; // process noise

float estimate = 0.0; // inital estimate
float measurement; 

  // measurement noise


float weight_value(float ultrasonicMeas, float rssiMeas) 
{ 

    float ultraWeight = 0.95; // 95% ultrasonic
    float rssiWeight = 0.05; // 5 % rssi 

    float weightedUltMeas = ultraWeight * ultrasonicMeas; 
    float weightedRssiMeas = rssiWeight * rssiMeas; 

    float weightAvg = weightedUltMeas + weightedRssiMeas; 

    return weightAvg; 

}

float kalman_update(float measurement) {
    // Prediction update

    kalmanGain = estError/(estError + measErr); 

    estimate = estimate + (kalmanGain * (measurement - estimate)); 

    estError = (1.0 - kalmanGain) * (estError); 

    return estimate; 
}

float get_kalman_position(float ultrasonicMeas, float rssiMeas) { 

    float weighted_val = weight_value(ultrasonicMeas, rssiMeas); 

    float est = kalman_update(weighted_val); 

    return est; 
}