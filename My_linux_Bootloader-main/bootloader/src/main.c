/**
  ******************************************************************************
  * @file    main.c
  * @brief   STM32F401 Bootloader 主程序
  * @details 该 Bootloader 初始化系统，并在应用程序有效时跳转到位于 0x08010000 的应用程序。
  *          否则，它将停留在 Bootloader 模式。
  ******************************************************************************
  */

#include "main.h"

/* 私有函数原型 */
static int32_t validate_application(void);
static void bootloader_jump_to_app(void);
static void bootloader_error_loop(void);

/* 私有变量 */
static volatile uint32_t boot_timeout = 0;
static volatile uint8_t boot_flag = 0;

/**
  * @brief  主程序
  * @retval 无
  */
int main(void)
{
  /* 初始化系统时钟和外设 */
  system_init();
  
  /* 初始化用于 LED 状态指示的 GPIO */
  gpio_init();
  
  /* 检查应用程序是否有效 */
  if (validate_application() == BOOTLOADER_OK) {
    /* 应用程序有效，短暂延迟后跳转 */
    // App 有效：LED 快闪 1 秒（100ms 间隔 × 10次），然后跳转
    boot_timeout = BOOT_TIMEOUT_MS;
    
    /* 等待超时（LED 快速闪烁） */
    while (boot_timeout > 0) {
      led_toggle();
      delay_ms(100);
      boot_timeout -= 100;
    }
    
    /* 跳转到应用程序 */
    bootloader_jump_to_app();
  } else {
    /* 没有有效的应用程序，停留在 Bootloader（LED 常亮） */
    // App 无效：LED 常亮，死循环
    bootloader_error_loop();
  }
  
  /* 不应到达此处 */
  while (1) {
    bootloader_error_loop();
  }
}

/**
  * @brief  验证位于 APP_START_ADDR 的应用程序
  * @retval 如果应用程序有效则返回 BOOTLOADER_OK，否则返回错误代码
  */
static int32_t validate_application(void)
{
  uint32_t app_stack_ptr;
  uint32_t app_reset_handler;
  
  /* 从应用程序向量表读取栈指针 */
  app_stack_ptr = *(volatile uint32_t *)(APP_START_ADDR);
  
  /* 从应用程序向量表读取复位处理程序 */
  app_reset_handler = *(volatile uint32_t *)(APP_START_ADDR + 4);
  
  /* 验证栈指针是否指向 SRAM */
  if (app_stack_ptr < SRAM_BASE || app_stack_ptr > (SRAM_END + 1)) {
    return BOOTLOADER_ERR_INVALID_APP;
  }
  
  /* 验证复位处理程序是否指向应用程序 Flash 区域 */
  if (app_reset_handler < APP_START_ADDR || app_reset_handler > APP_END_ADDR) {
    return BOOTLOADER_ERR_INVALID_APP;
  }
  
  /* 简单的校验和检查（可选，可增强） */
  /* 目前仅检查第一个位置是否不全为 0xFF 或 0x00 */
  if (app_stack_ptr == 0xFFFFFFFF || app_stack_ptr == 0x00000000) {
    return BOOTLOADER_ERR_NO_APP;
  }
  
  return BOOTLOADER_OK;
}

/**
  * @brief  跳转到位于 APP_START_ADDR 的应用程序
  * @retval 无
  */
static void bootloader_jump_to_app(void)
{
  typedef void (*pFunction)(void);
  
  uint32_t app_stack_ptr;
  uint32_t app_reset_handler;
  pFunction app_entry;
  
  /* 禁用所有中断 */
  __disable_irq();
  
  /* 获取应用程序栈指针和复位处理程序 */
  app_stack_ptr = *(volatile uint32_t *)(APP_START_ADDR);
  app_reset_handler = *(volatile uint32_t *)(APP_START_ADDR + 4);
  
  /* 将主栈指针设置为应用程序的栈 */
  __set_MSP(app_stack_ptr);
  
  /* 将向量表重定位到应用程序区域 */
  SCB->VTOR = APP_START_ADDR;
  
  /* 为应用程序启用中断 */
  __enable_irq();
  
  /* 跳转到应用程序 */
  app_entry = (pFunction)(app_reset_handler);
  app_entry();
  
  /* 不应返回 */
  while (1) {
    /* 如果到达此处则出错 */
  }
}

