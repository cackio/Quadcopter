# STM32F401 Bootloader 项目

这是一个在 Linux 环境下，使用 GCC 工具链和 ST-Link 进行编程的 STM32F401CCU6 微控制器完整两阶段 Bootloader 实现。

## 特性

- **两阶段 Bootloader 架构**：具有独立的内存区域
- **简单的 Bootloader**：验证并跳转到应用程序
- **应用程序**：带有 LED 状态指示
- **完整的 Linux 工具链支持**：无需 Keil
- **GDB + OpenOCD**：用于调试和烧录
- **WSL 兼容**：适用于 Windows 用户 (附带 USBIPD 指南)

## 硬件

- **MCU**：STM32F401RET6 (Cortex-M4, 84MHz, 512KB Flash, 96KB SRAM)
- **调试器**：ST-Link V2/V3 (板载或外接)
- **LED**：PA5 上的内置 LED (STM32 开发板常见配置)

## 内存布局

```
Flash Memory (512KB):
  0x08000000 - 0x0800FFFF  Bootloader 区域 (64KB)
  0x08010000 - 0x0807FFFF  应用程序区域 (448KB)

SRAM (96KB):
  0x20000000 - 0x20017FFF  主 SRAM
```

## 项目结构

```
Linux_Bootloader/
├── bootloader/              # Bootloader 代码
│   ├── src/                # 源文件
│   ├── inc/                # 头文件
│   ├── ld/                 # 链接脚本
│   └── Makefile
├── application/             # 应用程序代码
│   ├── src/
│   ├── inc/
│   ├── ld/
│   └── Makefile
├── common/                  # 通用文件
│   ├── startup/            # 启动汇编代码
│   └── cmsis/              # CMSIS 头文件
├── tools/                   # 烧录和调试脚本
│   ├── openocd_stm32f4.cfg
│   ├── flash_bootloader.sh
│   ├── flash_application.sh
│   ├── flash_all.sh
│   ├── debug_bootloader.sh
│   └── debug_application.sh
├── build.sh                 # 主构建脚本
├── AGENTS.md               # Agent 编码指南
└── README.md               # 本文件
```

## 要求

### 工具链安装

```bash
# Ubuntu/Debian/WSL
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi openocd

# 验证安装
arm-none-eabi-gcc --version
openocd --version
```
### 工具链安装

```bash
# Ubuntu/Debian/WSL
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi openocd

# 验证安装
arm-none-eabi-gcc --version
openocd --version
```

### WSL 环境配置 (Windows 用户)

如果你在 Windows 上使用 WSL，需要安装 `usbipd` 将 USB 设备（ST-Link）传递给 WSL。

1. **在 Windows 上安装 usbipd** (使用 PowerShell 管理员模式):
   ```powershell
   winget install usbipd
   ```
2. **列出 USB 设备**:
   ```powershell
   usbipd list
   ```
3. **绑定设备到 WSL**:
   ```powershell
   # 找到 ST-Link 的 BUSID (例如 2-4)
   usbipd attach --wsl --busid <BUSID>
   ```
4. **在 WSL 中验证**:
   ```bash
   lsusb
   # 应该能看到 STMicroelectronics ST-LINK/V2.1
   ```

### 硬件连接

- STM32F401 开发板 (例如：STM32F401CCU6 Black Pill / Nucleo)
- ST-Link 调试器
- USB 线

## 快速开始

### 1. 构建所有内容

```bash
./build.sh all
```

这将编译 Bootloader 和应用程序，生成：
- `bootloader/build/bootloader.bin` - Bootloader 二进制文件
- `application/build/application.bin` - 应用程序二进制文件

### 2. 烧录到设备

无需单独启动 OpenOCD，直接运行烧录脚本即可（脚本会自动处理 OpenOCD 连接）：

> **注意 (WSL 用户)**: 烧录脚本使用 `sudo` 来访问 USB 设备，运行时可能需要输入您的 WSL 密码。

```bash
# 烧录 Bootloader 和应用程序 (推荐)
./tools/flash_all.sh
```

或者单独烧录：

```bash
# 仅烧录 Bootloader
./tools/flash_bootloader.sh

# 仅烧录应用程序
./tools/flash_application.sh
```

### 3. 运行

按下板子上的 RESET 按钮。你应该会看到：

1. **快速闪烁 (2Hz)** - Bootloader 正在运行，正在验证应用程序
2. **约 1 秒后：** LED 变为 **慢速闪烁 (0.5Hz)** - 应用程序正在运行

### 4. 调试

