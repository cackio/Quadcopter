/**
 * @file    bsp.c
 * @brief   板级支持包：传感器、电机、PPM、DWT 等硬件驱动
 *
 *          本文件从 main.c 中拆分出来，包含：
 *            - DWT 周期计数器初始化
 *            - MPU6050 + HMC5883L + MS5611 传感器驱动
 *            - 电机 PWM 控制
 *            - PPM 输入捕获回调
 */

#include "bsp.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"

/* ================================================================
 *                         传感器地址与寄存器定义
 * ================================================================ */

/* MPU6050 */
#define MPU6050_ADDR          0xD0   /* (0x68 << 1)               */
#define PWR_MGMT_1_REG        0x6B
#define WHO_AM_I_REG          0x75
#define INT_PIN_CFG_REG       0x37   /* 旁路模式配置寄存器         */
#define ACCEL_XOUT_H_REG      0x3B
#define GYRO_XOUT_H_REG       0x43

/* HMC5883L */
#define HMC5883L_ADDR         0x3C   /* (0x1E << 1)               */
#define HMC_CONF_A_REG        0x00
#define HMC_MODE_REG          0x02
#define HMC_DATA_X_H_REG      0x03

/* MS5611 */
#define MS5611_ADDR           0xEE   /* 7位地址 0x77              */
#define MS_CMD_RESET          0x1E
#define MS_CMD_READ_ADC       0x00
#define MS_CMD_PROM_READ_BASE 0xA0
#define MS_CMD_D1_4096        0x48   /* 压力转换 (OSR=4096)       */
#define MS_CMD_D2_4096        0x58   /* 温度转换 (OSR=4096)       */


/* ================================================================
 *                            全局变量定义
 * ================================================================ */

/* MPU6050 原始数据 */
int16_t Accel_X_RAW, Accel_Y_RAW, Accel_Z_RAW;
int16_t Gyro_X_RAW,  Gyro_Y_RAW,  Gyro_Z_RAW;

/* HMC5883L 原始磁场数据 */
/* HMC5883L 的数据寄存器读取顺序是 X, Z, Y */
int16_t Mag_X_RAW, Mag_Y_RAW, Mag_Z_RAW;

/* MS5611 原始数据 (24位需要用 uint32_t 存储)*/
uint32_t D1_Pressure_RAW;
uint32_t D2_Temperature_RAW;

/* MS5611 校准参数 (PROM) */
uint16_t C[7];    /* C[1]~C[6] 有效 */

/* PPM 遥控信号 8 通道 */
volatile uint16_t PPM_Values[8] = {
    1500, 1500, 1000, 1500,
    1500, 1500, 1500, 1500
};

volatile uint8_t  PPM_Index    = 0;
volatile uint32_t last_capture = 0;


/* ================================================================
 *                        BSP 初始化（含 DWT）
 * ================================================================ */

void BSP_Init(void)
{
    /* ---- 6. ★ DWT 周期计数器初始化（SystemView 时间戳源）---- */

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   /* 使能 DWT 调试单元          */
    DWT->CYCCNT = 0;                                   /* 计数器清零                  */
    DWT->CTRL   |= DWT_CTRL_CYCCNTENA_Msk;            /* 启动 CYCCNT 自动计数        */

    /* ---- 启动四路电机 PWM ---- */

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

    /* ---- 启动 PPM 输入捕获中断 ---- */

    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);

    /* ---- 初始化所有传感器 ---- */

    All_Sensors_Init();

    HAL_UART_Transmit(&huart1, (uint8_t *)"BSP Init OK\r\n", 15, 1000);
}


/* ================================================================
 *                      MPU6050 驱动函数
 * ================================================================ */

void MPU6050_Init(void)
{
    uint8_t Data[2];
    uint8_t check;

    /* 检测 MPU6050 是否存在 */
    if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, WHO_AM_I_REG,
                         1, &check, 1, 100) != HAL_OK || check != 0x68) {
        return;   /* 芯片不存在或 I2C 通信失败 */
    }

    /* 唤醒传感器（退出睡眠模式） */
    Data[0] = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, PWR_MGMT_1_REG,
                      1, Data, 1, 100);
    HAL_Delay(10);

    /* 开启 I2C 旁路模式——使 MPU6050 后面的 HMC5883L 能被主机访问 */
    Data[0] = 0x02;   /* BYPASS_EN = 1 */
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, INT_PIN_CFG_REG,
                      1, Data, 1, 100);
}


/* ================================================================
 *                     HMC5883L 驱动函数
 * ================================================================ */

void HMC5883L_Init(void)
{
    uint8_t Data[2];

    /* 配置寄存器 A：8 次平均采样、75Hz 输出速率、正常测量 */
    Data[0] = 0x70;
    HAL_I2C_Mem_Write(&hi2c1, HMC5883L_ADDR, HMC_CONF_A_REG,
                      1, Data, 1, 100);

    /* 模式寄存器：连续测量模式 */
    Data[0] = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, HMC5883L_ADDR, HMC_MODE_REG,
                      1, Data, 1, 100);
}


/* ================================================================
 *                      MS5611 驱动函数
 * ================================================================ */

void MS5611_PROM_Read(void)
{
    uint8_t Data_Rx[2];
    uint8_t cmd;
    uint8_t i;

    /* 发送复位命令 */
    cmd = MS_CMD_RESET; // 假设宏定义是 0x1E
    HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, &cmd, 1, 100);
    HAL_Delay(10); // 复位需要一点时间

    /* 依次读取 6 个 PROM 校准值 C[1] ~ C[6] */
    for (i = 0; i < 6; i++) {
        uint8_t reg_addr = MS_CMD_PROM_READ_BASE + (i * 2);
        HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, &reg_addr, 1, 100);
        if (HAL_I2C_Master_Receive(&hi2c1, MS5611_ADDR, Data_Rx, 2, 100) == HAL_OK) {
            C[i + 1] = (uint16_t)(Data_Rx[0] << 8 | Data_Rx[1]);
        }
    }
}

