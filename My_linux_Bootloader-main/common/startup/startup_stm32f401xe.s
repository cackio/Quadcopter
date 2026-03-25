/**
  ******************************************************************************
  * @file      startup_stm32f401xe.s
  * @author    MCD Application Team
  * @brief     基于 GCC 工具链的 STM32F401xExx 设备中断向量表。
  *            本模块完成以下工作：
  *                - 设置初始 SP（栈指针）
  *                - 设置初始 PC = Reset_Handler
  *                - 配置异常与中断 ISR 地址到向量表
  *                - 跳转到 C 库中的 main（最终会调用用户 main()）
  *            复位后，Cortex-M4 处理器处于线程模式（Thread mode），
  *            优先级为特权级（Privileged），并使用主栈（Main Stack）。
  ******************************************************************************
  * @attention
  *
  * 版权所有 (c) 2017 STMicroelectronics.
  * 保留所有权利。
  *
  * 本软件受许可证条款约束，相关条款可在本软件组件根目录的 LICENSE 文件中查看。
  * 若未附带 LICENSE 文件，则本软件按“现状”提供（AS-IS）。
  *
  ******************************************************************************
  */
    
  .syntax unified   /* 使用统一汇编语法（ARM + Thumb 混合语法） */
  .cpu cortex-m4    /* 目标 CPU，影响指令集选择 */
  .fpu softvfp      /* 使用软件 FPU（注意：Makefile 里用的是 hard FPU，这里是汇编器设置）*/
  .thumb            /* 生成 Thumb 指令集代码 */

.global  g_pfnVectors
.global  Default_Handler

/* .data 段初始化值在 Flash 中的起始地址。
由链接脚本定义 */
.word  _sidata
/* .data 段在 RAM 中的起始地址。由链接脚本定义 */  
.word  _sdata
/* .data 段在 RAM 中的结束地址。由链接脚本定义 */
.word  _edata
/* .bss 段在 RAM 中的起始地址。由链接脚本定义 */
.word  _sbss
/* .bss 段在 RAM 中的结束地址。由链接脚本定义 */
.word  _ebss
/* SystemInit_ExtMemCtl 使用的栈；始终使用内部 RAM */

/**
 * @brief  处理器在复位后最先执行的代码。
 *         此处仅完成最必要的初始化，然后调用应用程序提供的 main()。
 * @param  无
 * @retval 无
*/

  .section  .text.Reset_Handler     /* 放入独立 section，支持 GC */
  .weak  Reset_Handler              /* weak 符号，可被用户重定义覆盖 */
  .type  Reset_Handler, %function
Reset_Handler:  
  ldr   sp, =_estack    		 /* 第1步：设置栈指针 SP = 0x20018000 */

  bl  SystemInit            /* 第2步：调用 SystemInit()
                              → 使能 FPU
                              → 重置 RCC 时钟到默认（HSI 16MHz）
                              → 设置 SCB->VTOR = FLASH_BASE（0x08000000） */

/* 第3步：将 .data 初始值从 Flash 复制到 RAM */
  ldr r0, =_sdata           /* r0 = RAM 目标起始地址 */
  ldr r1, =_edata           /* r1 = RAM 目标结束地址 */
  ldr r2, =_sidata          /* r2 = Flash 源数据起始地址 */
  movs r3, #0               /* r3 = 字节偏移量，初始为0 */
  b LoopCopyDataInit

CopyDataInit:
  ldr r4, [r2, r3]          /* 从 Flash 读4字节：r4 = *(r2+r3) */
  str r4, [r0, r3]          /* 写入 RAM：*(r0+r3) = r4 */
  adds r3, r3, #4           /* 偏移量 +4（每次复制4字节） */

LoopCopyDataInit:
  adds r4, r0, r3           /* r4 = 当前 RAM 写入地址 */
  cmp r4, r1                /* 是否到达 _edata？ */
  bcc CopyDataInit          /* 未到达则继续（bcc = branch if carry clear，即 <）*/

/* 第4步：将 .bss 清零 */
  ldr r2, =_sbss
  ldr r4, =_ebss
  movs r3, #0
  b LoopFillZerobss

FillZerobss:
  str r3, [r2]            /* 写0到当前地址 */
  adds r2, r2, #4         /* 地址 +4 */

