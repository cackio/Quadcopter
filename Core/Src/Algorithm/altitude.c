/**
 * @file    altitude.c
 * @brief   气压高度解算模块
 *
 * 本文件实现笔记 C7 推导的高度融合算法：
 *   - MS5611 温度补偿（一阶 + 二阶）
 *   - 气压转高度
 *   - 惯性滤波器融合（气压计 + 加速度计）
 *
 * 依赖：
 *   - altitude.h (本模块头文件)
 *   - bsp.h (MS5611 原始数据、PROM 校准字 C[1..6])
 *   - imu.h (姿态四元数，用于加速度计坐标转换)
 *   - main.h (STM32 HAL 类型)
 *   - math.h (标准数学库)
 *
 * 参考笔记：
 *   - C7 综合姿态解算与 Madgwick（传感器误差建模）
 *
 * 参考代码：
 *   - examplezong/GY86.c (MS5611 温度补偿实现)
 *   - examplezong/IMU.c (Height_Update 惯性滤波器思路)
 *
 * ========================================================================== */

#include "altitude.h"
#include "bsp.h"
#include "imu.h"

/* ============================================================================
 *  全局变量定义
 * ========================================================================== */

Altitude_t altitude = {
    0.0f, 0.0f, 0.0f,  // height, velocity, acceleration
    0.0f, 0.0f,       // pressure, temperature
    0.0f,             // offset_press
    0.0f              // K_press_to_high
};

uint8_t alt_calib_done = 0;

/* 校准采样缓冲区 */
#define ALT_CALIB_SAMPLE_SIZE  50
static float calib_pressure_buf[ALT_CALIB_SAMPLE_SIZE];
static uint16_t calib_sample_cnt = 0;

/* 惯性滤波器状态 */
static float z_est[3] = {0.0f, 0.0f, 0.0f};  // [height, velocity, acceleration]
static float accel_bias[3] = {0.0f, 0.0f, 0.0f};

/* ============================================================================
 *  初始化函数
 * ========================================================================== */

void ALT_Init(void)
{
    /* 步骤 1：初始化 altitude 结构体 */
    altitude.height = 0.0f;
    altitude.velocity = 0.0f;
    altitude.acceleration = 0.0f;
    altitude.pressure = 0.0f;
    altitude.temperature = 0.0f;
    altitude.offset_press = 0.0f;
    altitude.K_press_to_high = 0.0f;

    /* 步骤 2：初始化校准标志 */
    alt_calib_done = 0;
    calib_sample_cnt = 0;

    /* 步骤 3：初始化惯性滤波器状态 */
    z_est[0] = 0.0f;  // height
    z_est[1] = 0.0f;  // velocity
    z_est[2] = 0.0f;  // acceleration
    accel_bias[0] = 0.0f;
    accel_bias[1] = 0.0f;
    accel_bias[2] = 0.0f;

    /* 步骤 4：计算初始 K_press_to_high（使用标准大气压） */
    altitude.K_press_to_high = ALT_Calc_K_Press_To_High(STANDARD_PRESSURE);
}

