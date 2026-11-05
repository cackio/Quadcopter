/**
 * @file    ano.c
 * @brief   匿名上位机（AnoTC）通信协议模块
 *
 *  协议格式：
 *    帧头(0xAA) + 地址(0xFF) + 功能码 + 数据长度 + 数据区 + SUM校验 + ADD校验
 *
 *  校验算法：
 *    SUM  = 从帧头到数据区结束，逐字节累加，只取低 8 位
 *    ADD  = 每累加一字节，同时对 SUM 累加值再累加，只取低 8 位
 *
 *  数据格式：小端模式（多字节数据低位在前）
 *
 *  依赖：
 *    - ano.h       (本模块头文件)
 *    - usart.h     (huart1)
 *    - bsp.h       (传感器原始数据、PPM_Values)
 *    - imu.h       (euler_angle, gyro_body)
 *    - pid.h       (PID 参数结构体)
 *    - altitude.h  (altitude 结构体)
 *    - ucos_ii.h   (OS_ENTER_CRITICAL 保护共享变量)
 * ========================================================================== */

#include "ano.h"
#include "usart.h"
#include "bsp.h"
#include "imu.h"
#include "pid.h"
#include "altitude.h"
#include "ucos_ii.h"
#include <string.h>
#include <math.h>

/* ============================================================================
 *  发送缓冲区（静态，避免栈溢出）
 * ========================================================================== */

static uint8_t ano_tx_buf[96];    /* 发送帧缓冲（参数应答最长 12×6=72 字节 + 6 字节帧头） */

/* ============================================================================
 *  接收状态机
 * ========================================================================== */

#define ANO_RX_BUF_SIZE  32
static uint8_t ano_rx_buf[ANO_RX_BUF_SIZE];
static uint8_t ano_rx_idx = 0;
static uint8_t ano_rx_func = 0;
static uint8_t ano_rx_len = 0;

/* 自检用：已收到的“校验通过”的帧数（含上位机下发的 E0/E1/E2）。
 * 串口日志里打印它，可判断板子到底有没有收到上位机指令：
 *   - 一直为 0  → 板子没收到（RX 线没接 / 波特率不对 / 中断没进）
 *   - 在增长但上位机仍超时 → 收到了但应答没发出去/格式不被接受
 */
volatile uint16_t ano_rx_frames = 0;

/* 上位机请求标志：在 USART1 接收中断里置位，在 UartTask 里执行应答。
 * 中断里绝对不能做阻塞式串口发送（原因见 Ano_Rx_Parse 中的说明）。 */
volatile uint8_t ano_req_info = 0;   /* 收到 0xE0：读设备信息 */
volatile uint8_t ano_req_read = 0;   /* 收到 0xE1：读参数   */

/* ============================================================================
 *  初始化
 * ========================================================================== */

void Ano_Init(void)
{
    ano_rx_idx = 0;
    ano_rx_func = 0;
    ano_rx_len = 0;

    /* 开启 USART1 接收中断（单字节） */
    HAL_UART_Receive_IT(&huart1, &ano_rx_byte, 1);
}

/* ============================================================================
 *  发送一帧
 * ========================================================================== */

static void Ano_Calc_Check(uint8_t *buf, uint8_t data_len)
{
    uint8_t i;
    uint8_t sum = 0, add = 0;

    for (i = 0; i < data_len + 4; i++) {
        sum += buf[i];
        add += sum;
    }
    buf[data_len + 4] = sum;
    buf[data_len + 5] = add;
}

void Ano_Send_Frame(uint8_t func, const uint8_t *data, uint8_t len)
{
    uint8_t i = 0;

    if (len + 6 > sizeof(ano_tx_buf)) {
        return;    /* 数据过长，丢弃 */
    }

    ano_tx_buf[i++] = ANO_FRAME_HEADER;
    ano_tx_buf[i++] = ANO_ADDR_DEST;
    ano_tx_buf[i++] = func;
    ano_tx_buf[i++] = len;

    for (uint8_t k = 0; k < len; k++) {
        ano_tx_buf[i++] = data[k];
    }

    Ano_Calc_Check(ano_tx_buf, len);

    /* 阻塞发送，确保每帧完整发出（连续发送多帧时不会因 UART 忙而丢帧） */
    HAL_UART_Transmit(&huart1, ano_tx_buf, len + 6, 100);
}

