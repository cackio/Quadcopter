#ifndef __PID_H
#define __PID_H

/* ============================================================================
 *  pid.h — 串级 PID 控制与电机混控模块头文件
 *
 *  依赖：
 *    - main.h (STM32 HAL 类型定义)
 *    - imu.h (姿态解算输出)
 *    - bsp.h (传感器原始数据、PPM 遥控、电机控制)
 *
 *  功能模块：
 *    1. 遥控器期望值映射（PPM → 期望 roll/pitch/yaw/thr）
 *    2. 串级 PID 控制（角度环 P + 角速度环 PID）
 *    3. 电机混控（PID 输出 → 4 路电机 PWM）
 *    4. 安全保护（倾斜角保护、信号丢失保护）
 *    5. 飞行模式状态机（起飞/悬停/降落）
 *
 *
 *
 * ========================================================================== */

#include "main.h"
#include "imu.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  常量与类型定义
 * ========================================================================== */

/* ---- 单位约定（整份 PID 模块统一用这一套，避免"度/弧度"混算） ----
 *   角度       : 度 (°)          —— 由 euler_angle(rad) × RAD_TO_DEG 得到
 *   角速度     : 度/秒 (°/s)      —— 由 gyro_body(rad/s) × RAD_TO_DEG 得到
 *   PID 输出   : PWM 增量         —— 直接叠加到 1000~2000 的油门刻度上
 *   采样周期   : 秒 (s)           —— 由 TIM4 实测（标称 20ms）
 *
 * 为什么用度而不是弧度：默认增益（外环 4.4 / 内环 2.6）是按度整定的，
 * 若把弧度直接喂进 PID，数值会小 57 倍，输出几乎推不动电机。
 */

/* PID 输出限幅：单轴最多给油门叠加 ±500 */
#define PID_OUT_MAX      500.0f    /* PID 输出最大值 */
#define PID_OUT_MIN     -500.0f    /* PID 输出最小值 */
#define PWM_OUT_MAX      2000.0f   /* 电机 PWM 最大值 */
#define PWM_OUT_MIN      1000.0f   /* 电机 PWM 最小值 */
#define PWM_IDLE         1000.0f   /* 怠速 PWM */

/* 内环积分处理 */
#define CORE_INT_MAX     150.0f    /* 积分项最大"贡献"（PWM 单位，不是 eSum 本身） */
#define CORE_INT_SEP      20.0f    /* 积分分离阈值：|e_rate| > 20°/s 时不积分 */

/* 采样周期保护：TIM4 实测值离谱时退回标称值，防止 Ki/Kd 被算成荒谬的系数 */
#define PID_DT_NOMINAL    0.02f    /* 标称周期 20ms（与 MotorTask 周期一致） */
#define PID_DT_MAX        0.10f    /* 超过 100ms 视为无效读数 */

/* 安全保护阈值 */
#define ANGLE_LIMIT_DEG  65.0f     /* 倾斜角保护阈值 */
#define THR_MIN          1050.0f   /* 油门最小值（低于此值停转） */
#define THR_ARM_START    1350.0f   /* 起飞油门阈值 */
#define THR_DISARM_FINI  1050.0f   /* 降落油门阈值 */
#define THR_HOVER_to_UP  1650.0f   /* 状态转换阈值 */
#define THR_HOVER_to_DOWN  1350.0f /* 状态转换阈值 */

/* 默认 PID 参数 */
#define ROLL_SHELL_KP    4.4f      /* Roll 外环 Kp */
#define ROLL_CORE_KP    2.6f      /* Roll 内环 Kp */
#define ROLL_CORE_TI    0.5f      /* Roll 内环 Ti */
#define ROLL_CORE_TD    0.08f     /* Roll 内环 Td */

#define PITCH_SHELL_KP   4.4f     /* Pitch 外环 Kp */
#define PITCH_CORE_KP   2.6f      /* Pitch 内环 Kp */
#define PITCH_CORE_TI   0.5f      /* Pitch 内环 Ti */
#define PITCH_CORE_TD   0.08f     /* Pitch 内环 Td */

