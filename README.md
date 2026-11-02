# Quadcopter 姿态解算调试工程

STM32F401RE（84 MHz）＋μC/OS-II＋MPU6050/HMC5883L＋JDY-31-SPP。

当前版本完成 Madgwick 六轴／九轴融合、定时采样、匿名 V7 上报与电脑端校准工具。**用于拆桨台架，四路电机信号强制低电平，不提供解锁或飞行控制。**

## 先看这里

- [姿态解算调试手册](docs/姿态解算调试手册.md)：从固定帧、原始数据、轴映射、六轴到磁场校准和九轴的完整步骤。
- [软件验证记录](docs/软件验证记录.md)：编译、自动测试及尚未做的实机验证。
- `Core/Inc/flight_config.h`：串口、算法、上报模式和增益。
- `Core/Inc/sensor_calibration.h`：原生坐标标定参数与两个独立的轴映射矩阵。

## 默认配置

| 功能 | 配置 |
|---|---|
| MPU6050／姿态任务 | 200 Hz，TIM4 事件唤醒，优先级 7 |
| 时间源 | TIM2 1 MHz，支持 32 位微秒回绕 |
| 磁力计 | HMC5883L H43，30 Hz，检查新数据 |
| 蓝牙 UART | USART1 PA9/PA10，9600 8N1 |
| 普通上报 | 匿名 V7 欧拉角 20 Hz；115200 时欧拉角和四元数各 50 Hz |
| 诊断模式 | 私有二进制帧 10 Hz，电脑工具解码、保存和回放 |
| 启动校准 | 约 1 秒预热＋5 秒静止陀螺仪校准，移动则重新累计 |
| 初始融合 | 六轴；安装方向和磁场参数未确认，九轴不会默认启用 |
| PPM | TIM2_CH1 PA0，仅监测 4–8 通道完整帧，100 ms 过期 |
| 电机／气压计 | 电机信号关闭；气压计不进入本次采样链路 |

原 SensorTask/MotorTask/UartTask 与散落的传感器全局量，已替换为姿态任务、遥测任务和 `Flight_GetSnapshot()` 一致快照。I2C 只由姿态任务访问。蓝牙上不再混发启动字符串，日志走 RTT 通道 0。

## 编译与检查

Keil 打开 `MDK-ARM/hello.uvprojx`，使用 ARMCLANG V6，F7 编译。也可在工程根目录运行：

```powershell
python tools/build_firmware.py
python tools/build_firmware.py --name fixed9600 --define FLIGHT_TELEMETRY_MODE=2
python tools/build_firmware.py --name diag9600 --define FLIGHT_TELEMETRY_MODE=1
python tools/run_tests.py
```

命令行生成 `build/<name>/quadcopter.axf`、`.hex`、`.map`、`build.log`，不自动烧录。`--define` 不修改工程头文件，Keil F8 也不会自动下载这些输出目录中的文件，务必确认本次下载的产物。

固定帧模式先验证电脑连接；诊断模式使用：

```powershell
python tools/capture.py --list
python tools/capture.py --port COM12 --seconds 150 --output captures/mag.csv
python tools/calibrate.py mag --input captures/mag.csv
```

COM12 是先前查到的设备记录，现场重新核对。生成参数默认写入 `build/calibration/sensor_calibration.h`，检查质量后再复制到工程，不能将默认单位矩阵当作你们的真实标定结果。

## 目录与依赖

`Core/` 为应用与驱动，`ucos/` 为内核和移植，`SEGGER/` 保留 RTT/SystemView，`tools/` 为构建与校准工具，`tests/` 对实际 C 实现进行电脑端测试。Python 依赖见 `tools/requirements.txt`；本机原生测试编译器为 Qt 附带的 MinGW，可用环境变量 `QUADCOPTER_HOST_CC` 指定其他 gcc。

HAL/CMSIS、μC/OS-II、SEGGER 代码保留原有许可。姿态算法依据 Madgwick 梯度下降方法实现，使用显式 dt；本版本尚未完成机架实测或飞行验收。