/* ============================================================================
 *  发送各数据帧
 * ========================================================================== */

void Ano_Send_Attitude(void)
{
    uint8_t data[16];
    uint8_t i = 0;
    int16_t roll  = (int16_t)(euler_angle.roll  * RAD_TO_DEG * 100.0f);
    int16_t pitch = (int16_t)(euler_angle.pitch * RAD_TO_DEG * 100.0f);
    int16_t yaw   = (int16_t)(euler_angle.yaw   * RAD_TO_DEG * 100.0f);
    int16_t gx = (int16_t)(gyro_body.x * RAD_TO_DEG * 10.0f);
    int16_t gy = (int16_t)(gyro_body.y * RAD_TO_DEG * 10.0f);
    int16_t gz = (int16_t)(gyro_body.z * RAD_TO_DEG * 10.0f);

    data[i++] = ANO_BYTE0(roll);
    data[i++] = ANO_BYTE1(roll);
    data[i++] = ANO_BYTE0(pitch);
    data[i++] = ANO_BYTE1(pitch);
    data[i++] = ANO_BYTE0(yaw);
    data[i++] = ANO_BYTE1(yaw);
    data[i++] = ANO_BYTE0(gx);
    data[i++] = ANO_BYTE1(gx);
    data[i++] = ANO_BYTE0(gy);
    data[i++] = ANO_BYTE1(gy);
    data[i++] = ANO_BYTE0(gz);
    data[i++] = ANO_BYTE1(gz);

    Ano_Send_Frame(ANO_FUNC_ATTITUDE, data, i);
}

void Ano_Send_Sensor_Raw(void)
{
    uint8_t data[18];
    uint8_t i = 0;

    data[i++] = ANO_BYTE0(Accel_X_RAW);  data[i++] = ANO_BYTE1(Accel_X_RAW);
    data[i++] = ANO_BYTE0(Accel_Y_RAW);  data[i++] = ANO_BYTE1(Accel_Y_RAW);
    data[i++] = ANO_BYTE0(Accel_Z_RAW);  data[i++] = ANO_BYTE1(Accel_Z_RAW);
    data[i++] = ANO_BYTE0(Gyro_X_RAW);   data[i++] = ANO_BYTE1(Gyro_X_RAW);
    data[i++] = ANO_BYTE0(Gyro_Y_RAW);   data[i++] = ANO_BYTE1(Gyro_Y_RAW);
    data[i++] = ANO_BYTE0(Gyro_Z_RAW);   data[i++] = ANO_BYTE1(Gyro_Z_RAW);
    data[i++] = ANO_BYTE0(Mag_X_RAW);    data[i++] = ANO_BYTE1(Mag_X_RAW);
    data[i++] = ANO_BYTE0(Mag_Y_RAW);    data[i++] = ANO_BYTE1(Mag_Y_RAW);
    data[i++] = ANO_BYTE0(Mag_Z_RAW);    data[i++] = ANO_BYTE1(Mag_Z_RAW);

    Ano_Send_Frame(ANO_FUNC_SENSOR, data, i);
}

void Ano_Send_Altitude(void)
{
    uint8_t data[16];
    uint8_t i = 0;
    int32_t height = (int32_t)(altitude.height * 100.0f);
    int32_t velocity = (int32_t)(altitude.velocity * 100.0f);
    int32_t pressure = (int32_t)altitude.pressure;
    int32_t temp = (int32_t)(altitude.temperature * 10.0f);

    data[i++] = ANO_BYTE0(height);
    data[i++] = ANO_BYTE1(height);
    data[i++] = ANO_BYTE2(height);
    data[i++] = ANO_BYTE3(height);

    data[i++] = ANO_BYTE0(velocity);
    data[i++] = ANO_BYTE1(velocity);
    data[i++] = ANO_BYTE2(velocity);
    data[i++] = ANO_BYTE3(velocity);

    data[i++] = ANO_BYTE0(pressure);
    data[i++] = ANO_BYTE1(pressure);
    data[i++] = ANO_BYTE2(pressure);
    data[i++] = ANO_BYTE3(pressure);

    data[i++] = ANO_BYTE0(temp);
    data[i++] = ANO_BYTE1(temp);
    data[i++] = ANO_BYTE2(temp);
    data[i++] = ANO_BYTE3(temp);

    Ano_Send_Frame(ANO_FUNC_ALTITUDE, data, i);
}

