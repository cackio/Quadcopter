# STM32F401RET6 Linker Script 详解

本文档详细解释了 `bootloader/ld/STM32F401RET6_BOOTLOADER.ld` 链接脚本的各个组成部分。该脚本用于控制程序的内存布局，告诉链接器如何将编译生成的代码和数据放置在 STM32F401RET6 的 Flash 和 RAM 中。

## 1. 入口点与栈顶定义

```ld
/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM); /* end of "RAM" Ram type memory */
```

*   **`ENTRY(Reset_Handler)`**: 指定程序的入口点。复位后，CPU 会从 `Reset_Handler` 开始执行。
*   **`_estack`**: 定义栈顶地址。在 ARM Cortex-M 中，栈通常是向下生长的，因此栈顶通常设为 RAM 的最高地址（RAM 起始地址 + RAM 长度）。

## 2. 堆栈大小配置

```ld
_Min_Heap_Size = 0x200;      /* required amount of heap  */
_Min_Stack_Size = 0x400;     /* required amount of stack */
```

*   **`_Min_Heap_Size`**: 预留给堆（Heap）的最小空间 (512 Bytes)，用于 `malloc` 等动态内存分配。
*   **`_Min_Stack_Size`**: 预留给栈（Stack）的最小空间 (1024 Bytes)，用于函数调用和局部变量。链接器会检查剩余 RAM 是否足够容纳这些空间。

## 3. 内存区域定义 (MEMORY)

```ld
MEMORY
{
  RAM (xrw)    : ORIGIN = 0x20000000,   LENGTH = 96K
  FLASH (rx)   : ORIGIN = 0x08000000,   LENGTH = 64K  /* Bootloader region: first 64KB */
}
```

定义了芯片的物理存储区域：
*   **`RAM`**: 起始地址 `0x20000000`，大小 96KB。属性 `xrw` 表示可执行 (Execute)、可读 (Read)、可写 (Write)。
*   **`FLASH`**: 起始地址 `0x08000000`，大小 64KB。属性 `rx` 表示可读、可执行。
    *   **注意**: 这里的 Flash 大小被限制为 64KB，说明这是一个 Bootloader 项目，只占用了 Flash 的前一部分。

## 4. 输出段定义 (SECTIONS)

`SECTIONS` 块定义了输入段（来自 .o 文件）如何映射到输出段（最终的 .elf/.bin 文件），以及它们在内存中的位置。

### 4.1 中断向量表 (.isr_vector)

```ld
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))    /* Startup code */
    . = ALIGN(4);
  } >FLASH
```

*   **`KEEP(*(.isr_vector))`**: 强制保留中断向量表，即使未被引用也不优化掉。
*   **`>FLASH`**: 这一段直接存放在 FLASH 中。中断向量表必须位于 Flash 的起始位置（0x08000000），以便 CPU 启动时能找到复位向量。

### 4.2 代码段 (.text)

```ld
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH
```

*   包含所有的程序代码 (`.text`)。
*   `_etext`: 定义了一个全局符号，标记代码段的结束位置。

### 4.3 只读数据段 (.rodata)

```ld
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH
```

*   包含 `const` 变量和字符串字面量等只读数据。存放于 FLASH。

### 4.4 初始化数据段 (.data)

这是最关键的段之一，涉及到 **LMA (Load Memory Address)** 和 **VMA (Virtual Memory Address)** 的区别。

```ld
  _sidata = LOADADDR(.data);

  .data : 
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM AT> FLASH
```

*   **`>RAM AT> FLASH`**:
    *   **VMA (运行地址)**: `RAM`。程序运行时，变量位于 RAM 中，可以读写。
    *   **LMA (加载地址)**: `FLASH`。程序烧录时，初始值存储在 Flash 中。
*   **`_sidata = LOADADDR(.data)`**: 获取 `.data` 段在 Flash 中的起始地址。
*   **启动代码的作用**: 在系统启动（`Reset_Handler`）时，启动代码会把数据从 `_sidata` (Flash) 复制到 `_sdata` 到 `_edata` (RAM) 的区域，完成全局变量的初始化。

### 4.5 未初始化数据段 (.bss)

```ld
  .bss :
  {
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM
```

*   包含未初始化或初始化为 0 的全局变量。
*   **只占用 RAM，不占用 Flash**（除了段头信息）。
*   **启动代码的作用**: 启动时将 `_sbss` 到 `_ebss` 之间的 RAM 区域清零。

### 4.6 堆栈检查段 (._user_heap_stack)

```ld
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM
```

*   这是一个虚拟段，不包含实际数据。
*   它的作用是检查 RAM 剩余空间是否足够容纳最小堆 (`_Min_Heap_Size`) 和最小栈 (`_Min_Stack_Size`)。如果 RAM 满了，链接器会报错。

## 总结

链接脚本定义了：
1.  **Flash 布局**: 中断向量表 -> 代码 -> 只读数据 -> 初始化数据的初始值。
2.  **RAM 布局**: 初始化数据 (运行副本) ->未初始化数据 (BSS) -> 堆 -> 栈。
3.  **数据搬运**: 通过定义 `_sidata`, `_sdata`, `_edata`, `_sbss`, `_ebss` 等符号，协助启动代码完成变量初始化。
