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
#include "ano.h"
#include "imu.h"
#include "pid.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* 任务栈单位是“OS_STK 个数”，每个 OS_STK = 4 字节。
 * SensorTask 在做椭球/磁校准的最小二乘拟合时局部矩阵很大
 * （mat[9][10] / ATA[9][9] / JTJ[9][9] 各 300+ 字节），
 * 原来 256 个（1024 字节）不够用，会栈溢出 → HardFault → 板子静默不发了。
 * 这里统一放大，保证校准结束那一刻也不会溢出。 */
#define SENSOR_TASK_STK_SIZE   512   /* 2 KB   */
#define MOTOR_TASK_STK_SIZE    256   /* 1 KB   */
#define UART_TASK_STK_SIZE     512   /* 2 KB，含 char data[220] 和 snprintf 的临时空间 */
#define START_TASK_STK_SIZE    384   /* 1.5 KB，含 BSP_Init + 创建三个任务 */
#define PIDSHELL_TASK_SIZE     256
#define PIDCORE_TASK_SIZE      256

#define START_TASK_PRIO        5
#define PIDSHELL_TASK_PRIO     6
#define PIDCORE_TASK_PRIO      7
#define SENSOR_TASK_PRIO       9
#define MOTOR_TASK_PRIO        8
#define UART_TASK_PRIO         10

