/**
 * @file    pid.c
 * @brief   串级 PID 控制与电机混控模块
 *
 * 本文件实现笔记 PID.md 和 C3 推导的控制算法：
 *   - 遥控器期望值映射
 *   - 串级 PID 控制（角度环 + 角速度环）
 *   - 电机混控
 *   - 安全保护
 *   - 飞行模式状态机
 *
 * 依赖：
 *   - pid.h       (本模块头文件)
 *   - imu.h       (姿态解算输出 euler_angle, gyro_body)
 *   - bsp.h       (传感器原始数据、PPM_Values、Motor_SetSpeed)
 *   - main.h      (STM32 HAL 类型)
 *   - ucos_ii.h   (uC/OS-II，用于 OS_ENTER_CRITICAL 保护共享变量)
 *
 * 参考笔记：
 *   - PID.md (PID 控制算法、串级 PID)
 *   - C3 四轴飞行器运动机制与模型（控制力矩映射矩阵）
 *
 * 参考代码：
 *   - examplezong/pid.c (学长串级 PID 实现)
 *
 * ========================================================================== */

#include "pid.h"
#include "imu.h"
#include "bsp.h"
#include "tim.h"
#include "ucos_ii.h"

/* ============================================================================
 *  全局变量定义
 * ========================================================================== */

PID_t          roll_shell = {0, 0, 0, ROLL_SHELL_KP, 0, 0, 0};
PID_t          roll_core  = {0, 0, 0, ROLL_CORE_KP, ROLL_CORE_TI, ROLL_CORE_TD, 0};
PID_t          pitch_shell = {0, 0, 0, PITCH_SHELL_KP, 0, 0, 0};
PID_t          pitch_core  = {0, 0, 0, PITCH_CORE_KP, PITCH_CORE_TI, PITCH_CORE_TD, 0};
PID_t          yaw_core   = {0, 0, 0, YAW_CORE_KP, 0, YAW_CORE_TD, 0};
PID_t          thr_shell  = {0, 0, 0, THR_SHELL_KP, 0, THR_SHELL_TD, 0};

Expectation_t  expectation = {0, 0, 0, PWM_IDLE};
FlyMode_t      fly_mode    = FLY_MODE_STOP;
MotorOutput_t  motor_out   = {PWM_IDLE, PWM_IDLE, PWM_IDLE, PWM_IDLE};
float          pid_dt       = 0.02f;
uint8_t        armed        = 0;

/* ============================================================================
 *  初始化函数
 * ========================================================================== */

void PID_Init(void)
{
    /* 步骤 1：初始化 PID 结构体参数（使用默认值） */
    /* Roll 串级 */
    roll_shell.Kp = ROLL_SHELL_KP;
    roll_core.Kp  = ROLL_CORE_KP;
    roll_core.Ti  = ROLL_CORE_TI;
    roll_core.Td  = ROLL_CORE_TD;

    /* Pitch 串级 */
    pitch_shell.Kp = PITCH_SHELL_KP;
    pitch_core.Kp  = PITCH_CORE_KP;
    pitch_core.Ti  = PITCH_CORE_TI;
    pitch_core.Td  = PITCH_CORE_TD;

    /* Yaw 内环 */
    yaw_core.Kp = YAW_CORE_KP;
    yaw_core.Td = YAW_CORE_TD;

    /* 油门外环 */
    thr_shell.Kp = THR_SHELL_KP;
    thr_shell.Td = THR_SHELL_TD;

    /* 步骤 2：初始化期望值结构体 */
    expectation.roll  = 0.0f;
    expectation.pitch = 0.0f;
    expectation.yaw   = 0.0f;
    expectation.thr   = PWM_IDLE;

    /* 步骤 3：初始化飞行模式为 STOP */
    fly_mode = FLY_MODE_STOP;

    /* 步骤 4：初始化电机输出为怠速 */
    motor_out.motor1 = PWM_IDLE;
    motor_out.motor2 = PWM_IDLE;
    motor_out.motor3 = PWM_IDLE;
    motor_out.motor4 = PWM_IDLE;

    /* 步骤 5：初始化解锁标志为 0 */
    armed = 0;
}

