/**
 * @file pid_m.c
 * @brief PID算法实现
 * 
 */

#include "pid.h"
#include "imu.h"
#include "bsp.h"
#include "tim.h"
#include "ucos_ii.h"
#include <math.h>       // 用了一个fasbf算绝对值


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
float          dt           = PID_DT_NOMINAL;       // 默认是标称周期，正常时由PID_Get_Time测出来并赋值，异常时可以用这个兜底
uint8_t        armed        = 0;



/* ============================================================================
 *  初始化函数
 * ========================================================================== */

void PID_Init(void){
    /* PID控制器初始化 */
    /* main中调用 */
    /* 1. 初始化 PID 结构体参数 */
    /* 姿态角 */
    roll_shell.Kp = ROLL_SHELL_KP;
    roll_core.Kp  = ROLL_CORE_KP;
    roll_core.Ti  = ROLL_CORE_TI;
    roll_core.Td  = ROLL_CORE_TD;
    pitch_shell.Kp = PITCH_SHELL_KP;
    pitch_core.Kp  = PITCH_CORE_KP;
    pitch_core.Ti  = PITCH_CORE_TI;
    pitch_core.Td  = PITCH_CORE_TD;
    yaw_core.Kp = YAW_CORE_KP;
    yaw_core.Td = YAW_CORE_TD;

    /* 油门外环 */
    thr_shell.Kp = THR_SHELL_KP;
    thr_shell.Td = THR_SHELL_TD;

    /* 2. 初始化期望值结构体 */
    expectation.roll  = 0.0f;
    expectation.pitch = 0.0f;
    expectation.yaw   = 0.0f;
    expectation.thr   = PWM_IDLE;       // 期望怠速

    /* 3. 初始化飞行模式为 STOP */

    /* 4. 初始化电机输出为怠速 */

    /* 5. 初始化解锁标志为 0 */


}

void PID_Time_Init(void){
    /* PID采样周期初始化。配置TIM4为1MHz计数*/
    /* main中调用 */

    __HAL_RCC_TIM4_CLK_ENABLE();
    // SystemCoreClock为84MHz，PSC为83
    // f_cnt = f_tim / (PSC + 1)，对定时器时钟做84分频，得到1MHz计数频率
    TIM4->PSC = (uint16_t)((SystemCoreClock/1000000)-1);
    TIM4->ARR = 0xFFFF;     // 自动重装载寄存器，计数到0xFFFF后重置为0
    TIM4->CNT = 0;          // 计数寄存器，初始值为0
    TIM4->CR1 |= TIM_CR1_CEN; // 使能计数器

}

float PID_Get_Time(void){
    /**
     * 获取 PID 采样周期，用在PID计算里的Δt
     * 这里是根据MotorTask里的OSTimeDlyHMSM(0,0,0,20)监测TIM4，所以周期和MotorTask一样，后面需要改
     * PID_Time_Init() 把TIM4配置成1MHz，转化成秒除1000000
     * 读TIM4计数差值，返回格式秒
     */
    float temp = 0.0f;
    static uint32_t now_time = 0;

    // now_time读取当前时间，转换为秒给temp输出。同时把TIM4计数清零。
    // 公式：dt = CNT / 1000000.0f (s)
    now_time = TIM4->CNT;
    TIM4->CNT = 0;
    temp = (float)now_time/1000000.0f;
    return temp;
}


/* ============================================================================
 *  遥控器期望值映射
 * ========================================================================== */

void PID_PPM_to_Exp_Calc(void){
    /**
     * 遥控器期望值计算，将PPM_Values映射为期望姿态和油门。
     * PPM_Values在bsp.h中，PPM遥控信号8通道值：
     * 通道0:横滚  通道1:俯仰  通道2:油门  通道3:偏航  通道4-7: 辅助通道（拨杆、旋钮等）
     * 公式：expRoll = (PPM[ROLL] - 1500) * 0.03f (最大 20°) 
     *      expPitch = (PPM[PITCH] - 1500) * 0.04f (最大 20°) 
     *      expYaw = (PPM[YAW] - 1500) * 0.02f (最大 10°/s) 
     *      expThr = PPM[THR]
     */

    uint16_t ppm_roll, ppm_pitch, ppm_yaw, ppm_thr;

    /* 1. 读取 PPM_Values（关中断保护） */
    OS_CPU_SR cpu_sr = 0;
    OS_ENTER_CRITICAL();
    ppm_roll = PPM_Values[PPM_CH_ROLL];
    ppm_pitch = PPM_Values[PPM_CH_PITCH];
    ppm_thr = PPM_Values[PPM_CH_THR];
    ppm_yaw = PPM_Values[PPM_CH_YAW];
    OS_EXIT_CRITICAL();

    /* 2. 限幅到有效范围 */
    ppm_roll = PID_Limit(ppm_roll, 1000, 2000);
    ppm_pitch = PID_Limit(ppm_pitch, 1000, 2000);
    ppm_thr = PID_Limit(ppm_thr, 1000, 2000);
    ppm_yaw = PID_Limit(ppm_yaw, 1000, 2000);

    /* 3. 转换为期望值（°或者°/s） */
    /* 这里先做归一化，然后转换成期望单位。方便之后改动。 */
    float exp_roll = PID_To1(ppm_roll, 1000, 2000, -1, 1);
    float exp_pitch = PID_To1(ppm_pitch, 1000, 2000, -1, 1);
    float exp_thr = PID_To1(ppm_thr, 1000, 2000, 0, 1);
    float exp_yaw = PID_To1(ppm_yaw, 1000, 2000, -1, 1);

    expectation.roll = PID_To1(exp_roll, -1, 1, -EXP_ROLL_MAX, EXP_ROLL_MAX);
    expectation.pitch = PID_To1(exp_pitch, -1, 1, -EXP_PITCH_MAX, EXP_PITCH_MAX);
    expectation.yaw = PID_To1(exp_yaw, -1, 1, -EXP_YAW_RATE_MAX, EXP_YAW_RATE_MAX);
    expectation.thr = exp_thr;

}


