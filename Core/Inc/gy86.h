#ifndef GY86_H
#define GY86_H
#include <stdio.h>
#include <string.h>

// MPU6050 地址和寄存器
#define MPU6050_ADDR         0xD0  // (0x68 << 1)
#define PWR_MGMT_1_REG       0x6B
#define WHO_AM_I_REG         0x75
#define INT_PIN_CFG_REG      0x37  // 旁路模式配置寄存器
#define ACCEL_XOUT_H_REG     0x3B
#define GYRO_XOUT_H_REG      0x43

// HMC5883L 地址和寄存器
#define HMC5883L_ADDR        0x3C  // (0x1E << 1)
#define HMC_CONF_A_REG       0x00
#define HMC_MODE_REG         0x02
#define HMC_DATA_X_H_REG     0x03

// MS5611 地址和命令
#define MS5611_ADDR          0xEE  // 7位地址 0x77
#define MS_CMD_RESET         0x1E
#define MS_CMD_READ_ADC      0x00
#define MS_CMD_PROM_READ_BASE 0xA0
#define MS_CMD_D1_4096       0x48  // 压力转换 (OSR=4096)
#define MS_CMD_D2_4096       0x58  // 温度转换 (OSR=4096)

void MPU6050_Init(void);
void HMC5883L_Init(void);
void MS5611_PROM_Read(void);
void All_Sensors_Init(void);
	
void MPU6050_Read_Raw(void);
void HMC5883L_Read_Raw(void);
void MS5611_Read_Raw(void);

#endif