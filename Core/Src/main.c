/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdarg.h>
#include "ucos_ii.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define SENSOR_TASK_STK_SIZE   256
#define MOTOR_TASK_STK_SIZE    256
#define UART_TASK_STK_SIZE     384

#define SENSOR_TASK_PRIO       8
#define MOTOR_TASK_PRIO        7
#define UART_TASK_PRIO         9

OS_STK SensorTaskStk[SENSOR_TASK_STK_SIZE];
OS_STK MotorTaskStk[MOTOR_TASK_STK_SIZE];
OS_STK UartTaskStk[UART_TASK_STK_SIZE];

// MPU6050 地址和寄存器
#define MPU6050_ADDR         0xD0  // (0x68 << 1)
#define PWR_MGMT_1_REG       0x6B
#define WHO_AM_I_REG         0x75
#define INT_PIN_CFG_REG      0x37  // 旁路模式配置寄存器
#define ACCEL_XOUT_H_REG     0x3B
#define GYRO_XOUT_H_REG      0x43

// HMC5883L 地址和寄存器
#define HMC5883L_ADDR        0x3C  // (0x1E << 1)
#define HMC_CONF_A_REG       0x00
#define HMC_MODE_REG         0x02
#define HMC_DATA_X_H_REG     0x03

// MS5611 地址和命令
#define MS5611_ADDR          0xEE  // 7位地址 0x77
#define MS_CMD_RESET         0x1E
#define MS_CMD_READ_ADC      0x00
#define MS_CMD_PROM_READ_BASE 0xA0
#define MS_CMD_D1_4096       0x48  // 压力转换 (OSR=4096)
#define MS_CMD_D2_4096       0x58  // 温度转换 (OSR=4096)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// ----------------- MPU6050/HMC5883L/MS5611 存储变量 -----------------
// MPU6050 原始数据
int16_t Accel_X_RAW, Accel_Y_RAW, Accel_Z_RAW;
int16_t Gyro_X_RAW, Gyro_Y_RAW, Gyro_Z_RAW;

// HMC5883L 原始数据
// HMC5883L 的数据寄存器读取顺序是 X, Z, Y
int16_t Mag_X_RAW, Mag_Y_RAW, Mag_Z_RAW;

// MS5611 原始数据 (24位需要用 uint32_t 存储)
uint32_t D1_Pressure_RAW;  // 原始压力值
uint32_t D2_Temperature_RAW; // 原始温度值

// MS5611 校准参数 (PROM数据)
uint16_t C[7]; // C[1]~C[6] 有效

// PPM 协议的 8 个通道
volatile uint16_t PPM_Values[8] = {
    1500, 1500, 1000, 1500,
    1500, 1500, 1500, 1500
};

volatile uint8_t  PPM_Index = 0;
volatile uint32_t last_capture = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
void SystemClock_Config(void);
void Motor_SetSpeed(TIM_HandleTypeDef *htim, uint32_t Channel, uint16_t speed);
void Motor_SetDirection(uint8_t direction);
void Motor_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void MPU6050_Init(void);
void HMC5883L_Init(void);
void MS5611_PROM_Read(void);
void All_Sensors_Init(void);
void MPU6050_Read_Raw(void);
void HMC5883L_Read_Raw(void);
void MS5611_Read_Raw(void);
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);

