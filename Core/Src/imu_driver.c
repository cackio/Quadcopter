#include "imu_driver.h"
#include "flight_config.h"
#include "i2c.h"
#include <string.h>

static SensorDiagnostic diagnostic[2];
const SensorDiagnostic *Sensor_Diagnostic(unsigned sensor)
{
    return &diagnostic[sensor ? 1 : 0];
}
static SensorDiagnostic *diag(uint16_t address)
{
    return &diagnostic[address == (0x1EU << 1)];
}
static void fault(uint16_t address, uint8_t stage, uint8_t reg, uint8_t expected, uint8_t actual,
                  uint8_t status)
{
    SensorDiagnostic *d = diag(address);
    d->address = (uint8_t)(address >> 1);
    d->stage = stage;
    d->reg = reg;
    d->expected = expected;
    d->actual = actual;
    d->status = status;
    d->hal_error = HAL_I2C_GetError(&hi2c1);
}
static void success(SensorDiagnostic *d)
{
    d->initialized = 1;
    d->stage = 0;
    d->status = 0;
    d->hal_error = 0;
}
static uint16_t imu_address = 0x68U << 1;
static int16_t s16(const uint8_t *p)
{
    return (int16_t)((uint16_t)p[0] << 8 | p[1]);
}
/* HAL can return before the physical STOP has released BUSY. Consecutive
 * transactions (WHO_AM_I -> reset, configuration write -> readback, MPU -> mag)
 * must allow that short tail. Bound it here instead of HAL's 25 ms BUSY wait. */
