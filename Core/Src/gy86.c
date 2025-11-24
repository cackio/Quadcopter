#include "gy86.h"

// 延时函数，确保MPU6050上电稳定和复位完成[citation:2]
static void delay(uint32_t ms) {
    HAL_Delay(ms);
}

/**
  * @brief  初始化MPU6050
  * @param  hi2c: I2C句柄指针
  * @retval HAL状态值，HAL_OK为成功
  */
uint8_t MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t check;
    uint8_t Data;

    // 检查设备是否就绪，尝试多次[citation:8]
    if (HAL_I2C_IsDeviceReady(hi2c, MPU6050_ADDR, 10, 100) != HAL_OK) {
        return HAL_ERROR;
    }

    // 读取WHO_AM_I寄存器验证设备[citation:2]
    if (HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, WHO_AM_I_REG, 1, &check, 1, 1000) != HAL_OK) {
        return HAL_ERROR;
    }
    if (check != 0x68) { // 期望的器件ID是0x68[citation:2]
        return HAL_ERROR;
    }

    // 唤醒MPU6050，配置电源管理寄存器1[citation:2][citation:8]
    Data = 0x00;
    if (HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, PWR_MGMT_1_REG, 1, &Data, 1, 1000) != HAL_OK) {
        return HAL_ERROR;
    }

    // 设置陀螺仪满量程范围 (±2000°/s)[citation:2]
    Data = 0x18;
    if (HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, GYRO_CONFIG_REG, 1, &Data, 1, 1000) != HAL_OK) {
        return HAL_ERROR;
    }

    // 设置加速度计满量程范围 (±2g)[citation:2]
    Data = 0x00;
    if (HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, ACCEL_CONFIG_REG, 1, &Data, 1, 1000) != HAL_OK) {
        return HAL_ERROR;
    }

    // 设置采样率分频器[citation:8]
    Data = 0x07;
    if (HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, SMPLRT_DIV_REG, 1, &Data, 1, 1000) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
  * @brief  读取陀螺仪原始数据
  * @param  hi2c: I2C句柄指针
  * @param  gx, gy, gz: 指向存储陀螺仪XYZ轴数据的指针
  * @retval HAL状态值
  */
uint8_t MPU6050_Read_Gyroscope(I2C_HandleTypeDef *hi2c, int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t buffer[6];
    
    // 从陀螺仪数据寄存器连续读取6个字节[citation:2]
    if (HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, GYRO_XOUT_H_REG, 1, buffer, 6, 1000) != HAL_OK) {
        return HAL_ERROR;
    }
    
    // 组合高8位和低8位数据 (数据为Big-endian)
    *gx = (int16_t)((buffer[0] << 8) | buffer[1]);
    *gy = (int16_t)((buffer[2] << 8) | buffer[3]);
    *gz = (int16_t)((buffer[4] << 8) | buffer[5]);
    
    return HAL_OK;
}

/**
  * @brief  读取加速度计原始数据
  * @param  hi2c: I2C句柄指针
  * @param  ax, ay, az: 指向存储加速度计XYZ轴数据的指针
  * @retval HAL状态值
  */
uint8_t MPU6050_Read_Accelerometer(I2C_HandleTypeDef *hi2c, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buffer[6];
    
    // 从加速度计数据寄存器连续读取6个字节[citation:2]
    if (HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, ACCEL_XOUT_H_REG, 1, buffer, 6, 1000) != HAL_OK) {
        return HAL_ERROR;
    }
    
    // 组合高8位和低8位数据 (数据为Big-endian)
    *ax = (int16_t)((buffer[0] << 8) | buffer[1]);
    *ay = (int16_t)((buffer[2] << 8) | buffer[3]);
    *az = (int16_t)((buffer[4] << 8) | buffer[5]);
    
    return HAL_OK;
}