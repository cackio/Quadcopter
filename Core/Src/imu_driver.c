#include "imu_driver.h"
#include "flight_config.h"
#include "i2c.h"
#include <string.h>

static uint16_t imu_address = 0x68U << 1;
static int16_t s16(const uint8_t *p)
{
    return (int16_t)((uint16_t)p[0] << 8 | p[1]);
}
/* HAL otherwise waits 25 ms for BUSY, independent of the caller timeout.
 * With one bus owner, a BUSY bus at entry indicates a fault. */
static int read_reg(uint16_t address, uint8_t reg, uint8_t *data, uint16_t count)
{
    if (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_BUSY))
        return 0;
    return HAL_I2C_Mem_Read(&hi2c1, address, reg, I2C_MEMADD_SIZE_8BIT, data, count,
                            FLIGHT_I2C_TIMEOUT_MS) == HAL_OK;
}
static int write_reg(uint16_t address, uint8_t reg, uint8_t value)
{
    if (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_BUSY))
        return 0;
    return HAL_I2C_Mem_Write(&hi2c1, address, reg, I2C_MEMADD_SIZE_8BIT, &value, 1,
                             FLIGHT_I2C_TIMEOUT_MS) == HAL_OK;
}
int IMU_Init(void)
{
    static const uint8_t registers[] = {0x6B, 0x19, 0x1A, 0x1B, 0x1C, 0x6A, 0x37, 0x38};
    static const uint8_t values[] = {0x01, 0x04, 0x03, 0x08, 0x00, 0x00, 0x02, 0x01};
    uint8_t id, value;
    unsigned i;
    imu_address = 0x68U << 1;
    if (!read_reg(imu_address, 0x75, &id, 1) || id != 0x68)
    {
        imu_address = 0x69U << 1;
        if (!read_reg(imu_address, 0x75, &id, 1) || id != 0x68)
            return 0;
    }
    if (!write_reg(imu_address, 0x6B, 0x80))
        return 0;
    HAL_Delay(100); /* Startup only, before the sampling timer starts. */
    for (i = 0; i < sizeof(registers); ++i)
    {
        if (!write_reg(imu_address, registers[i], values[i]))
            return 0;
        if (i == 0)
            HAL_Delay(30);
        if (!read_reg(imu_address, registers[i], &value, 1) || value != values[i])
            return 0;
    }
    return 1;
}
int IMU_Read(ImuRawSample *sample)
{
    uint8_t bytes[15];
    ImuRawSample next;
    unsigned i;
    if (!read_reg(imu_address, 0x3A, bytes, sizeof(bytes)))
        return SENSOR_READ_ERROR;
    if (!(bytes[0] & 1U))
        return SENSOR_NOT_READY;
    for (i = 0; i < 3; ++i)
    {
        next.accel[i] = s16(bytes + 1 + 2 * i);
        next.gyro[i] = s16(bytes + 9 + 2 * i);
    }
    next.temperature = s16(bytes + 7);
    *sample = next;
    return SENSOR_NEW;
}
int Magnetometer_Init(void)
{
    uint8_t id[3], value;
    if (!read_reg(0x1EU << 1, 0x0A, id, 3) || memcmp(id, "H43", 3) != 0)
        return 0;
    if (!write_reg(0x1EU << 1, 0x00, 0x74) || /* 8 averages, 30 Hz */
        !write_reg(0x1EU << 1, 0x01, 0x20) || /* +/-1.3 gauss */
        !write_reg(0x1EU << 1, 0x02, 0x00))
        return 0;
    if (!read_reg(0x1EU << 1, 0x00, &value, 1) || value != 0x74)
        return 0;
    if (!read_reg(0x1EU << 1, 0x01, &value, 1) || value != 0x20)
        return 0;
    if (!read_reg(0x1EU << 1, 0x02, &value, 1) || (value & 3U) != 0)
        return 0;
    return 1;
}
int Magnetometer_Read(int16_t xyz[3])
{
    uint8_t status, b[6];
    int16_t next[3];
    unsigned i;
    if (!read_reg(0x1EU << 1, 0x09, &status, 1))
        return SENSOR_READ_ERROR;
    if (!(status & 3U))
        return SENSOR_NOT_READY;
    if (!read_reg(0x1EU << 1, 0x03, b, 6))
        return SENSOR_READ_ERROR;
    next[0] = s16(b);
    next[2] = s16(b + 2);
    next[1] = s16(b + 4);
    for (i = 0; i < 3; ++i)
        if (next[i] == -4096)
            return SENSOR_OVERFLOW;
    if (status & 2U)
        return SENSOR_NOT_READY;
    memcpy(xyz, next, sizeof(next));
    return SENSOR_NEW;
}