/* 每个任务的周期（单位：tick） 当前tick为10ms */
#define SENSOR_TASK_PERIOD    1
#define MOTOR_TASK_PERIOD     2
#define UART_TASK_PERIOD      5
#define PIDSHELL_TASK_PERIOD  4
#define PIDCORE_TASK_PERIOD   1

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
OS_STK StartTaskStk[START_TASK_STK_SIZE];
OS_STK SensorTaskStk[SENSOR_TASK_STK_SIZE];
OS_STK MotorTaskStk[MOTOR_TASK_STK_SIZE];
OS_STK UartTaskStk[UART_TASK_STK_SIZE];
OS_STK PIDShellTaskStk[PIDSHELL_TASK_SIZE];
OS_STK PIDCoreTaskStk[PIDCORE_TASK_SIZE];
/* USER CODE END PV */

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
         * 姿态校准完成前禁止输出油门，防止校准过程中误转电机。
         */
        if (!IMU_Calib_Done())
        {
            throttle_input = 1000;
        }

        /*
         * 四路电机同步油门
         */
        Motor_SetSpeed(&htim3, TIM_CHANNEL_1, (uint16_t)PWM_IDLE);
        Motor_SetSpeed(&htim3, TIM_CHANNEL_2, (uint16_t)PWM_IDLE);
        Motor_SetSpeed(&htim3, TIM_CHANNEL_3, (uint16_t)PWM_IDLE);
        Motor_SetSpeed(&htim3, TIM_CHANNEL_4, (uint16_t)PWM_IDLE);

        OSTimeDlyHMSM(0, 0, 0, 20);
        continue;
      

      /*
        * 串级 PID 主流程（外环角度环 + 内环角速度环都在这一个任务里）：
        *   1. PID_Exp_Calc      PPM → 期望角度(°)/期望角速度(°·s⁻¹)/油门
        *   2. PID_FlyMode_Update 油门 > 1350 才 armed=1
        *   3. PID_Motor_Calc    Roll/Pitch 串级 + Yaw 单环，共用同一个 Δt
        *   4. PID_Motor_Output  混控结果写进 TIM3 四路 CCR
        *
        * 控制周期 20ms（50Hz），和 PID_Get_Time() 实测的 Δt 一致。
        * 先暂时把PID放这里，之后会把PID做成单独的任务独立出来。
        */
      PID_Control_Update();

      OSTimeDly(MOTOR_TASK_PERIOD);
    }
}
void SensorTask(void *p_arg)
{
    static uint8_t ms_div = 0;

    (void)p_arg;

    while (1)
    {
        /*
         * 姿态通道：MPU6050 + HMC5883L 每个周期都读，
         * 校准期间走校准状态机，校准完成后走 Madgwick 姿态解算。
         */
        MPU6050_Read_Raw();
        HMC5883L_Read_Raw();

        if (IMU_Calib_Done())
        {
            IMU_Attitude_Update();
        }
        else
        {
            IMU_Calib_Update();
        }

        /*
         * 气压计与姿态无关，且阻塞约 20ms，降频到 200ms 读取一次。
         */
        if (++ms_div >= 20)
        {
            ms_div = 0;
            MS5611_Read_Raw();
        }

        /*
         * 1 tick = 10ms（OS_TICKS_PER_SEC = 100），姿态更新 100Hz。
         * imu_dt 由 DWT 实测，周期抖动不会影响积分精度。
         */
        OSTimeDly(SENSOR_TASK_PERIOD);
    }
}
void UartTask(void *p_arg)
{
		OS_CPU_SR cpu_sr = 0;
    char data[220];
    static uint8_t last_stage = 0xFF;
    static uint8_t txt_div = 0;      /* 文本日志分频：20 × 50ms = 1s */
    static uint8_t led_div = 0;      /* LED 心跳分频：10 × 50ms = 0.5s */
    uint8_t stage;

    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    int16_t mx, my, mz;
    uint32_t p_raw, t_raw;
    uint16_t throttle;
    float roll_deg, pitch_deg, yaw_deg;

    (void)p_arg;

    HAL_UART_Transmit(&huart1,
                      (uint8_t *)"UartTask running\r\n",
                      18,
                      300);

    while (1)
    {
        /*
         * 0) 先处理上位机下发的 0xE0 / 0xE1 请求。
         *    协议解析跑在 USART1 接收中断里，但“应答帧”必须在这里发：
         *    中断里调用 HAL_UART_Transmit 会和本任务的发送抢同一个 huart1，
         *    结果是应答被丢掉，上位机就一直“等待校验数据超时 / 参数读取失败”。
         */
        Ano_Process_Requests();

        /*
         * 1) 校准阶段提示：阶段变化时打印一次。
         */
        stage = IMU_Calib_Stage();
        if (stage != last_stage)
        {
            const char *msg = NULL;

            last_stage = stage;
            if (stage == 0)
            {
                msg = "CALIB: keep the board STILL for 2s...\r\n";
            }
            else if (stage == 1)
            {
                msg = "CALIB: rotate the board SLOWLY in all directions (10s)...\r\n";
            }
            else
            {
                msg = "CALIB DONE. Attitude running.\r\n";
            }
            HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), 300);
        }

        /*
         * 2) 姿态帧单独走高频通道：50ms 一帧 = 20Hz。
         *    上位机的波形/仪表需要连续数据才动得起来，
         *    原来 500ms 一帧（2Hz）看起来就像卡住了。
         */
        Ano_Send_Attitude();

        /*
         * 3) 1Hz：文本日志 + 其余数据帧（原始传感器 / 高度 / 电机 / 状态）
         */
        if (++txt_div >= 20)
        {
            txt_div = 0;

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

            roll_deg  = euler_angle.roll  * RAD_TO_DEG;
            pitch_deg = euler_angle.pitch * RAD_TO_DEG;
            yaw_deg   = euler_angle.yaw   * RAD_TO_DEG;

            OS_EXIT_CRITICAL();

            snprintf(data, sizeof(data),
                     "THR:%u RX:%u\r\n"
                     "Euler:%d,%d,%d\r\n"
                     "Accel:%d,%d,%d\r\n"
                     "Gyro:%d,%d,%d\r\n"
                     "Mag:%d,%d,%d\r\n"
                     "Pre_RAW:%lu Tem_RAW:%lu\r\n\r\n",
                     throttle,
                     (unsigned)ano_rx_frames,
                     (int)roll_deg, (int)pitch_deg, (int)yaw_deg,
                     ax, ay, az,
                     gx, gy, gz,
                     mx, my, mz,
                     (unsigned long)p_raw,
                     (unsigned long)t_raw);

            HAL_UART_Transmit(&huart1,
                              (uint8_t *)data,
                              strlen(data),
                              300);

            /*
             * 文本与协议帧共用 USART1（PA9/PA10），蓝牙模块作为无线透传。
             * 文本只是给人看的，协议帧才是上位机解析的对象，两者可以混发。
             */
            Ano_Send_Sensor_Raw();
            Ano_Send_Altitude();
            Ano_Send_Motor_Out();
            Ano_Send_Status();
        }

        /*
         * 4) LED 心跳：每 10 个周期（约 0.5s）翻转一次 LED0(PA5)。
         *    LED 在闪 → 调度器和 UartTask 都活着；
         *    LED 不闪 → 板子根本没跑起来（时钟/启动/硬件问题）。
         */
        if (++led_div >= 10)
        {
            led_div = 0;
            HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
        }

        OSTimeDly(UART_TASK_PERIOD);
    }
}

