#ifndef __ALTITUDE_H
#define __ALTITUDE_H

/* ============================================================================
 *  altitude.h — 气压高度解算模块头文件
 *
 *  依赖：
 *    - main.h (STM32 HAL 类型定义)
 *    - bsp.h (MS5611 原始数据、PROM 校准字 C[1..6])
 *    - imu.h (姿态四元数，用于加速度计高度融合)
 *    - math.h (标准数学库)
 *
 *  功能模块：
 *    1. MS5611 温度补偿（一阶 + 二阶温度补偿）
 *    2. 气压转高度（国际标准大气模型）
 *    3. 高度融合（气压计 + 加速度计惯性滤波器）
 *    4. 垂直速度/加速度估计
 *
 *  参考笔记：
 *    - C7 综合姿态解算与 Madgwick（传感器误差建模）
 *    - 学长 examplezong/GY86.c (MS5611 温度补偿实现)
 *    - 学长 examplezong/IMU.c (Height_Update 惯性滤波器思路)
 *
 * ========================================================================== */

#include "main.h"
#include "math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  常量与类型定义
 * ========================================================================== */

/* MS5611 PROM 寄存器索引（C[1]~C[6]） */
#define MS5611_C1_SENS    1
#define MS5611_C2_OFF     2
#define MS5611_C3_TSENS   3
#define MS5611_C4_TCOEF   4
#define MS5611_C5_Tref    5
#define MS5611_C6_TDES    6

/* 标准大气压 (Pa) */
#define STANDARD_PRESSURE  101325.0f

/* 重力加速度 (m/s²) */
#define GRAVITY            9.80665f

/* 加速度计量程 (LSB/g) */
#define ACCEL_LSB_PER_G    8192.0f

/* 高度滤波参数 */
#define ALT_W_BARO         0.5f     /* 气压计权重 */
#define ALT_W_ACCEL        20.0f    /* 加速度计权重 */
#define ALT_W_ACCEL_BIAS   0.05f    /* 加速度零偏收敛权重 */

/* 高度结构体 */
typedef struct
{
    float height;        /* 高度 (m) */
    float velocity;      /* 垂直速度 (m/s) */
    float acceleration;  /* 垂直加速度 (m/s²) */
    float pressure;      /* 气压 (Pa) */
    float temperature;   /* 温度 (°C) */
    float offset_press;  /* 气压零偏 (Pa) */
    float K_press_to_high; /* 气压转高度系数 */
} Altitude_t;

/* ============================================================================
 *  全局变量（定义在 altitude.c，extern 引用）
 * ========================================================================== */

extern Altitude_t altitude;        /* 高度解算结果 */
extern uint8_t    alt_calib_done;  /* 高度校准完成标志 */

/* ============================================================================
 *  初始化函数
 * ========================================================================== */

/**
 * @brief 高度模块初始化
 *
 * 在 StartTask 中 BSP_Init 之后调用。
 *
 * 步骤：
 *   1. 初始化 altitude 结构体
 *   2. 初始化校准标志
 *   3. 计算初始 K_press_to_high
 */
void ALT_Init(void);

/**
 * @brief 高度校准（采集气压零偏）
 *
 * 在 HeightTask 中循环调用，直到 alt_calib_done == 1。
 *
 * 步骤：
 *   1. 读取 MS5611 原始数据
 *   2. 温度补偿
 *   3. 计算气压
 *   4. 累加气压值
 *   5. 采样满后计算零偏 offset_press
 *   6. 计算 K_press_to_high
 */
void ALT_Calib_Update(void);

/**
 * @brief 检查高度校准是否完成
 * @return 1 = 完成, 0 = 未完成
 */
uint8_t ALT_Calib_Done(void);

/* ============================================================================
 *  高度解算函数
 * ========================================================================== */

/**
 * @brief 高度解算主函数（在 HeightTask 中循环调用）
 *
 * 执行流程：
 *   步骤 1：读取 MS5611 原始数据（由 bsp.c 完成）
 *   步骤 2：温度补偿计算
 *       - dT = D2 - C5<<8
 *       - Temperature = 2000 + dT * C6 / 0x800000
 *       - 二阶补偿（T < 20°C 时）
 *   步骤 3：气压计算
 *       - OFF = C2<<16 + C4*dT/0x80
 *       - SENS = C1<<15 + C3*dT/0x100
 *       - pressure = (D1*SENS/0x200000 - OFF) / 0x8000
 *   步骤 4：气压转高度
 *       - baro_height = (pressure - offset_press) * K_press_to_high
 *   步骤 5：加速度计高度融合
 *       - 将加速度从机体系转到地理系
 *       - 扣除重力加速度
 *       - 惯性滤波器融合
 *   步骤 6：更新 altitude 结构体
 *
 * 公式：
 *   - 温度：T = 2000 + dT*C6/0x800000
 *   - 气压：p = (D1*SENS/0x200000 - OFF)/0x8000
 *   - 高度：h = (p - p0) * K_press_to_high
 *   - K_press_to_high = -44300 * (p0/101325)^(-4.256/5.256) / (5.256*101325)
 *
 * 参考：
 *   - 学长 GY86.c MS561101BA_GetTemperature / MS561101BA_GetPressure
 *   - 学长 IMU.c Height_Update（惯性滤波器思路，注释部分）
 */