调试时需要保持 OpenOCD 在后台运行。

**调试 Bootloader：**

```bash
# 终端 1：启动 OpenOCD (需要 sudo 权限以访问 USB)
sudo openocd -f tools/openocd_stm32f4.cfg

# 终端 2：启动 GDB
./tools/debug_bootloader.sh
```

**调试应用程序：**

```bash
# 终端 1：启动 OpenOCD (需要 sudo 权限以访问 USB)
sudo openocd -f tools/openocd_stm32f4.cfg

# 终端 2：启动 GDB
./tools/debug_application.sh
```

## LED 状态码

| LED 行为 | 含义 |
|-------------|---------|
| 快速闪烁 (2Hz) | Bootloader 运行中，等待超时 |
| 慢速闪烁 (0.5Hz) | 应用程序成功运行 |
| 常亮 | 错误 - 未找到有效的应用程序 (校验失败) |
| 熄灭 | 硬件/电源问题 |

## 构建命令

```bash
# 构建所有组件
./build.sh all

# 仅构建 Bootloader
./build.sh bootloader

# 仅构建应用程序
./build.sh application

# 清理构建产物
./build.sh clean
```

## Bootloader 运行机制

Bootloader 执行以下步骤：

1. **系统初始化**
   - 配置系统时钟 (HSI 16MHz)
   - 初始化 LED 的 GPIO
   - 启用中断

2. **应用程序验证**
   - 从应用程序向量表 (0x08010000) 读取栈指针
   - 从应用程序向量表 (0x08010004) 读取复位处理程序
   - 验证栈指针是否指向 SRAM 范围 (`SRAM_BASE` ~ `SRAM_END`)
   - 验证复位处理程序是否指向应用程序 Flash 区域

3. **跳转到应用程序**
   - 禁用中断
   - 将主栈指针设置为应用程序的栈
   - 将向量表重定位到应用程序区域 (SCB->VTOR = 0x08010000)
   - 启用中断
   - 跳转到应用程序的复位处理程序

## 故障排除

### Bootloader 后应用程序不启动

**问题：** LED 持续快速闪烁 (Bootloader 从未跳转)

**解决方案：**
1. 检查应用程序链接脚本是否有正确的 ORIGIN (0x08010000)
2. 验证应用程序二进制文件是否正确烧录：
   ```bash
   arm-none-eabi-objdump -h application/build/application.elf
   ```

### 无法通过 ST-Link 烧录

**问题：** OpenOCD/GDB 无法连接

**解决方案：**
1. **WSL 用户**：确保已通过 `usbipd` 挂载 USB 设备。
2. 检查 ST-Link 连接线。
3. 尝试按住复位键的同时运行烧录脚本。
4. 如果遇到 "Address already in use" 错误，请运行 `pkill openocd`。

### OpenOCD 端口占用

**问题：** `Error: couldn't bind gdb to socket on port 3333: Address already in use`

**解决方案：**
这意味着 OpenOCD 已经在运行了。杀死旧进程：
```bash
pgrep -a openocd
pkill openocd
```

## 开发

### 向 Bootloader 添加功能

要向 Bootloader 添加新功能：

1. 编辑 `bootloader/src/main.c`
2. 如果需要，更新 `bootloader/inc/main.h`
3. 重新构建：
   ```bash
   ./build.sh bootloader
   ```

### 向应用程序添加功能

要向应用程序添加新功能：

1. 编辑 `application/src/main.c`
2. 如果需要，更新 `application/inc/main.h`
3. 重新构建：
   ```bash
   ./build.sh application
   ```

## 资源

- [STM32F401 参考手册 (RM0368)](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbxc-and-stm32f401xdxe-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [ARM Cortex-M4 编程手册 (PM0214)](https://developer.arm.com/documentation/ddi0439/latest/)
- [STM32F4 CMSIS](https://github.com/STMicroelectronics/cmsis_device_f4)
- [GNU LD 链接脚本](https://sourceware.org/binutils/docs/ld/Scripts.html)

## 许可证

本项目按“原样”提供，用于教育目的。

## 更新日志

### 版本 1.0.1 (2025-03-11)
- **修复**: 修正了 Bootloader 中的栈指针校验逻辑 (`main.c`)。
- **改进**: 优化了烧录脚本 (`flash_all.sh`)，现在直接调用 OpenOCD 而不需要手动开启服务。
- **文档**: 添加了 WSL 环境下的 USBIPD 配置指南。

### 版本 1.0.0 (2025-03-10)
- 初始发布
