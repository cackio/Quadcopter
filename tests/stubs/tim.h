#ifndef TEST_TIM_H
#define TEST_TIM_H
#include "i2c.h"
typedef struct { uint32_t Instance; } TIM_HandleTypeDef;
extern TIM_HandleTypeDef htim4;
#define TIM_FLAG_UPDATE 0
#define __HAL_TIM_CLEAR_FLAG(h,f) ((void)(h))
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *);
#endif