void Ano_Send_Pid_Params(void)
{
    uint8_t data[24];
    uint8_t i = 0;

    /* 12 个 PID 参数，每个 int16（缩放 ×100） */
    int16_t p[12];
    p[0] = (int16_t)(roll_shell.Kp  * 100.0f);
    p[1] = (int16_t)(roll_core.Kp   * 100.0f);
    p[2] = (int16_t)(roll_core.Ti   * 100.0f);
    p[3] = (int16_t)(roll_core.Td   * 100.0f);
    p[4] = (int16_t)(pitch_shell.Kp * 100.0f);
    p[5] = (int16_t)(pitch_core.Kp  * 100.0f);
    p[6] = (int16_t)(pitch_core.Ti  * 100.0f);
    p[7] = (int16_t)(pitch_core.Td  * 100.0f);
    p[8] = (int16_t)(yaw_core.Kp    * 100.0f);
    p[9] = (int16_t)(yaw_core.Td    * 100.0f);
    p[10] = (int16_t)(thr_shell.Kp  * 100.0f);
    p[11] = (int16_t)(thr_shell.Td  * 100.0f);

    for (uint8_t k = 0; k < 12; k++) {
        data[i++] = ANO_BYTE0(p[k]);
        data[i++] = ANO_BYTE1(p[k]);
    }

    Ano_Send_Frame(ANO_FUNC_PID, data, i);
}

void Ano_Send_Motor_Out(void)
{
    uint8_t data[8];
    uint8_t i = 0;
    int16_t m1 = (int16_t)motor_out.motor1;
    int16_t m2 = (int16_t)motor_out.motor2;
    int16_t m3 = (int16_t)motor_out.motor3;
    int16_t m4 = (int16_t)motor_out.motor4;

    data[i++] = ANO_BYTE0(m1); data[i++] = ANO_BYTE1(m1);
    data[i++] = ANO_BYTE0(m2); data[i++] = ANO_BYTE1(m2);
    data[i++] = ANO_BYTE0(m3); data[i++] = ANO_BYTE1(m3);
    data[i++] = ANO_BYTE0(m4); data[i++] = ANO_BYTE1(m4);

    Ano_Send_Frame(ANO_FUNC_MOTOR, data, i);
}

void Ano_Send_Status(void)
{
    uint8_t data[20];
    uint8_t i = 0;
    uint16_t ppm[4];
    OS_CPU_SR cpu_sr = 0;

    OS_ENTER_CRITICAL();
    ppm[0] = PPM_Values[PPM_CH_ROLL];
    ppm[1] = PPM_Values[PPM_CH_PITCH];
    ppm[2] = PPM_Values[PPM_CH_THR];
    ppm[3] = PPM_Values[PPM_CH_YAW];
    OS_EXIT_CRITICAL();

    data[i++] = (uint8_t)fly_mode;      /* 飞行模式 */
    data[i++] = armed;                   /* 解锁标志 */

    for (uint8_t k = 0; k < 4; k++) {
        data[i++] = ANO_BYTE0(ppm[k]);
        data[i++] = ANO_BYTE1(ppm[k]);
    }

    Ano_Send_Frame(ANO_FUNC_STATUS, data, i);
}

void Ano_Send_All(void)
{
    Ano_Send_Attitude();
    Ano_Send_Sensor_Raw();
    Ano_Send_Altitude();
    Ano_Send_Motor_Out();
    Ano_Send_Status();
}

/* ============================================================================
 *  接收解析（状态机）
 * ========================================================================== */