static int bus_ready(uint16_t address, uint8_t reg, uint8_t expected)
{
    uint32_t start;
    if (!__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_BUSY))
        return 1;
    start = HAL_GetTick();
    while (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_BUSY))
    {
        if ((uint32_t)(HAL_GetTick() - start) >= FLIGHT_I2C_BUS_IDLE_TIMEOUT_MS)
        {
            fault(address, DRIVER_BUS_BUSY, reg, expected, 0, HAL_BUSY);
            return 0;
        }
    }
    return 1;
}
static int read_reg(uint16_t address, uint8_t reg, uint8_t *data, uint16_t count)
{
    HAL_StatusTypeDef result;
    if (!bus_ready(address, reg, 0))
        return 0;
    result = HAL_I2C_Mem_Read(&hi2c1, address, reg, I2C_MEMADD_SIZE_8BIT, data, count,
                              FLIGHT_I2C_TIMEOUT_MS);
    if (result != HAL_OK)
        fault(address, DRIVER_READ, reg, 0, 0, (uint8_t)result);
    return result == HAL_OK;
}
static int write_reg(uint16_t address, uint8_t reg, uint8_t value)
{
    HAL_StatusTypeDef result;
    if (!bus_ready(address, reg, value))
        return 0;
    result = HAL_I2C_Mem_Write(&hi2c1, address, reg, I2C_MEMADD_SIZE_8BIT, &value, 1,
                               FLIGHT_I2C_TIMEOUT_MS);
    if (result != HAL_OK)
        fault(address, DRIVER_WRITE, reg, value, 0, (uint8_t)result);
    return result == HAL_OK;
}
static int check_reg(uint16_t address, uint8_t reg, uint8_t expected, uint8_t mask)
{
    uint8_t actual = 0;
    if (!read_reg(address, reg, &actual, 1))
        return 0;
    if ((actual & mask) != expected)
    {
        fault(address, DRIVER_READBACK, reg, expected, actual, HAL_OK);
        return 0;
    }
    return 1;
}
int IMU_Init(void)
{
    static const uint8_t registers[] = {0x6B, 0x19, 0x1A, 0x1B, 0x1C, 0x6A, 0x37, 0x38};
    static const uint8_t values[] = {0x01, 0x04, 0x03, 0x08, 0x00, 0x00, 0x02, 0x01};
    uint8_t id = 0;
    unsigned i;
    memset(&diagnostic[0], 0, sizeof(diagnostic[0]));
    for (i = 0; i < 2; ++i)
    {
        imu_address = (uint16_t)((0x68U + i) << 1);
        diagnostic[0].address = (uint8_t)(imu_address >> 1);
        if (read_reg(imu_address, 0x75, &id, 1))
        {
            diagnostic[0].id[0] = id;
            if (id == 0x68)
                break;
            fault(imu_address, DRIVER_ID, 0x75, 0x68, id, HAL_OK);
        }
    }
    if (i == 2)
        return 0;
    if (!write_reg(imu_address, 0x6B, 0x80))
        return 0;
    HAL_Delay(100); /* Startup only, before the sampling timer starts. */
    for (i = 0; i < sizeof(registers); ++i)
    {
        if (!write_reg(imu_address, registers[i], values[i]))
            return 0;
        if (i == 0)
            HAL_Delay(30);
        if (!check_reg(imu_address, registers[i], values[i], 255))
            return 0;
    }
    success(&diagnostic[0]);
    return 1;
}
int IMU_Read(ImuRawSample *sample)
{
    uint8_t bytes[15];
    ImuRawSample next;
    unsigned i;
    if (!read_reg(imu_address, 0x3A, bytes, sizeof(bytes)))
    {
        ++diagnostic[0].read_errors;
        return SENSOR_READ_ERROR;
    }
    if (!(bytes[0] & 1U))
    {
        ++diagnostic[0].not_ready;
        return SENSOR_NOT_READY;
    }
    for (i = 0; i < 3; ++i)
    {
        next.accel[i] = s16(bytes + 1 + 2 * i);
        next.gyro[i] = s16(bytes + 9 + 2 * i);
    }
    next.temperature = s16(bytes + 7);
    *sample = next;
    ++diagnostic[0].new_samples;
    return SENSOR_NEW;
}
int Magnetometer_Init(void)
{
    uint8_t id[3];
    unsigned i;
    memset(&diagnostic[1], 0, sizeof(diagnostic[1]));
    diagnostic[1].address = 0x1E;
    if (!read_reg(0x1EU << 1, 0x0A, id, 3))
        return 0;
    memcpy(diagnostic[1].id, id, 3);
    for (i = 0; i < 3; ++i)
        if (id[i] != (uint8_t)"H43"[i])
        {
            fault(0x1EU << 1, DRIVER_ID, (uint8_t)(0x0A + i), (uint8_t)"H43"[i], id[i], HAL_OK);
            return 0;
        }
    if (!write_reg(0x1EU << 1, 0x00, 0x74) || /* 8 averages, 30 Hz */
        !write_reg(0x1EU << 1, 0x01, 0x20) || /* +/-1.3 gauss */
        !write_reg(0x1EU << 1, 0x02, 0x00))
        return 0;
    if (!check_reg(0x1EU << 1, 0, 0x74, 255) || !check_reg(0x1EU << 1, 1, 0x20, 255) ||
        !check_reg(0x1EU << 1, 2, 0, 3))
        return 0;
    success(&diagnostic[1]);
    return 1;
}
int Magnetometer_Read(int16_t xyz[3])
{
    uint8_t status, b[6];
    int16_t next[3];
    unsigned i;
    if (!read_reg(0x1EU << 1, 0x09, &status, 1))
    {
        ++diagnostic[1].read_errors;
        return SENSOR_READ_ERROR;
    }
    if (!(status & 3U))
    {
        ++diagnostic[1].not_ready;
        return SENSOR_NOT_READY;
    }
    if (!read_reg(0x1EU << 1, 0x03, b, 6))
    {
        ++diagnostic[1].read_errors;
        return SENSOR_READ_ERROR;
    }
    next[0] = s16(b);
    next[2] = s16(b + 2);
    next[1] = s16(b + 4);
    for (i = 0; i < 3; ++i)
        if (next[i] == -4096)
        {
            ++diagnostic[1].overflow;
            return SENSOR_OVERFLOW;
        }
    if (status & 2U)
    {
        ++diagnostic[1].not_ready;
        return SENSOR_NOT_READY;
    }
    memcpy(xyz, next, sizeof(next));
    ++diagnostic[1].new_samples;
    return SENSOR_NEW;
}