void MotorTask(void *p_arg)
{
		OS_CPU_SR cpu_sr = 0;
    uint16_t throttle_input;

    (void)p_arg;

    while (1)
    {
        /*
         * T8FB 一般通道 3 是油门，对应数组下标 2
         * 为了防止中断正在改 PPM_Values，这里关中断短暂读取。
         */
        OS_ENTER_CRITICAL();
        throttle_input = PPM_Values[2];
        OS_EXIT_CRITICAL();

        /*
         * 信号异常保护
         */
        if (throttle_input < 900 || throttle_input > 2200)
        {
            throttle_input = 1000;
        }

        /*
         * 四路电机同步油门
         */
        Motor_SetSpeed(&htim3, TIM_CHANNEL_1, throttle_input);
        Motor_SetSpeed(&htim3, TIM_CHANNEL_2, throttle_input);
        Motor_SetSpeed(&htim3, TIM_CHANNEL_3, throttle_input);
        Motor_SetSpeed(&htim3, TIM_CHANNEL_4, throttle_input);

        /*
         * 电机控制一般 20ms 更新一次
         */
        OSTimeDlyHMSM(0, 0, 0, 20);
    }
}
void SensorTask(void *p_arg)
{
    (void)p_arg;

    while (1)
    {
        MPU6050_Read_Raw();
        HMC5883L_Read_Raw();
        MS5611_Read_Raw();

        /*
         * 传感器读取周期可以先设 50ms。
         * 后面做姿态解算时，MPU6050 可以单独提高到 5ms 或 10ms。
         */
        OSTimeDlyHMSM(0, 0, 0, 50);
    }
}
void UartTask(void *p_arg)
{
		OS_CPU_SR cpu_sr = 0;
    char data[160];

    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    int16_t mx, my, mz;
    uint32_t p_raw, t_raw;
    uint16_t throttle;

    (void)p_arg;

    HAL_UART_Transmit(&huart1,
                      (uint8_t *)"UartTask running\r\n",
                      18,
                      300);

    while (1)
    {
        /*
         * 拷贝一份全局变量，防止打印过程中数据被传感器任务改动。
         */
        OS_ENTER_CRITICAL();

        ax = Accel_X_RAW;
        ay = Accel_Y_RAW;
        az = Accel_Z_RAW;

        gx = Gyro_X_RAW;
        gy = Gyro_Y_RAW;
        gz = Gyro_Z_RAW;

        mx = Mag_X_RAW;
        my = Mag_Y_RAW;
        mz = Mag_Z_RAW;

        p_raw = D1_Pressure_RAW;
        t_raw = D2_Temperature_RAW;

        throttle = PPM_Values[2];

        OS_EXIT_CRITICAL();

        snprintf(data, sizeof(data),
                 "THR:%u\r\n"
                 "Accel:%d,%d,%d\r\n"
                 "Gyro:%d,%d,%d\r\n"
                 "Mag:%d,%d,%d\r\n"
                 "Pre_RAW:%lu Tem_RAW:%lu\r\n\r\n",
                 throttle,
                 ax, ay, az,
                 gx, gy, gz,
                 mx, my, mz,
                 (unsigned long)p_raw,
                 (unsigned long)t_raw);

        HAL_UART_Transmit(&huart1,
                          (uint8_t *)data,
                          strlen(data),
                          300);

        OSTimeDlyHMSM(0, 0, 0, 500);
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

		/* USER CODE BEGIN 1 */
		INT8U os_err;

		HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    MX_TIM3_Init();
    MX_TIM2_Init();

		HAL_UART_Transmit(&huart1,
                  (uint8_t *)"BOOT OK\r\n",
                  9,
                  1000);
    /*
     * 启动四路 PWM
     * 如果你实际只接了一个电机，也可以只启动对应通道。
     */
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

    /*
     * 启动 PPM 输入捕获中断
     */
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);

    /*
     * 初始化传感器
     */
    All_Sensors_Init();

    /*
     * 初始化 uC/OS-II
     */
    OSInit();

    /*
     * 创建任务
     */
    os_err = OSTaskCreate(MotorTask,
                          (void *)0,
                          &MotorTaskStk[MOTOR_TASK_STK_SIZE - 1],
                          MOTOR_TASK_PRIO);
    if (os_err != OS_ERR_NONE)
    {
        Error_Handler();
    }

    os_err = OSTaskCreate(SensorTask,
                          (void *)0,
                          &SensorTaskStk[SENSOR_TASK_STK_SIZE - 1],
                          SENSOR_TASK_PRIO);
    if (os_err != OS_ERR_NONE)
    {
        Error_Handler();
    }

    os_err = OSTaskCreate(UartTask,
                          (void *)0,
                          &UartTaskStk[UART_TASK_STK_SIZE - 1],
                          UART_TASK_PRIO);
    if (os_err != OS_ERR_NONE)
    {
        Error_Handler();
    }

    OSStart();

  /* USER CODE END 1 */


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
		
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
  * @brief  设置电机速度
  * @param  htim: 定时器句柄
  * @param  Channel: 定时器通道
  * @param  speed: 速度值，范围0-ARR值 (ARR是自动重载值)
  * @retval None
  */
void Motor_SetSpeed(TIM_HandleTypeDef *htim, uint32_t Channel, uint16_t speed)
{
  // 确保speed不超过ARR值，防止占空比超过100%
  uint16_t arr = __HAL_TIM_GET_AUTORELOAD(htim);
  if(speed > arr) {
    speed = arr;
  }
  __HAL_TIM_SET_COMPARE(htim, Channel, speed); // 设置捕获/比较寄存器值(CCR)来控制占空比[citation:1]
}


void MPU6050_Init(void)
{
    uint8_t Data[2];
    uint8_t check;

    // 检测 MPU6050
    if(HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, WHO_AM_I_REG, 1, &check, 1, 100) != HAL_OK || check != 0x68)
    {
        // 检查失败
        return;
    }

    // 唤醒传感器
    Data[0] = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, PWR_MGMT_1_REG, 1, Data, 1, 100);
    HAL_Delay(10); // 等待唤醒

    // 3. 开启 I2C 旁路模式 (INT_PIN_CFG, 0x37 -> 0x02)
    Data[0] = 0x02; // BYPASS_EN = 1
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, INT_PIN_CFG_REG, 1, Data, 1, 100);
}

