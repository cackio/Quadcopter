#ifndef __IMU_H
#define __IMU_H

/* ============================================================================
 *  imu.h — 传感器校准与姿态解算模块头文件
 *
 *  依赖：
 *    - main.h (STM32 HAL 类型定义)
 *    - math.h (标准数学库，用于 sqrt/atan2/asin/cos 等)
 *    - bsp.h (传感器原始数据全局变量)
 *
 *  功能模块：
 *    1. 传感器校准（加速度计椭球拟合、陀螺仪零偏、地磁计椭球拟合）
 *    2. 姿态解算（四元数初始化、Madgwick 梯度下降、欧拉角提取）
 *    3. 坐标系转换（机体系 ↔ 地球系）
 *
 *  参考笔记：
 *    - C4 加速度计数据采集与校准
 *    - C5 陀螺仪和地磁数据采集及处理
 *    - C6 梯度下降与数据处理
 *    - C7 综合姿态解算与 Madgwick
 *
 *  参考代码：
 *    - examplezong/IMU.h / IMU.c (学长 Mahony 实现，可作简化版对比)
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

/* 角度单位转换 */
#define RAD_TO_DEG       57.2957795f   /* 弧度转角度 */
#define DEG_TO_RAD       0.017453292f  /* 角度转弧度 */

/* MPU6050 量程配置（需在 bsp.c 的 MPU6050_Init 中写入） */
/* GYRO_CONFIG = 0x18 → ±2000°/s，分辨率 16.4 LSB/°/s */
/* ACCEL_CONFIG = 0x08 → ±4g，分辨率 8192 LSB/g */
#define GYRO_LSB_PER_DPS 16.4f
#define ACCEL_LSB_PER_G  8192.0f

/* 姿态解算参数 */
#define IMU_KP_DEFAULT    2.0f      /* Mahony 互补滤波比例增益 */
#define IMU_KI_DEFAULT    0.005f    /* Mahony 互补滤波积分增益 */
#define IMU_BETA_DEFAULT  0.1f      /* Madgwick 梯度下降权重 β */
#define IMU_DT_DEFAULT    0.001f    /* 采样周期默认值（由定时器测量覆盖） */

/* 四元数结构体（w, x, y, z） */
typedef struct
{
    float w;   /* 实部（标量部分） */
    float x;   /* 虚部 i */
    float y;   /* 虚部 j */
    float z;   /* 虚部 k */
} Quaternion_t;

/* 欧拉角结构体 */
typedef struct
{
    float roll;     /* 横滚角 φ (rad) */
    float pitch;    /* 俯仰角 θ (rad) */
    float yaw;      /* 偏航角 ψ (rad) */
} EulerAngle_t;

/* 传感器原始数据结构体 */
typedef struct
{
    float x;
    float y;
    float z;
} Vec3f_t;

/* 加速度计校准参数 */
typedef struct
{
    float offset_x;     /* 零偏 (g，采样时已由 LSB 归一化到 g) */
    float offset_y;
    float offset_z;
    float scale_x;      /* 刻度因子 (无量纲，理想=1) */
    float scale_y;
    float scale_z;
} AccelCalib_t;

/* 陀螺仪校准参数 */
typedef struct
{
    float offset_x;     /* 零偏 (rad/s) */
    float offset_y;
    float offset_z;
} GyroCalib_t;

/* 地磁计校准参数 */
typedef struct
{
    float offset_x;     /* 硬铁偏置 */
    float offset_y;
    float offset_z;
    float soft_iron[3][3];  /* 软铁校正矩阵 A (3×3) */
} MagCalib_t;

/* 校准状态枚举 */
typedef enum
{
    CALIB_PENDING = 0,   /* 待校准 */
    CALIB_IN_PROGRESS,   /* 校准进行中 */
    CALIB_DONE           /* 校准完成 */
} CalibState_t;

/* ============================================================================
 *  全局变量（定义在 imu.c，extern 引用）
 * ========================================================================== */

