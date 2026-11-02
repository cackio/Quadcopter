#ifndef FLIGHT_APP_H
#define FLIGHT_APP_H
#include "telemetry_schema.h"
#include <stdint.h>
typedef struct
{
    uint32_t sequence, timestamp_us, dt_us, exec_us;
    uint32_t dt_min_us, dt_max_us, exec_max_us;
    uint32_t dt_total, dt_in_band;
    uint32_t release_missed, i2c_errors, dt_errors, filter_errors, overruns, tx_dropped;
    uint32_t mag_sequence, mag_timestamp_us, mag_age_us;
    uint16_t flags, cal_samples;
    int16_t imu_raw[6], mag_raw[3];
    float accel_g[3], gyro_rad_s[3], mag_body[3], gyro_bias_dps[3], q[4], euler_deg[3];
    uint32_t attitude_stack_free, telemetry_stack_free;
} FlightSnapshot;
void Flight_Start(void);
void Flight_GetSnapshot(FlightSnapshot *out);
void Flight_TimerISR(void);
#endif
