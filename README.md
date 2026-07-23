# Quadcopter 四轴飞行器

> 基于 STM32F401 + uC/OS-II + SEGGER SystemView 的飞控固件

[![MCU](https://img.shields.io/badge/MCU-STM32F401RE-03234B?logo=stmicroelectronics)](https://www.st.com/)
[![RTOS](https://img.shields.io/badge/RTOS-uC/OS--II-orange)](https://westes.github.io/ucos/)
[![Toolchain](https://img.shields.io/badge/Toolchain-Keil%20MDK%20%2B%20ARMCLANG%20V6-blue)](https://www.keil.com/)
[![Trace](https://img.shields.io/badge/Trace-SEGGER%20SystemView-green)](https://www.segger.com/products/development-tools/systemview/)
[![Status](https://img.shields.io/badge/Status-底层骨架%20完成-yellow)]()

---

## 📋 项目简介

本项目实现四轴飞行器飞控的**底层骨架**，已完成：

- 多任务实时调度（uC/OS-II）
- 传感器数据采集（IMU / 磁力计 / 气压计）
- 遥控器 PPM 信号解析
- 电机 PWM 输出
- 任务级调试信息打印（UART）
- SEGGER SystemView 实时跟踪

**未完成**（下一阶段）：姿态解算（Mahony / 四元数）、PID 控制环、飞行模式切换。

---

## 🛠️ 硬件平台

| 模块 | 型号 | 接口 |
|------|------|------|
| MCU | STM32F401RETx (Cortex-M4, 84 MHz) | — |
| IMU | MPU6050 (6 轴) | I2C1 |
| 磁力计 | HMC5883L (3 轴, MPU6050 旁路) | I2C1 |
| 气压计 | MS5611 | I2C1 |
| 遥控 | T8FB (PPM) | TIM2_CH1 |
| 电机 | ×4 | TIM3_CH1~4 |
| 调试串口 | USART1 | 9600 8N1 |
| 调试器 | J-Link / ST-Link | SWD |

### 时钟配置

- HSE = 8 MHz → PLL × 84 → SYSCLK = **84 MHz**
- APB1 = 42 MHz, APB2 = 84 MHz
- TIM3 (电机 PWM): 50 Hz, 周期 20 ms
- TIM2 (PPM 捕获): 1 MHz 计数频率

---

## ⚙️ 软件架构

### 任务划分

| 任务 | 优先级 | 周期 | 功能 |
|------|--------|------|------|
| `StartTask` | 5 | 一次性 | BSP 初始化 + 创建子任务后自删 |
| `MotorTask` | 7 | 20 ms | 读取油门 → 输出 4 路 PWM |
| `SensorTask` | 8 | 50 ms | 轮询 MPU6050 / HMC5883L / MS5611 |
| `UartTask` | 9 | 500 ms | 串口打印传感器原始数据 |

> uC/OS-II 中**优先级数字越小，优先级越高**（0 最高，`OS_LOWEST_PRIO` 最低）。
> 共享变量（如 `PPM_Values`、`Accel_X_RAW`）通过 `OS_ENTER_CRITICAL()` / `OS_EXIT_CRITICAL()` 保护。

### 软件栈

```
┌────────────────────────────────────────┐
│        应用任务层 (uC/OS-II Tasks)       │
│  StartTask / MotorTask / SensorTask /  │
│              UartTask                  │
├────────────────────────────────────────┤
│   Board Support Package  (bsp.c/h)     │
│  MPU6050 / HMC5883L / MS5611 / Motor / │
│  PPM / DWT                             │
├────────────────────────────────────────┤
│   STM32 HAL + CMSIS                    │
├────────────────────────────────────────┤
│   SEGGER RTT  +  SystemView  (Trace)   │
└────────────────────────────────────────┘
```

---

## 📂 目录结构

```
Quadcopter/
├── Core/                  # 应用层 (main.c / bsp.c / ...)
├── Drivers/               # HAL + CMSIS
├── ucos/                  # uC/OS-II 移植
├── SEGGER/                # RTT + SystemView
└── MDK-ARM/               # Keil 工程
    └── hello.uvprojx
```

---

## 🚀 快速开始

### 编译

1. 用 **Keil MDK V5.30+** 打开 `MDK-ARM/hello.uvprojx`
2. 编译器选择 **ARMCLANG V6**（项目默认）
3. `F7` 编译

> ⚠️ **常见错误**：`SEGGER_RTT_ConfDefaults.h` 找不到 → 从 [SEGGER 官方 RTT 仓库](https://github.com/SEGGERMicro/RTT) 下载该文件，放入 `SEGGER/RTT/` 目录。

### 烧录 & 运行

- 用 **J-Link** 或 **ST-Link** 连接 SWD 接口
- Keil 中 `F8` 下载 `hello.axf`
- 打开串口工具（9600 8N1）观察打印：

```
BOOT OK
BSP Init OK
UartTask running
THR:1000
Accel:0,0,16384
Gyro:0,0,0
Mag:0,0,0
Pre_RAW:0 Tem_RAW:0
...
```

---

## 📊 SystemView 调试

SystemView 用于实时观察任务调度、ISR、中断响应时间。

### 上位机

1. 打开 [SEGGER SystemView](https://www.segger.com/downloads/systemview/)
2. 菜单 `Target → Recorder Configuration`：
   - Target Connection: **J-Link**
   - Interface: **SWD**
   - Target Device: **STM32F401RE**
3. 点击 `Start Recording` 开始录制

---

## ✅ 后续路线

- [ ] 姿态解算（互补滤波 / Mahony / EKF）
- [ ] 串级 PID（角度环 + 角速度环）
- [ ] 飞行模式（自稳 / 定高 / 定点）
- [ ] 安全保护（信号丢失自动降落）

---

## 许可

本项目为本科综合设计用途。HAL/CMSIS 来自 ST 官方，uC/OS-II 来自 Micrium，SystemView 来自 SEGGER —— 各自遵循对应许可。
