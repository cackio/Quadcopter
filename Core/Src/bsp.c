#include "bsp.h"
#include "tim.h"
#include "timebase.h"
#include "ucos_ii.h"
#include <string.h>
static PpmSnapshot ppm;
static uint16_t building[8];
static uint32_t last_capture;
static uint8_t channel_index, frame_started;
void BSP_Init(void)
{
    GPIO_InitTypeDef motor_pins = {0};
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    /* Never start PWM; additionally hold all ESC signal pins low in bench mode. */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_RESET);
    motor_pins.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    motor_pins.Mode = GPIO_MODE_OUTPUT_PP;
    motor_pins.Pull = GPIO_PULLDOWN;
    motor_pins.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &motor_pins);
    motor_pins.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    HAL_GPIO_Init(GPIOB, &motor_pins);
    if (HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1) != HAL_OK)
        Error_Handler();
}
uint32_t BSP_TimeUs(void)
{
    return __HAL_TIM_GET_COUNTER(&htim2);
}
void PPM_GetSnapshot(PpmSnapshot *out)
{
    OS_CPU_SR cpu_sr = 0;
    OS_ENTER_CRITICAL();
    Time_ExpireUs(BSP_TimeUs(), ppm.timestamp_us, 100000U, &ppm.valid);
    *out = ppm;
    OS_EXIT_CRITICAL();
}
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *timer)
{
    uint32_t now, diff;
    if (timer->Instance != TIM2 || timer->Channel != HAL_TIM_ACTIVE_CHANNEL_1)
        return;
    now = HAL_TIM_ReadCapturedValue(timer, TIM_CHANNEL_1);
    diff = Time_ElapsedUs(now, last_capture);
    last_capture = now;
    if (diff > 2500U)
    {
        if (frame_started && channel_index >= 4U && channel_index <= 8U)
        {
            memcpy(ppm.channel, building, sizeof(building));
            ppm.timestamp_us = now;
            ++ppm.sequence;
            ppm.valid = 1;
            ppm.channel_count = channel_index;
        }
        frame_started = 1;
        channel_index = 0;
        for (unsigned i = 0; i < 8; ++i)
            building[i] = (i == 2) ? 1000U : 1500U;
    }
    else if (frame_started && diff >= 900U && diff <= 2100U && channel_index < 8U)
    {
        building[channel_index++] = (uint16_t)diff;
    }
    else
    {
        frame_started = 0;
        channel_index = 0;
    }
}
void Motor_Stop(TIM_HandleTypeDef *timer, uint32_t channel)
{
    __HAL_TIM_SET_COMPARE(timer, channel, 0);
}
void Motor_SetSpeed(TIM_HandleTypeDef *timer, uint32_t channel, uint16_t speed)
{
    (void)speed;
    Motor_Stop(timer, channel);
}
void Motor_SetDirection(uint8_t direction)
{
    (void)direction;
}