void PIDShellTask(void *p_arg){

    while(1) {

      PID_PPM_to_Exp_Calc();      // 遥控器期望值计算
      PID_FlyMode_Update(expectation.thr);    // 更新飞行模式.油门期望值只在外环监测期望值后发生变化，所以只在这里用就行
      exp_w.roll  = PID_Cascade_Shell_Calc(expectation.roll  - euler_angle.roll  * RAD_TO_DEG, &roll_shell);
      exp_w.pitch = PID_Cascade_Shell_Calc(expectation.pitch - euler_angle.pitch * RAD_TO_DEG, &pitch_shell);

      OSTimeDly(PIDSHELL_TASK_PERIOD);
    }
}

void PIDCoreTask(void *p_arg){

    while(1) {

      PID_Motor_Calc();           // 电机混控输出计算
      PID_Motor_Output();         // 电机输出配置

      OSTimeDly(PIDCORE_TASK_PERIOD);
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

    HAL_UART_Transmit(&huart1, (uint8_t *)"BOOT OK v2 (USART1 115200)\r\n",
                      strlen("BOOT OK v2 (USART1 115200)\r\n"), 1000);

    /* ----------------------------------------------------------------------
     *  冒烟测试（OS 启动之前执行）
     *
     *  目的：把“硬件链路”和“飞控逻辑”彻底分开，一上电就能判断问题在哪。
     *    - 上位机若马上出现一行 AA FF F1 0C
     *      → 时钟、波特率、接线、帧格式、校验全部正确，问题只在后面的任务；
     *    - 若连 CHR 模式下的 "SMOKE: ..." 文字都看不到
     *      → 问题在串口链路本身：TX/RX 是否交叉、串口模块、
     *        或者板子晶振频率与 PLLM 不匹配（波特率整体偏了）。
     *
     *  数据区 12 字节（小端），对应 6 个 int16：
     *    roll=10.00°  pitch=-5.00°  yaw=30.00°   gx=10.0°/s  gy=0  gz=0
     *  固定值不跳动，方便在上位机仪表上直接看出“这就是冒烟帧”。
     * ---------------------------------------------------------------------- */
    HAL_UART_Transmit(&huart1,
                      (uint8_t *)"SMOKE: AA FF F1 0C -> roll10 pitch-5 yaw30\r\n",
                      strlen("SMOKE: AA FF F1 0C -> roll10 pitch-5 yaw30\r\n"), 1000);
    {
        uint8_t demo[12];
        const int16_t v[6] = { 1000, -500, 3000, 100, 0, 0 };
        uint8_t k;

        for (k = 0; k < 6u; k++)
        {
            demo[k * 2]     = (uint8_t)(v[k] & 0xFF);          /* 低字节在前（小端） */
            demo[k * 2 + 1] = (uint8_t)((v[k] >> 8) & 0xFF);
        }
        Ano_Send_Frame(ANO_FUNC_ATTITUDE, demo, 12u);
    }

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


  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* USER CODE END 3 */
  }
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

    /*  匿名上位机协议初始化 */
    Ano_Init();

    /* 姿态模块校准状态初始化（陀螺零偏 -> 翻转采样 -> 四元数初始化） */
    IMU_Calib_Init();

    /* PID 初始化：默认参数 + 历史状态清零 + TIM4 1MHz 计时（测 Δt） */
    PID_Init();
    PID_Time_Init();

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

    os_err = OSTaskCreate(PIDShellTask,
                          (void *)0,
                          &PIDShellTaskStk[PIDSHELL_TASK_SIZE - 1],
                          PIDSHELL_TASK_PRIO);
    OSTaskNameSet(PIDSHELL_TASK_PRIO, (INT8U *)"PIDShell Task", &os_err);

    os_err = OSTaskCreate(PIDCoreTask,
                          (void *)0,
                          &PIDCoreTaskStk[PIDCORE_TASK_SIZE - 1],
                          PIDCORE_TASK_PRIO);
    OSTaskNameSet(PIDCORE_TASK_PRIO, (INT8U *)"PIDCore Task", &os_err);

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