extern Quaternion_t  quaternion;           /* 当前姿态四元数 */
extern EulerAngle_t  euler_angle;          /* 当前姿态欧拉角 */
extern Vec3f_t       accel_body;           /* 校准后的加速度 (g) */
extern Vec3f_t       gyro_body;            /* 校准后的角速度 (rad/s) */
extern Vec3f_t       mag_body;             /* 校准后的磁场 */
extern CalibState_t  calib_state;          /* 校准状态 */
extern AccelCalib_t  accel_calib;         /* 加速度计校准参数 */
extern GyroCalib_t   gyro_calib;          /* 陀螺仪校准参数 */
extern MagCalib_t    mag_calib;           /* 地磁计校准参数 */
extern uint8_t       mag_calib_valid;     /* 地磁椭球拟合是否有效 (1=有效) */
extern float         imu_dt;               /* 采样周期 (由 DWT 实测) */

/* ============================================================================
 *  校准函数
 * ========================================================================== */

/**
 * @brief 初始化校准状态（开机时调用）
 *
 * 清零校准参数，置位 CALIB_PENDING。
 * 在 StartTask 中 BSP_Init 之后调用。
 *
 * 步骤：
 *   1. 置 calib_state = CALIB_PENDING
 *   2. 初始化 accel_calib / gyro_calib / mag_calib 为 0
 *   3. 初始化 quaternion = (1, 0, 0, 0) 单位四元数
 */
void IMU_Calib_Init(void);

/**
 * @brief 传感器校准数据采集与参数拟合
 *
 * 在 AngelTask 中循环调用，直到 calib_state == CALIB_DONE。
 *
 * 校准顺序（参考笔记 C4、C5）：
 *   步骤 1：加速度计椭球拟合
 *       - 收集多组静止数据 (accel_raw)
 *       - 构建线性方程组 A·Θ = Y（笔记 C4 第 2 节）
 *       - 最小二乘求解 Θ = (AᵀA)⁻¹AᵀY
 *       - 还原 O_x/O_y/O_z 和 S_x/S_y/S_z（笔记 C4 第 2.3、2.4 节）
 *   步骤 2：陀螺仪零偏
 *       - 静止时 100 次平均（参考学长 IMU.c）
 *   步骤 3：地磁计椭球拟合
 *       - 收集多组旋转数据 (mag_raw)
 *       - 构建 9 参数椭球方程（笔记 C5 第二节）
 *       - 高斯牛顿迭代求解（笔记 C5 第三节）
 *       - 还原硬铁偏置 b = -½M⁻¹C（笔记 C5 第 110 行）
 *       - 软铁矩阵 A（楚列斯基分解或特征值分解，笔记 C5 第四节）
 *   步骤 4：四元数初始化
 *       - 由地磁计原始数据求初始 yaw（参考学长 Quat_Init）
 *       - 由初始欧拉角构造四元数（笔记 C2 第三节）
 *       - 置 calib_state = CALIB_DONE
 *
 * 公式：
 *   - 加速度计：S_x²(X-O_x)² + S_y²(Y-O_y)² + S_z²(Z-O_z)² = 1
 *   - 地磁计：(m-b)ᵀM(m-b) = 1，b = -½M⁻¹C
 *   - 四元数初始化：q = [cos(φ/2)cos(θ/2)cos(ψ/2)+..., ...]
 */
void IMU_Calib_Update(void);

/**
 * @brief 检查校准是否完成
 * @return 1 = 完成, 0 = 未完成
 */
uint8_t IMU_Calib_Done(void);

/**
 * @brief 当前校准阶段（供上位机/串口提示）
 * @return 0 = 请保持静止（陀螺零偏）  1 = 请缓慢翻转板子（加计/地磁）
 *         2 = 校准完成
 */
uint8_t IMU_Calib_Stage(void);

/* ============================================================================
 *  姿态解算函数
 * ========================================================================== */