void PID_Time_Init(void)
{
    /* PID 采样周期初始化（TIM4，1 MHz 计数）
     *
     * 原实现用的是标准外设库（RCC_APB1PeriphClockCmd / TIM_TimeBaseInit），
     * 本工程是 HAL 库，这里改成等价的寄存器配置：
     *   TIM4 计数时钟 = 84 MHz（APB1 = 42 MHz，定时器时钟再 x2）
     *   PSC = 84 - 1  → 计数频率 1 MHz，1 个计数 = 1 us
     *   ARR = 0xFFFF  → 最长计时 65.535 ms
     *
     * PID_Get_Time() 用 "CNT / 1e6" 换算成秒，与此处的 1 MHz 保持一致。
     */

    __HAL_RCC_TIM4_CLK_ENABLE();

    TIM4->PSC = (uint16_t)((SystemCoreClock / 1000000UL) - 1UL);
    TIM4->ARR = 0xFFFFUL;
    TIM4->CNT = 0;
    TIM4->CR1 |= TIM_CR1_CEN;      /* 启动计数器 */
}

float PID_Get_Time(void)
{
    /* 获取 PID 采样周期
     *
     * 通过读取 TIM4->CNT 并清零获得。
     *
     * 公式：dt = CNT / 1000000.0f (s)
     *
     * 步骤：
     *   1. 读取 TIM4->CNT
     *   2. 清零 TIM4->CNT
     *   3. 转换为秒
     */

    float temp = 0;
    static uint32_t now = 0;

    /* 步骤 1：读取计数器值 */
    now = TIM4->CNT;

    /* 步骤 2：清零计数器 */
    TIM4->CNT = 0;

    /* 步骤 3：转换为秒 */
    temp = (float)now / 1000000.0f;

    return temp;
}

/* ============================================================================
 *  遥控器期望值映射
 * ========================================================================== */

void PID_Exp_Calc(void)
{
    /* 遥控器期望值计算
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

    uint16_t ppm_roll, ppm_pitch, ppm_thr, ppm_yaw;

    /* 步骤 1：读取 PPM_Values（关中断保护） */
    OS_CPU_SR cpu_sr = 0;
    OS_ENTER_CRITICAL();
    ppm_roll  = PPM_Values[PPM_CH_ROLL];
    ppm_pitch = PPM_Values[PPM_CH_PITCH];
    ppm_thr   = PPM_Values[PPM_CH_THR];
    ppm_yaw   = PPM_Values[PPM_CH_YAW];
    OS_EXIT_CRITICAL();

    /* 步骤 2：限幅到有效范围 */
    ppm_roll  = PID_Limit(ppm_roll, 1000, 2000);
    ppm_pitch = PID_Limit(ppm_pitch, 1000, 2000);
    ppm_thr   = PID_Limit(ppm_thr, 1000, 2000);
    ppm_yaw   = PID_Limit(ppm_yaw, 1000, 2000);

    /* 步骤 3：转换为期望值（角度） */
    float exp_roll_deg  = (float)(ppm_roll - 1500) * 0.03f;  /* 最大 20° */
    float exp_pitch_deg = (float)(ppm_pitch - 1500) * 0.04f; /* 最大 20° */
    float exp_yaw_deg   = (float)(ppm_yaw - 1500) * 0.02f;   /* 最大 10°/s */

    /* 步骤 4：转换单位（角度 → 弧度） */
    expectation.roll  = exp_roll_deg * DEG_TO_RAD;
    expectation.pitch = exp_pitch_deg * DEG_TO_RAD;
    expectation.yaw   = exp_yaw_deg * DEG_TO_RAD;
    expectation.thr   = (float)ppm_thr;
}

/* ============================================================================
 *  PID 计算
 * ========================================================================== */

