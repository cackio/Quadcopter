#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H
#include <stdint.h>
int Sensor_ConfigValid(void);
void Sensor_Accel(const int16_t raw[3], float native[3], float body[3]);
void Sensor_Gyro(const int16_t raw[3], const float bias_dps[3], float body_rad_s[3]);
void Sensor_Mag(const int16_t raw[3], float body[3]);
/* Compiled coefficients, independent of any PC candidate file. Pages 3/4/5. */
unsigned Sensor_Coefficients(unsigned page, float out[18]);
#endif
