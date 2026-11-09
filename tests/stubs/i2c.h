#ifndef TEST_I2C_H
#define TEST_I2C_H
#include <stdint.h>
typedef struct
{
    int unused;
} I2C_HandleTypeDef;
typedef enum
{
    HAL_OK = 0,
    HAL_ERROR = 1, HAL_BUSY = 2
} HAL_StatusTypeDef;
extern I2C_HandleTypeDef hi2c1;
extern int mock_busy;
#define I2C_FLAG_BUSY 1
#define I2C_MEMADD_SIZE_8BIT 1
unsigned mock_busy_status(void);
#define __HAL_I2C_GET_FLAG(handle, flag) mock_busy_status()
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *, uint16_t, uint16_t, uint16_t, uint8_t *,
                                   uint16_t, uint32_t);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *, uint16_t, uint16_t, uint16_t, uint8_t *,
                                    uint16_t, uint32_t);
void HAL_Delay(uint32_t milliseconds);
uint32_t HAL_I2C_GetError(I2C_HandleTypeDef *h);
uint32_t HAL_GetTick(void);
#endif
