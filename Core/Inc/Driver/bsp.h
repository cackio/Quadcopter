#ifndef BSP_H
#define BSP_H

#include "main.h"        // HAL 库类型定义

#ifdef __cplusplus
extern "C" {
#endif

/* ======================== 全局变量（extern 声明） ======================== */

// MPU6050 原始数据
extern int16_t Accel_X_RAW, Accel_Y_RAW, Accel_Z_RAW;
extern int16_t Gyro_X_RAW,  Gyro_Y_RAW,  Gyro_Z_RAW;

// HMC5883L 原始磁场数据
extern int16_t Mag_X_RAW, Mag_Y_RAW, Mag_Z_RAW;

// MS5611 原始数据（24位用 uint32_t 存储）
extern uint32_t D1_Pressure_RAW;
extern uint32_t D2_Temperature_RAW;

// MS5611 校准参数 (PROM)
extern uint16_t C[7];     // C[1]~C[6] 有效

// PPM 遥控信号 8 通道值
// 通道0:横滚 通道1:俯仰 通道2:油门 通道3:偏航
// 通道4-7: 辅助通道（拨杆、旋钮等）
extern volatile uint16_t PPM_Values[8];


/* ======================== BSP 初始化 ======================== */

/**
 * @brief 板级初始化（包括 DWT + 传感器 + PWM + PPM）
 *        必须在本函数中调用 OS_TRACE_INIT() 之前执行
 */
void BSP_Init(void);


/* ======================== 传感器驱动 ======================== */

void MPU6050_Init(void);
void HMC5883L_Init(void);
void MS5611_PROM_Read(void);
void All_Sensors_Init(void);

void MPU6050_Read_Raw(void);
void HMC5883L_Read_Raw(void);
void MS5611_Read_Raw(void);
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);


/* ======================== 电机控制 ======================== */

/**
 * @brief  设置电机速度（PWM 占空比）
 * @param  htim:    定时器句柄
 * @param  Channel: 定时器通道
 * @param  speed:   速度值（0 ~ ARR，ARR 是自动重载值）
 */
void Motor_SetSpeed(TIM_HandleTypeDef *htim, uint32_t Channel, uint16_t speed);

/**
 * @brief  设置电机转动方向（预留接口，尚未实现）
 * @param  direction: 方向（0=正转, 1=反转，具体编码待定）
 */
void Motor_SetDirection(uint8_t direction);

/**
 * @brief  停止电机（设置 PWM 占空比为 0）
 */
void Motor_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);


#ifdef __cplusplus
}
#endif

#endif /* BSP_H */