void ALT_Update(void);

/**
 * @brief MS5611 温度补偿计算
 * @param D2 原始温度值
 * @param C PROM 校准字数组
 * @param temp 输出温度 (°C * 100)
 * @param dT 输出温度差
 *
 * 公式：
 *   dT = D2 - (C5 << 8)
 *   T = 2000 + dT * C6 / 0x800000
 *   二阶补偿（T < 2000 时）：
 *     T2 = (dT*dT) / 0x80000000
 *     Aux = (T - 2000)²
 *     OFF2 = 2.5 * Aux
 *     SENS2 = 1.25 * Aux
 */
void ALT_Temperature_Compensate(uint32_t D2, const uint16_t *C,
                               float *temp, float *dT);

/**
 * @brief MS5611 气压计算
 * @param D1 原始压力值
 * @param dT 温度差
 * @param C PROM 校准字数组
 * @param temp 温度 (°C * 100)
 * @param pressure 输出气压 (Pa)
 *
 * 公式：
 *   OFF = C2<<16 + C4*dT/0x80
 *   SENS = C1<<15 + C3*dT/0x100
 *   二阶补偿（T < 2000 时）
 *   pressure = (D1*SENS/0x200000 - OFF) / 0x8000
 */
void ALT_Pressure_Calc(uint32_t D1, float dT, const uint16_t *C,
                      float temp, float *pressure);

/**
 * @brief 气压转高度
 * @param pressure 气压 (Pa)
 * @param offset 气压零偏 (Pa)
 * @param K 气压转高度系数
 * @return 高度 (m)
 *
 * 公式：
 *   height = (pressure - offset) * K
 *   K = -44300 * (offset/101325)^(-4.256/5.256) / (5.256*101325)
 */
float ALT_Pressure_To_Height(float pressure, float offset, float K);

/**
 * @brief 计算气压转高度系数
 * @param offset_press 气压零偏 (Pa)
 * @return K_press_to_high
 *
 * 公式（参考学长 IMU.c）：
 *   K = -44300 * (offset/101325)^(-4.256/5.256) / (5.256*101325)
 */
float ALT_Calc_K_Press_To_High(float offset_press);

/* ============================================================================
 *  惯性滤波器（加速度计高度融合）
 * ========================================================================== */

/**
 * @brief 惯性滤波器预测
 * @param dt 采样周期 (s)
 * @param z 状态向量 [height, velocity, acceleration]
 *
 * 公式（参考学长 IMU.c 注释部分）：
 *   z[2] += z[1] * dt + z[2] * dt² / 2
 *   z[1] += z[2] * dt
 *   （先更新加速度，再速度，再高度）
 *
 * 步骤：
 *   1. 更新加速度积分速度
 *   2. 更新速度积分高度
 */
void ALT_Inertial_Filter_Predict(float dt, float z[3]);

/**
 * @brief 惯性滤波器校正
 * @param error 误差
 * @param dt 采样周期 (s)
 * @param z 状态向量 [height, velocity, acceleration]
 * @param index 校正分量索引 (0=height, 2=acceleration)
 * @param weight 权重
 *
 * 公式（参考学长 IMU.c 注释部分）：
 *   ewdt = error * weight * dt
 *   z[index] += ewdt
 *   if (index == 0) {
 *       z[1] += weight * ewdt
 *       z[2] += weight² * ewdt / 3
 *   } else if (index == 1) {
 *       z[2] += weight * ewdt
 *   }
 *
 * 步骤：
 *   1. 计算 ewdt = error * weight * dt
 *   2. 更新对应分量
 *   3. 更新关联分量
 */
void ALT_Inertial_Filter_Correct(float error, float dt, float z[3],
                                int index, float weight);

#ifdef __cplusplus
}
#endif

#endif /* __ALTITUDE_H */
