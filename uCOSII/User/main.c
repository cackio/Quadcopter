#include "includes.h"
#include "os_cpu.h"
#include "led.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// 定义任务优先级
#define TASK_STARTUP_PRIO 4
#define MUTEX_IIC_PRIO 5
#define TASK_ANGEL_PRIO 6
#define TASK_HEIGHT_PRIO 7
#define TASK_PID_PRIO 8
#define TASK_COM_PRIO 9
#define TASK_GY86_PRIO 9
// 定义任务栈大小
#define TASK_START_STK_SIZE 1024
#define TASK_COM_STK_SIZE 512
#define TASK_GY86_STK_SIZE 512
#define TASK_ANGEL_STK_SIZE 512
#define TASK_HEIGHT_STK_SIZE 512
#define TASK_PID_STK_SIZE 512
// 栈内存分配
static OS_STK Task_Start_STK[TASK_START_STK_SIZE];
static OS_STK Task_COM_STK[TASK_COM_STK_SIZE];
static OS_STK Task_GY86_STK[TASK_GY86_STK_SIZE];
static OS_STK Task_Angel_STK[TASK_ANGEL_STK_SIZE];
static OS_STK Task_Height_STK[TASK_HEIGHT_STK_SIZE];
static OS_STK Task_PID_STK[TASK_PID_STK_SIZE];
// 函数定义
static void Task_Start(void* p_arg);
static void Task_COM(void* p_arg);
static void Task_GY86(void* p_arg);
static void Task_Angel(void* p_arg);
static void Task_Height(void* p_arg);
static void Task_PID(void* p_arg);

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

void SystemClock_Config(void);

void MPU6050_Init(void);
void HMC5883L_Init(void);
void MS5611_PROM_Read(void);
void MPU6050_Read_Raw(void);
void HMC5883L_Read_Raw(void);
void MS5611_Read_Raw(void);


extern void OSTimeTick(void);
void SysTick_Handler(void) {
    OSIntEnter();
    OSTimeTick();
    OSIntExit();
}
void CauseStackCrash(int x);

int main(void) {

    SystemInit(); 
    //LED_Init(); LED_Toggle(); while(1);
	OSInit(); 

    OSTaskCreate(Task_Start, (void *)0, &Task_Start_STK[TASK_START_STK_SIZE - 1], TASK_STARTUP_PRIO);
	
    OSStart();

    return 0;
}


// 启动任务
static void Task_Start(void* p_arg){
	INT8U err;
    //BSP_Init();
	MPU6050_Init();
    HMC5883L_Init();
    MS5611_PROM_Read();
    OSTimeDly(5000);

    // Create functional task
    OSTaskCreate(Task_GY86, (void*)0, &Task_GY86_STK[TASK_GY86_STK_SIZE - 1], TASK_GY86_PRIO);

    // Delete itself
    OSTaskDel(OS_PRIO_SELF);
}


// GY86任务
static void Task_GY86(void* p_arg)
{
	SystemClock_Config();

	MX_GPIO_Init();
	MX_I2C1_Init();
	MX_USART1_UART_Init();

	while (1)
    {
      // ==========================================
      // 传感器数据读取部分
      // ==========================================
			MPU6050_Read_Raw();
			HMC5883L_Read_Raw();
			MS5611_Read_Raw();

			// 串口输出所有模块的原始数据
			char data[100]; 
			
			// MPU6050 (Accel/Gyro) - int16_t -> %d
			sprintf(data, "Accel: %d,%d,%d\nGyro:%d,%d,%d\n", 
							Accel_X_RAW, Accel_Y_RAW, Accel_Z_RAW, 
							Gyro_X_RAW, Gyro_Y_RAW, Gyro_Z_RAW);
			HAL_UART_Transmit(&huart1, (uint8_t *)data, strlen(data), 0xFFFF);
			
			// HMC5883L (Magnetometer) - int16_t -> %d
			sprintf(data, "Mag:%d,%d,%d\n",
							Mag_X_RAW, Mag_Y_RAW, Mag_Z_RAW);
			HAL_UART_Transmit(&huart1, (uint8_t *)data, strlen(data), 0xFFFF);

			// MS5611 (Pressure/Temperature) - uint32_t -> %lu (unsigned long)
			// MS5611的原始值较大，通常超过 16位 (65535)
			sprintf(data, "Pre_RAW:%u Tem_RAW:%u\n", 
							D1_Pressure_RAW, D2_Temperature_RAW);
			HAL_UART_Transmit(&huart1, (uint8_t *)data, strlen(data), 0xFFFF);

			HAL_Delay(500);
    }
  while (1) {}
    }

	
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

	