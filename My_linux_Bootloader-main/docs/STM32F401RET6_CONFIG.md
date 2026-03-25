# STM32F401RET6 配置说明

## 芯片规格

- **型号**: STM32F401RET6
- **内核**: ARM Cortex-M4F
- **主频**: 84 MHz
- **Flash**: 512 KB
- **SRAM**: 96 KB
- **封装**: LQFP64
- **电压**: 1.7V - 3.6V

## 内存布局

### Flash 分配 (512KB)

```
地址范围          大小     用途
--------------   ------   ------------------
0x08000000       64 KB    Bootloader (固定)
0x08010000       448 KB   Application (可更新)
0x08080000       -        Flash 结束
```

### SRAM 分配 (96KB)

```
地址范围          大小     用途
--------------   ------   ------------------
0x20000000       96 KB    主 SRAM
0x20018000       -        SRAM 结束
```

<br />

## 开发板

### Nucleo F401RE

ST官方开发板，板载ST-Link调试器。

**连接方式:**

- 板载ST-Link通过USB连接到电脑
- 无需额外的调试器

**LED位置:**

- LD2 (绿色): PA5 - 用户LED
- LD1 (红色): 电源指示
- LD3 (红色): USB通信指示

### Black Pill (如果有512KB版本)

一些Black Pill板子使用STM32F401RET6。

**注意事项:**

- 确认芯片型号
- 可能需要外接ST-Link

## GPIO 引脚分配

### Bootloader 使用的引脚

- **PA5**: LED输出（状态指示）

### Application 使用的引脚

- **PA5**: LED输出（慢速闪烁）
- **PA2**: USART2\_TX（可选，用于调试输出）
- **PA3**: USART2\_RX（可选）

### 可用的外设引脚

更多可用GPIO可以参考Nucleo F401RE的原理图或STM32F401RET6数据手册。

## 编译配置

项目已配置为使用正确的内存大小：

- Flash: 512 KB
- SRAM: 96 KB

所有链接脚本和头文件已更新。

## LED 状态指示

对于Nucleo F401RE：

| LED      | 状态           | 含义            |
| -------- | ------------ | ------------- |
| LD2 (绿色) | 快速闪烁 (2Hz)   | Bootloader运行  |
| LD2 (绿色) | 慢速闪烁 (0.5Hz) | Application运行 |
| LD2 (绿色) | 常亮           | 错误            |
| LD2 (绿色) | 不亮           | 硬件问题          |

## 调试

### 使用板载ST-Link

Nucleo F401RE板载ST-Link，使用方法：

```bash
# 启动OpenOCD
openocd -f tools/openocd_stm32f4.cfg

# 烧录
./tools/flash_all.sh

# 调试
./tools/debug_bootloader.sh
# 或
./tools/debug_application.sh
```

### ST-Link连接

ST-Link通过板载连接：

- SWDIO
- SWCLK
- GND
- 3.3V
- NRST (可选)

## 常见问题

### Q: 为什么我的板子LED不闪烁？

A: 确认以下几点：

1. 检查芯片型号是否为STM32F401RET6
2. 确认LD2（PA5）LED是否焊接
3. 检查电源是否正常
4. 使用ST-Link Utility检查固件是否烧录成功

### Q: 如何确认芯片型号？

A: 查看芯片表面的丝印：

```
STM32F401 RET6
       ^    ^
       |    |
       |    +-- Flash大小 (512KB)
       +------- 温度范围
```

或者使用ST-Link读取：

```
st-info --probe
```

### Q: 可以在Black Pill上使用吗？

A: 如果Black Pill使用STM32F401RET6（不是CCU6），可以直接使用。请确认芯片型号。

## 性能优化

### 时钟配置

默认使用HSI (16MHz)，可以修改为使用HSE获得更高性能（84MHz）。

### Flash等待周期

STM32F401在84MHz需要2个Flash等待周期。

### 功耗

如果需要低功耗，可以：

1. 降低系统时钟
2. 使用睡眠模式
3. 关闭不用的外设时钟

## 参考资源

- [STM32F401RE数据手册](https://www.st.com/resource/en/datasheet/stm32f401re.pdf)
- [STM32F401参考手册](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbxc-and-stm32f401xdxe-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [Nucleo F401RE用户手册](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf)

## 更新日志

### 2025-03-10

- 从STM32F401CCU6迁移到STM32F401RET6
- 更新Flash大小：256KB → 512KB
- 更新SRAM大小：64KB → 96KB
- 更新所有链接脚本和配置文件

