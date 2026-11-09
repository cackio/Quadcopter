/**
 * @file pid_m.c
 * @brief PID算法实现
 * 
 * 目前已完成的功能：
 *      遥控器期望值映射为目标角度
 *      串级PID控制
 *      四电机混控输出
 *      安全保护
 *      飞行模式状态机
 * 
 */

#include "pid.h"
#include "imu.h"
#include "bsp.h"
#include "tim.h"        // 控制电机输出
#include "ucos_ii.h"    // 进临界区
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
FlyMode_t      fly_mode    = STOP; 
MotorOutput_t  motor_out   = {PWM_IDLE, PWM_IDLE, PWM_IDLE, PWM_IDLE};
float          dt          = PID_DT_NOMINAL;       // 默认是标称周期，正常时由PID_Get_Time测出来并赋值，异常时可以用这个兜底
uint8_t        armed       = 0;     // 重要标志，锁定电机输出为0，防止上电/校准/异常时电机转动
Rate_t         exp_w       = {0, 0, 0};


/** 
 * PID历史状态清零（不动整定参数Kp/Ti/Td）
 * 电机停转（未解锁/油门低于THR_MIN/倾角超限）时清零用。
 * 防止停在地面时，起飞遇到积分饱和。
 */
static void PID_Reset_State(PID_t *p)
{
    p->eK     = 0.0f;
    p->eK_1   = 0.0f;
    p->eSum   = 0.0f;
    p->output = 0.0f;
}

static void PID_Reset_Shell(void)
{
    PID_Reset_State(&roll_shell);  
    PID_Reset_State(&pitch_shell); 

}

static void PID_Reset_Core(void)
{
    PID_Reset_State(&roll_core);
    PID_Reset_State(&pitch_core);
    PID_Reset_State(&yaw_core);
}


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
    exp_w.roll        = 0.0f;
    exp_w.pitch       = 0.0f;
    exp_w.yaw         = 0.0f;

    /* 3. 初始化飞行模式为 STOP */
    fly_mode = STOP;

    /* 4. 初始化电机输出为怠速 */
    motor_out.motor1 = PWM_IDLE;
    motor_out.motor2 = PWM_IDLE;
    motor_out.motor3 = PWM_IDLE;
    motor_out.motor4 = PWM_IDLE;

    /* 5. 初始化解锁标志为 0 */
    armed = 0;

    PID_Reset_Shell();
    PID_Reset_Core();

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
    uint32_t now_time = 0;

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
     * 利用归一化将PPM值映射到期望值（°或者°/s）
     */

    uint16_t ppm_roll, ppm_pitch, ppm_yaw, ppm_thr;

    /* 1. 读取 PPM_Values（关中断保护，防止读的时候摇杆继续动传来中断打断） */
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
    float exp_yaw = PID_To1(ppm_yaw, 1000, 2000, -1, 1);
    //float exp_thr = PID_To1(ppm_thr, 1000, 2000, 0, 1);     // 油门暂时不用归一化
    float exp_thr = ppm_thr;

    expectation.roll = PID_To1(exp_roll, -1, 1, -EXP_ROLL_MAX, EXP_ROLL_MAX);
    expectation.pitch = PID_To1(exp_pitch, -1, 1, -EXP_PITCH_MAX, EXP_PITCH_MAX);
    expectation.yaw = PID_To1(exp_yaw, -1, 1, -EXP_YAW_RATE_MAX, EXP_YAW_RATE_MAX);
    expectation.thr = exp_thr;
    /* 这里用了归一化就不用再限幅了 */

}


/* ============================================================================
 *  PID 计算
 * ========================================================================== */

