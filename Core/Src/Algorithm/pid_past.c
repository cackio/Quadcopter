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
#include <math.h>      /* fabsf：积分分离/安全保护里判断误差大小 */

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

/* ----------------------------------------------------------------------------
 *  PID 历史状态清零（不动整定参数 Kp/Ti/Td）
 *
 * 什么时候用：电机停转（未解锁 / 油门低于 THR_MIN / 倾角超限）时清零。
 * 为什么：积分项是"历史上误差的累积"，停在地面推杆时如果不清零，
 *         eSum 会越积越大，一起飞立刻满舵（积分饱和/windup）。
 * -------------------------------------------------------------------------- */
static void PID_Reset_State(PID_t *p)
{
    p->eK     = 0.0f;
    p->eK_1   = 0.0f;
    p->eSum   = 0.0f;
    p->output = 0.0f;
}

static void PID_Reset_All(void)
{
    PID_Reset_State(&roll_shell);  PID_Reset_State(&roll_core);
    PID_Reset_State(&pitch_shell); PID_Reset_State(&pitch_core);
    PID_Reset_State(&yaw_core);
}

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
    fly_mode = STOP;

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

void PID_PPM_to_Exp_Calc(void)
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

    /* 步骤 3：转换为期望值（单位：度 / 度每秒） */
    float exp_roll_deg  = (float)(ppm_roll - 1500) * 0.03f;  /* ±500 → ±15° */
    float exp_pitch_deg = (float)(ppm_pitch - 1500) * 0.04f; /* ±500 → ±20° */
    float exp_yaw_deg   = (float)(ppm_yaw - 1500) * 0.02f;   /* ±500 → ±10°/s */

    /* 步骤 4：再限一次幅（摇杆端点可能略超 1000~2000）
     *        这里保持"度"，不再转弧度——PID 模块全程用度，
     *        弧度的换算只在 PID_Motor_Calc 读传感器时做一次。 */
    expectation.roll  = PID_Limit(exp_roll_deg,  -EXP_ROLL_MAX,   EXP_ROLL_MAX);
    expectation.pitch = PID_Limit(exp_pitch_deg, -EXP_PITCH_MAX,  EXP_PITCH_MAX);
    expectation.yaw   = PID_Limit(exp_yaw_deg,   -EXP_YAW_RATE_MAX, EXP_YAW_RATE_MAX);
    expectation.thr   = (float)ppm_thr;
}

/* ============================================================================
 *  PID 计算
 * ========================================================================== */

