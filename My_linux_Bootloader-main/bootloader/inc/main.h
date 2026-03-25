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

/* Application memory addresses */
#define APP_START_ADDR           0x08010000U  /* Application start address (64KB offset) */
#define APP_END_ADDR             0x0807FFFFU  /* Application end address (512KB total) */

/* SRAM boundaries */
#define SRAM_BASE                0x20000000U
#define SRAM_END                 0x20017FFFU  /* 96KB SRAM */

/* Boot timeout in milliseconds */
#define BOOT_TIMEOUT_MS          1000U        /* 1 second timeout before jumping to app */

/* Status codes */
typedef enum {
  BOOTLOADER_OK = 0,
  BOOTLOADER_ERR_NO_APP = -1,
  BOOTLOADER_ERR_INVALID_APP = -2,
  BOOTLOADER_ERR_FLASH = -3
} BootloaderStatus_t;

int main(void);
void system_init(void);
void gpio_init(void);
void led_toggle(void);
void delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
