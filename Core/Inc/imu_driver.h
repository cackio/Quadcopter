#ifndef IMU_DRIVER_H
#define IMU_DRIVER_H
#include <stdint.h>
typedef struct
{
    int16_t accel[3], gyro[3], temperature;
} ImuRawSample;
enum
{
    SENSOR_READ_ERROR = -1,
    SENSOR_OVERFLOW = -2,
    SENSOR_NOT_READY = 0,
    SENSOR_NEW = 1
};
int IMU_Init(void);
int IMU_Read(ImuRawSample *sample);
int Magnetometer_Init(void);
int Magnetometer_Read(int16_t xyz[3]);
#endif