float PID_Cascade_Calc(float shell_err, float core_status, PID_t *shell, PID_t *core)
{
    /* 串级 PID 计算（Roll/Pitch）
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

    float shell_Kd, core_Ki, core_Kd;

    /* 步骤 1：计算外环误差 */
    shell->eK = shell_err;
    shell->output = shell->Kp * shell->eK;
    shell->eK_1 = shell->eK;

    /* 步骤 2：外环输出作为内环期望 */
    /* 步骤 3：计算内环误差 */
    core->eK = shell->output - core_status;

    /* 步骤 4：内环积分分离限幅 */
    core_Ki = pid_dt / core->Ti;
    core_Kd = core->Td / pid_dt;

    if (core->eSum > CORE_INT_MAX) {
        if (core->eK < 0) {
            core->eSum += core->eK;
        }
    } else if (core->eSum < -CORE_INT_MAX) {
        if (core->eK > 0) {
            core->eSum += core->eK;
        }
    } else {
        core->eSum += core->eK;
    }

    /* 步骤 5：内环输出 */
    core->output = core->Kp * (core->eK + core->eSum * core_Ki + (core->eK - core->eK_1) * core_Kd);
    core->output = PID_Limit(core->output, PID_OUT_MIN, PID_OUT_MAX);
    core->eK_1 = core->eK;

    return core->output;
}

float PID_Single_Calc(float err, float status, PID_t *pid)
{
    /* 单环 PID 计算（Yaw/Thr）
     *
     * 公式：output = Kp*(e + (e-eK_1)*Kd)  (PD)
     *
     * 步骤：
     *   1. 计算误差 e = err
     *   2. 计算微分项 (e - eK_1) * Kd
     *   3. 输出 = Kp * (e + 微分项)
     *   4. 限幅
     */

    float Kd = pid->Td / pid_dt;

    pid->eK = err;
    pid->output = pid->Kp * (pid->eK + (pid->eK - pid->eK_1) * Kd);
    pid->output = PID_Limit(pid->output, PID_OUT_MIN, PID_OUT_MAX);
    pid->eK_1 = pid->eK;

    return pid->output;
}

void PID_Motor_Calc(void)
{
    /* 电机混控计算
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

    /* 步骤 1：计算 PID 采样周期 */
    pid_dt = PID_Get_Time();

    /* 步骤 1：计算 Roll/Pitch/Yaw PID 输出 */
    float pid_roll  = PID_Cascade_Calc(expectation.roll - euler_angle.roll,
                                       gyro_body.y, &roll_shell, &roll_core);
    float pid_pitch = PID_Cascade_Calc(expectation.pitch - euler_angle.pitch,
                                       -gyro_body.x, &pitch_shell, &pitch_core);
    float pid_yaw   = PID_Single_Calc(expectation.yaw - gyro_body.z,
                                      gyro_body.z, &yaw_core);

    /* 步骤 2：按混控公式计算 4 路电机 */
    /* X 型布局：电机 1=左前, 2=右前, 3=左后, 4=右后 */
    /* 注意：这里的符号需要根据实际电机转向调整 */
    motor_out.motor1 = PID_Limit(expectation.thr - pid_pitch + pid_roll - pid_yaw,
                                 PWM_OUT_MIN, PWM_OUT_MAX);
    motor_out.motor2 = PID_Limit(expectation.thr - pid_pitch - pid_roll + pid_yaw,
                                 PWM_OUT_MIN, PWM_OUT_MAX);
    motor_out.motor3 = PID_Limit(expectation.thr + pid_pitch + pid_roll + pid_yaw,
                                 PWM_OUT_MIN, PWM_OUT_MAX);
    motor_out.motor4 = PID_Limit(expectation.thr + pid_pitch - pid_roll - pid_yaw,
                                 PWM_OUT_MIN, PWM_OUT_MAX);

    /* 步骤 4：安全保护检查 */
    PID_Safety_Check();
}