float PID_Cascade_Calc(float shell_err, float core_w_last, PID_t *shell, PID_t *core)
{
    /* 串级 PID 计算（Roll/Pitch）
     *
     * 结构（笔记 PID.md 第五节）：
     *
     *   θ_des ──►(−)──►[外环 P]──ω_des──►(−)──►[内环 PID]──► M (PWM 增量)
     *             ▲                        ▲
     *             │ θ (°)                 │ ω (°/s)
     *          欧拉角(SensorTask)        陀螺仪(SensorTask)
     *
     * 为什么拆两级：单环"角度 → 力矩"是二阶对象，Kp 大了就振、小了就软；
     * 拆成"角度 → 期望转速（P）"和"转速 → 力矩（PID）"两个一阶环后，
     * 外环决定"回中快慢"，内环决定"跟随转速的阻尼和抗风能力"，各自独立整定。
     *
     * 公式：
     *   外环：ω_des = Kp_shell · e_angle
     *   内环：M     = Kp_core · ( e_rate + Ki·Σe_rate + Kd·Δe_rate )
     *   其中  Ki = Δt / Ti     （位置式积分，Δt 由 TIM4 实测）
     *         Kd = Td / Δt     （差分做微分）
     *   单位：e_angle 为度，e_rate 为度/秒，M 为 PWM 增量。
     *
     * 步骤：
     *   1. 外环误差 e_angle = θ_des - θ
     *   2. 外环 P → 期望角速度 ω_des
     *   3. 内环误差 e_rate = ω_des - ω
     *   4. 积分分离 + 积分限幅（抗饱和）
     *   5. 内环 PID 输出
     *   6. 输出限幅
     */

    float Ki = (core->Ti > 0.0f) ? (pid_dt / core->Ti) : 0.0f;  /* Ki = Δt/Ti，Ti=0 表示不用积分 */
    float Kd = (pid_dt > 0.0f) ? (core->Td / pid_dt) : 0.0f;    /* Kd = Td/Δt */

    /* ---- 步骤 1、2：外环（角度环，纯 P）---- */
    shell->eK     = shell_err;
    shell->output = shell->Kp * shell->eK;      /* 输出就是期望角速度 ω_des */
    shell->eK_1   = shell->eK;

    /* ---- 步骤 3：内环误差 ---- */
    core->eK = shell->output - core_w_last;

    /* ---- 步骤 4：积分处理 ---- */
    /* 4.1 积分分离：误差很大时先不积分（大偏差下积分只会让回中更超调），
     *     只有转速误差回到 CORE_INT_SEP(20°/s) 以内才开始累积。 */
    if ((Ki > 0.0f) && (fabsf(core->eK) < CORE_INT_SEP)) {
        core->eSum += core->eK;

        /* 4.2 积分限幅（windup 保护）：限制的是"积分项的贡献"Ki·Σe，
         *     不是 Σe 本身，这样限幅值和 PID 输出在同一量纲（PWM），
         *     调参时直觉更清楚：积分最多只能贡献 ±CORE_INT_MAX。 */
        if ((core->eSum * Ki) > CORE_INT_MAX) {
            core->eSum = CORE_INT_MAX / Ki;
        } else if ((core->eSum * Ki) < -CORE_INT_MAX) {
            core->eSum = -CORE_INT_MAX / Ki;
        }
    }

    /* ---- 步骤 5：内环 PID 输出 ---- */
    core->output = core->Kp * (core->eK
                               + core->eSum * Ki
                               + (core->eK - core->eK_1) * Kd);

    /* ---- 步骤 6：输出限幅（单轴不允许把油门直接顶到饱和）---- */
    core->output = PID_Limit(core->output, PID_OUT_MIN, PID_OUT_MAX);
    core->eK_1   = core->eK;

    return core->output;
}

float PID_Single_Calc(float err, PID_t *pid)
{
    /* 单环 PD 计算（偏航）
     *
     * 偏航不接角度环：摇杆直接给"期望角速度"，所以只有一级。
     * 公式：M = Kp·( e_rate + Kd·Δe_rate )
     * 单位：e_rate 为度/秒，M 为 PWM 增量。
     *
     * 说明：偏航不加积分，是因为偏航没有"绝对期望角度"，
     *      积分容易把陀螺零偏的残差一直累加，导致缓慢自旋。
     */

    float Kd = (pid_dt > 0.0f) ? (pid->Td / pid_dt) : 0.0f;

    pid->eK     = err;
    pid->output = pid->Kp * (pid->eK + (pid->eK - pid->eK_1) * Kd);
    pid->output = PID_Limit(pid->output, PID_OUT_MIN, PID_OUT_MAX);
    pid->eK_1   = pid->eK;

    return pid->output;
}