#define YAW_CORE_KP      2.6f     /* Yaw 内环 Kp */
#define YAW_CORE_TD     0.08f     /* Yaw 内环 Td */

#define THR_SHELL_KP     1.0f     /* 油门外环 Kp */
#define THR_SHELL_TD     0.0f     /* 油门外环 Td */

/* 遥控器通道映射（PPM_Values 索引） */
#define PPM_CH_ROLL      0        /* 横滚通道 */
#define PPM_CH_PITCH     1        /* 俯仰通道 */
#define PPM_CH_THR       2        /* 油门通道 */
#define PPM_CH_YAW       3        /* 偏航通道 */

/* 期望值限幅 */
#define EXP_ROLL_MAX     20.0f    /* 最大期望横滚角 (deg) */
#define EXP_PITCH_MAX    20.0f    /* 最大期望俯仰角 (deg) */
#define EXP_YAW_RATE_MAX 10.0f    /* 最大期望偏航角速度 (deg/s) */

/* PID 结构体 */
typedef struct
{
    float eK;          /* 本次误差 */
    float eK_1;        /* 上次误差 */
    float eSum;        /* 误差累积和 */
    float Kp;          /* 比例系数 */
    float Ti;          /* 积分时间常数 */
    float Td;          /* 微分时间常数 */
    float output;      /* PID 输出 */
} PID_t;

/* 飞行模式枚举 */
typedef enum
{
    STOP = 0,    /* 停止 */
    DOWN,        /* 下降 */
    HOVER,       /* 悬停 */
    UP           /* 上升 */
} FlyMode_t;

/* 电机输出结构体 */
typedef struct
{
    float motor1;   /* 左前 */
    float motor2;   /* 右前 */
    float motor3;   /* 左后 */
    float motor4;   /* 右后 */
} MotorOutput_t;

/* 期望值结构体
 * 注意：这里统一用"度"而不是弧度，和 PID 内部单位保持一致。 */
typedef struct
{
    float roll;       /* 期望横滚角 (°) */
    float pitch;      /* 期望俯仰角 (°) */
    float yaw;        /* 期望偏航角速度 (°/s) */
    float thr;        /* 期望油门 (PWM，1000~2000) */
} Expectation_t;

/* 角速度结构体 */
typedef struct
{
    float roll;
    float pitch;
    float yaw;
} Rate_t;

/* ============================================================================
 *  全局变量（定义在 pid.c，extern 引用）
 * ========================================================================== */

extern PID_t          roll_shell, roll_core;     /* Roll 串级 PID */
extern PID_t          pitch_shell, pitch_core;   /* Pitch 串级 PID */
extern PID_t          yaw_core;                  /* Yaw 内环 PID */
extern PID_t          thr_shell;                 /* 油门外环 PID */

extern Expectation_t  expectation;                /* 遥控器期望值 */
extern FlyMode_t      fly_mode;                   /* 飞行模式 */
extern MotorOutput_t  motor_out;                  /* 电机输出 */
extern float          dt;                         /* PID 采样周期 */
extern uint8_t        armed;                      /* 解锁标志 */
extern Rate_t         exp_w;                      /* 期望角速度 */

/* ============================================================================
 *  初始化函数
 * ========================================================================== */

/**
 * @brief PID 控制器初始化
 *
 * 在 StartTask 中 BSP_Init 之后调用。
 *
 * 步骤：
 *   1. 初始化 PID 结构体参数（使用默认值）
 *   2. 初始化期望值结构体
 *   3. 初始化飞行模式为 STOP
 *   4. 初始化电机输出为怠速
 *   5. 初始化解锁标志为 0
 */
void PID_Init(void);

/**
 * @brief PID 采样周期初始化（TIM4）
 *
 * 用于精确测量 PID 计算间隔。
 *
 * 步骤：
 *   1. 使能 TIM4 时钟
 *   2. 配置 TIM4：PSC = 84-1（APB1 定时器时钟 84MHz → 1MHz 计数）, ARR=0xFFFF
 *      → 1 个计数 = 1us，最长计 65.5ms
 *   3. 启动 TIM4
 */