/**
 * @brief HMC5883L 初始化
 */
void HMC5883L_Init(void)
{
    uint8_t Data[2];

    // 配置寄存器A (HMC_CONF_A_REG, 0x00) -> 0x70
    Data[0] = 0x70;
    HAL_I2C_Mem_Write(&hi2c1, HMC5883L_ADDR, HMC_CONF_A_REG, 1, Data, 1, 100);

    // 2. 模式寄存器 (HMC_MODE_REG, 0x02) -> 0x00
    Data[0] = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, HMC5883L_ADDR, HMC_MODE_REG, 1, Data, 1, 100);
}


/**
 * @brief MS5611 重置并读取 PROM 校准参数
 */
void MS5611_PROM_Read(void)
{
    uint8_t Data_Rx[2];
    uint8_t cmd; // 定义一个变量来存命令
		uint8_t i;

    //  发送 Reset 命令
    cmd = MS_CMD_RESET; // 假设宏定义是 0x1E
    HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, &cmd, 1, 100);
    HAL_Delay(10); // 复位需要一点时间

    for (i = 0; i < 6; i++)
    {
        uint8_t reg_addr = MS_CMD_PROM_READ_BASE + (i * 2);
        HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, &reg_addr, 1, 100);
        if (HAL_I2C_Master_Receive(&hi2c1, MS5611_ADDR, Data_Rx, 2, 100) == HAL_OK)
        {
            C[i+1] = (uint16_t)(Data_Rx[0] << 8 | Data_Rx[1]);
        }
    }
}


/**
 * @brief 在 main() 函数的 USER CODE BEGIN 2 区域调用所有初始化函数
 */
void All_Sensors_Init(void)
{
    MPU6050_Init();
    HMC5883L_Init();
    MS5611_PROM_Read();
}

// ------------------- 读取函数 -------------------

/**
 * @brief 读取 MPU6050 原始数据 (Acc + Gyro)
 */
