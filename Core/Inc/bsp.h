#ifndef BSP_H
#define BSP_H
#include "main.h"
typedef struct
{
    uint16_t channel[8];
    uint32_t timestamp_us, sequence;
    uint8_t valid, channel_count;
} PpmSnapshot;
void BSP_Init(void);
uint32_t BSP_TimeUs(void);
void PPM_GetSnapshot(PpmSnapshot *out);
/* Bench firmware: these compatibility entry points always force CCR to zero. */
void Motor_SetSpeed(TIM_HandleTypeDef *timer, uint32_t channel, uint16_t speed);
void Motor_SetDirection(uint8_t direction);
void Motor_Stop(TIM_HandleTypeDef *timer, uint32_t channel);
#endif
