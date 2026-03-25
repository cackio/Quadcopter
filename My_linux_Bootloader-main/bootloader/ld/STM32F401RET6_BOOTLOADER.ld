/* Entry Point */
ENTRY(Reset_Handler)
/* 告诉链接器：程序入口是 Reset_Handler 函数
  这不影响硬件行为（硬件从向量表读取），但 GDB 调试时会从这里开始 */

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM); /* end of "RAM" Ram type memory */
/* 定义栈顶地址符号：0x20000000 + 96*1024 = 0x20018000
  这个符号会被启动文件和向量表第0项引用 */

_Min_Heap_Size = 0x200; /* 最小堆空间：512 字节 */
_Min_Stack_Size = 0x400; /* 最小栈空间：1024 字节 */

/* Memory definition for STM32F401RET6 */
MEMORY
{
  RAM (xrw)    : ORIGIN = 0x20000000,   LENGTH = 96K
  FLASH (rx)   : ORIGIN = 0x08000000,   LENGTH = 64K  /* Bootloader region: first 64KB */
}

/* Sections */
/* SECTIONS段负责将所有.o文件里的输入段映射到最终的.elf文件中，本质上是一个规则 */
SECTIONS
{
  /* The startup code into "FLASH" Rom type memory */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))    /* 保留中断向量表，即使没有代码引用它也不被 GC 删除 */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data into "FLASH" Rom type memory */
  .text :
{
    . = ALIGN(4);
    *(.text)          /* 所有目标文件的 .text section（普通函数代码） */
    *(.text*)         /* .text.functionName 等子 section（-ffunction-sections 产生
    的） */
    *(.glue_7)        /* ARM 到 Thumb 的胶合代码（混合指令集时需要） */
    *(.glue_7t)       /* Thumb 到 ARM 的胶合代码 */
    *(.eh_frame)      /* 异常处理帧信息（C++ 异常或 unwind） */
    KEEP (*(.init))   /* 初始化例程（C 运行时启动） */
    KEEP (*(.fini))   /* 析构例程 */
    . = ALIGN(4);
    _etext = .;       /* 导出符号：代码段结束地址（启动文件用来定位 .data 的初始值） */
} >FLASH

  /* Constant data into "FLASH" Rom type memory */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* 字符串常量、const 变量等 */
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  /* ARM sections */
  .ARM.extab   : { 
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >FLASH

  .ARM : {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)      /* 异常处理索引表（C++ 异常 / stack unwind 用） */
    __exidx_end = .;
    . = ALIGN(4);
  } >FLASH

  .preinit_array     :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .init_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .fini_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } >FLASH

  /* Used by the startup to initialize data */
  _sidata = LOADADDR(.data);    /* 符号：.data 在 Flash 中的存储地址 */

  /* Initialized data sections into "RAM" Ram type memory */
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start 符号：.data 在 RAM 中的起始地址*/
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    *(.RamFunc)        /* 需要在 RAM 中运行的函数
（__attribute__((section(".RamFunc")))） */
    *(.RamFunc*)       /* .RamFunc* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end 符号：.data 在 RAM 中的结束地址*/

  } >RAM AT> FLASH

  /* Uninitialized data section into "RAM" Ram type memory */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss section */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)         /* 未初始化的全局变量（老式 C 编译器生成） */

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM

  /* User_heap_stack section, used to check that there is enough "RAM" Ram type memory left */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;   /* 预留 0x200 字节堆空间 */
    . = . + _Min_Stack_Size;  /* 预留 0x400 字节栈空间 */
    . = ALIGN(8);
  } >RAM

  /* Remove information from the compiler libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