void PID_Cascade_Shell_Calc_Quater(void){
    /**
     * 四元数PID外环计算
     * 这里主要是利用四元数表示出误差，然后输给外环计算函数
     */
    Quaternion_t exp_q, conj_q, err_q;
    EulerAngle_t exp_e;
    float err_roll, err_pitch;      // 误差旋转向量的分量

    /* 1. 计算目标姿态并转四元数 */
    exp_e.roll  = expectation.roll  * DEG_TO_RAD;
    exp_e.pitch = expectation.pitch * DEG_TO_RAD;
    exp_e.yaw   = euler_angle.yaw;
    IMU_Euler_To_Quaternion(&exp_e, &exp_q);

    /* 2. 计算误差四元数 */
    /**
     * 公式：err_q =  conj(cur_q) ⊗ exp_q
     */
    Quaternion_t cur_q = quaternion;
    PID_Quater_to1(&cur_q);     // 当前姿态四元数归一化
    PID_Quater_Conj(&cur_q, &conj_q);     // 算当前姿态四元数共轭
    PID_Quater_Mul(&conj_q, &exp_q, &err_q);    // 四元数乘法

    /* 3. 最短路径：w<0 就整体取反，否则会绕 360° */
    if (err_q.w < 0.0f) {
        err_q.w = -err_q.w;
        err_q.x = -err_q.x;
        err_q.y = -err_q.y;
        err_q.z = -err_q.z;
    }

    /* 4.计算外环输入的误差旋转向量分量  */
    /**
     * 一个单位四元数可以写成q = [cosθ/2, nsinθ/2] = [w, x, y, z]
     * 其中θ为旋转角度，n为旋转轴的单位向量
     * 小角度时，可以近似:nθ = 2q_v
     * 左边定义成误差旋转向量，那么误差旋转向量在三个轴上的分量就是：r_x = 2q.x
     * 拿这个误差旋转向量分量去算PID.
     */
    err_roll  = 2.0f * err_q.x;
    err_pitch = 2.0f * err_q.y;

    /* 5. 给外环PID计算函数算输出 */
    /*  注意这个旋转向量的三个分量数值上都是弧度，这里需要转化成角度. */
    exp_w.roll  = PID_Cascade_Shell_Calc(err_roll * RAD_TO_DEG, &roll_shell);
    exp_w.pitch = PID_Cascade_Shell_Calc(err_pitch * RAD_TO_DEG, &pitch_shell);
    exp_w.yaw   = expectation.yaw;

}

float PID_Cascade_Shell_Calc(float shell_err, PID_t *shell){

    /**
     * 外环PID计算（Roll/Pitch）
     *
     * 公式：
     *   外环：ω_exp = Kp_shell · err
     *
     * 
     */

    float Kp_shell = shell->Kp;

    /* 1. 计算外环输入误差 err = shell_err */
    shell->eK = shell_err;
    /* 2. 外环纯P输出目标角速度 = Kp*err */
    shell->output = Kp_shell * shell->eK;
    shell->eK_1 = shell->eK;         // 把这次的记成上一次的误差

    return shell->output;

}

float PID_Cascade_Core_Calc(float exp_w, float core_w_last, PID_t *core){

    /**
     * 内环PID计算（Roll/Pitch）
     *
     * 公式：
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

    float Kp_core = core->Kp;
    float Ki_core = (core->Ti==0) ?  0.0f : (dt / core->Ti);
    float Kd_core = core->Td / dt;

    /* 1. 计算内环误差 eK = 外环输出 - core_w_last */
    core->eK = exp_w - core_w_last;
    /* 2. 积分项处理 */
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

    /* 3. 内环PID输出 = Kp*(e + Ki*eSum + Kd*(e-eK_1)) */
    core->output = Kp_core * (core->eK 
                        + Ki_core * core->eSum
                        + Kd_core * (core->eK - core->eK_1) );
    core->eK_1 = core->eK; 

    /* 4. 输出限幅 */
    core->output = PID_Limit(core->output, PID_OUT_MIN, PID_OUT_MAX);

    return core->output;

}


// float PID_Cascade_Calc(float shell_err, float core_w_last, PID_t *shell, PID_t *core){

