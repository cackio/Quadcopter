#ifndef GY86_H
#define GY86_H

#include "stm32f4xx_hal.h"  // 包含HAL库

// MPU6050 寄存器地址定义 (部分关键寄存器)
#define MPU6050_ADDR (0x68 << 1)      // MPU6050的I2C地址，左移一位。若AD0接高电平则为0x69[citation:2]
#define SMPLRT_DIV_REG 0x19
#define GYRO_CONFIG_REG 0x1B
#define ACCEL_CONFIG_REG 0x1C
#define ACCEL_XOUT_H_REG 0x3B
#define GYRO_XOUT_H_REG 0x43
#define PWR_MGMT_1_REG 0x6B
#define WHO_AM_I_REG 0x75             // 器件ID寄存器[citation:8]

// 函数声明
uint8_t MPU6050_Init(I2C_HandleTypeDef *hi2c);
uint8_t MPU6050_Read_Gyroscope(I2C_HandleTypeDef *hi2c, int16_t *gx, int16_t *gy, int16_t *gz);
uint8_t MPU6050_Read_Accelerometer(I2C_HandleTypeDef *hi2c, int16_t *ax, int16_t *ay, int16_t *az);

#endif