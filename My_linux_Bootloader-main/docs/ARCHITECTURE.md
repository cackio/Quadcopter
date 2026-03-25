# STM32 Bootloader - 架构与代码详情

## 目录

1. [系统概览](#system-overview)
2. [Bootloader 实现](#bootloader-implementation)
3. [应用程序实现](#application-implementation)
4. [启动序列](#startup-sequence)
5. [链接脚本详解](#linker-scripts-explained)
6. [内存布局详情](#memory-layout-in-detail)
7. [向量表管理](#vector-table-management)
8. [Bootloader 跳转机制](#bootloader-jump-mechanism)
9. [构建系统](#build-system)
10. [调试指南](#debugging-guide)

## 系统概览

本项目 STM32 Bootloader 实现了一个两阶段的固件架构：

```
复位 (Reset) → Bootloader (0x08000000) → 应用程序 (Application) (0x08010000)
```

### 为什么需要两阶段？

1. **现场更新 (Field Updates):** Bootloader 保持不变，应用程序可以更新。
2. **恢复 (Recovery):** 如果应用程序失败，Bootloader 可以进行恢复。
3. **安全性 (Security):** Bootloader 可以在运行前验证应用程序。
4. **灵活性 (Flexibility):** 可以交换多个应用程序。

## Bootloader 实现

### 入口点：`main.c`

位于 `bootloader/src/main.c`

#### 1. 系统初始化

```c
void system_init(void)
{
  /* 启用 HSI (内部 16MHz RC 振荡器) */
  RCC->CR |= RCC_CR_HSION;
  while (!(RCC->CR & RCC_CR_HSIRDY));
  
  /* 将所有时钟设置重置为默认值 */
  RCC->CFGR = 0x00000000;
  
  /* 禁用所有中断 */
  RCC->CIR = 0x00000000;
}
```

**为什么使用 HSI？**
- 内部振荡器，无需外部晶振。
- 复位后始终可用。
- 足以满足 Bootloader 的运行需求。

#### 2. 应用程序验证

```c
static int32_t validate_application(void)
{
  uint32_t app_stack_ptr = *(volatile uint32_t *)(APP_START_ADDR);
  uint32_t app_reset_handler = *(volatile uint32_t *)(APP_START_ADDR + 4);
  
  /* 验证栈指针是否指向 SRAM */
  if (app_stack_ptr < SRAM_BASE || app_stack_ptr > SRAM_END) {
    return BOOTLOADER_ERR_INVALID_APP;
  }
  
  /* 验证复位处理程序是否指向应用程序 Flash */
  if (app_reset_handler < APP_START_ADDR || app_reset_handler > APP_END_ADDR) {
    return BOOTLOADER_ERR_INVALID_APP;
  }
  
  return BOOTLOADER_OK;
}
```

**检查了什么？**

Cortex-M 向量表结构：
```
偏移 0: 初始栈指针 (MSP)
偏移 4: 复位处理程序地址 (Reset Handler)
偏移 8+: 中断处理程序
```

我们读取前两个条目并验证：
- 栈指针是否在有效的 SRAM 范围内 (0x20000000-0x2000FFFF)
- 复位处理程序是否指向应用程序 Flash 区域 (0x08010000-0x0803FFFF)

#### 3. 跳转

```c
static void bootloader_jump_to_app(void)
{
  typedef void (*pFunction)(void);
  
  uint32_t app_stack_ptr = *(volatile uint32_t *)(APP_START_ADDR);
  uint32_t app_reset_handler = *(volatile uint32_t *)(APP_START_ADDR + 4);
  
  /* 关键：在切换向量表之前禁用中断 */
  __disable_irq();
  
  /* 将主栈指针设置为应用程序的栈 */
  __set_MSP(app_stack_ptr);
  
  /* 重定位向量表 */
  SCB->VTOR = APP_START_ADDR;
  
  /* 重新启用中断 */
  __enable_irq();
  
  /* 跳转到应用程序 */
  pFunction app_entry = (pFunction)app_reset_handler;
  app_entry();
}
```

**为什么要禁用中断？**
- 防止在转换期间运行中断处理程序。
- 确保原子状态更改。
- 避免使用应用程序上下文调用 Bootloader 的 ISR。

**为什么要设置 MSP？**
- 每个程序都有自己的栈。
- 应用程序期望其栈指针在复位时已设置。
- 防止栈损坏。

**为什么要重定位 VTOR？**
- Cortex-M4 具有可配置的向量表偏移量。
- Bootloader: VTOR = 0x08000000
- 应用程序: VTOR = 0x08010000
- 中断处理程序必须来自应用程序的向量表。

## 应用程序实现

### 入口点：`main.c`

位于 `application/src/main.c`

#### 关键的第一步

```c
void main(void)
{
  /* 必须首先执行此操作！ */
  __disable_irq();
  SCB->VTOR = 0x08010000;  /* 应用程序向量表 */
  __enable_irq();
  
  /* 现在可以安全地初始化外设 */
  system_init();
  gpio_init();
  
  /* 应用程序循环 */
  while (1) {
    led_toggle();
    delay_ms(500);
  }
}
```

**为什么这是必要的？**

当 Bootloader 跳转到应用程序时：
1. VTOR 仍然指向 Bootloader 的向量表 (0x08000000)。
2. 应用程序的 ISR 位于 0x08010000。
3. 如果在 VTOR 更新之前触发中断，将运行错误的 ISR！
4. **导致崩溃或未定义行为**。

## 启动序列

### 上电复位流程

```
1. 硬件复位
   ↓
2. CPU 从 0x00000000 (0x08000000) 获取初始 SP
   ↓
3. CPU 从 0x00000004 获取复位向量
   ↓
4. Reset_Handler (位于 startup_stm32f401xc.s)
   ↓
5. 将 .data 段从 Flash 复制到 RAM
   ↓
6. 将 .bss 段清零
   ↓
7. 调用 SystemInit()
   ↓
8. 调用 __libc_init_array (C++ 构造函数)
   ↓
9. 调用 main()
   ↓
10. Bootloader 验证应用程序
   ↓
11. Bootloader 跳转到应用程序
   ↓
12. 应用程序 main() 运行
```

### 启动文件分析

文件：`common/startup/startup_stm32f401xc.s`

#### Reset_Handler

```assembly
Reset_Handler:
  ldr   sp, =_estack      /* 设置栈指针 */
  
  /* 将 .data 从 Flash 复制到 RAM */
  movs  r1, #0
  b     LoopCopyDataInit

CopyDataInit:
  ldr   r3, =_sidata      /* 源：Flash */
  ldr   r3, [r3, r1]
  str   r3, [r0, r1]      /* 目标：RAM */
  adds  r1, r1, #4

LoopCopyDataInit:
  ldr   r0, =_sdata
  ldr   r3, =_edata
  adds  r2, r0, r1
  cmp   r2, r3
  bcc   CopyDataInit
  
  /* 将 .bss 段清零 */
  ldr   r2, =_sbss
  ldr   r4, =_ebss
  movs  r3, #0

FillZerobss:
  str   r3, [r2], #4
  cmp   r2, r4
  bcc   FillZerobss

  /* 调用系统初始化 */
  bl  SystemInit
  
  /* 调用 C++ 构造函数 */
  bl __libc_init_array
  
  /* 调用 main */
  bl  main
  
  /* 不应返回 */
  bx  lr
```

**发生了什么？**

1. **设置栈指针：** 将 `_estack` (RAM 顶部) 加载到 SP。
2. **复制 .data：** 使用默认值初始化变量。
3. **清零 .bss：** 清除未初始化的变量。
4. **SystemInit：** 配置时钟等。
5. **Main：** 进入 C 代码。

## 链接脚本详解

### Bootloader 链接脚本

文件：`bootloader/ld/STM32F401VCTx_BOOTLOADER.ld`

```ld
MEMORY
{
  RAM (xrw)    : ORIGIN = 0x20000000,   LENGTH = 64K
  FLASH (rx)   : ORIGIN = 0x08000000,   LENGTH = 64K  /* Bootloader 区域 */
}
```

**关键段：**

```ld
.isr_vector :
{
  . = ALIGN(4);
  KEEP(*(.isr_vector))    /* 向量表位于 Flash 起始处 */
  . = ALIGN(4);
} >FLASH

.text :
{
  *(.text)                /* 代码 */
  *(.rodata*)             /* 常量 */
  _etext = .;
} >FLASH

.data :
{
  _sdata = .;
  *(.data)                /* 已初始化的数据 */
  _edata = .;
} >RAM AT> FLASH          /* 从 Flash 加载，在 RAM 中执行 */

.bss :
{
  _sbss = .;
  *(.bss)                 /* 未初始化的数据 */
  *(COMMON)
  _ebss = .;
} >RAM
```

**这些是什么意思？**

- `.isr_vector`: 中断向量表（必须位于 Flash 起始处）。
- `.text`: 程序代码（存储在 Flash 中）。
- `.data`: 已初始化的全局变量（启动时 Flash→RAM）。
- `.bss`: 未初始化的全局变量（启动时清零）。

### 应用程序链接脚本

文件：`application/ld/STM32F401VCTx_APPLICATION.ld`

```ld
MEMORY
{
  RAM (xrw)    : ORIGIN = 0x20000000,   LENGTH = 64K
  FLASH (rx)   : ORIGIN = 0x08010000,   LENGTH = 192K  /* 64KB 偏移！ */
}
```

**关键区别：**
- Bootloader 开始于 `0x08000000`
- 应用程序开始于 `0x08010000` (64KB 偏移)

这创建了两个独立的固件镜像！

## 内存布局详情

### Flash 内存映射

```
地址         大小    内容
--------   ----    --------------------------
0x08000000 64KB    Bootloader (固定)
  ├─ 0x0000       向量表
  ├─ 0x0100       Bootloader 代码
  └─ 0xFFFF       Bootloader 结束

0x08010000 192KB   应用程序 (可更新)
  ├─ 0x0000       向量表
  ├─ 0x0100       应用程序代码
  └─ 0x2FFFF      应用程序结束

0x08040000 -       未使用 (总共 256KB)
```

### RAM 使用情况

```
地址         大小    内容
--------   ----    --------------------------
0x20000000 64KB    主 SRAM
  ├─ 0x0000       Bootloader 栈 (顶部)
  ├─ 0x1000       Bootloader .data/.bss
  ├─ 0x2000       应用程序栈
  └─ 0x3FFF       应用程序 .data/.bss (底部)
```

Bootloader 和应用程序都使用相同的 RAM，但在不同的时间！

## 向量表管理

### 什么是向量表？

位于 Flash 起始处的函数指针数组：

```c
__attribute__((section(".isr_vector")))
const void* g_vector_table[] = {
  (void*)0x20010000,      // 初始栈指针
  Reset_Handler,          // 复位处理程序
  NMI_Handler,            // NMI 处理程序
  HardFault_Handler,      // Hard Fault 处理程序
  // ... 更多处理程序
};
```

### VTOR 寄存器

**向量表偏移寄存器** (VTOR) 告诉 CPU 在哪里可以找到向量表：

```
Bootloader 模式:  SCB->VTOR = 0x08000000
应用程序 模式:    SCB->VTOR = 0x08010000
```

当中断触发时：
1. CPU 读取 VTOR。
2. 加上中断号 × 4。
3. 获取处理程序地址。
4. 跳转到处理程序。

**这就是为什么 VTOR 必须匹配当前活动的固件！**

## Bootloader 跳转机制

### 详细步骤

```c
void bootloader_jump_to_app(void) {
  // 步骤 1: 读取应用程序的初始状态
  uint32_t app_sp = *(uint32_t*)(0x08010000 + 0x00);  // 栈指针
  uint32_t app_pc = *(uint32_t*)(0x08010000 + 0x04);  // 复位处理程序
  
  // 步骤 2: 禁用中断 (关键！)
  __disable_irq();
  
  // 步骤 3: 切换到应用程序的栈
  __set_MSP(app_sp);
  
  // 步骤 4: 指向应用程序的向量表
  SCB->VTOR = 0x08010000;
  
  // 步骤 5: 重新启用中断
  __enable_irq();
  
  // 步骤 6: 跳转到应用程序
  ((void(*)())app_pc)();
}
```

### CPU 发生了什么？

**跳转前：**
- PC = Bootloader 中的某处
- SP = Bootloader 的栈
- VTOR = Bootloader 的向量表
- 中断 = 已启用

**跳转后：**
- PC = 应用程序的复位处理程序
- SP = 应用程序的栈
- VTOR = 应用程序的向量表
- 中断 = 已启用

转换必须是 **原子的** - 中间没有中断！

## 构建系统

### Makefile 解析

```makefile
# 编译器
CC = arm-none-eabi-gcc

# Cortex-M4 的标志
MCU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# 包含路径
C_INCLUDES = -I../bootloader/inc -I../common/cmsis/core

# 编译
$(BUILD_DIR)/%.o: %.c
    $(CC) -c $(CFLAGS) $< -o $@

# 链接
$(BUILD_DIR)/bootloader.elf: $(OBJECTS)
    $(CC) $(OBJECTS) $(LDFLAGS) -o $@

# 创建二进制文件
$(BUILD_DIR)/bootloader.bin: $(BUILD_DIR)/bootloader.elf
    arm-none-eabi-objcopy -O binary $< $@
```

### 构建脚本

`build.sh` 编排构建过程：

```bash
build_bootloader() {
    cd bootloader
    make clean
    make all
    cd ..
}

build_application() {
    cd application
    make clean
    make all
    cd ..
}
```

## 调试指南

### 使用 GDB 和 OpenOCD

**设置：**
```bash
# 终端 1: 启动 OpenOCD
openocd -f tools/openocd_stm32f4.cfg

# 终端 2: 启动 GDB
arm-none-eabi-gdb bootloader/build/bootloader.elf
```

**常用 GDB 命令：**

```gdb
# 连接到目标
(gdb) target extended-remote :3333

# 加载固件
(gdb) load

# 设置断点
(gdb) break main

# 运行
(gdb) continue

# 单步执行
(gdb) step

# 显示寄存器
(gdb) info registers

# 检查内存
(gdb) x/10x 0x08000000

# 显示向量表
(gdb) x/10x 0x08010000
```

### 调试跳转

**问题：** Bootloader 跳转后应用程序崩溃。

**调试步骤：**

1. **检查应用程序向量表：**
   ```gdb
   (gdb) x/2x 0x08010000
   0x08010000:    0x20010000    0x08010189
   ```
   - 第一个值：栈指针 (应在 RAM 中)
   - 第二个值：复位处理程序 (应在应用程序 Flash 中)

2. **跳转前检查 VTOR：**
   ```gdb
   (gdb) print/x $systick
   (gdb) print/x SCB->VTOR
   ```

3. **跳转后检查 SP：**
   ```gdb
   (gdb) print $sp
   ```

4. **单步执行跳转：**
   ```gdb
   (gdb) break bootloader_jump_to_app
   (gdb) continue
   (gdb) stepi
   ```

### 常见问题

**问题 1：跳转后立即 HardFault**

原因：栈指针或复位处理程序无效。

解决方案：检查向量表有效性。
```gdb
(gdb) x/2x 0x08010000
```

**问题 2：中断崩溃**

原因：VTOR 未更新。

解决方案：确保在启用中断前设置 VTOR。
```c
__disable_irq();
SCB->VTOR = 0x08010000;  // 必须执行此操作！
__enable_irq();
```

**问题 3：应用程序无法启动**

原因：Bootloader 验证失败。

解决方案：检查应用程序是否为正确的地址构建。
```bash
arm-none-eabi-objdump -h application/build/application.elf
```

查找：
```
Idx Name          Size      VMA       LMA
  0 .isr_vector   0x000188  08010000  08010000
```

VMA 应该是 `08010000` 而不是 `08000000`！

---

## 结论

此架构为以下功能提供了坚实的基础：
- **无线更新 (Over-the-air updates)** (添加无线通信)
- **安全启动 (Secure boot)** (添加加密验证)
- **出厂重置 (Factory reset)** (在 Flash 中保留回退镜像)
- **多应用程序 (Multiple applications)** (在不同固件之间切换)

关键原则是：
1. **隔离 (Isolate)** Bootloader 和应用程序在不同的 Flash 区域。
2. **验证 (Validate)** 跳转前验证应用程序。
3. **小心 (Carefully)** 管理向量表和栈指针。
4. **确保 (Make)** 跳转是原子的，并禁用中断。

编码愉快！ 🚀