void PID_Motor_Calc(void)
{
    /* 串级 PID 主计算 + 电机混控
     *
     * 两个串（外环角度环、内环角速度环）都在本函数里、用同一个 dt 算完：
     *   - 时间基准一致，不会出现"外环用 20ms、内环用 10ms"的混乱；
     *   - 只被 MotorTask 调用，不存在多任务并发改同一组 PID 状态的问题。
     *
     * 步骤：
     *   0. 取采样周期 Δt（TIM4 实测，带异常保护）
     *   1. 传感器量换算单位（rad → °, rad/s → °/s）
     *   2. Roll / Pitch 串级 PID
     *   3. Yaw 单环 PD
     *   4. X 型混控 → 4 路 PWM
     *   5. 安全保护检查
     */

    /* ---- 步骤 0：采样周期 ---- */
    float dt = PID_Get_Time();
    if ((dt <= 0.0005f) || (dt > PID_DT_MAX)) {
        /* 首次调用、被高优先级任务打断很久、或 TIM4 计数器回绕时会读到离谱值，
         * 这时退回标称 20ms，否则 Ki=Δt/Ti 会瞬间变得极大 → 输出爆掉 */
        dt = PID_DT_NOMINAL;
    }
    pid_dt = dt;

    /* ---- 步骤 1：单位换算 ----
     * 轴对应关系（与 imu.c 的四元数/欧拉角约定一致）：
     *   imu.c 里 R_q = C_b^e，欧拉角按 Z-Y-X 外旋提取：
     *     roll  φ 绕机体系 +x 轴
     *     pitch θ 绕机体系 +y 轴
     *     yaw   ψ 绕机体系 +z 轴
     *   因此角速度反馈也必须是同一组轴：
     *     φ̇ ≈ ω_x = gyro_body.x
     *     θ̇ ≈ ω_y = gyro_body.y
     *     ψ̇ ≈ ω_z = gyro_body.z
     *   （小角度下三个欧拉角速度就近似等于机体角速度的三个分量。）
     * 实机若发现某个轴"越修正越偏"，就是这里的符号取反了，改一行即可。
     */
    float roll_deg   =  euler_angle.roll  * RAD_TO_DEG;
    float pitch_deg  =  euler_angle.pitch * RAD_TO_DEG;
    float roll_rate  =  gyro_body.x * RAD_TO_DEG;
    float pitch_rate =  gyro_body.y * RAD_TO_DEG;
    float yaw_rate   =  gyro_body.z * RAD_TO_DEG;

    /* ---- 步骤 2：Roll / Pitch 串级 PID（外环 + 内环）---- */
    float pid_roll  = PID_Cascade_Calc(expectation.roll  - roll_deg,
                                      roll_rate,  &roll_shell,  &roll_core);
    float pid_pitch = PID_Cascade_Calc(expectation.pitch - pitch_deg,
                                       pitch_rate, &pitch_shell, &pitch_core);

    /* ---- 步骤 3：Yaw 单环 PD ---- */
    float pid_yaw   = PID_Single_Calc(expectation.yaw - yaw_rate, &yaw_core);

    /* ---- 步骤 4：X 型混控（笔记 C3 的控制力矩映射矩阵）----
     * 电机布局：1=左前  2=右前  3=左后  4=右后
     * 符号前提（见说明文档第二节"全局约定"）：
     *   roll > 0  = 左侧抬起 / 右侧下沉
     *   pitch > 0 = 机头下沉（低头）
     *   yaw  > 0  = 从机顶俯视逆时针（机头左转）
     * 于是各轴 PID 输出的物理含义是：
     *   pid_pitch > 0 → 前电机减速、后电机加速 → 机头下沉
     *   pid_roll  > 0 → 左电机加速、右电机减速 → 左侧抬起
     *   pid_yaw   > 0 → 1、4 减速、2、3 加速 → 对角差动
     * 注意：yaw 的正负与电机转向（正反桨）有关，首飞前务必核对螺旋桨转向。
     */
    motor_out.motor1 = PID_Limit(expectation.thr - pid_pitch + pid_roll - pid_yaw,
                                 PWM_OUT_MIN, PWM_OUT_MAX);
    motor_out.motor2 = PID_Limit(expectation.thr - pid_pitch - pid_roll + pid_yaw,
                                 PWM_OUT_MIN, PWM_OUT_MAX);
    motor_out.motor3 = PID_Limit(expectation.thr + pid_pitch + pid_roll + pid_yaw,
                                 PWM_OUT_MIN, PWM_OUT_MAX);
    motor_out.motor4 = PID_Limit(expectation.thr + pid_pitch - pid_roll - pid_yaw,
                                 PWM_OUT_MIN, PWM_OUT_MAX);

    /* ---- 步骤 5：安全保护（未解锁/倾角超限时会把上面算出的值全部改回怠速）---- */
    PID_Safety_Check();
}