LoopFillZerobss:
  cmp r2, r4              /* 是否到达 _ebss？ */
  bcc FillZerobss

  bl __libc_init_array    /* 第5步：调用 C++ 全局构造函数 */
  bl main                 /* 第6步：调用 main() */
  bx lr                   /* main() 返回后（理论上不应到这里） */
.size  Reset_Handler, .-Reset_Handler

/**
 * @brief  处理器收到未预期中断时执行的代码。
 *         该处理函数仅进入死循环，以便调试器检查系统状态。
 * @param  无
 * @retval 无
*/
    .section  .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b  Infinite_Loop            /* 未处理中断：死循环（便于调试时定位问题） */
  .size  Default_Handler, .-Default_Handler
/******************************************************************************
*
* Cortex-M3 的最小向量表模板。
* 注意：必须使用正确的段与构造方式，
* 才能确保其最终位于物理地址 0x0000.0000。
* 
*******************************************************************************/
   .section  .isr_vector,"a",%progbits
  .type  g_pfnVectors, %object
    
g_pfnVectors:
  .word _estack             /* 偏移 0x00：初始栈指针（SP） */
  .word Reset_Handler       /* 偏移 0x04：复位向量（CPU 上电后跳转地址） */
  .word NMI_Handler         /* 偏移 0x08：不可屏蔽中断 */
  .word HardFault_Handler   /* 偏移 0x0C：硬件错误 */
  .word MemManage_Handler   /* 偏移 0x10：内存管理错误 */
  .word BusFault_Handler    /* 偏移 0x14：总线错误 */
  .word UsageFault_Handler  /* 偏移 0x18：使用错误 */
  .word 0                   /* 偏移 0x1C~0x28：保留 */
  .word 0
  .word 0
  .word 0
  .word SVC_Handler         /* 偏移 0x2C：系统调用 */
  .word DebugMon_Handler    /* 偏移 0x30：调试监视器 */
  .word 0                   /* 保留 */
  .word PendSV_Handler      /* 偏移 0x38：可挂起系统调用（RTOS 上下文切换） */
  .word SysTick_Handler     /* 偏移 0x3C：系统定时器 */
  
  /* 外部中断 */
  .word     WWDG_IRQHandler                   /* 窗口看门狗 */                                        
  .word     PVD_IRQHandler                    /* 通过 EXTI 线触发的 PVD 检测 */                        
  .word     TAMP_STAMP_IRQHandler             /* 通过 EXTI 线触发的防篡改与时间戳 */            
  .word     RTC_WKUP_IRQHandler               /* 通过 EXTI 线触发的 RTC 唤醒 */                      
  .word     FLASH_IRQHandler                  /* FLASH */                                          
  .word     RCC_IRQHandler                    /* RCC */                                            
  .word     EXTI0_IRQHandler                  /* EXTI 线 0 */                        
  .word     EXTI1_IRQHandler                  /* EXTI 线 1 */                          
  .word     EXTI2_IRQHandler                  /* EXTI 线 2 */                          
  .word     EXTI3_IRQHandler                  /* EXTI 线 3 */                          
  .word     EXTI4_IRQHandler                  /* EXTI 线 4 */                          
  .word     DMA1_Stream0_IRQHandler           /* DMA1 流 0 */                  
  .word     DMA1_Stream1_IRQHandler           /* DMA1 流 1 */                   
  .word     DMA1_Stream2_IRQHandler           /* DMA1 流 2 */                   
  .word     DMA1_Stream3_IRQHandler           /* DMA1 流 3 */                   
  .word     DMA1_Stream4_IRQHandler           /* DMA1 流 4 */                   
  .word     DMA1_Stream5_IRQHandler           /* DMA1 流 5 */                   
  .word     DMA1_Stream6_IRQHandler           /* DMA1 流 6 */                   
  .word     ADC_IRQHandler                    /* ADC1、ADC2、ADC3 */                   
  .word     0               				  /* 保留 */                         
  .word     0              					  /* 保留 */                          
  .word     0                                 /* 保留 */                          
  .word     0                                 /* 保留 */                          
  .word     EXTI9_5_IRQHandler                /* 外部线 [9:5] */                          
  .word     TIM1_BRK_TIM9_IRQHandler          /* TIM1 Break 与 TIM9 */         
  .word     TIM1_UP_TIM10_IRQHandler          /* TIM1 Update 与 TIM10 */         
  .word     TIM1_TRG_COM_TIM11_IRQHandler     /* TIM1 Trigger/Commutation 与 TIM11 */
  .word     TIM1_CC_IRQHandler                /* TIM1 捕获比较 */                          
  .word     TIM2_IRQHandler                   /* TIM2 */                   
  .word     TIM3_IRQHandler                   /* TIM3 */                   
  .word     TIM4_IRQHandler                   /* TIM4 */                   
  .word     I2C1_EV_IRQHandler                /* I2C1 事件 */                          
  .word     I2C1_ER_IRQHandler                /* I2C1 错误 */                          
  .word     I2C2_EV_IRQHandler                /* I2C2 事件 */                          
  .word     I2C2_ER_IRQHandler                /* I2C2 错误 */                            
  .word     SPI1_IRQHandler                   /* SPI1 */                   
  .word     SPI2_IRQHandler                   /* SPI2 */                   
  .word     USART1_IRQHandler                 /* USART1 */                   
  .word     USART2_IRQHandler                 /* USART2 */                   
  .word     0               				  /* 保留 */                   
  .word     EXTI15_10_IRQHandler              /* 外部线 [15:10] */                          
  .word     RTC_Alarm_IRQHandler              /* 通过 EXTI 线触发的 RTC 闹钟 A/B */                 
  .word     OTG_FS_WKUP_IRQHandler            /* 通过 EXTI 线触发的 USB OTG FS 唤醒 */                       
  .word     0                                 /* 保留 */         
  .word     0                                 /* 保留 */         
  .word     0                                 /* 保留 */
  .word     0                                 /* 保留 */                          
  .word     DMA1_Stream7_IRQHandler           /* DMA1 流 7 */                          
  .word     0                                 /* 保留 */                   
  .word     SDIO_IRQHandler                   /* SDIO */                   
  .word     TIM5_IRQHandler                   /* TIM5 */                   
  .word     SPI3_IRQHandler                   /* SPI3 */                   
  .word     0                                 /* 保留 */                   
  .word     0                                 /* 保留 */                   
  .word     0                                 /* 保留 */                   
  .word     0                                 /* 保留 */
  .word     DMA2_Stream0_IRQHandler           /* DMA2 流 0 */                   
  .word     DMA2_Stream1_IRQHandler           /* DMA2 流 1 */                   
  .word     DMA2_Stream2_IRQHandler           /* DMA2 流 2 */                   
  .word     DMA2_Stream3_IRQHandler           /* DMA2 流 3 */                   
  .word     DMA2_Stream4_IRQHandler           /* DMA2 流 4 */                   
  .word     0                    			  /* 保留 */                   
  .word     0              					  /* 保留 */                     
  .word     0              					  /* 保留 */                          
  .word     0             					  /* 保留 */                          
  .word     0              					  /* 保留 */                          
  .word     0              					  /* 保留 */                          
  .word     OTG_FS_IRQHandler                 /* USB OTG FS */                   
  .word     DMA2_Stream5_IRQHandler           /* DMA2 流 5 */                   
  .word     DMA2_Stream6_IRQHandler           /* DMA2 流 6 */                   
  .word     DMA2_Stream7_IRQHandler           /* DMA2 流 7 */                   
  .word     USART6_IRQHandler                 /* USART6 */                    
  .word     I2C3_EV_IRQHandler                /* I2C3 事件 */                          
  .word     I2C3_ER_IRQHandler                /* I2C3 错误 */                          
  .word     0                                 /* 保留 */                   
  .word     0                                 /* 保留 */                   
  .word     0                                 /* 保留 */                         
  .word     0                                 /* 保留 */                   
  .word     0                                 /* 保留 */                   
  .word     0                                 /* 保留 */                   
  .word     0                                 /* 保留 */
  .word     FPU_IRQHandler                    /* FPU */
  .word     0                                 /* 保留 */                   
  .word     0                                 /* 保留 */
  .word     SPI4_IRQHandler                   /* SPI4 */     
                    

  .size  g_pfnVectors, .-g_pfnVectors

