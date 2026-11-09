#ifndef TEST_USART_H
#define TEST_USART_H
#include "i2c.h"
#define USART1 1U
#define HAL_UART_STATE_READY 0U
#define HAL_UART_STATE_BUSY 1U
#define __DMB() ((void)0)
typedef struct { uint32_t Instance, gState, RxState; } UART_HandleTypeDef;
extern UART_HandleTypeDef huart1;
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *,uint8_t *,uint16_t);
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *,uint8_t *,uint16_t);
HAL_StatusTypeDef HAL_UART_AbortTransmit(UART_HandleTypeDef *);
uint32_t HAL_GetTick(void);
#endif