void MPU6050_Read_Raw(void)
{
    uint8_t Rec_Data[14]; // 从 ACCEL_XOUT_H 到 GYRO_ZOUT_L 共 14 字节

    // 一次性从 0x3B 开始连续读取 14 个字节
    if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, ACCEL_XOUT_H_REG, 1, Rec_Data, 14, 100) == HAL_OK)
    {
        // 加速度计 (X, Y, Z)
        Accel_X_RAW = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
        Accel_Y_RAW = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]);
        Accel_Z_RAW = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]);

        // 忽略温度 Rec_Data[6] 和 Rec_Data[7]

        // 陀螺仪 (X, Y, Z)
        Gyro_X_RAW = (int16_t)(Rec_Data[8] << 8 | Rec_Data[9]);
        Gyro_Y_RAW = (int16_t)(Rec_Data[10] << 8 | Rec_Data[11]);
        Gyro_Z_RAW = (int16_t)(Rec_Data[12] << 8 | Rec_Data[13]);
    }
}

/**
 * @brief 读取 HMC5883L 原始磁场数据
 */
void HMC5883L_Read_Raw(void)
{
    uint8_t Rec_Data[6];

    // 从 HMC_DATA_X_H_REG (0x03) 开始连续读取 6 个字节
    // 顺序是 X高, X低, Z高, Z低, Y高, Y低
    if (HAL_I2C_Mem_Read(&hi2c1, HMC5883L_ADDR, HMC_DATA_X_H_REG, 1, Rec_Data, 6, 100) == HAL_OK)
    {
        Mag_X_RAW = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
        Mag_Z_RAW = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]); // Z轴
        Mag_Y_RAW = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]); // Y轴
    }
}

/**
 * @brief 读取 MS5611 原始 D1 和 D2 数据
 * MS5611 读数需要：发送转换命令 -> 等待 -> 发送读取 ADC 命令
 */
void MS5611_Read_Raw(void)
{
    uint8_t Rx_Buf[3];
    uint8_t cmd;

    // --- 1. 读取原始压力 (D1) ---
    cmd = MS_CMD_D1_4096; // 0x48
    HAL_StatusTypeDef status;
    
    // 正确发送转换命令
    status = HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, &cmd, 1, 100);
    if (status != HAL_OK) return; 

    HAL_Delay(10); // 等待转换完成

    cmd = MS_CMD_READ_ADC; // 0x00
    HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, &cmd, 1, 100);

    if (HAL_I2C_Master_Receive(&hi2c1, MS5611_ADDR, Rx_Buf, 3, 100) == HAL_OK)
    {
        D1_Pressure_RAW = (uint32_t)(Rx_Buf[0] << 16 | Rx_Buf[1] << 8 | Rx_Buf[2]);
    }

    // --- 2. 读取原始温度 (D2) ---
    cmd = MS_CMD_D2_4096; // 0x58
    HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, &cmd, 1, 100);
    HAL_Delay(10); 

    cmd = MS_CMD_READ_ADC; // 0x00
    HAL_I2C_Master_Transmit(&hi2c1, MS5611_ADDR, &cmd, 1, 100);

    if (HAL_I2C_Master_Receive(&hi2c1, MS5611_ADDR, Rx_Buf, 3, 100) == HAL_OK)
    {
        D2_Temperature_RAW = (uint32_t)(Rx_Buf[0] << 16 | Rx_Buf[1] << 8 | Rx_Buf[2]);
    }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
		if (htim->Instance == TIM2) // 确保是 PPM 定时器
				{
						uint32_t current_capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
						uint32_t diff;

						// 计算脉宽（处理溢出）
						if (current_capture >= last_capture) {
								diff = current_capture - last_capture;
						} else {
								diff = (0xFFFFFFFF - last_capture) + current_capture + 1;
						}

						// 1. 同步帧检测 (>2.5ms)
						if (diff > 2500) {
								PPM_Index = 0;
						}
						// 2. 有效数据采集 (900us - 2100us)
						else if (diff >= 900 && diff <= 2100) {
								if (PPM_Index < 8) {
										PPM_Values[PPM_Index] = diff;
										PPM_Index++;
								}
						}
						last_capture = current_capture;
				}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
		
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