//     /**
//      * ！！核心部分！！
//      * 串级PID运算（Roll/Pitch）
//      * 
//      * shell:外环  core:内环
//      * 公式：
//      *   外环：ω_exp = Kp_shell · e_angle
//      *   内环：M     = Kp_core · ( e_w + Ki·Σe_w + Kd·Δe_w )
//      *   其中  Ki = Δt / Ti     （位置式积分，Δt 由 TIM4 实测）
//      *         Kd = Td / Δt     （差分做微分）
//      *   单位：e_angle 为度，e_rate 为度/秒，M 为 PWM 增量。
//      * 
//      * 补充说明：查阅资料得到，PID公式可以写成M = Kp_core · ( e_w + Ki·Σe_w + Kd·Δe_w )
//      *          其中Ki = dt/Ti, Kd = Td/dt
//      *          和原本的u = Kp·e + Ki·dt·Σe + Kd·Δe/dt相比，
//      *          这样做的优点在于：和Kp解耦，Ti/Td形式里改Kp不影响积分时间和微分时间，三个参数可以独立整定。
//      *          且采样周期Δt变了不用重调参。
//      */

//     float Kp_shell = shell->Kp;
//     float Kp_core = core->Kp;
//     float Ki_core = (core->Ti==0) ?  0.0f : (dt / core->Ti);
//     float Kd_core = core->Td / dt;

//     /* 1. 计算外环角度误差 e_angle = shell_err */
//     shell->eK = shell_err;
//     /* 2. 外环纯P输出目标角速度 = Kp*e_angle */
//     shell->output = Kp_shell * shell->eK;
//     shell->eK_1 = shell->eK;         // 把这次的记成上一次的误差

//     /* 3. 计算内环误差 eK = 外环输出 - core_w_last */
//     core->eK = shell->output - core_w_last;
//     /* 4. 积分项处理 */
//     /* 此处会处理积分饱和问题，暂时用积分分离，后续待尝试其他方法。 */
//     /* a.积分分离：误差绝对值很大时先不积分，误差回到CORE_INT_SEP以内再积分 */
//     if ((Ki_core > 0.0f) && (fabsf(core->eK) <= CORE_INT_SEP)){
//         core->eSum += core->eK;
//         /* b.积分限幅：限制积分项Ki·Σe在CORE_INT_MAX以内 */
//         if ((Ki_core * core->eSum) > CORE_INT_MAX) {
//             core->eSum = CORE_INT_MAX / Ki_core;
//         }else if ((Ki_core * core->eSum) < -CORE_INT_MAX) {
//             core->eSum = - CORE_INT_MAX / Ki_core;
//         }
//     }

//     /* 5. 内环PID输出 = Kp*(e + Ki*eSum + Kd*(e-eK_1)) */
//     core->output = Kp_core * (core->eK 
//                         + Ki_core * core->eSum
//                         + Kd_core * (core->eK - core->eK_1) );
//     core->eK_1 = core->eK; 

//     /* 6. 输出限幅 */
//     core->output = PID_Limit(core->output, PID_OUT_MIN, PID_OUT_MAX);

//     return core->output;

// }