/**
 * @brief 姿态解算主函数（在 AngelTask 中循环调用）
 *
 * 执行流程（参考笔记 C5、C6、C7）：
 *   步骤 1：读取传感器原始数据（由 bsp.c 的 MPU6050_Read_Raw / HMC5883L_Read_Raw 完成）
 *   步骤 2：应用校准参数
 *       - accel_body = (raw_accel - offset) * scale
 *       - gyro_body  = raw_gyro - offset  (转 rad/s)
 *       - mag_body   = A * (raw_mag - offset)
 *   步骤 3：四元数预测更新（陀螺仪积分）
 *       - q̇_ω = ½ · q ⊗ ω（笔记 C5 第 81 行）
 *       - 离散化：q_pred = q + q̇_ω · dt（一阶欧拉）
 *   步骤 4：梯度下降校正（Madgwick）
 *       - 构建残差 f = [q·g·q* - a, q·b·q* - m]（笔记 C7 第 113 行）
 *       - 计算梯度 ∇E = Jᵀf（笔记 C7 第 154 行）
 *       - q_corrected = q_pred - β · ∇E/‖∇E‖ · dt
 *   步骤 5：融合更新
 *       - q_new = q_pred + (q̇_ω - β · ∇E/‖∇E‖) · dt（笔记 C7 第 175 行）
 *   步骤 6：归一化
 *       - q_new = q_new / ‖q_new‖
 *   步骤 7：四元数转欧拉角
 *       - roll  = atan2(2(q0q2 - q1q3), 1-2(q2²+q3²))
 *       - pitch = asin(2(q0q1 - q2q3))
 *       - yaw   = atan2(2(q0q3 + q1q2), 1-2(q1²+q2²))
 *       （笔记 C2 第二节）
 *
 * 公式汇总：
 *   - 四元数乘法：q1⊗q2 = [s1s2 - v1·v2, s1v2 + s2v1 + v1×v2]
 *   - 四元数共轭：q* = [w, -x, -y, -z]
 *   - 旋转矩阵：R = [1-2(c²+d²), 2(bc-ad), 2(ac+bd); ...]
 *   - Madgwick 更新：q_t = q_{t-1} + (q̇_ω - β·∇E/‖∇E‖)·Δt
 *
 * 注意：
 *   - 必须在校准完成后调用（calib_state == CALIB_DONE）
 *   - imu_dt 由定时器测量，确保积分精度
 */
void IMU_Attitude_Update(void);

/**
 * @brief 四元数转欧拉角
 * @param q 输入四元数
 * @param e 输出欧拉角
 *
 * 公式（笔记 C2 第 88-91 行）：
 *   θ = arcsin(2ac + 2bd)
 *   ψ = atan2(2ad - 2bc, 1 - 2c² - 2d²)
 *   φ = atan2(2ab - 2cd, 1 - 2b² - 2c²)
 */
void IMU_Quaternion_To_Euler(const Quaternion_t *q, EulerAngle_t *e);

/**
 * @brief 欧拉角转四元数
 * @param e 输入欧拉角
 * @param q 输出四元数
 *
 * 公式（笔记 C2 第 185 行）：
 *   q = [cos(ψ/2)cos(θ/2)cos(φ/2) - sin(ψ/2)sin(θ/2)sin(φ/2), ...]
 *   （完整展开式见笔记）
 *
 * 用途：校准时由初始 yaw 构造四元数
 */
void IMU_Euler_To_Quaternion(const EulerAngle_t *e, Quaternion_t *q);

/* ============================================================================
 *  工具函数
 * ========================================================================== */

/**
 * @brief 快速平方根倒数（近似）
 * @param x 输入
 * @return 1/√x
 *
 * 参考学长 IMU.c 的 invSqrt，用于归一化加速
 */
float IMU_InvSqrt(float x);

/**
 * @brief 向量归一化
 * @param v 输入向量（会被修改）
 * @return 模长
 */
float IMU_Normalize(Vec3f_t *v);

/**
 * @brief 限制角度到 [-π, π]
 * @param angle 输入角度 (rad)
 * @return 限制后的角度
 */
float IMU_WrapAngle(float angle);

/* ============================================================================
 *  校准数据采集接口（供 AngelTask 调用）
 * ========================================================================== */

