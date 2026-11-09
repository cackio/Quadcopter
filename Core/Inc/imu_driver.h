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
enum
{
    DRIVER_NONE,
    DRIVER_READ,
    DRIVER_WRITE,
    DRIVER_ID,
    DRIVER_READBACK,
    DRIVER_BUS_BUSY
};
typedef struct
{
    uint8_t address, id[3], stage, reg, expected, actual, status, initialized;
    uint32_t hal_error, read_errors, overflow, not_ready, new_samples;
} SensorDiagnostic;
const SensorDiagnostic *Sensor_Diagnostic(unsigned sensor);
int IMU_Init(void);
int IMU_Read(ImuRawSample *sample);
int Magnetometer_Init(void);
int Magnetometer_Read(int16_t xyz[3]);
#endif