void ALT_Calib_Update(void)
{
    /* 高度校准（采集气压零偏）
     *
     * 在 HeightTask 中循环调用，直到高度校准标志 alt_calib_done == 1。
     *
     * 步骤：
     *   1. 读取 MS5611 原始数据（由 bsp.c 完成）
     *   2. 温度补偿
     *   3. 计算气压
     *   4. 累加气压值
     *   5. 采样满后计算零偏 offset_press
     *   6. 计算 K_press_to_high
     */

    /* 步骤 1：读取 MS5611 原始数据 */
    uint32_t D1 = D1_Pressure_RAW;
    uint32_t D2 = D2_Temperature_RAW;

    /* 步骤 2：温度补偿 */
    float temp = 0.0f, dT = 0.0f;
    ALT_Temperature_Compensate(D2, C, &temp, &dT);

    /* 步骤 3：气压计算 */
    float pressure = 0.0f;
    ALT_Pressure_Calc(D1, dT, C, temp, &pressure);

    /* 步骤 4：累加气压值 */
    if (calib_sample_cnt < ALT_CALIB_SAMPLE_SIZE) {
        calib_pressure_buf[calib_sample_cnt] = pressure;
        calib_sample_cnt++;
    }

    /* 步骤 5：采样满后计算零偏 */
    if (calib_sample_cnt >= ALT_CALIB_SAMPLE_SIZE) {
        float sum = 0.0f;
        for (uint16_t i = 0; i < ALT_CALIB_SAMPLE_SIZE; i++) {
            sum += calib_pressure_buf[i];
        }
        altitude.offset_press = sum / (float)ALT_CALIB_SAMPLE_SIZE;

        /* 步骤 6：计算 K_press_to_high */
        altitude.K_press_to_high = ALT_Calc_K_Press_To_High(altitude.offset_press);

        /* 校准完成 */
        alt_calib_done = 1;
    }
}

uint8_t ALT_Calib_Done(void)
{
    return alt_calib_done;
}

/* ============================================================================
 *  高度解算函数
 * ========================================================================== */

void ALT_Update(void)
{
    /* 高度解算主函数（在 HeightTask 中循环调用）
     *
     * 执行流程：
     *   步骤 1：读取 MS5611 原始数据（由 bsp.c 完成）
     *   步骤 2：温度补偿计算
     *   步骤 3：气压计算
     *   步骤 4：气压转高度
     *   步骤 5：加速度计高度融合
     *   步骤 6：更新 altitude 结构体
     *
     * 公式：
     *   - 温度：T = 2000 + dT*C6/0x800000
     *   - 气压：p = (D1*SENS/0x200000 - OFF)/0x8000
     *   - 高度：h = (p - p0) * K_press_to_high
     *   - K_press_to_high = -44300 * (p0/101325)^(-4.256/5.256) / (5.256*101325)
     */

    /* 步骤 1：读取 MS5611 原始数据 */
    uint32_t D1 = D1_Pressure_RAW;
    uint32_t D2 = D2_Temperature_RAW;

    /* 步骤 2：温度补偿 */
    float temp = 0.0f, dT = 0.0f;
    ALT_Temperature_Compensate(D2, C, &temp, &dT);

    /* 步骤 3：气压计算 */
    float pressure = 0.0f;
    ALT_Pressure_Calc(D1, dT, C, temp, &pressure);

    /* 更新气压和温度 */
    altitude.pressure = pressure;
    altitude.temperature = temp / 100.0f;  // 转 °C

    /* 步骤 4：气压转高度 */
    float baro_height = ALT_Pressure_To_Height(pressure,
                                               altitude.offset_press,
                                               altitude.K_press_to_high);

    /* 步骤 5：加速度计高度融合（惯性滤波器）
     *
     * 参考学长 IMU.c Height_Update 注释部分
     *
     * 公式：
     *   - 加速度从机体系转到地理系
     *   - 扣除重力加速度
     *   - 惯性滤波器融合
     */

    /* 获取采样周期（与 IMU 共用 imu_dt） */
    float dt = imu_dt * 2.0f;  // HeightTask 周期约为 IMU 的 2 倍

    /* 加速度计原始数据（已校准） */
    float ax = accel_body.x * ACCEL_LSB_PER_G;  // 转 g
    float ay = accel_body.y * ACCEL_LSB_PER_G;
    float az = accel_body.z * ACCEL_LSB_PER_G;

    /* 扣除零偏 */
    ax -= accel_bias[0];
    ay -= accel_bias[1];
    az -= accel_bias[2];

    /* 将加速度从机体系转到地理系 */
    /* 使用四元数旋转矩阵 */
    float q0 = quaternion.w, q1 = quaternion.x, q2 = quaternion.y, q3 = quaternion.z;
    float q0q0 = q0*q0, q0q1 = q0*q1, q0q2 = q0*q2, q0q3 = q0*q3;
    float q1q1 = q1*q1, q1q2 = q1*q2, q1q3 = q1*q3;
    float q2q2 = q2*q2, q2q3 = q2*q3, q3q3 = q3*q3;

    float corr_acc_x = 2.0f*ax*(q1q2 - q0q3) + 2.0f*ay*(q0q2 + q1q3) + 2.0f*az*(q0q1 - q2q3);
    float corr_acc_y = 2.0f*ax*(q0q2 + q1q3) + 2.0f*ay*(q0q0 - q1q1 + q2q2 - q3q3) + 2.0f*az*(q0q3 - q1q2);
    float corr_acc_z = 2.0f*ax*(q0q1 - q2q3) + 2.0f*ay*(q0q3 - q1q2) + 2.0f*az*(q0q0 + q1q1 - q2q2 - q3q3);

    /* 转 m/s² */
    corr_acc_x *= GRAVITY;
    corr_acc_y *= GRAVITY;
    corr_acc_z *= GRAVITY;

    /* 扣除重力加速度 */
    corr_acc_z -= GRAVITY;

    /* 惯性滤波器预测 */
    ALT_Inertial_Filter_Predict(dt, z_est);

    /* 气压计校正 */
    float corr_baro = baro_height - z_est[0];
    ALT_Inertial_Filter_Correct(corr_baro, dt, z_est, 0, ALT_W_BARO);

    /* 加速度计校正 */
    ALT_Inertial_Filter_Correct(corr_acc_z, dt, z_est, 2, ALT_W_ACCEL);

    /* 加速度零偏校正 */
    /* TODO: 实现加速度零偏在线估计 */
    /* accel_bias[2] -= corr_baro * ALT_W_BARO * ALT_W_BARO; */

    /* 步骤 6：更新 altitude 结构体 */
    altitude.height = z_est[0];
    altitude.velocity = z_est[1];
    altitude.acceleration = z_est[2];
}