/**
 * @brief 加速度计椭球拟合
 * @param samples 采样数据数组
 * @param count 采样数量
 * @param calib 输出校准参数
 *
 * 公式（笔记 C4）：
 *   线性化：θ₁X² + θ₂Y² + θ₃Z² + θ₄X + θ₅Y + θ₆Z = 1
 *   求解：Θ = (AᵀA)⁻¹AᵀY
 *   还原：O_x = -θ₄/(2θ₁), S_x = √(θ₁D)
 *         D = 1/(1 + S)，S = θ₄²/(4θ₁) + θ₅²/(4θ₂) + θ₆²/(4θ₃)
 *
 * 步骤：
 *   1. 构建 A 矩阵 (m×6) 和 Y 向量 (m×1)
 *   2. 计算 AᵀA (6×6) 和 AᵀY (6×1)
 *   3. 求解线性方程组 (AᵀA)Θ = AᵀY
 *   4. 还原 O 和 S
 */
void IMU_Accel_Calib_Fit(const Vec3f_t *samples, uint16_t count, AccelCalib_t *calib);

/**
 * @brief 陀螺仪零偏计算
 * @param samples 采样数据数组
 * @param count 采样数量
 * @param calib 输出校准参数
 *
 * 公式：offset = (1/n) Σ gyro_raw
 * 步骤：
 *   1. 累加所有采样值
 *   2. 求平均
 *   3. 转 rad/s：offset_rad = offset_raw * RAW_TO_RAD
 */
void IMU_Gyro_Calib_Fit(const Vec3f_t *samples, uint16_t count, GyroCalib_t *calib);

/**
 * @brief 地磁计椭球拟合
 * @param samples 采样数据数组
 * @param count 采样数量
 * @param calib 输出校准参数
 *
 * 公式（笔记 C5）：
 *   椭球方程：A_xx x² + A_yy y² + A_zz z² + B_xy xy + B_xz xz + B_yz yz + C_x x + C_y y + C_z z = 1
 *   高斯牛顿迭代：JᵀJ·Δθ = -Jᵀr
 *   还原：b = -½M⁻¹C
 *   软铁矩阵：M = AᵀA，A = Lᵀ（楚列斯基）或 A = V√D Vᵀ（特征值分解）
 *
 * 步骤：
 *   1. 构建雅可比矩阵 J (n×9)
 *   2. 高斯牛顿迭代求解 θ
 *   3. 构造 M 和 C
 *   4. 求 M⁻¹（伴随矩阵法）
 *   5. 计算 b = -½M⁻¹C
 *   6. 软铁分解：M = AᵀA → A
 */
void IMU_Mag_Calib_Fit(const Vec3f_t *samples, uint16_t count, MagCalib_t *calib);

/**
 * @brief 四元数初始化（由地磁计求初始 yaw）
 *
 * 公式（笔记 C2）：
 *   初始 yaw = atan2(M_x cosφ + M_y sinφ sinθ + M_z sinφ cosθ,
 *                     M_y cosθ - M_z sinθ)
 *   （假设初始 roll=0, pitch=0，简化为 atan2(M_x, M_y)）
 *
 * 步骤：
 *   1. 读取地磁计原始数据
 *   2. 归一化
 *   3. 计算初始 yaw
 *   4. 由初始欧拉角 (0, 0, yaw) 构造四元数
 */
void IMU_Quaternion_Init(void);

/* ============================================================================
 *  Madgwick 梯度下降内部函数
 * ========================================================================== */

/**
 * @brief 计算梯度下降方向
 * @param q 当前四元数
 * @param a 加速度计测量值 (归一化)
 * @param m 地磁计测量值 (归一化)
 * @param grad 输出梯度向量 (4×1)
 * @param use_mag 1=使用地磁残差构成 6×1 残差；0=只用重力 3×1 残差
 *
 * 公式（笔记 C6/C7）：
 *   重力 3×4 转置雅可比：
 *     Jᵀ = [-2q2,  2q1,  2q0;
 *            2q3,  2q0, -2q1;
 *           -2q0,  2q3, -2q2;
 *            2q1,  2q2,  2q3]
 *   地磁 3×4 转置雅可比由 w = Rᵀ[bx,0,bz] 对 q 求偏导得到
 *   ∇E = Jᵀ · f，f = [v - a; w - m]（6×1）
 */
void IMU_Madgwick_Gradient(const Quaternion_t *q,
                           const Vec3f_t *a,
                           const Vec3f_t *m,
                           uint8_t use_mag,
                           float grad[4]);

#ifdef __cplusplus
}
#endif

#endif /* __IMU_H */
