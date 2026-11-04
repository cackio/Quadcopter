#ifndef __ANO_H
#define __ANO_H

/* ============================================================================
 *  ano.h — 匿名上位机（AnoTC）通信协议模块
 *
 *  协议格式：
 *    帧头(0xAA) + 地址(0xFF) + 功能码 + 数据长度 + 数据区 + SUM校验 + ADD校验
 *
 *  功能码：
 *    0xF1~0xFA : 用户数据帧（灵活格式帧，可自定义内容）
 *    0xE0      : 读设备信息（上位机下发）
 *    0xE1      : 读参数（上位机下发）
 *    0xE2      : 写参数（上位机下发）
 *
 *  依赖：
 *    - main.h   (STM32 HAL 类型)
 *    - usart.h  (huart1)
 *    - bsp.h    (传感器原始数据)
 *    - imu.h    (姿态角、陀螺仪)
 *    - pid.h    (PID 参数)
 *    - altitude.h (高度数据)
 * ========================================================================== */

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  协议常量
 * ========================================================================== */

#define ANO_FRAME_HEADER   0xAA    /* 帧头 */
#define ANO_ADDR_DEST      0xFF    /* 目标地址（广播） */
#define ANO_ADDR_HOST      0xAF    /* 源地址（向上位机输出） */

/* 用户数据帧功能码（0xF1~0xFA） */
#define ANO_FUNC_ATTITUDE  0xF1    /* 姿态数据（roll/pitch/yaw + 陀螺仪） */
#define ANO_FUNC_SENSOR    0xF2    /* 传感器原始数据（9 路 int16） */
#define ANO_FUNC_ALTITUDE  0xF3    /* 高度数据（height/velocity/pressure/temp） */
#define ANO_FUNC_PID       0xF4    /* PID 参数（所有 PID 值） */
#define ANO_FUNC_MOTOR     0xF5    /* 电机输出（4 路） */
#define ANO_FUNC_STATUS    0xF6    /* 飞行状态（模式 + armed + PPM 通道） */

/* 飞控参数功能码（上位机 <-> 下位机） */
#define ANO_FUNC_GET_INFO  0xE0    /* 读设备信息（上位机下发） */
#define ANO_FUNC_READ_PARA 0xE1    /* 读参数（上位机下发） */
#define ANO_FUNC_WRITE_PARA 0xE2   /* 写参数（上位机下发） */

/* ============================================================================
 *  参数 ID（匿名协议参数区，1~12）
 * ========================================================================== */

#define PARAM_ROLL_SHELL_KP   1
#define PARAM_ROLL_CORE_KP    2
#define PARAM_ROLL_CORE_TI    3
#define PARAM_ROLL_CORE_TD    4
#define PARAM_PITCH_SHELL_KP  5
#define PARAM_PITCH_CORE_KP   6
#define PARAM_PITCH_CORE_TI   7
#define PARAM_PITCH_CORE_TD   8
#define PARAM_YAW_CORE_KP     9
#define PARAM_YAW_CORE_TD     10
#define PARAM_THR_SHELL_KP    11
#define PARAM_THR_SHELL_TD    12

#define ANO_SCALE_FLOAT   1000.0f   /* float 参数缩放系数（×1000） */

/* ============================================================================
 *  小端字节拆分宏（匿名协议数据采用小端模式，低位在前）
 * ========================================================================== */

#define ANO_BYTE0(v) (*(char *)(&v))
#define ANO_BYTE1(v) (*((char *)(&v) + 1))
#define ANO_BYTE2(v) (*((char *)(&v) + 2))
#define ANO_BYTE3(v) (*((char *)(&v) + 3))

/* ============================================================================
 *  函数原型
 * ========================================================================== */

/**
 * @brief  匿名协议模块初始化
 *
 * 在 StartTask 中 BSP_Init 之后调用。
 * 初始化发送缓冲区、接收状态机等。
 */
void Ano_Init(void);

/**
 * @brief  发送一帧用户数据
 * @param  func 功能码（0xF1~0xFA）
 * @param  data 数据区指针
 * @param  len  数据长度（字节数）
 */
void Ano_Send_Frame(uint8_t func, const uint8_t *data, uint8_t len);

/* ---- 发送接口（供 UartTask 调用） ---- */

void Ano_Send_Attitude(void);    /* 姿态 + 陀螺仪 */
void Ano_Send_Sensor_Raw(void);  /* 传感器原始数据 */
void Ano_Send_Altitude(void);    /* 高度数据 */
void Ano_Send_Pid_Params(void);  /* PID 参数 */
void Ano_Send_Motor_Out(void);   /* 电机输出 */
void Ano_Send_Status(void);      /* 飞行状态 */

/**
 * @brief  轮流发送所有数据帧（供 UartTask 周期调用）
 */
void Ano_Send_All(void);

extern volatile uint16_t ano_rx_frames;   /* 已收到的有效帧计数（自检用） */

/* ---- 接收解析（在 UART 中断回调中调用） ---- */

/**
 * @brief  逐字节接收解析（状态机）
 * @param  byte 接收到的一个字节
 *
 * 在 HAL_UART_RxCpltCallback 中调用，处理上位机下发的命令。
 */
void Ano_Rx_Parse(uint8_t byte);

/* ---- 参数读写处理 ---- */

/**
 * @brief  处理"读参数"命令（上位机下发 0xE1）
 *         把当前 PID 参数打包成数据帧发回上位机
 */
void Ano_Handle_Read_Params(void);

/**
 * @brief  处理"写参数"命令（上位机下发 0xE2）
 * @param  data 数据区（[参数ID(2字节)] + [参数值(4字节)]，多组）
 * @param  len  数据长度
 *
 * 将新参数值写回 PID 结构体，实现在线调参。
 */
void Ano_Handle_Write_Params(const uint8_t *data, uint8_t len);

/**
 * @brief  处理"读设备信息"命令（上位机下发 0xE0）
 *         返回设备类型和版本信息
 */
void Ano_Handle_Get_Info(void);

/**
 * @brief  在任务上下文里处理上位机请求（0xE0 / 0xE1 的应答）
 *
 * 串口中断里只置请求标志，应答统一在这里发送，避免与 UartTask 的
 * 发送抢同一个 huart1。UartTask 每个循环调用一次即可。
 */
void Ano_Process_Requests(void);

/* ---- 参数读写接口 ---- */

/**
 * @brief  按参数 ID 读取当前参数值（缩放后）
 * @param  id 参数 ID
 * @return 参数值（int32，已缩放）
 */
int32_t Ano_Get_Param(uint8_t id);

/**
 * @brief  按参数 ID 写入参数值（缩放后还原为 float）
 * @param  id 参数 ID
 * @param  value 参数值（int32，已缩放）
 */
void Ano_Set_Param(uint8_t id, int32_t value);

#ifdef __cplusplus
}
#endif

#endif /* __ANO_H */