float PID_Single_Calc(float err, PID_t *pid){
    /**
     * 单环PID计算（Yaw）
     * @todo:其实Yaw也该用串级。后续会修改。
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


/* ============================================================================
 *  电机输出相关
 * ========================================================================== */

void PID_Motor_Calc(void){
    /**
     * 电机混控计算
     * 将PID输出映射到4路电机PWM。
     * 公式： motor1 = thr - pitch + roll - yaw
     *       motor2 = thr - pitch - roll + yaw
     *       motor3 = thr + pitch + roll + yaw
     *       motor4 = thr + pitch - roll - yaw
     */

    /* 这里只在内环用I和D，所以只在内环用dt。如果外环也引入ID，那需要维护多个dt */
    dt = PID_Get_Time();
    if ((dt <= 0.0005f) || (dt > PID_DT_MAX)){
        /* 首次调用、被高优先级任务打断很久、或TIM4计数器回绕时dt很大，
         * 这时退回标称，否则Ki=dt/Ti会瞬间变得极大 */
        dt = PID_DT_NOMINAL;
    }
    
    /* 1. 换算 */
    /**
     * 轴对应关系（与 imu.c 的四元数/欧拉角约定一致）：
     *  imu.c 里 R_q = C_b^e，欧拉角按 Z-Y-X 外旋提取：
     *     roll  φ 绕机体系 +x 轴
     *     pitch θ 绕机体系 +y 轴
     *     yaw   ψ 绕机体系 +z 轴
     *   因此角速度反馈也必须是同一组轴：
     *     φ̇ ≈ ω_x = gyro_body.x
     *     θ̇ ≈ ω_y = gyro_body.y
     *     ψ̇ ≈ ω_z = gyro_body.z
     *   （小角度下三个欧拉角速度就近似等于机体角速度的三个分量。）
     * 实机若发现某个轴"越修正越偏"，就是这里的符号取反了，改一行即可。
     * @todo:此处有严重问题。待修改为四元数。
     */
    // float roll_deg = euler_angle.roll * RAD_TO_DEG;
    // float pitch_deg = euler_angle.pitch * RAD_TO_DEG;
    // float yaw_deg = euler_angle.yaw * RAD_TO_DEG;
    float roll_rate = gyro_body.x * RAD_TO_DEG;
    float pitch_rate = gyro_body.y * RAD_TO_DEG;
    float yaw_rate = gyro_body.z * RAD_TO_DEG;

    /* 2. 计算 Roll/Pitch/Yaw PID 内环输出 */
    float M_roll = PID_Cascade_Core_Calc(exp_w.roll, roll_rate, &roll_core);
    float M_pitch = PID_Cascade_Core_Calc(exp_w.pitch, pitch_rate, &pitch_core);

    float e_yaw_rate = expectation.yaw - yaw_rate;
    float M_yaw = PID_Single_Calc(e_yaw_rate, &yaw_core);

    /* 3. 按混控公式计算4路电机 */
    /**
     * 电机布局：1=左前  2=右前  3=左后  4=右后
     * 符号前提：
     *   roll > 0  = 左侧抬起 / 右侧下沉
     *   pitch > 0 = 机头下沉（低头）
     *   yaw  > 0  = 从机顶俯视逆时针（机头左转）
     * 于是各轴 PID 输出的物理含义是：
     *   pid_pitch > 0 → 前电机减速、后电机加速 → 机头下沉
     *   pid_roll  > 0 → 左电机加速、右电机减速 → 左侧抬起
     *   pid_yaw   > 0 → 1、4 减速、2、3 加速 → 对角差动
     * 注意：yaw 的正负与电机转向（正反桨）有关，首飞前务必核对螺旋桨转向。
     * @todo:研究具体分配
     */
    motor_out.motor1 = PID_Limit(expectation.thr - M_pitch + M_roll - M_yaw,
                                 PWM_OUT_MIN, PWM_OUT_MAX);
    motor_out.motor2 = PID_Limit(expectation.thr - M_pitch - M_roll + M_yaw,
                                 PWM_OUT_MIN, PWM_OUT_MAX);
    motor_out.motor3 = PID_Limit(expectation.thr + M_pitch + M_roll + M_yaw,
                                 PWM_OUT_MIN, PWM_OUT_MAX);
    motor_out.motor4 = PID_Limit(expectation.thr + M_pitch - M_roll - M_yaw,
                                 PWM_OUT_MIN, PWM_OUT_MAX);

    /* 4. 安全保护检查（未解锁/倾角超限时会把上面算出的值全部改回怠速） */
    PID_Safety_Check();

}

void PID_Motor_Output(void){
    /**
     * 电机PWM输出，将motor_out写入TIM3的CCR寄存器。
     * 使用bsp.c中的Motor_SetSpeed
     * 1. motor1 → TIM3_CH1 (PA6) 
     * 2. motor2 → TIM3_CH2 (PA7) 
     * 3. motor3 → TIM3_CH3 (PB0) 
     * 4. motor4 → TIM3_CH4 (PB1)
     */

    Motor_SetSpeed(&htim3, TIM_CHANNEL_1, (uint16_t)motor_out.motor1);
    Motor_SetSpeed(&htim3, TIM_CHANNEL_2, (uint16_t)motor_out.motor2);
    Motor_SetSpeed(&htim3, TIM_CHANNEL_3, (uint16_t)motor_out.motor3);
    Motor_SetSpeed(&htim3, TIM_CHANNEL_4, (uint16_t)motor_out.motor4);

}

void PID_Control_Update(void){
    /* 串级 PID 唯一对外入口 */
    /* 每个计算PID的周期，都按序调用一次 */
    PID_PPM_to_Exp_Calc();      // 遥控器期望值计算
    PID_FlyMode_Update(expectation.thr);    // 更新飞行模式
    PID_Motor_Calc();           // 电机混控输出计算
    PID_Motor_Output();         // 电机输出配置

}


/* ============================================================================
 *  飞行状态相关
 * ========================================================================== */

void PID_Safety_Check(void){
    /**
     * 安全保护检查
     * 条件：thr < THR_MIN : 停转 
     *      |roll| > ANGLE_LIMIT_DEG : 停转 
     *      |pitch| > ANGLE_LIMIT_DEG : 停转 
     *      信号丢失 : 停转
     */

     /* 姿态校准完成前禁止输出油门，防止校准过程中误转电机 */
    if (!IMU_Calib_Done())
    {
        motor_out.motor1 = PWM_IDLE;
        motor_out.motor2 = PWM_IDLE;
        motor_out.motor3 = PWM_IDLE;
        motor_out.motor4 = PWM_IDLE;
        PID_Reset_Core();
        return;
    }

    /**
     * @TODO:这个地方需要仔细想一想，油门拨杆拨到底时，是否直接让它停转？
     *      这种设计思路的目的是：开机但没起飞时油门摇杆在最底部，此时要保持不起飞
     *                         接收机失联时PPM捕获不到，期望值为0会直接停机，防止乱飞
     *                         落地时收油，armed归零，积分清零
     *      问题是在空中误触时风险非常大，需要把油门推回重新解锁的门槛，瞬间加速比较危险
     *      目前初步想的两种改进方法：
     *          1.独立解锁开关，用CH5拨杆控制armed，油门只负责推力。
     *            油门见底只是推力最小，不会停机。但需要接收机多一个通道改状态机
     *          2.保留一个怠速下限，最低油门时仍有输出。但是需要额外判断它是否在空中，因为在地面时不能让它飞
     */
    if (expectation.thr < THR_MIN) {    // 摇杆在底部附近停转
        motor_out.motor1 = PWM_IDLE;
        motor_out.motor2 = PWM_IDLE;
        motor_out.motor3 = PWM_IDLE;
        motor_out.motor4 = PWM_IDLE;
        armed = 0;
        PID_Reset_Core();
        return;
    }

    // /* 这里是倾斜角超过最大限幅后紧急停机，前期调试的时候感觉可以不用。 */
    // float roll_deg = euler_angle.roll * RAD_TO_DEG;
    // float pitch_deg = euler_angle.pitch * RAD_TO_DEG;
    // if (fabsf(roll_deg) > ANGLE_LIMIT_DEG || fabsf(pitch_deg) > ANGLE_LIMIT_DEG) {
    //     motor_out.motor1 = PWM_IDLE;
    //     motor_out.motor2 = PWM_IDLE;
    //     motor_out.motor3 = PWM_IDLE;
    //     motor_out.motor4 = PWM_IDLE;
    //     armed = 0;
    //     PID_Reset_All();
    //     return;
    // }

    if (!armed) {           // arm标志为0未解锁，电机不转
        motor_out.motor1 = PWM_IDLE;
        motor_out.motor2 = PWM_IDLE;
        motor_out.motor3 = PWM_IDLE;
        motor_out.motor4 = PWM_IDLE;
        PID_Reset_Core();    // 解锁前始终保持积分器为0 
    }

}

void PID_FlyMode_Update(float exp_thr){
    /**
     * 飞行模式状态机
     * 四种状态：STOP, HOVER, UP, DOWN
     * 状态转换：
     *   STOP → HOVER: exp_thr > THR_ARM_START
     *   HOVER → UP: exp_thr > 1650
     *   HOVER → DOWN: exp_thr < THR_ARM_START
     *   DOWN → STOP: exp_thr < THR_DISARM_FINI
     *   DOWN → HOVER: exp_thr > THR_ARM_START
     *   UP → HOVER: exp_thr < 1650
     */

    /** 
     * 为了安全，加了一个限制条件：上电后如果油门有幅度，是不允许启动的，armed=1锁定。 
     * 只有油门推到底再启动才能解锁。
     */ 
    static uint8_t thr_lowest_signal = 0;   /* 上电后必须先收到底，才允许解锁 */
    if (exp_thr < THR_MIN)  thr_lowest_signal = 1;
    switch(fly_mode){
        case STOP:
            if(thr_lowest_signal && exp_thr >= THR_ARM_START){
                fly_mode = HOVER;
                armed = 1;
            }
            break;

        case HOVER:
            if(exp_thr > THR_HOVER_to_UP){
                fly_mode = UP;
            }
            if(exp_thr < THR_HOVER_to_DOWN){
                fly_mode = DOWN;
            }
            break;

        case UP:
            if(exp_thr <= THR_HOVER_to_UP){
                fly_mode = HOVER;
            }
            break;

        case DOWN:
            if(exp_thr >= THR_HOVER_to_DOWN){
                fly_mode = HOVER;
            }
            if(exp_thr < THR_DISARM_FINI){
                fly_mode = STOP;
                armed = 0;
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

float PID_Limit(float value, float min, float max){
    /* 限制value的输出范围:[min, max] */
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float PID_To1(float value, float min_in, float max_in, float min_out, float max_out){
    /* 归一化 */
    return ((value-min_in)/(max_in-min_in)) * ((max_out-min_out)) + min_out;
}

void PID_Quater_Mul(Quaternion_t *a, Quaternion_t *b, Quaternion_t *c){
    /* 四元数乘法运算 */
    float w = a->w*b->w - a->x*b->x - a->y*b->y - a->z*b->z;
    float x = a->w*b->x + a->x*b->w + a->y*b->z - a->z*b->y;
    float y = a->w*b->y - a->x*b->z + a->y*b->w + a->z*b->x;
    float z = a->w*b->z + a->x*b->y - a->y*b->x + a->z*b->w;
    c->w = w; c->x = x; c->y = y; c->z = z;
}

void PID_Quater_to1(Quaternion_t *q){
    /* 四元数归一化 */
    float norm = sqrtf(q->w * q->w + q->x * q->x + q->y * q->y + q->z * q->z);
    if (norm < 1e-6f) { q->w=1; q->x=q->y=q->z=0; return; }
    q->w /= norm;
    q->x /= norm;
    q->y /= norm;
    q->z /= norm;
}

void PID_Quater_Conj(Quaternion_t *q, Quaternion_t *conj){
    /* 四元数共轭运算 */
    conj->w = q->w;
    conj->x = -q->x;
    conj->y = -q->y;
    conj->z = -q->z;
}
