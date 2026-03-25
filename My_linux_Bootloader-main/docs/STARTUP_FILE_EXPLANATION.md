# STM32F401RET6 启动文件 (Startup File) 详解

本文档详细解析了 `common/startup/startup_stm32f401xe.s` 文件。这是 STM32 微控制器上电复位后执行的第一段代码，采用 ARM 汇编语言编写。

## 1. 文件的作用

启动文件的主要职责是建立 C 语言运行环境，具体包括：

1. **初始化堆栈指针 (SP)**。
2. **初始化程序计数器 (PC)**，指向复位处理函数 (`Reset_Handler`)。
3. **初始化中断向量表**，设置各个异常和中断的处理函数地址。
4. **初始化系统时钟**（调用 `SystemInit`）。
5. **初始化数据段 (.data)**：从 Flash 复制初始值到 RAM。
6. **初始化 BSS 段 (.bss)**：将未初始化变量区域清零。
7. **调用静态构造函数**（针对 C++）。
8. **跳转到 main 函数**。

## 2. 汇编指令设置

```asm
  .syntax unified    /* 使用统一汇编语法 (ARM/Thumb) */
  .cpu cortex-m4     /* 指定目标处理器为 Cortex-M4 */
  .fpu softvfp       /* 指定浮点运算单元配置 */
  .thumb             /* 生成 Thumb 指令 */
```

## 3. 外部符号引用

这些符号在链接脚本 (`.ld`) 中定义，启动文件通过它们知道内存布局：

```asm
.word  _sidata       /* .data 段在 Flash 中的起始地址 (Source) */
.word  _sdata        /* .data 段在 RAM 中的起始地址 (Destination Start) */
.word  _edata        /* .data 段在 RAM 中的结束地址 (Destination End) */
.word  _sbss         /* .bss 段在 RAM 中的起始地址 */
.word  _ebss         /* .bss 段在 RAM 中的结束地址 */
```

## 4. 复位处理函数 (Reset\_Handler)

这是程序执行的入口。

### 4.1 设置堆栈和时钟

```asm
Reset_Handler:  
  ldr   sp, =_estack      /* 1. 设置主堆栈指针 (MSP) 到 RAM 顶端 */
  bl  SystemInit          /* 2. 调用 SystemInit 初始化系统时钟 */
```

### 4.2 数据段复制 (.data)

将已初始化的全局变量从 Flash 搬运到 RAM。

```asm
/* Copy the data segment initializers from flash to SRAM */  
  ldr r0, =_sdata         /* R0 = 目标地址 (RAM) */
  ldr r1, =_edata         /* R1 = 目标结束地址 */
  ldr r2, =_sidata        /* R2 = 源地址 (Flash) */
  movs r3, #0
  b LoopCopyDataInit

CopyDataInit:
  ldr r4, [r2, r3]        /* 从 Flash 读取数据 */
  str r4, [r0, r3]        /* 写入到 RAM */
  adds r3, r3, #4         /* 偏移量 + 4 */

LoopCopyDataInit:
  adds r4, r0, r3         /* 计算当前 RAM 地址 */
  cmp r4, r1              /* 检查是否到达结束地址 */
  bcc CopyDataInit        /* 如果未到达，继续循环 */
```

### 4.3 BSS 段清零 (.bss)

将未初始化的全局变量所在的 RAM 区域清零。

```asm
/* Zero fill the bss segment. */
  ldr r2, =_sbss          /* R2 = BSS 起始地址 */
  ldr r4, =_ebss          /* R4 = BSS 结束地址 */
  movs r3, #0
  b LoopFillZerobss

FillZerobss:
  str  r3, [r2]           /* 写入 0 */
  adds r2, r2, #4         /* 地址 + 4 */

LoopFillZerobss:
  cmp r2, r4              /* 检查是否到达结束地址 */
  bcc FillZerobss
```

### 4.4 进入 Main 函数

```asm
  bl __libc_init_array    /* 调用 C++ 静态构造函数 (如果使用标准库) */
  bl  main                /* 跳转到 main() 函数 */
```

## 5. 中断向量表 (Vector Table)

向量表必须放置在 Flash 的起始位置 (0x08000000)。前两个条目最为关键：

```asm
g_pfnVectors:
  .word  _estack          /* 0x00: 初始堆栈指针 (MSP) */
  .word  Reset_Handler    /* 0x04: 复位处理函数地址 (PC 初始值) */
  .word  NMI_Handler      /* 0x08: 不可屏蔽中断 */
  .word  HardFault_Handler/* 0x0C: 硬件错误 */
  ...
  .word  SysTick_Handler  /* 系统滴答定时器 */
  ...
  .word  USART1_IRQHandler /* 外设中断... */
```

当发生中断时，CPU 会根据中断号在表中查找对应的函数地址并跳转执行。

## 6. 弱定义 (Weak Definitions)

启动文件为所有中断处理函数提供了默认实现（死循环）。

```asm
    .section  .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b  Infinite_Loop
```

使用 `.weak` 声明，允许用户在 C 代码中重新定义同名函数来覆盖默认实现。例如，如果你在 C 代码中写了 `void SysTick_Handler(void) { ... }`，链接器就会使用你的版本，而不是这里的死循环。

```asm
  .weak  NMI_Handler
  .thumb_set NMI_Handler,Default_Handler
  /* ... 对所有中断都进行了类似的弱定义 ... */
```