/* ============================================================================
 *  PID 计算
 * ========================================================================== */

float PID_Cascade_Calc(float shell_err, float core_w_last, PID_t *shell, PID_t *core){

    /**
     * ！！核心部分！！
     * 串级PID运算（Roll/Pitch）
     * 
     * shell:外环  core:内环
     * 公式：
     *   外环：ω_exp = Kp_shell · e_angle
     *   内环：M     = Kp_core · ( e_w + Ki·Σe_w + Kd·Δe_w )
     *   其中  Ki = Δt / Ti     （位置式积分，Δt 由 TIM4 实测）
     *         Kd = Td / Δt     （差分做微分）
     *   单位：e_angle 为度，e_rate 为度/秒，M 为 PWM 增量。
     * 
     * 补充说明：查阅资料得到，PID公式可以写成M = Kp_core · ( e_w + Ki·Σe_w + Kd·Δe_w )
     *          其中Ki = dt/Ti, Kd = Td/dt
     *          和原本的u = Kp·e + Ki·dt·Σe + Kd·Δe/dt相比，
     *          这样做的优点在于：和Kp解耦，Ti/Td形式里改Kp不影响积分时间和微分时间，三个参数可以独立整定。
     *          且采样周期Δt变了不用重调参。
     */

    float Kp_shell = shell->Kp;
    float Kp_core = core->Kp;
    float Ki_core = (core->Ti=0) ?  0.0f : (dt / core->Ti);
    float Kd_core = core->Td / dt;

    /* 1. 计算外环角度误差 e_angle = shell_err */
    shell->eK = shell_err;
    /* 2. 外环纯P输出目标角速度 = Kp*e_angle */
    shell->output = Kp_shell * shell->eK;
    shell->eK_1 = shell->eK;         // 把这次的记成上一次的误差

    /* 3. 计算内环误差 eK = 外环输出 - core_w_last */
    core->eK = shell->output - core_w_last;
    /* 4. 积分项处理 */
    /* 此处会处理积分饱和问题，暂时用积分分离，后续待尝试其他方法。 */
    /* a.积分分离：误差绝对值很大时先不积分，误差回到CORE_INT_SEP以内再积分 */
    if ((Ki_core > 0.0f) && (fabsf(core->eK) <= CORE_INT_SEP)){
        core->eSum += core->eK;
        /* b.积分限幅：限制积分项Ki·Σe在CORE_INT_MAX以内 */
        if ((Ki_core * core->eSum) > CORE_INT_MAX) {
            core->eSum = CORE_INT_MAX / Ki_core;
        }else if ((Ki_core * core->eSum) < -CORE_INT_MAX) {
            core->eSum = - CORE_INT_MAX / Ki_core;
        }
    }

    /* 5. 内环PID输出 = Kp*(e + Ki*eSum + Kd*(e-eK_1)) */
    core->output = Kp_core * (core->eK 
                        + Ki_core * core->eSum
                        + Kd_core * (core->eK - core->eK_1) );
    core->eK_1 = core->eK; 

    /* 6. 输出限幅 */
    core->output = PID_Limit(core->output, PID_OUT_MIN, PID_OUT_MAX);

    return core->output;

}

float PID_Single_Calc(float err, PID_t *pid){
    /**
     * 单环PID计算（Yaw）
     * 其实Yaw也该用串级。后续会修改。
     * 公式：output = Kp*(e + (e-eK_1)*Kd) 
     */

    float Kp = pid->Kp;
    float Kd = pid->Td / dt;
    pid->eK = err;

    pid->output = Kp * (pid->eK + Kd * (pid->eK - pid->eK_1));
    pid->eK_1 = pid->eK;

    pid->output = PID_Limit(pid->output, PID_OUT_MIN, PID_OUT_MAX);

    return pid->output;

}

void PID_Motor_Calc(void){

}

void PID_Motor_Output(void){

}

void PID_Control_Update(void){

}

void PID_Safety_Check(void){

}

void PID_FlyMode_Update(float exp_thr){

}

/* ============================================================================
 *  工具函数
 * ========================================================================== */

float PID_Limit(float value, float min, float max){
    /* 限制value的输出范围:[min, max] */
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float PID_To1(float value, float min_in, float max_in, float min_out, float max_out){
    /* 归一化 */
    return ((value-min_in)/(max_in-min_in)) * ((max_out-max_in)) + min_out;
}