/*******************************************************************************
*
* 为每个异常处理函数提供到 Default_Handler 的弱别名。
* 由于是弱别名，用户若实现同名函数，将自动覆盖此处定义。
* 
*******************************************************************************/
   .weak      NMI_Handler
   .thumb_set NMI_Handler,Default_Handler
  
   .weak      HardFault_Handler
   .thumb_set HardFault_Handler,Default_Handler
  
   .weak      MemManage_Handler
   .thumb_set MemManage_Handler,Default_Handler
  
   .weak      BusFault_Handler
   .thumb_set BusFault_Handler,Default_Handler

   .weak      UsageFault_Handler
   .thumb_set UsageFault_Handler,Default_Handler

   .weak      SVC_Handler
   .thumb_set SVC_Handler,Default_Handler

   .weak      DebugMon_Handler
   .thumb_set DebugMon_Handler,Default_Handler

   .weak      PendSV_Handler
   .thumb_set PendSV_Handler,Default_Handler

   .weak      SysTick_Handler
   .thumb_set SysTick_Handler,Default_Handler              
  
   .weak      WWDG_IRQHandler                   
   .thumb_set WWDG_IRQHandler,Default_Handler      
                  
   .weak      PVD_IRQHandler      
   .thumb_set PVD_IRQHandler,Default_Handler
               
   .weak      TAMP_STAMP_IRQHandler            
   .thumb_set TAMP_STAMP_IRQHandler,Default_Handler
            
   .weak      RTC_WKUP_IRQHandler                  
   .thumb_set RTC_WKUP_IRQHandler,Default_Handler
            
   .weak      FLASH_IRQHandler         
   .thumb_set FLASH_IRQHandler,Default_Handler
                  
   .weak      RCC_IRQHandler      
   .thumb_set RCC_IRQHandler,Default_Handler
                  
   .weak      EXTI0_IRQHandler         
   .thumb_set EXTI0_IRQHandler,Default_Handler
                  
   .weak      EXTI1_IRQHandler         
   .thumb_set EXTI1_IRQHandler,Default_Handler
                     
   .weak      EXTI2_IRQHandler         
   .thumb_set EXTI2_IRQHandler,Default_Handler 
                 
   .weak      EXTI3_IRQHandler         
   .thumb_set EXTI3_IRQHandler,Default_Handler
                        
   .weak      EXTI4_IRQHandler         
   .thumb_set EXTI4_IRQHandler,Default_Handler
                  
   .weak      DMA1_Stream0_IRQHandler               
   .thumb_set DMA1_Stream0_IRQHandler,Default_Handler
         
   .weak      DMA1_Stream1_IRQHandler               
   .thumb_set DMA1_Stream1_IRQHandler,Default_Handler
                  
   .weak      DMA1_Stream2_IRQHandler               
   .thumb_set DMA1_Stream2_IRQHandler,Default_Handler
                  
   .weak      DMA1_Stream3_IRQHandler               
   .thumb_set DMA1_Stream3_IRQHandler,Default_Handler 
                 
   .weak      DMA1_Stream4_IRQHandler              
   .thumb_set DMA1_Stream4_IRQHandler,Default_Handler
                  
   .weak      DMA1_Stream5_IRQHandler               
   .thumb_set DMA1_Stream5_IRQHandler,Default_Handler
                  
   .weak      DMA1_Stream6_IRQHandler               
   .thumb_set DMA1_Stream6_IRQHandler,Default_Handler
                  
   .weak      ADC_IRQHandler      
   .thumb_set ADC_IRQHandler,Default_Handler
            
   .weak      EXTI9_5_IRQHandler   
   .thumb_set EXTI9_5_IRQHandler,Default_Handler
            
   .weak      TIM1_BRK_TIM9_IRQHandler            
   .thumb_set TIM1_BRK_TIM9_IRQHandler,Default_Handler
            
   .weak      TIM1_UP_TIM10_IRQHandler            
   .thumb_set TIM1_UP_TIM10_IRQHandler,Default_Handler
      
   .weak      TIM1_TRG_COM_TIM11_IRQHandler      
   .thumb_set TIM1_TRG_COM_TIM11_IRQHandler,Default_Handler
      
   .weak      TIM1_CC_IRQHandler   
   .thumb_set TIM1_CC_IRQHandler,Default_Handler
                  
   .weak      TIM2_IRQHandler            
   .thumb_set TIM2_IRQHandler,Default_Handler
                  
   .weak      TIM3_IRQHandler            
   .thumb_set TIM3_IRQHandler,Default_Handler
                  
   .weak      TIM4_IRQHandler            
   .thumb_set TIM4_IRQHandler,Default_Handler
                  
   .weak      I2C1_EV_IRQHandler   
   .thumb_set I2C1_EV_IRQHandler,Default_Handler
                     
   .weak      I2C1_ER_IRQHandler   
   .thumb_set I2C1_ER_IRQHandler,Default_Handler
                     
   .weak      I2C2_EV_IRQHandler   
   .thumb_set I2C2_EV_IRQHandler,Default_Handler
                  
   .weak      I2C2_ER_IRQHandler   
   .thumb_set I2C2_ER_IRQHandler,Default_Handler
                           
   .weak      SPI1_IRQHandler            
   .thumb_set SPI1_IRQHandler,Default_Handler
                        
   .weak      SPI2_IRQHandler            
   .thumb_set SPI2_IRQHandler,Default_Handler
                  
   .weak      USART1_IRQHandler      
   .thumb_set USART1_IRQHandler,Default_Handler
                     
   .weak      USART2_IRQHandler      
   .thumb_set USART2_IRQHandler,Default_Handler
                                  
   .weak      EXTI15_10_IRQHandler               
   .thumb_set EXTI15_10_IRQHandler,Default_Handler
               
   .weak      RTC_Alarm_IRQHandler               
   .thumb_set RTC_Alarm_IRQHandler,Default_Handler
            
   .weak      OTG_FS_WKUP_IRQHandler         
   .thumb_set OTG_FS_WKUP_IRQHandler,Default_Handler
            
   .weak      DMA1_Stream7_IRQHandler               
   .thumb_set DMA1_Stream7_IRQHandler,Default_Handler
                     
   .weak      SDIO_IRQHandler            
   .thumb_set SDIO_IRQHandler,Default_Handler
                     
   .weak      TIM5_IRQHandler            
   .thumb_set TIM5_IRQHandler,Default_Handler
                     
   .weak      SPI3_IRQHandler            
   .thumb_set SPI3_IRQHandler,Default_Handler
                     
   .weak      DMA2_Stream0_IRQHandler               
   .thumb_set DMA2_Stream0_IRQHandler,Default_Handler
               
   .weak      DMA2_Stream1_IRQHandler               
   .thumb_set DMA2_Stream1_IRQHandler,Default_Handler
                  
   .weak      DMA2_Stream2_IRQHandler               
   .thumb_set DMA2_Stream2_IRQHandler,Default_Handler
            
   .weak      DMA2_Stream3_IRQHandler               
   .thumb_set DMA2_Stream3_IRQHandler,Default_Handler
            
   .weak      DMA2_Stream4_IRQHandler               
   .thumb_set DMA2_Stream4_IRQHandler,Default_Handler
            
   .weak      OTG_FS_IRQHandler      
   .thumb_set OTG_FS_IRQHandler,Default_Handler
                     
   .weak      DMA2_Stream5_IRQHandler               
   .thumb_set DMA2_Stream5_IRQHandler,Default_Handler
                  
   .weak      DMA2_Stream6_IRQHandler               
   .thumb_set DMA2_Stream6_IRQHandler,Default_Handler
                  
   .weak      DMA2_Stream7_IRQHandler               
   .thumb_set DMA2_Stream7_IRQHandler,Default_Handler
                  
   .weak      USART6_IRQHandler      
   .thumb_set USART6_IRQHandler,Default_Handler
                        
   .weak      I2C3_EV_IRQHandler   
   .thumb_set I2C3_EV_IRQHandler,Default_Handler
                        
   .weak      I2C3_ER_IRQHandler   
   .thumb_set I2C3_ER_IRQHandler,Default_Handler
                        
   .weak      FPU_IRQHandler                  
   .thumb_set FPU_IRQHandler,Default_Handler  

   .weak      SPI4_IRQHandler                  
   .thumb_set SPI4_IRQHandler,Default_Handler 
