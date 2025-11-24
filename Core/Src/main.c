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
#include <stdio.h>
#include "gy86.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Motor_SetSpeed(TIM_HandleTypeDef *htim, uint32_t Channel, uint16_t speed);
void Motor_SetDirection(uint8_t direction);
void Motor_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);
void UART_Printf(UART_HandleTypeDef *huart, const char *fmt, ...);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
	I2C_HandleTypeDef hi2c1; // 假设I2C1句柄
	int16_t ax, ay, az; // 加速度计原始数据
	int16_t gx, gy, gz; // 陀螺仪原始数据
	MPU6050_Init(&hi2c1);
	while (1)
    {
			//通过串口发送字符串
			char txData[] = "Hello Bluetooth\r\n";
			HAL_UART_Transmit(&huart1, (uint8_t *)txData, strlen(txData), 100);
			HAL_Delay(5000);
			Motor_SetSpeed(&htim3, TIM_CHANNEL_4, 1000);
			
			HAL_Delay(3000);
			Motor_SetSpeed(&htim3, TIM_CHANNEL_4, 1200);
			HAL_Delay(10000);
			Motor_SetSpeed(&htim3, TIM_CHANNEL_4, 1600);
			HAL_Delay(10000);
			Motor_SetSpeed(&htim3, TIM_CHANNEL_4, 2000);
			HAL_Delay(10000);
			
			// 读取加速度计数据
			if (MPU6050_Read_Accelerometer(&hi2c1, &ax, &ay, &az) == HAL_OK) {
					UART_Printf(&huart1, "Accel: X=%6d, Y=%6d, Z=%6d\r\n", ax, ay, az);
			}
			
			// 读取陀螺仪数据
			if (MPU6050_Read_Gyroscope(&hi2c1, &gx, &gy, &gz) == HAL_OK) {
					UART_Printf(&huart1, "gyro: X=%6d, Y=%6d, Z=%6d\r\n", gx, gy, gz);
			}
			
			HAL_Delay(500); // 延时500ms，降低读取频率
    }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
		
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
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
void UART_Printf(UART_HandleTypeDef *huart, const char *fmt, ...)
{
  char buffer[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  
  HAL_UART_Transmit(huart, (uint8_t *)buffer, strlen(buffer), HAL_MAX_DELAY);
}
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

/**
  * @brief  控制电机方向
  * @param  direction: 方向，0-正转，1-反转 (具体逻辑根据驱动模块定义)
  * @retval None
  */
void Motor_SetDirection(uint8_t direction)
{
  // 假设使用PA0和PA1控制方向，根据电机驱动模块真值表设置
  if(direction == 0) { // 正转
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
  } else { // 反转
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
  }
}

/**
  * @brief  停止电机
  * @param  htim: 定时器句柄
  * @param  Channel: 定时器通道
  * @retval None
  */
void Motor_Stop(TIM_HandleTypeDef *htim, uint32_t Channel)
{
  __HAL_TIM_SET_COMPARE(htim, Channel, 0); // 占空比设为0
  // 同时可以将方向控制引脚都置为低电平（具体逻辑取决于驱动模块）
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
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