/**
  * @brief  错误循环 - 停留在 Bootloader 模式
  * @retval 无
  */
static void bootloader_error_loop(void)
{
  /* LED 常亮以指示错误 */
  GPIOA->BSRR = (1 << 5);  /* 置位 PA5 (LED 点亮) */
  
  while (1) {
    /* 停留在此处 - 没有有效的应用程序 */
  }
}

/**
  * @brief  初始化系统时钟
  * @retval 无
  */
void system_init(void)
{
  /* 将 RCC 时钟配置重置为默认复位状态 */
  RCC->CR |= RCC_CR_HSION;                /* 启用 HSI */
  while (!(RCC->CR & RCC_CR_HSIRDY));     /* 等待 HSI 就绪 */
  
  RCC->CFGR = 0x00000000;                 /* 重置 CFGR 寄存器 */
  RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_CSSON | RCC_CR_PLLON);  /* 禁用 HSE, CSS, PLL */
  RCC->CR &= ~RCC_CR_HSEBYP;              /* 禁用 HSE 旁路 */
  RCC->CFGR &= ~RCC_CFGR_HPRE;            /* 重置 HPRE */
  RCC->CFGR &= ~RCC_CFGR_PPRE1;           /* 重置 PPRE1 */
  RCC->CFGR &= ~RCC_CFGR_PPRE2;           /* 重置 PPRE2 */
  
  /* 禁用所有中断 */
  RCC->CIR = 0x00000000;
  
  /* SystemInit() 由启动代码调用 */
  /* 向量表已设置为 FLASH_BASE */
  
  /* 更新 SystemCoreClock 变量 */
  SystemCoreClockUpdate();
}

/**
  * @brief  初始化用于 LED 的 GPIO (PA5 - 许多 STM32 开发板上的内置 LED)
  * @retval 无
  */
void gpio_init(void)
{
  /* 启用 GPIOA 时钟 */
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  
  /* 配置 PA5 为输出 */
  GPIOA->MODER &= ~(3 << (5 * 2));        /* 清除 PA5 的模式位 */
  GPIOA->MODER |= (1 << (5 * 2));         /* 设置 PA5 为输出模式 */
  
  /* 配置 PA5 输出类型（推挽） */
  GPIOA->OTYPER &= ~(1 << 5);             /* 输出推挽 */
  
  /* 配置 PA5 速度（高速） */
  GPIOA->OSPEEDR &= ~(3 << (5 * 2));      /* 清除速度位 */
  GPIOA->OSPEEDR |= (2 << (5 * 2));       /* 高速 */
  
  /* 配置 PA5 无上拉/下拉 */
  GPIOA->PUPDR &= ~(3 << (5 * 2));        /* 无上拉/下拉 */
  
  /* 初始关闭 LED */
  GPIOA->BSRR = (1 << (5 + 16));          /* 复位 PA5 */
}

/**
  * @brief  翻转 LED 状态
  * @retval 无
  */
void led_toggle(void)
{
  /* 翻转 PA5 */
  if (GPIOA->ODR & (1 << 5)) {
    GPIOA->BSRR = (1 << (5 + 16));        /* 复位 PA5 */
  } else {
    GPIOA->BSRR = (1 << 5);               /* 置位 PA5 */
  }
}

/**
  * @brief  使用 SysTick 的简单延迟函数（未校准，仅为忙等待）
  * @param  ms: 延迟时间（毫秒）
  * @retval 无
  */
void delay_ms(uint32_t ms)
{
  /* 简单的忙等待延迟 - 不精确但对 Bootloader 足够 */
  /* 假设 16MHz HSI 时钟（复位后默认） */
  volatile uint32_t count;
  volatile uint32_t i;
  
  /* 根据实际时钟频率调整此值 */
  /* 对于 16MHz：每毫秒约 16000 个周期 */
  count = ms * 1600;
  
  for (i = 0; i < count; i++) {
    __NOP();
  }
}
