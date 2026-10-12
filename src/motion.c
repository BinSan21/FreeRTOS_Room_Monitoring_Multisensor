#include "motion.h"
#include "stm32f1xx_hal.h"

#define PIR_PORT GPIOA
#define PIR_PIN  GPIO_PIN_4

void Motion_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = PIR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(PIR_PORT, &GPIO_InitStruct);
}

bool Motion_ReadPIR(void) {
    return (HAL_GPIO_ReadPin(PIR_PORT, PIR_PIN) == GPIO_PIN_SET);
}