void All_Sensors_Init(void)
{
    MPU6050_Init();
    HMC5883L_Init();
    MS5611_PROM_Read();
}


/* ================================================================
 *                      传感器数据读取函数
 * ================================================================ */

void MPU6050_Read_Raw(void)
{
    uint8_t Rec_Data[14];   /* ACCEL_XOUT_H 到 GYRO_ZOUT_L 共 14 字节 */
	
	// 一次性从 0x3B 开始连续读取 14 个字节
    if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, ACCEL_XOUT_H_REG,
                         1, Rec_Data, 14, 100) == HAL_OK) {
		// 加速度计 (X, Y, Z)
        Accel_X_RAW = (int16_t)(Rec_Data[0]  << 8 | Rec_Data[1]);
        Accel_Y_RAW = (int16_t)(Rec_Data[2]  << 8 | Rec_Data[3]);
        Accel_Z_RAW = (int16_t)(Rec_Data[4]  << 8 | Rec_Data[5]);
		
        /* 忽略 Rec_Data[6..7]（温度） */
	
		// 陀螺仪 (X, Y, Z)
        Gyro_X_RAW  = (int16_t)(Rec_Data[8]  << 8 | Rec_Data[9]);
        Gyro_Y_RAW  = (int16_t)(Rec_Data[10] << 8 | Rec_Data[11]);
        Gyro_Z_RAW  = (int16_t)(Rec_Data[12] << 8 | Rec_Data[13]);
    }
}

void HMC5883L_Read_Raw(void)
{
    uint8_t Rec_Data[6];
	
	// 从 HMC_DATA_X_H_REG (0x03) 开始连续读取 6 个字节
    // 顺序是 X高, X低, Z高, Z低, Y高, Y低
    if (HAL_I2C_Mem_Read(&hi2c1, HMC5883L_ADDR, HMC_DATA_X_H_REG,
                         1, Rec_Data, 6, 100) == HAL_OK) {
        Mag_X_RAW = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
        Mag_Z_RAW = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]);  /* Z 在 Y 之前！ */
        Mag_Y_RAW = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]);
    }
}

void MS5611_Read_Raw(void)
{
    uint8_t Rx_Buf[3];
    uint8_t cmd;

    /* ---- 读取原始压力 (D1) ---- */
    cmd = MS_CMD_D1_4096;
    if (HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, &cmd, 1, 100) != HAL_OK)
        return;
    HAL_Delay(10);   /* 等待 ADC 转换完成 */

    cmd = MS_CMD_READ_ADC;
    HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, &cmd, 1, 100);
    if (HAL_I2C_Master_Receive(&hi2c1, MS5611_ADDR, Rx_Buf, 3, 100) == HAL_OK) {
        D1_Pressure_RAW = (uint32_t)(Rx_Buf[0] << 16 | Rx_Buf[1] << 8 | Rx_Buf[2]);
    }

    /* ---- 读取原始温度 (D2) ---- */
    cmd = MS_CMD_D2_4096;
    HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, &cmd, 1, 100);
    HAL_Delay(10);

    cmd = MS_CMD_READ_ADC;
    HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, &cmd, 1, 100);
    if (HAL_I2C_Master_Receive(&hi2c1, MS5611_ADDR, Rx_Buf, 3, 100) == HAL_OK) {
        D2_Temperature_RAW = (uint32_t)(Rx_Buf[0] << 16 | Rx_Buf[1] << 8 | Rx_Buf[2]);
    }
}


/* ================================================================
 *                       电机控制函数
 * ================================================================ */
/**
  * @brief  设置电机速度
  * @param  htim: 定时器句柄
  * @param  Channel: 定时器通道
  * @param  speed: 速度值，范围0-ARR值 (ARR是自动重载值)
  * @retval None
  */

void Motor_SetSpeed(TIM_HandleTypeDef *htim, uint32_t Channel, uint16_t speed)
{
    uint16_t arr = __HAL_TIM_GET_AUTORELOAD(htim);
    if (speed > arr) {
        speed = arr;    /* 确保speed不超过ARR值，防止占空比超过 100% */
    }
    __HAL_TIM_SET_COMPARE(htim, Channel, speed);
}

void Motor_SetDirection(uint8_t direction)
{
    /* TODO: 实现电机方向控制。
     * 例如通过 GPIO 控制 L298N/H 桥的 IN1/IN2 或 DIR 引脚。
     * direction = 0: 正转, 1: 反转（编码待定）*/
    (void)direction;   /* 消除编译器警告 */
}

void Motor_Stop(TIM_HandleTypeDef *htim, uint32_t Channel)
{
    __HAL_TIM_SET_COMPARE(htim, Channel, 0);
}



/* ================================================================
 *                     PPM 输入捕获回调（ISR 上下文）
 * ================================================================ */

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        uint32_t current_capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
        uint32_t diff;

        /* 计算脉宽（考虑定时器溢出回绕）*/
        if (current_capture >= last_capture) {
            diff = current_capture - last_capture;
        } else {
            diff = (0xFFFFFFFF - last_capture) + current_capture + 1;
        }

        if (diff > 2500) {
            /* 同步帧检测：帧间隔 > 2.5ms */
            PPM_Index = 0;
        } else if (diff >= 900 && diff <= 2100) {
            /* 有效通道数据采集（900μs ~ 2100μs）*/
            if (PPM_Index < 8) {
                PPM_Values[PPM_Index] = diff;
                PPM_Index++;
            }
        }
        last_capture = current_capture;
    }
}