void PID_Time_Init(void);

/**
 * @brief 获取 PID 采样周期
 * @return 采样周期 (s)
 *
 * 通过读取 TIM4->CNT 并清零获得。
 */
float PID_Get_Time(void);

/* ============================================================================
 *  遥控器期望值映射
 * ========================================================================== */

/**
 * @brief 遥控器期望值计算
 *
 * 将 PPM_Values 映射为期望姿态和油门。
 * 得到的结果输出在全局变量expectation中（包括roll, pitch, yaw, thr）
 *
 *
 * 步骤：
 *   1. 读取 PPM_Values（关中断保护）
 *   2. 限幅到有效范围
 *   3. 转换为期望值（度 / 度每秒，全程不转弧度）
 */
void PID_PPM_to_Exp_Calc(void);

/* ============================================================================
 *  PID 计算
 * ========================================================================== */

/**
 * @brief 四元数PID外环计算
 * @param exp_e 期望值（弧度）
 * @param shell 外环 PID
 * @return 外环输出目标角速度
 */
void PID_Cascade_Shell_Calc_Quater(PID_t *shell);

 /**
 * @brief 外环PID计算（Roll/Pitch）
 * @param shell_err 外环需要输入的角度误差（°）
 * @param shell 外环 PID
 * @return 外环输出目标角速度
 *
 * 公式：
 *   外环（角度环）：output = Kp * e
 *
 * 步骤：
 *   1. 计算外环误差 e = shell_err
 *   2. 外环输出 = Kp * e
 * 
 */
float PID_Cascade_Shell_Calc(float shell_err, PID_t *shell);

/**
 * @brief 内环PID计算（Roll/Pitch）
 * @param exp_w （可能是外环输出的）目标角速度（°/s）
 * @param core_w_last 内环需要输入的反馈角速度（°/s）
 * @param core 内环 PID
 * @return 内环输出
 *
 * 公式：
 *   内环（角速度环）：output = Kp*(e + eSum*Ki + (e-eK_1)*Kd)
 *
 * 步骤：
 *   1. 计算内环误差 e = 外环输出 - core_w_last
 *   2. 内环积分分离限幅
 *   3. 内环输出 = Kp*(e + eSum*Ki + (e-eK_1)*Kd)
 *   4. 内环输出限幅
 */
float PID_Cascade_Core_Calc(float exp_w, float core_w_last, PID_t *core);


// /**
//  * @brief 串级 PID 计算（Roll/Pitch）
//  * @param shell_err 外环需要输入的角度误差（°）
//  * @param core_w_last 内环需要输入的反馈角速度（°/s）
//  * @param shell 外环 PID
//  * @param core 内环 PID
//  * @return 内环输出
//  *
//  * 公式：
//  *   外环（角度环）：output = Kp * e
//  *   内环（角速度环）：output = Kp*(e + eSum*Ki + (e-eK_1)*Kd)
//  *
//  * 步骤：
//  *   1. 计算外环误差 e = shell_err
//  *   2. 外环输出 = Kp * e
//  *   3. 计算内环误差 e = 外环输出 - core_w_last
//  *   4. 内环积分分离限幅
//  *   5. 内环输出 = Kp*(e + eSum*Ki + (e-eK_1)*Kd)
//  *   6. 内环输出限幅
//  */
// float PID_Cascade_Calc(float shell_err, float core_w_last, PID_t *shell, PID_t *core);

/**
 * @brief 单环 PD 计算（偏航）
 * @param err 角速度误差 e = ω_des - ω (°/s)
 * @param pid PID 结构体
 * @return PID 输出（PWM 增量）
 *
 * 公式：output = Kp*(e + (e-eK_1)*Kd)  （偏航只要 PD，不需要外环）
 */
float PID_Single_Calc(float err, PID_t *pid);

