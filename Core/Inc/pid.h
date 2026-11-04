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
 *  参考笔记：
 *    - PID.md (PID 控制算法、串级 PID)
 *    - C3 四轴飞行器运动机制与模型（控制力矩映射矩阵）
 *
 *  参考代码：
 *    - examplezong/pid.h / pid.c (学长串级 PID 实现)
 *
 * ========================================================================== */

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  常量与类型定义
 * ========================================================================== */

/* PID 参数限幅 */
#define PID_OUT_MAX      500.0f    /* PID 输出最大值 */
#define PID_OUT_MIN     -500.0f    /* PID 输出最小值 */
#define PWM_OUT_MAX      2000.0f   /* PWM 输出最大值 */
#define PWM_OUT_MIN      1000.0f   /* PWM 输出最小值 */
#define PWM_IDLE         1000.0f   /* 怠速 PWM */

/* 内环积分限幅 */
#define CORE_INT_MAX     4000.0f   /* 内环积分幅值 */
#define CORE_INT_SEP     300.0f    /* 内环积分分离幅值 */

/* 安全保护阈值 */
#define ANGLE_LIMIT_DEG  65.0f     /* 倾斜角保护阈值 */
#define THR_MIN          1050.0f   /* 油门最小值（低于此值停转） */
#define THR_ARM          1350.0f   /* 起飞油门阈值 */
#define THR_DISARM       1050.0f   /* 降落油门阈值 */

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
    FLY_MODE_STOP = 0,    /* 停止 */
    FLY_MODE_DOWN,        /* 下降 */
    FLY_MODE_HOVER,       /* 悬停 */
    FLY_MODE_UP           /* 上升 */
} FlyMode_t;

/* 电机输出结构体 */
typedef struct
{
    float motor1;   /* 左前 */
    float motor2;   /* 右前 */
    float motor3;   /* 左后 */
    float motor4;   /* 右后 */
} MotorOutput_t;

/* 期望值结构体 */
typedef struct
{
    float roll;       /* 期望横滚角 (rad) */
    float pitch;      /* 期望俯仰角 (rad) */
    float yaw;        /* 期望偏航角速度 (rad/s) */
    float thr;        /* 期望油门 (PWM) */
} Expectation_t;

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
extern float          pid_dt;                     /* PID 采样周期 */
extern uint8_t        armed;                      /* 解锁标志 */

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
 * 参考学长 pid.c 的 PID_Time_Init。
 *
 * 步骤：
 *   1. 使能 TIM4 时钟
 *   2. 配置 TIM4：PSC=80-1, ARR=0xFFFF（1MHz 计数）
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
 *
 * 公式（参考学长 Motor_Exp_Calc）：
 *   expRoll  = (PPM[ROLL] - 1500) * 0.03f  (最大 20°)
 *   expPitch = (PPM[PITCH] - 1500) * 0.04f  (最大 20°)
 *   expYaw   = (PPM[YAW] - 1500) * 0.02f  (最大 10°/s)
 *   expThr   = PPM[THR]
 *
 * 步骤：
 *   1. 读取 PPM_Values（关中断保护）
 *   2. 限幅到有效范围
 *   3. 转换为期望值
 *   4. 转换单位（角度 → 弧度）
 */
void PID_Exp_Calc(void);

/* ============================================================================
 *  PID 计算
 * ========================================================================== */

/**
 * @brief 串级 PID 计算（Roll/Pitch）
 * @param shell_err 外环误差（角度误差，rad）
 * @param core_status 内环反馈（角速度，rad/s）
 * @param shell 外环 PID
 * @param core 内环 PID
 * @return 内环输出
 *
 * 公式（参考笔记 PID.md 第五节）：
 *   外环（角度环）：output = Kp * e
 *   内环（角速度环）：output = Kp*(e + eSum*Ki + (e-eK_1)*Kd)
 *
 * 步骤：
 *   1. 计算外环误差 e = shell_err
 *   2. 外环输出 = Kp * e
 *   3. 计算内环误差 e = 外环输出 - core_status
 *   4. 内环积分分离限幅
 *   5. 内环输出 = Kp*(e + eSum*Ki + (e-eK_1)*Kd)
 *   6. 内环输出限幅
 */
float PID_Cascade_Calc(float shell_err, float core_status, PID_t *shell, PID_t *core);

/**
 * @brief 单环 PID 计算（Yaw/Thr）
 * @param err 误差
 * @param status 反馈值
 * @param pid PID 结构体
 * @return PID 输出
 *
 * 公式：output = Kp*(e + (e-eK_1)*Kd)  (PD)
 */
float PID_Single_Calc(float err, float status, PID_t *pid);

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
 * 状态转换（参考学长 Judge_FlyMode）：
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
 * @brief 限幅函数
 * @param value 输入值
 * @param min 最小值
 * @param max 最大值
 * @return 限幅后的值
 */
float PID_Limit(float value, float min, float max);

#ifdef __cplusplus
}
#endif

#endif /* __PID_H */
