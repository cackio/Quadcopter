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
#include "bsp.h"
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
#include "os_trace.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/

#define SENSOR_TASK_STK_SIZE   256
#define MOTOR_TASK_STK_SIZE    256
#define UART_TASK_STK_SIZE     384
#define START_TASK_STK_SIZE    256

#define START_TASK_PRIO         5
#define SENSOR_TASK_PRIO       8
#define MOTOR_TASK_PRIO        7
#define UART_TASK_PRIO         9

OS_STK StartTaskStk[START_TASK_STK_SIZE];
OS_STK SensorTaskStk[SENSOR_TASK_STK_SIZE];
OS_STK MotorTaskStk[MOTOR_TASK_STK_SIZE];
OS_STK UartTaskStk[UART_TASK_STK_SIZE];



/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
void SystemClock_Config(void);
void StartTask(void *p_arg);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


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

    /* ---- HAL 库初始化 ---- */
    HAL_Init();
    SystemClock_Config();

    /* ---- 外设初始化 ---- */
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    MX_TIM3_Init();
    MX_TIM2_Init();

    HAL_UART_Transmit(&huart1, (uint8_t *)"BOOT OK\r\n", 9, 1000);

    /* ---- uC/OS-II 内核初始化 ---- */
    OSInit();

    /* ---- 创建启动任务（最高优先级，负责后续初始化）---- */
    os_err = OSTaskCreate(StartTask,
                          (void *)0,
                          &StartTaskStk[START_TASK_STK_SIZE - 1],
                          START_TASK_PRIO);
    if (os_err != OS_ERR_NONE) {
        Error_Handler();
    }

    /* ---- 启动多任务调度 ---- */
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
/* ====================== StartTask（初始化任务） ====================== */

void StartTask(void *p_arg)
{
    INT8U os_err;

    (void)p_arg;

    /* 1. 板级初始化（DWT + 传感器 + PWM + PPM，全在 BSP_Init 里）*/
    BSP_Init();

    /* 2. 启动 SystemView 事件录制 */
    OS_TRACE_INIT();

    /* 3. 创建子任务 */
    os_err = OSTaskCreate(MotorTask,
                          (void *)0,
                          &MotorTaskStk[MOTOR_TASK_STK_SIZE - 1],
                          MOTOR_TASK_PRIO);
    OSTaskNameSet(MOTOR_TASK_PRIO, (INT8U *)"Motor Task", &os_err);

    os_err = OSTaskCreate(SensorTask,
                          (void *)0,
                          &SensorTaskStk[SENSOR_TASK_STK_SIZE - 1],
                          SENSOR_TASK_PRIO);
    OSTaskNameSet(SENSOR_TASK_PRIO, (INT8U *)"Sensor Task", &os_err);

    os_err = OSTaskCreate(UartTask,
                          (void *)0,
                          &UartTaskStk[UART_TASK_STK_SIZE - 1],
                          UART_TASK_PRIO);
    OSTaskNameSet(UART_TASK_PRIO, (INT8U *)"Uart Task", &os_err);

    /* 4. 初始化任务完成，自我删除 */
    (void)OSTaskDel(OS_PRIO_SELF);
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
