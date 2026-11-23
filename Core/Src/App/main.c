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
#define UART_TASK_STK_SIZE     512   /* 2 KB，含 char data[220] 和 snprintf 的临时空间 */
#define BARO_TASK_STK_SIZE     256   /* 1 KB，只有 MS5611_Read_Raw */
#define START_TASK_STK_SIZE    384   /* 1.5 KB，含 BSP_Init + 创建三个任务 */
#define PIDSHELL_TASK_STK_SIZE     256   /* 1 KB，外环只有几次浮点运算 */
#define PIDCORE_TASK_STK_SIZE      256   /* 1 KB，内环只有几次浮点运算 */

#define START_TASK_PRIO        5
#define SENSOR_TASK_PRIO       6     
#define PIDSHELL_TASK_PRIO     7     
#define PIDCORE_TASK_PRIO      8     
#define UART_TASK_PRIO         9
#define BARO_TASK_PRIO         10

/* 慢任务周期（单位：tick） 当前 tick = 10ms
 * 采样/解算/内外环已经不看 tick 了，由 TIM5 的 2ms 中断驱动，见 SensorTask。 */
#define UART_TASK_PERIOD      5
#define BARO_TASK_PERIOD      20   /* 200ms */

/* 500Hz 节拍的分频：
 *   SensorTask 由 TIM5 每 2ms 唤醒一次；
 *   地磁每 10 拍读一次 = 50Hz（HMC5883L 本身只有 75Hz 输出，读快了是重复值）；
 *   外环每 5 拍跑一次 = 10ms（100Hz）。
 */
#define MAG_READ_DIV          10
#define OUTER_LOOP_DIV        5

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
OS_STK StartTaskStk[START_TASK_STK_SIZE];
OS_STK SensorTaskStk[SENSOR_TASK_STK_SIZE];
OS_STK UartTaskStk[UART_TASK_STK_SIZE];
OS_STK BaroTaskStk[BARO_TASK_STK_SIZE];
OS_STK PIDShellTaskStk[PIDSHELL_TASK_STK_SIZE];
OS_STK PIDCoreTaskStk[PIDCORE_TASK_STK_SIZE];

/*
 * 三个节拍信号量，把 2ms 中断串成一条链：
 *
 *   TIM5(2ms) --SensorSem--> SensorTask   采样 + 姿态解算
 *                                   |
 *                                   +--CoreSem--> PIDCoreTask   内环 2ms
 *
 *   TIM5 每第 5 拍另外发一次 ShellSem --> PIDShellTask  外环 10ms
 *
 * 外环任务优先级(7)高于内环(8)：第 5 拍时两个任务同时就绪，
 * 外环先跑完、把 exp_w 更新掉，然后自己阻塞在 ShellSem 上，
 * 内环才被调度到，拿到的就是这一拍刚算出来的角速度期望。
 */
OS_EVENT *SensorSem;
OS_EVENT *ShellSem;
OS_EVENT *CoreSem;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
void SystemClock_Config(void);
void StartTask(void *p_arg);
void SensorTask(void *p_arg);
void PIDShellTask(void *p_arg);
void PIDCoreTask(void *p_arg);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/*
 * TIM5 更新中断回调：只发信号量，不做别的。
 * 放在这里而不是 tim.c，是因为它要用 uC/OS-II 的信号量，属于应用层。
 *
 * 每 2ms 发一次 SensorSem（采样+姿态）；
 * 每第 5 次再发一次 ShellSem，也就是外环精确 10ms 一拍。
 * 用中断里自己数拍，而不是让外环任务 OSTimeDly，是因为
 * OSTimeDly 是“相对延时”，实际周期 = 10ms + 任务执行时间，会慢慢漂。
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static uint8_t tick_div = 0;

    if (htim->Instance == TIM5)
    {
        OSSemPost(SensorSem);

        if (++tick_div >= OUTER_LOOP_DIV)
        {
            tick_div = 0;
            OSSemPost(ShellSem);
        }
    }
}

void SensorTask(void *p_arg)
{
    INT8U   err;
    uint8_t mag_div = 0;   /* 地磁降频计数：10 拍读一次 = 50Hz */

    (void)p_arg;

    while (1)
    {
        /*
         * 等 TIM5 的 500Hz 节拍（2ms）。
         * 后面用 OSSemAccept 把积压的节拍清掉：万一某一拍超时，
         * 只补跑一次，不会越积越多、最后变成一串 dt≈0 的空转。
         * （不能用 OSSemPend(…,0,…) 来清，timeout=0 是“永久等待”，会卡死）
         */
        OSSemPend(SensorSem, 0, &err);
        while (OSSemAccept(SensorSem) > 0)
        {
            /* 只为清计数 */
        }

        /*
         * 1) 采样：MPU6050 每拍都读（2ms 一个新样本），
         *    地磁降频到 50Hz —— HMC5883L 输出率只有 75Hz，读快了是同一个值。
         *    气压计不在这里：MS5611_Read_Raw 内含两段 HAL_Delay(10)，
         *    放进 2ms 循环会直接把周期撑爆，它已经挪到 BaroTask。
         */
        if (!MPU6050_Read_Raw()) {
            IMU_Sample_Missed();
            OSSemPost(CoreSem);   /* 保持内环节拍，姿态保持在最近一次有效值 */
            continue;
        }
        if (++mag_div >= MAG_READ_DIV)
        {
            mag_div = 0;
            HMC5883L_Read_Raw();
        }

        /* 2) 姿态：校准期间走状态机，校准完成后走 Madgwick 融合 */
        if (IMU_Calib_Done())
        {
            IMU_Attitude_Update();
        }
        else
        {
            IMU_Calib_Update();
        }

        /* 3) 解算完再放内环这一拍的车：内环读到的就是刚更新的姿态 */
        OSSemPost(CoreSem);
    }
}

