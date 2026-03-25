/**
  ******************************************************************************
  * @file    main.c
  * @brief   STM32F401 应用程序主函数
  * @details 本应用通过让 LED 以比 Bootloader 更慢的频率闪烁，
  *          来直观指示应用程序正在运行。
  ******************************************************************************
  */

#include "main.h"

/* 私有变量 */
static volatile uint32_t tick_counter = 0;

/**
  * @brief  主程序
  * @retval 无
  */
void main(void)
{
  /* 将向量表重定位到应用区（重要！） */
  /* Bootloader 将 VTOR 设为 0x08000000，应用程序需要 0x08010000 */
  __disable_irq();
  SCB->VTOR = 0x08010000;  /* 应用程序向量表偏移地址 */
  __enable_irq();
  
  /* 初始化系统时钟与外设 */
  system_init();
  
  /* 初始化用于 LED 状态指示的 GPIO */
  gpio_init();
  
  /* 初始化 UART（可选，用于调试输出） */
  // uart_init();
  
  /* 应用主循环 */
  while (1) {
    /* 以低于 Bootloader 的频率翻转 LED */
    led_toggle();
    delay_ms(LED_BLINK_DELAY_MS);
  }
}

/**
  * @brief  初始化系统时钟
  * @details 使用 HSI 作为 PLL 输入，将系统时钟配置为 84MHz
  * @retval 无
  */
void system_init(void)
{
  /* 使能 HSI */
  RCC->CR |= RCC_CR_HSION;
  while (!(RCC->CR & RCC_CR_HSIRDY));  /* 等待 HSI 就绪 */
  
  /* 将 PLL 配置为 84MHz 系统时钟 */
  /* PLL 输入源 = HSI (16MHz) */
  /* PLLM = 16, PLLN = 336, PLLP = 4 => 16MHz * 336 / 16 / 4 = 84MHz */
  RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLSRC;  /* 清除 PLL 输入源配置 */
  /* 说明：为简化示例，此处保持默认 HSI 配置 */
  
  /* 更新 SystemCoreClock 变量 */
  SystemCoreClockUpdate();
  
  /* 启动代码中的 SystemInit() 已将向量表设置为 APP_START_ADDR */
}

/**
  * @brief  初始化 LED(GPIO PA5)
  * @retval 无
  */
void gpio_init(void)
{
  /* 使能 GPIOA 时钟 */
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  
  /* 将 PA5 配置为输出模式 */
  GPIOA->MODER &= ~(3 << (5 * 2));        /* 清除 PA5 模式位 */
  GPIOA->MODER |= (1 << (5 * 2));         /* 设置 PA5 为输出模式 */
  
  /* 配置 PA5 输出类型（推挽） */
  GPIOA->OTYPER &= ~(1 << 5);             /* 推挽输出 */
  
  /* 配置 PA5 速度（高速） */
  GPIOA->OSPEEDR &= ~(3 << (5 * 2));      /* 清除速度位 */
  GPIOA->OSPEEDR |= (2 << (5 * 2));       /* 高速 */
  
  /* 配置 PA5 无上拉/下拉 */
  GPIOA->PUPDR &= ~(3 << (5 * 2));        /* 无上拉/下拉 */
  
  /* 初始关闭 LED */
  GPIOA->BSRR = (1 << (5 + 16));          /* 复位 PA5 */
}

/**
  * @brief  初始化 UART2 用于调试（PA2: TX, PA3: RX）
  * @details 将 UART2 配置为 115200 波特率，8N1
  * @retval 无
  */
void uart_init(void)
{
  /* 为 UART 引脚使能 GPIOA 时钟 */
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  
  /* 使能 USART2 时钟 */
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
  
  /* 将 PA2 (TX) 与 PA3 (RX) 配置为 UART 功能 */
  /* 为 USART2 设置复用功能 AF7 */
  GPIOA->AFR[0] &= ~(0xF << (2 * 4));     /* 清除 AF2 (PA2) */
  GPIOA->AFR[0] |= (7 << (2 * 4));        /* PA2 设置为 AF7 */
  GPIOA->AFR[0] &= ~(0xF << (3 * 4));     /* 清除 AF3 (PA3) */
  GPIOA->AFR[0] |= (7 << (3 * 4));        /* PA3 设置为 AF7 */
  
  /* 将 PA2 和 PA3 设置为复用功能模式 */
  GPIOA->MODER &= ~(3 << (2 * 2));        /* 清除 PA2 模式 */
  GPIOA->MODER |= (2 << (2 * 2));         /* PA2 设为复用模式 */
  GPIOA->MODER &= ~(3 << (3 * 2));        /* 清除 PA3 模式 */
  GPIOA->MODER |= (2 << (3 * 2));         /* PA3 设为复用模式 */
  
  /* 配置 UART2 */
  /* 配置前先关闭 USART2 */
  USART2->CR1 &= ~USART_CR1_UE;
  
  /* 设置波特率为 115200（假设 APB1 时钟 = 16MHz） */
  /* USART_BRR = f_clk / baud = 16MHz / 115200 ≈ 139 */
  USART2->BRR = 139;
  
  /* 配置 8N1：8 位数据位，无校验，1 位停止位 */
  /* 默认值：M=0 (8位数据), PCE=0 (无校验), STOP=00 (1位停止位) */
  
  /* 使能发送器和接收器 */
  USART2->CR1 |= USART_CR1_TE;
  USART2->CR1 |= USART_CR1_RE;
  
  /* 使能 USART2 */
  USART2->CR1 |= USART_CR1_UE;
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
  * @brief  简单延时函数
  * @param  ms: 延时时间（毫秒）
  * @retval 无
  */
void delay_ms(uint32_t ms)
{
  /* 简单忙等待延时 */
  /* 在 16MHz HSI 下：每毫秒约 16000 个周期 */
  volatile uint32_t count;
  volatile uint32_t i;
  
  count = ms * 1600;
  
  for (i = 0; i < count; i++) {
    __NOP();
  }
}

/**
  * @brief  通过 UART2 发送单个字符
  * @param  ch: 待发送字符
  * @retval 无
  */
void uart_send_char(uint8_t ch)
{
  /* 等待 TXE 标志置位 */
  while (!(USART2->SR & USART_SR_TXE));
  
  /* 发送字符 */
  USART2->DR = ch;
}

/**
  * @brief  通过 UART2 发送字符串
  * @param  str: 待发送字符串
  * @retval 无
  */
void uart_send_string(const char *str)
{
  while (*str) {
    uart_send_char(*str);
    str++;
  }
}