void PID_Control_Update(void)
{
    /* 串级 PID 的唯一对外入口（MotorTask 每 20ms 调一次）
     *
     * 顺序很重要：
     *   期望值 → 飞行模式/解锁 → PID+混控 → 输出 PWM
     * 如果先算 PID 再判解锁，会出现"解锁瞬间用了上一周期没清零的积分"的问题。
     */
    PID_PPM_to_Exp_Calc();                          /* 遥控器 → 期望姿态/油门（°、°/s、PWM） */
    PID_FlyMode_Update(expectation.thr);     /* 油门推过 1350 才解锁 armed=1 */
    PID_Motor_Calc();                        /* 外环+内环+混控（同一个 dt） */
    PID_Motor_Output();                      /* 写入 TIM3 四路 CCR */
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
        PID_Reset_All();    /* 电机停转 → 积分清零，防止地面推杆把积分撑满 */
        return;
    }

    /* 步骤 2：检查倾斜角 */
    if (fabsf(roll_deg) > ANGLE_LIMIT_DEG || fabsf(pitch_deg) > ANGLE_LIMIT_DEG) {
        /* 步骤 4：停转 */
        motor_out.motor1 = PWM_IDLE;
        motor_out.motor2 = PWM_IDLE;
        motor_out.motor3 = PWM_IDLE;
        motor_out.motor4 = PWM_IDLE;
        armed = 0;
        PID_Reset_All();    /* 姿态已经救不回来 → 清积分，落地后重新开始 */
        return;
    }

    /* 步骤 3：检查解锁状态 */
    if (!armed) {
        /* 未解锁，不输出电机 */
        motor_out.motor1 = PWM_IDLE;
        motor_out.motor2 = PWM_IDLE;
        motor_out.motor3 = PWM_IDLE;
        motor_out.motor4 = PWM_IDLE;
        PID_Reset_All();    /* 解锁前始终保持积分器为 0 */
    }
}

void PID_FlyMode_Update(float exp_thr)
{
    /* 飞行模式状态机
     *
     * 状态转换（参考学长 Judge_FlyMode）：
     *   STOP → HOVER: exp_thr > THR_ARM_START
     *   HOVER → UP: exp_thr > 1650
     *   HOVER → DOWN: exp_thr < THR_ARM_START
     *   DOWN → STOP: exp_thr < THR_DISARM_FINI
     *   DOWN → HOVER: exp_thr > THR_ARM_START
     *   UP → HOVER: exp_thr < 1650
     *
     * 步骤：
     *   1. 根据 exp_thr 判断状态转换
     *   2. 更新 fly_mode
     *   3. 解锁/锁定 armed 标志
     */

    switch (fly_mode) {
    case STOP:
        if (exp_thr > THR_ARM_START) {
            fly_mode = HOVER;
            armed = 1;
        }
        break;

    case HOVER:
        if (exp_thr > 1650.0f) {
            fly_mode = UP;
        }
        if (exp_thr < THR_ARM_START) {
            fly_mode = DOWN;
        }
        break;

    case DOWN:
        if (exp_thr > THR_ARM_START) {
            fly_mode = HOVER;
        }
        if (exp_thr < THR_DISARM_FINI) {
            fly_mode = STOP;
            armed = 0;
        }
        break;

    case UP:
        if (exp_thr < 1650.0f) {
            fly_mode = HOVER;
        }
        break;

    default:
        fly_mode = STOP;
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
