/**
  ******************************************************************************
  * @file    main.h
  * @brief   Header for main.c file.
  *          This file contains the common defines of the application.
  ******************************************************************************
  */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"
#include "system_stm32f4xx.h"

/* Application information */
#define APP_VERSION_MAJOR    1
#define APP_VERSION_MINOR    0
#define APP_VERSION_PATCH    0

/* LED blink delay (slower than bootloader to distinguish) */
#define LED_BLINK_DELAY_MS   500U

void main(void);
void system_init(void);
void gpio_init(void);
void uart_init(void);
void led_toggle(void);
void delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