void PID_Motor_Output(void)
{
    /* 电机 PWM 输出
     *
     * 将 motor_out 写入 TIM3 的 CCR 寄存器。
     *
     * 步骤：
     *   1. motor1 → TIM3_CH1 (PA6)
     *   2. motor2 → TIM3_CH2 (PA7)
     *   3. motor3 → TIM3_CH3 (PB0)
     *   4. motor4 → TIM3_CH4 (PB1)
     */

    /* 步骤 1：设置电机 1 PWM */
    Motor_SetSpeed(&htim3, TIM_CHANNEL_1, (uint16_t)motor_out.motor1);

    /* 步骤 2：设置电机 2 PWM */
    Motor_SetSpeed(&htim3, TIM_CHANNEL_2, (uint16_t)motor_out.motor2);

    /* 步骤 3：设置电机 3 PWM */
    Motor_SetSpeed(&htim3, TIM_CHANNEL_3, (uint16_t)motor_out.motor3);

    /* 步骤 4：设置电机 4 PWM */
    Motor_SetSpeed(&htim3, TIM_CHANNEL_4, (uint16_t)motor_out.motor4);
}

void PID_Safety_Check(void)
{
    /* 安全保护检查
     *
     * 条件：
     *   - 油门 < THR_MIN → 停转
     *   - |roll| > ANGLE_LIMIT_DEG → 停转
     *   - |pitch| > ANGLE_LIMIT_DEG → 停转
     *   - 信号丢失 → 停转
     *
     * 步骤：
     *   1. 检查油门是否低于阈值
     *   2. 检查倾斜角是否超过阈值
     *   3. 检查是否解锁
     *   4. 触发保护时将所有电机设为怠速
     */

    float roll_deg = euler_angle.roll * RAD_TO_DEG;
    float pitch_deg = euler_angle.pitch * RAD_TO_DEG;

    /* 步骤 1：检查油门 */
    if (expectation.thr < THR_MIN) {
        /* 步骤 4：停转 */
        motor_out.motor1 = PWM_IDLE;
        motor_out.motor2 = PWM_IDLE;
        motor_out.motor3 = PWM_IDLE;
        motor_out.motor4 = PWM_IDLE;
        armed = 0;
        return;
    }

    /* 步骤 2：检查倾斜角 */
    if (fabs(roll_deg) > ANGLE_LIMIT_DEG || fabs(pitch_deg) > ANGLE_LIMIT_DEG) {
        /* 步骤 4：停转 */
        motor_out.motor1 = PWM_IDLE;
        motor_out.motor2 = PWM_IDLE;
        motor_out.motor3 = PWM_IDLE;
        motor_out.motor4 = PWM_IDLE;
        armed = 0;
        return;
    }

    /* 步骤 3：检查解锁状态 */
    if (!armed) {
        /* 未解锁，不输出电机 */
        motor_out.motor1 = PWM_IDLE;
        motor_out.motor2 = PWM_IDLE;
        motor_out.motor3 = PWM_IDLE;
        motor_out.motor4 = PWM_IDLE;
    }
}

void PID_FlyMode_Update(float exp_thr)
{
    /* 飞行模式状态机
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

    switch (fly_mode) {
    case FLY_MODE_STOP:
        if (exp_thr > THR_ARM) {
            fly_mode = FLY_MODE_HOVER;
            armed = 1;
        }
        break;

    case FLY_MODE_HOVER:
        if (exp_thr > 1650.0f) {
            fly_mode = FLY_MODE_UP;
        }
        if (exp_thr < THR_ARM) {
            fly_mode = FLY_MODE_DOWN;
        }
        break;

    case FLY_MODE_DOWN:
        if (exp_thr > THR_ARM) {
            fly_mode = FLY_MODE_HOVER;
        }
        if (exp_thr < THR_DISARM) {
            fly_mode = FLY_MODE_STOP;
            armed = 0;
        }
        break;

    case FLY_MODE_UP:
        if (exp_thr < 1650.0f) {
            fly_mode = FLY_MODE_HOVER;
        }
        break;

    default:
        fly_mode = FLY_MODE_STOP;
        armed = 0;
        break;
    }
}

/* ============================================================================
 *  工具函数
 * ========================================================================== */

float PID_Limit(float value, float min, float max)
{
    /* 限幅函数 */
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