/*
 * 外环任务：10ms 一拍，由 TIM5 每第 5 拍发的 ShellSem 唤醒。
 * 遥控器期望 → 角度误差 → exp_w 角速度期望。
 * 优先级比内环高，所以第 5 拍能先于内环跑完。
 */
void PIDShellTask(void *p_arg)
{
    INT8U err;

    (void)p_arg;

    while (1)
    {
        OSSemPend(ShellSem, 0, &err);
        while (OSSemAccept(ShellSem) > 0)
        {
            /* 只为清计数 */
        }

        PID_PPM_to_Exp_Calc();                  /* 遥控器期望值计算 */
        PID_FlyMode_Update(expectation.thr);    /* 更新飞行模式 */

        if (PID_SHELL_MODE)
        {
            /* 通过四元数计算 */
            PID_Cascade_Shell_Calc_Quater();
        }
        else
        {
            /* 通过姿态角计算 */
            exp_w.roll  = PID_Cascade_Shell_Calc(expectation.roll  - euler_angle.roll  * RAD_TO_DEG, &roll_shell);
            exp_w.pitch = PID_Cascade_Shell_Calc(expectation.pitch - euler_angle.pitch * RAD_TO_DEG, &pitch_shell);
        }
    }
}

/*
 * 内环任务：2ms 一拍，由 SensorTask 在姿态解算完以后放行。
 * 角速度环 → 混控 → 写 PWM。
 */
void PIDCoreTask(void *p_arg)
{
    INT8U err;

    (void)p_arg;

    while (1)
    {
        OSSemPend(CoreSem, 0, &err);
        while (OSSemAccept(CoreSem) > 0)
        {
            /* 只为清计数 */
        }

        PID_Motor_Calc();
        PID_Motor_Output();
    }
}

/*
 * 气压计任务：和姿态彻底解耦，200ms 读一次。
 * 里面约 20ms 的阻塞（HAL_Delay 等 ADC 转换）只会占空闲时间，
 * 不影响 SensorTask 的 500Hz 姿态采样。
 */