/* ============================================================================
 *  MS5611 温度补偿
 * ========================================================================== */

void ALT_Temperature_Compensate(uint32_t D2, const uint16_t *C,
                               float *temp, float *dT)
{
    /* MS5611 温度补偿计算
     *
     * 公式：
     *   dT = D2 - (C5 << 8)
     *   T = 2000 + dT * C6 / 0x800000
     *
     * 步骤：
     *   1. 计算 dT
     *   2. 计算一阶温度
     *   3. 二阶补偿（T < 2000 时）
     */

    /* 步骤 1：计算 dT */
    float dT_val = (float)D2 - (float)((uint32_t)C[MS5611_C5_Tref] << 8);
    *dT = dT_val;

    /* 步骤 2：计算一阶温度 */
    float T = 2000.0f + dT_val * ((float)C[MS5611_C6_TDES]) / 8388608.0f;   /* 2^23 */

    /* 步骤 3：二阶补偿（T < 2000 时） */
    if (T < 2000.0f) {
        float T2 = (dT_val * dT_val) / 2147483648.0f;   /* 2^31 */
        float aux = (T - 2000.0f) * (T - 2000.0f);
        T = T - T2;
    }

    *temp = T;  // 输出 °C * 100
}

/* ============================================================================
 *  MS5611 气压计算
 * ========================================================================== */

