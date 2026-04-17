#include "led.h"
#define RCC_AHB1ENR  (*(volatile unsigned int *)(0x40023800 + 0x30))
#define GPIOA_MODER  (*(volatile unsigned int *)(0x40020000 + 0x00))
#define GPIOA_ODR    (*(volatile unsigned int *)(0x40020000 + 0x14))

void LED_Init(void) {
    // 1. 开启 GPIOA 时钟 (Bit 0)
    RCC_AHB1ENR |= (1 << 0);
    
    // 2. 配置 PA5 为输出模式 (Bits 11:10 -> 01)
    GPIOA_MODER &= ~(3 << (5 * 2)); // 清零
    GPIOA_MODER |=  (1 << (5 * 2)); // 设置为 01
}

void LED_Toggle(void) {
    // 翻转 PA5 状态
    GPIOA_ODR ^= (1 << 5);
}