void BaroTask(void *p_arg)
{
    (void)p_arg;

    while (1)
    {
        MS5611_Read_Raw();
        OSTimeDly(BARO_TASK_PERIOD);
    }
}
void UartTask(void *p_arg)
{
		OS_CPU_SR cpu_sr = 0;
    char data[220];
    static uint8_t last_stage = 0xFF;
    static uint8_t txt_div = 0;      /* 文本日志分频：20 × 50ms = 1s */
    static uint8_t led_div = 0;      /* LED 心跳分频：10 × 50ms = 0.5s */
    uint8_t stage, mag_fit, mag_used;

    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    int16_t mx, my, mz;
    uint32_t p_raw, t_raw;
    uint16_t throttle;
    float roll_deg, pitch_deg, yaw_deg;
    float gx_dps, gy_dps, gz_dps, dt_ms;

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
            else if (stage == 3)
            {
                msg = "CALIB: set the board down and keep STILL...\r\n";
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
         *
         *    这里连发三帧，0x03/0x04 是匿名协议内置的标准姿态帧：
         *      0xF1 自定义帧   → 姿态 + 陀螺仪，需在"用户数据帧"里手工配置
         *      0x03 标准帧     → 欧拉角，上位机"3D 姿态显示"直接认
         *      0x04 标准帧     → 四元数，大角度时比欧拉角连续
         *    三帧合计约 46 字节，20Hz 才 920 B/s，115200 波特率完全够。
         */
        Ano_Send_Attitude();
        Ano_Send_Euler();
        Ano_Send_Quater();

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

            mag_fit  = mag_calib_valid;
            mag_used = mag_yaw_used;
            mx = Mag_X_RAW;
            my = Mag_Y_RAW;
            mz = Mag_Z_RAW;

            p_raw = D1_Pressure_RAW;
            t_raw = D2_Temperature_RAW;

            throttle = PPM_Values[2];

            roll_deg  = euler_angle.roll  * RAD_TO_DEG;
            pitch_deg = euler_angle.pitch * RAD_TO_DEG;
            yaw_deg   = euler_angle.yaw   * RAD_TO_DEG;

            /* 折算成 °/s 和 ms，方便直接和手上的动作对比：
             * 转 90°/s 时这里应该读到 90 左右，dt 应该稳定在 2.0 ms 附近。 */
            gx_dps = gyro_body.x * RAD_TO_DEG;
            gy_dps = gyro_body.y * RAD_TO_DEG;
            gz_dps = gyro_body.z * RAD_TO_DEG;
            dt_ms  = imu_dt * 1000.0f;

            OS_EXIT_CRITICAL();

            snprintf(data, sizeof(data),
                     "THR:%u RX:%u\r\n"
                     "Euler:%d,%d,%d\r\n"
                     "Accel:%d,%d,%d\r\n"
                     "Gyro:%d,%d,%d\r\n"
                     "Mag:%d,%d,%d fit:%u used:%u\r\n"
                     "Rate:%.0f,%.0f,%.0f dps  dt:%.1fms\r\n"
                     "Pre_RAW:%lu Tem_RAW:%lu\r\n\r\n",
                     throttle,
                     (unsigned)ano_rx_frames,
                     (int)roll_deg, (int)pitch_deg, (int)yaw_deg,
                     ax, ay, az,
                     gx, gy, gz,
                     mx, my, mz, (unsigned)mag_fit, (unsigned)mag_used,
                     gx_dps, gy_dps, gz_dps, dt_ms,
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
    MX_TIM5_Init();     /* 500Hz 控制节拍，中断里只发信号量 */

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

    /* ---- 节拍信号量（TIM5 中断里 Post，任务里 Pend）---- */
    SensorSem = OSSemCreate(0);
    ShellSem  = OSSemCreate(0);
    CoreSem   = OSSemCreate(0);
    if (SensorSem == (OS_EVENT *)0 || ShellSem == (OS_EVENT *)0 || CoreSem == (OS_EVENT *)0) {
        Error_Handler();
    }

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

    os_err = OSTaskCreate(SensorTask,
                          (void *)0,
                          &SensorTaskStk[SENSOR_TASK_STK_SIZE - 1],
                          SENSOR_TASK_PRIO);
    OSTaskNameSet(SENSOR_TASK_PRIO, (INT8U *)"Sensor Task", &os_err);

    os_err = OSTaskCreate(PIDShellTask,
                          (void *)0,
                          &PIDShellTaskStk[PIDSHELL_TASK_STK_SIZE - 1],
                          PIDSHELL_TASK_PRIO);
    OSTaskNameSet(PIDSHELL_TASK_PRIO, (INT8U *)"PID Shell Task", &os_err);

    os_err = OSTaskCreate(PIDCoreTask,
                          (void *)0,
                          &PIDCoreTaskStk[PIDCORE_TASK_STK_SIZE - 1],
                          PIDCORE_TASK_PRIO);
    OSTaskNameSet(PIDCORE_TASK_PRIO, (INT8U *)"PID Core Task", &os_err);

    os_err = OSTaskCreate(UartTask,
                          (void *)0,
                          &UartTaskStk[UART_TASK_STK_SIZE - 1],
                          UART_TASK_PRIO);
    OSTaskNameSet(UART_TASK_PRIO, (INT8U *)"Uart Task", &os_err);

    os_err = OSTaskCreate(BaroTask,
                          (void *)0,
                          &BaroTaskStk[BARO_TASK_STK_SIZE - 1],
                          BARO_TASK_PRIO);
    OSTaskNameSet(BARO_TASK_PRIO, (INT8U *)"Baro Task", &os_err);

    /* 4. 启动 500Hz 节拍（放在任务创建之后，保证第一个节拍一定有人接）*/
    HAL_TIM_Base_Start_IT(&htim5);

    /* 5. 初始化任务完成，自我删除 */
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
