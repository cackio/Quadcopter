// 1. 手动定义外设基地址（根据 STM32F4 参考手册）
#define PERIPH_BASE         0x40000000UL
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)

// GPIOA 基地址
#define GPIOA_BASE          (AHB1PERIPH_BASE + 0x0000UL)

// RCC 基地址
#define RCC_BASE            (AHB1PERIPH_BASE + 0x3800UL)

// 2. 定义关键寄存器的偏移（或者直接定义寄存器地址）
// GPIO 寄存器结构（仅列出需要的）
#define GPIOA_MODER         (*(volatile unsigned int *)(GPIOA_BASE + 0x00))
#define GPIOA_ODR           (*(volatile unsigned int *)(GPIOA_BASE + 0x14))

// RCC 寄存器
#define RCC_AHB1ENR         (*(volatile unsigned int *)(RCC_BASE + 0x30))

// 3. 位掩码定义（可用宏或直接数值）
#define RCC_AHB1ENR_GPIOAEN (1 << 0)   // GPIOA 时钟使能位

void LED_Init(void) 
{
    /* 1. 使能 GPIOA 的时钟 (RCC_AHB1ENR 寄存器的第 0 位) */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN; 
    
    /* 2. 配置 PA5 为通用输出模式 (MODER 寄存器的第 10、11 位设为 01) */
    GPIOA_MODER &= ~(3 << 10); // 先清零 11 和 10 位
    GPIOA_MODER |= (1 << 10);  // 再把第 10 位置 1
}

void LED_On(void)
{
    GPIOA_ODR |= (1u << 5);
}

void LED_Toggle(void) 
{
    /* 3. 翻转 PA5 的输出电平 (异或 ODR 寄存器的第 5 位) */
    GPIOA_ODR ^= (1 << 5); 
}