/**
 * @brief 电机混控计算
 *
 * 将 PID 输出映射到 4 路电机 PWM。
 *
 * 公式（参考笔记 C3 控制力矩映射矩阵）：
 *   motor1 = thr - pitch + roll - yaw
 *   motor2 = thr - pitch - roll + yaw
 *   motor3 = thr + pitch + roll + yaw
 *   motor4 = thr + pitch - roll - yaw
 *
 * 步骤：
 *   1. 计算 Roll/Pitch/Yaw PID 输出
 *   2. 按混控公式计算 4 路电机
 *   3. 限幅到 [PWM_OUT_MIN, PWM_OUT_MAX]
 *   4. 安全保护检查
 */
void PID_Motor_Calc(void);

/**
 * @brief 电机 PWM 输出
 *
 * 将 motor_out 写入 TIM3 的 CCR 寄存器。
 *
 * 步骤：
 *   1. motor1 → TIM3_CH1 (PA6)
 *   2. motor2 → TIM3_CH2 (PA7)
 *   3. motor3 → TIM3_CH3 (PB0)
 *   4. motor4 → TIM3_CH4 (PB1)
 */
void PID_Motor_Output(void);

/**
 * @brief 串级 PID 唯一对外入口（MotorTask 每 20ms 调用一次）
 *
 * 内部顺序：期望值映射 → 飞行模式/解锁 → 外环+内环 PID + 混控 → PWM 输出。
 * 外环（角度环）和内环（角速度环）都在这一次调用里、用同一个 Δt 算完。
 */
void PID_Control_Update(void);

/**
 * @brief 安全保护检查
 *
 * 检查是否触发安全保护条件。
 *
 * 条件：
 *   - 油门 < THR_MIN → 停转
 *   - |roll| > ANGLE_LIMIT_DEG → 停转
 *   - |pitch| > ANGLE_LIMIT_DEG → 停转
 *   - 信号丢失 → 停转
 */
void PID_Safety_Check(void);

/**
 * @brief 飞行模式状态机
 * @param exp_thr 期望油门
 *
 * 状态转换：
 *   STOP → HOVER: exp_thr > THR_ARM
 *   HOVER → UP: exp_thr > 1650
 *   HOVER → DOWN: exp_thr < THR_ARM
 *   DOWN → STOP: exp_thr < THR_DISARM
 *   DOWN → HOVER: exp_thr > THR_ARM
 *   UP → HOVER: exp_thr < 1650
 *
 * 步骤：
 *   1. 根据 exp_thr 判断状态转换
 *   2. 更新 fly_mode
 *   3. 解锁/锁定 armed 标志
 */
void PID_FlyMode_Update(float exp_thr);

/* ============================================================================
 *  工具函数
 * ========================================================================== */

/**
 * @brief 限幅函数，限制value的输出范围:[min, max]
 * @param value 输入值
 * @param min 最小值（含）
 * @param max 最大值（含）
 * @return 限幅后的值
 */
float PID_Limit(float value, float min, float max);

/**
 * @brief 归一化函数，把value[min_in:max_in]归化到范围:[min_out:max_out]
 * @param value 输入值
 * @param min 最小值（含）
 * @param max 最大值（含）
 * @return 归一化后的值
 */
float PID_To1(float value, float min_in, float max_in, float min_out, float max_out);

/**
 * @brief 四元数乘法运算
 * @param a 第一个乘数
 * @param b 第二个乘数
 * @param c 输出结果
 */
void PID_Quater_Mul(Quaternion_t *a, Quaternion_t *b, Quaternion_t *c);

/**
 * @brief 四元数归一化
 * @param q 输入四元数
 */
void PID_Quater_to1(Quaternion_t *q);

/**
 * @brief 四元数共轭运算
 * @param q 输入四元数
 * @param conj 输出共轭四元数
 */
void PID_Quater_Conj(Quaternion_t *q, Quaternion_t *conj);

#ifdef __cplusplus
}
#endif

#endif /* __PID_H */