static uint8_t Ano_Check_Frame(const uint8_t *buf, uint8_t data_len)
{
    uint8_t i, sum = 0, add = 0;

    for (i = 0; i < data_len + 4; i++) {
        sum += buf[i];
        add += sum;
    }

    return (sum == buf[data_len + 4]) && (add == buf[data_len + 5]);
}

void Ano_Rx_Parse(uint8_t byte)
{
    /* 状态 0：等待帧头 */
    if (ano_rx_idx == 0) {
        if (byte == ANO_FRAME_HEADER) {
            ano_rx_buf[ano_rx_idx++] = byte;
        }
        return;
    }

    ano_rx_buf[ano_rx_idx++] = byte;

    /* 刚收到第 4 个字节（数据长度） */
    if (ano_rx_idx == 4) {
        ano_rx_len = ano_rx_buf[3];
        ano_rx_func = ano_rx_buf[2];

        if (ano_rx_len > ANO_RX_BUF_SIZE - 6) {
            ano_rx_idx = 0;    /* 长度异常，丢弃 */
        }
        return;
    }

    /* 收够一整帧 */
    if (ano_rx_idx >= 4 && ano_rx_idx >= ano_rx_len + 6) {
        if (Ano_Check_Frame(ano_rx_buf, ano_rx_len)) {
            ano_rx_frames++;

            /* 根据功能码分发。
             *
             * ★ 本函数运行在 USART1 接收中断里，绝不能在这里调用
             *   HAL_UART_Transmit（阻塞发送）：
             *     1) UartTask 也在用同一个 huart1 发姿态帧，中断里再发就会
             *        抢 huart1.Lock，HAL 直接返回 HAL_BUSY，应答帧被丢掉，
             *        上位机表现为“等待校验数据超时 / 参数读取失败”；
             *     2) 阻塞发送期间所有中断被挂起，SysTick 不计数，HAL 的
             *        超时判断也失效，容易把整个系统卡死。
             *   所以这里只置“请求标志”，真正的应答交给 UartTask 里的
             *   Ano_Process_Requests() 去做。
             */
            switch (ano_rx_func) {
            case ANO_FUNC_GET_INFO:      /* 0xE0 读设备信息 */
                ano_req_info = 1;
                break;
            case ANO_FUNC_READ_PARA:     /* 0xE1 读参数     */
                ano_req_read = 1;
                break;
            case ANO_FUNC_WRITE_PARA:    /* 0xE2 写参数：只改内存变量，不发送 */
                Ano_Handle_Write_Params(&ano_rx_buf[4], ano_rx_len);
                break;
            default:
                break;
            }
        }
        ano_rx_idx = 0;    /* 复位，等下一帧 */
    }
}

/* ============================================================================
 *  参数读写
 * ========================================================================== */

int32_t Ano_Get_Param(uint8_t id)
{
    float v = 0.0f;

    switch (id) {
    case PARAM_ROLL_SHELL_KP:   v = roll_shell.Kp;  break;
    case PARAM_ROLL_CORE_KP:    v = roll_core.Kp;   break;
    case PARAM_ROLL_CORE_TI:    v = roll_core.Ti;   break;
    case PARAM_ROLL_CORE_TD:    v = roll_core.Td;   break;
    case PARAM_PITCH_SHELL_KP:  v = pitch_shell.Kp; break;
    case PARAM_PITCH_CORE_KP:   v = pitch_core.Kp;  break;
    case PARAM_PITCH_CORE_TI:   v = pitch_core.Ti;  break;
    case PARAM_PITCH_CORE_TD:   v = pitch_core.Td;  break;
    case PARAM_YAW_CORE_KP:     v = yaw_core.Kp;    break;
    case PARAM_YAW_CORE_TD:     v = yaw_core.Td;    break;
    case PARAM_THR_SHELL_KP:    v = thr_shell.Kp;   break;
    case PARAM_THR_SHELL_TD:    v = thr_shell.Td;   break;
    default:
        return 0;
    }

    return (int32_t)(v * ANO_SCALE_FLOAT);
}