void ALT_Pressure_Calc(uint32_t D1, float dT, const uint16_t *C,
                      float temp, float *pressure)
{
    /* MS5611 气压计算
     *
     * 公式：
     *   OFF = C2<<16 + C4*dT/0x80
     *   SENS = C1<<15 + C3*dT/0x100
     *   二阶补偿（T < 2000 时）
     *   pressure = (D1*SENS/0x200000 - OFF) / 0x8000
     *
     * 步骤：
     *   1. 计算一阶 OFF 和 SENS
     *   2. 二阶补偿（T < 2000 时）
     *   3. 计算气压
     */

    /* 步骤 1：计算一阶 OFF 和 SENS */
    float OFF = (float)((uint32_t)C[MS5611_C2_OFF] << 16) +
                ((float)C[MS5611_C4_TCOEF] * dT) / 128.0f;   /* 2^7 */
    float SENS = (float)((uint32_t)C[MS5611_C1_SENS] << 15) +
                 ((float)C[MS5611_C3_TSENS] * dT) / 256.0f;   /* 2^8 */

    /* 步骤 2：二阶补偿（T < 2000 时） */
    if (temp < 2000.0f) {
        float T2 = (dT * dT) / 2147483648.0f;   /* 2^31 */
        float aux = (temp - 2000.0f) * (temp - 2000.0f);
        float OFF2 = 2.5f * aux;
        float SENS2 = 1.25f * aux;

        if (temp < -1500.0f) {
            aux = (temp + 1500.0f) * (temp + 1500.0f);
            OFF2 = OFF2 + 7.0f * aux;
            SENS2 = SENS2 + 5.5f * aux;
        }

        OFF -= OFF2;
        SENS -= SENS2;
    }

    /* 步骤 3：计算气压 */
    *pressure = ((float)D1 * SENS / 2097152.0f - OFF) / 32768.0f;   /* 2^21, 2^15 */
}

/* ============================================================================
 *  气压转高度
 * ========================================================================== */

float ALT_Pressure_To_Height(float pressure, float offset, float K)
{
    /* 气压转高度
     *
     * 公式：
     *   height = (pressure - offset) * K
     *
     * 步骤：
     *   1. 计算气压差
     *   2. 乘以系数
     */

    /* 步骤 1：计算气压差 */
    float delta_p = pressure - offset;

    /* 步骤 2：乘以系数 */
    return delta_p * K;
}

float ALT_Calc_K_Press_To_High(float offset_press)
{
    /* 计算气压转高度系数
     *
     * 公式（参考学长 IMU.c）：
     *   K = -44300 * (offset/101325)^(-4.256/5.256) / (5.256*101325)
     *
     * 步骤：
     *   1. 计算 ratio = offset / 101325
     *   2. 计算 ratio^(-4.256/5.256)
     *   3. 乘以系数
     */

    /* 步骤 1：计算 ratio */
    float ratio = offset_press / STANDARD_PRESSURE;

    /* 步骤 2：计算 ratio^(-4.256/5.256) */
    float exponent = -4.256f / 5.256f;
    float ratio_pow = powf(ratio, exponent);

    /* 步骤 3：乘以系数 */
    float K = -44300.0f * ratio_pow / (5.256f * STANDARD_PRESSURE);

    return K;
}

/* ============================================================================
 *  惯性滤波器
 * ========================================================================== */

void ALT_Inertial_Filter_Predict(float dt, float z[3])
{
    /* 惯性滤波器预测
     *
     * 公式（参考学长 IMU.c 注释部分）：
     *   z[2] += z[1] * dt + z[2] * dt² / 2
     *   z[1] += z[2] * dt
     *
     * 步骤：
     *   1. 更新加速度积分速度
     *   2. 更新速度积分高度
     *
     * 注意：这里先更新速度，再更新高度
     */

    /* 步骤 1：更新速度 */
    z[1] += z[2] * dt;

    /* 步骤 2：更新高度 */
    z[0] += z[1] * dt + 0.5f * z[2] * dt * dt;
}

void ALT_Inertial_Filter_Correct(float error, float dt, float z[3],
                                int index, float weight)
{
    /* 惯性滤波器校正
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

    /* 步骤 1：计算 ewdt */
    float ewdt = error * weight * dt;

    /* 步骤 2：更新对应分量 */
    z[index] += ewdt;

    /* 步骤 3：更新关联分量 */
    if (index == 0) {
        /* 高度校正，更新速度和加速度 */
        z[1] += weight * ewdt;
        z[2] += weight * weight * ewdt / 3.0f;
    } else if (index == 1) {
        /* 速度校正，更新加速度 */
        z[2] += weight * ewdt;
    }
}
