#ifndef ATTITUDE_H
#define ATTITUDE_H
#include <stdint.h>
typedef struct
{
    float q[4];
    float beta;
} AttitudeFilter;
typedef struct
{
    uint32_t count;
    float mean[6], m2[6], bias_dps[3];
    uint8_t ready;
} GyroCalibration;
int Attitude_Normalize(float q[4]);
int Attitude_MatrixValid(const float r[3][3]);
void Attitude_Rotate(const float r[3][3], const float in[3], float out[3]);
void Attitude_FromEuler(float roll_deg, float pitch_deg, float yaw_deg, float q[4]);
void Attitude_ToEuler(const float q[4], float degrees[3]);
int Attitude_Align(AttitudeFilter *f, const float accel[3], const float *mag);
/* Explicit seconds, rad/s, normalized-or-physical accel and mag.
 * Returns 0 on invalid input, 1 gyro only, 2 gravity feedback, 3 MARG feedback.
 * Invalid updates leave the last good quaternion intact. */
int Attitude_Update(AttitudeFilter *f, const float gyro[3], const float accel[3], const float *mag,
                    float dt, int feedback);
void Attitude_Display(const float q[4], const float basis[4], float out[4]);
void GyroCalibration_Reset(GyroCalibration *c);
int GyroCalibration_Push(GyroCalibration *c, const float accel[3], const float dps[3],
                         uint32_t required);
#endif
