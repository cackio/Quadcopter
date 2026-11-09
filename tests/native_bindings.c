#include "ano_protocol.h"
#include "attitude.h"
#include "i2c.h"
#include "timebase.h"
#include <string.h>

I2C_HandleTypeDef hi2c1;
int mock_busy;
unsigned mock_busy_cycles, mock_stop_cycles;
unsigned mock_busy_status(void)
{
    if (mock_busy) return 1;
    if (mock_busy_cycles) { --mock_busy_cycles; return 1; }
    return 0;
}
static uint8_t imu_regs[128], mag_regs[16];
static int failed_register = -1, corrupt_register = -1;
static uint16_t mock_imu_address = 0xD0;
unsigned mock_reads, mock_timeout;
void mock_reset(void)
{
    memset(imu_regs, 0, sizeof(imu_regs));
    memset(mag_regs, 0, sizeof(mag_regs));
    imu_regs[0x75] = 0x68;
    memcpy(mag_regs + 0x0A, "H43", 3);
    mock_busy = 0;
    mock_busy_cycles = mock_stop_cycles = 0;
    failed_register = corrupt_register = -1;
    mock_reads = mock_timeout = 0;
    mock_imu_address = 0xD0;
}
void mock_register(int mag, unsigned reg, unsigned value)
{
    (mag ? mag_regs : imu_regs)[reg] = (uint8_t)value;
}
void mock_fail_register(int reg)
{
    failed_register = reg;
}
void mock_corrupt_register(int reg)
{
    corrupt_register = reg;
}
void mock_imu_addr(unsigned value)
{
    mock_imu_address = (uint16_t)value;
}
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h, uint16_t address, uint16_t reg,
                                   uint16_t size, uint8_t *data, uint16_t count, uint32_t timeout)
{
    uint8_t *regs;
    (void)h;
    (void)size;
    ++mock_reads;
    mock_timeout = timeout;
    if ((int)reg == failed_register)
        return HAL_ERROR;
    if (address == mock_imu_address && reg + count <= 128)
        regs = imu_regs;
    else if (address == 0x3C && reg + count <= 16)
        regs = mag_regs;
    else
        return HAL_ERROR;
    memcpy(data, regs + reg, count);
    if ((int)reg == corrupt_register)
        data[0] ^= 1;
    if (address == mock_imu_address && reg == 0x3A)
        imu_regs[0x3A] = 0;
    if (address == 0x3C && reg == 0x03 && count == 6)
        mag_regs[0x09] = 0;
    mock_busy_cycles=mock_stop_cycles;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h, uint16_t address, uint16_t reg,
                                    uint16_t size, uint8_t *data, uint16_t count, uint32_t timeout)
{
    (void)h;
    (void)size;
    mock_timeout = timeout;
    if ((int)reg == failed_register)
        return HAL_ERROR;
    if (address == mock_imu_address && reg + count <= 128)
        memcpy(imu_regs + reg, data, count);
    else if (address == 0x3C && reg + count <= 16)
        memcpy(mag_regs + reg, data, count);
    else
        return HAL_ERROR;
    mock_busy_cycles=mock_stop_cycles;
    return HAL_OK;
}
void HAL_Delay(uint32_t milliseconds)
{
    (void)milliseconds;
}
uint32_t test_elapsed(uint32_t now, uint32_t before)
{
    return Time_ElapsedUs(now, before);
}
void test_expire(uint32_t now, uint32_t before, uint32_t limit, uint8_t *valid)
{
    Time_ExpireUs(now, before, limit, valid);
}
uint16_t test_diagnostic(uint8_t *out, size_t cap)
{
    FlightSnapshot s;
    unsigned i;
    memset(&s, 0, sizeof(s));
    s.sequence = 0x12345678;
    s.timestamp_us = 0xFFFFFFF0;
    s.flags = 0x008F;
    s.dt_us = 5000;
    s.exec_us = 1200;
    s.mag_sequence = 42;
    s.mag_age_us = 33000;
    s.release_missed = 7;
    s.i2c_errors = 2;
    s.cal_samples = 1000;
    s.tx_dropped = 3;
    for (i = 0; i < 6; ++i)
        s.imu_raw[i] = (int16_t)((int)i * 1000 - 2500);
    s.mag_raw[0] = -123;
    s.mag_raw[1] = 456;
    s.mag_raw[2] = -789;
    s.q[0] = 1;
    return ANO_PackDiagnostic(&s, out, cap);
}

uint32_t HAL_I2C_GetError(I2C_HandleTypeDef *h) { (void)h; return 0; }