void Ano_Set_Param(uint8_t id, int32_t value)
{
    float v = (float)value / ANO_SCALE_FLOAT;

    switch (id) {
    case PARAM_ROLL_SHELL_KP:   roll_shell.Kp  = v; break;
    case PARAM_ROLL_CORE_KP:    roll_core.Kp   = v; break;
    case PARAM_ROLL_CORE_TI:    roll_core.Ti   = v; break;
    case PARAM_ROLL_CORE_TD:    roll_core.Td   = v; break;
    case PARAM_PITCH_SHELL_KP:  pitch_shell.Kp = v; break;
    case PARAM_PITCH_CORE_KP:   pitch_core.Kp  = v; break;
    case PARAM_PITCH_CORE_TI:   pitch_core.Ti  = v; break;
    case PARAM_PITCH_CORE_TD:   pitch_core.Td  = v; break;
    case PARAM_YAW_CORE_KP:     yaw_core.Kp    = v; break;
    case PARAM_YAW_CORE_TD:     yaw_core.Td    = v; break;
    case PARAM_THR_SHELL_KP:    thr_shell.Kp   = v; break;
    case PARAM_THR_SHELL_TD:    thr_shell.Td   = v; break;
    default:
        break;
    }
}

void Ano_Handle_Read_Params(void)
{
    /* 应答协议（与上位机“参数设置/读取参数”一致）：
     *   功能码必须是 0xE1（读参数应答），不能回 0xE2，
     *   否则上位机认为不是它要的帧，一直重试直到“等待校验数据超时”。
     *   数据区每组 6 字节：参数ID(uint16) + 参数值(int32)，小端。
     */
    uint8_t data[12 * 6];
    uint8_t i = 0;

    for (uint8_t k = 1; k <= 12; k++) {
        int16_t id  = (int16_t)k;
        int32_t val = Ano_Get_Param(k);

        data[i++] = ANO_BYTE0(id);
        data[i++] = ANO_BYTE1(id);
        data[i++] = ANO_BYTE0(val);
        data[i++] = ANO_BYTE1(val);
        data[i++] = ANO_BYTE2(val);
        data[i++] = ANO_BYTE3(val);
    }

    Ano_Send_Frame(ANO_FUNC_READ_PARA, data, i);
}

void Ano_Handle_Write_Params(const uint8_t *data, uint8_t len)
{
    uint8_t i = 0;

    /* 数据格式：[参数ID(2字节 int16)] + [参数值(4字节 int32)]，每组 6 字节 */
    while (i + 6 <= len) {
        int16_t pid = (int16_t)(data[i] | (data[i + 1] << 8));
        int32_t val = (int32_t)(data[i + 2] | (data[i + 3] << 8) |
                                (data[i + 4] << 16) | ((int32_t)data[i + 5] << 24));

        if (pid >= 1 && pid <= 12) {
            Ano_Set_Param((uint8_t)pid, val);
        }
        i += 6;
    }
}

void Ano_Handle_Get_Info(void)
{
    uint8_t data[8];
    uint8_t i = 0;

    /* 设备信息：类型(1) + 版本(1) + 传感器配置(2) + 空(4) */
    data[i++] = 0x05;       /* 设备类型：拓空者飞控（借用） */
    data[i++] = 0x01;       /* 版本 */
    data[i++] = 0x00;       /* 传感器配置高字节 */
    data[i++] = 0x00;       /* 传感器配置低字节 */
    data[i++] = 0x00;
    data[i++] = 0x00;
    data[i++] = 0x00;
    data[i++] = 0x00;

    Ano_Send_Frame(ANO_FUNC_GET_INFO, data, i);
}

/* ============================================================================
 *  在任务上下文里处理上位机请求
 * ========================================================================== */

void Ano_Process_Requests(void)
{
    /* UartTask 每个周期调一次；有请求才发应答，没请求时开销几乎为 0。
     * 置标志→清标志的顺序保证：中断在两次判断之间再次来请求也不会丢。 */
    if (ano_req_info) {
        ano_req_info = 0;
        Ano_Handle_Get_Info();
    }
    if (ano_req_read) {
        ano_req_read = 0;
        Ano_Handle_Read_Params();
    }
}
