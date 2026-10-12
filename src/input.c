#include "input.h"
#ifdef STM32F103xB
#include "stm32f1xx_hal.h"
#endif

DisplayMode nextDisplayMode(DisplayMode current) {
    switch (current) {
        case DISPLAY_MODE_TEMPERATURE: return DISPLAY_MODE_HUMIDITY;
        case DISPLAY_MODE_HUMIDITY:    return DISPLAY_MODE_LIGHT;
        case DISPLAY_MODE_LIGHT:       return DISPLAY_MODE_MOTION;
        case DISPLAY_MODE_MOTION:      return DISPLAY_MODE_TEMPERATURE;
        default:                       return DISPLAY_MODE_TEMPERATURE;
    }
}

DisplayMode previousDisplayMode(DisplayMode current) {
    switch (current) {
        case DISPLAY_MODE_TEMPERATURE: return DISPLAY_MODE_MOTION;
        case DISPLAY_MODE_HUMIDITY:    return DISPLAY_MODE_TEMPERATURE;
        case DISPLAY_MODE_LIGHT:       return DISPLAY_MODE_HUMIDITY;
        case DISPLAY_MODE_MOTION:      return DISPLAY_MODE_LIGHT;
        default:                       return DISPLAY_MODE_TEMPERATURE;
    }
}

#ifdef STM32F103xB
#define ENC_PORT    GPIOB
#define ENC_CLK_PIN GPIO_PIN_12
#define ENC_DT_PIN  GPIO_PIN_13
#define ENC_SW_PIN  GPIO_PIN_14

static uint8_t last_clk = 1;

void Input_Init(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = ENC_CLK_PIN | ENC_DT_PIN | ENC_SW_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(ENC_PORT, &GPIO_InitStruct);

    last_clk = (uint8_t)HAL_GPIO_ReadPin(ENC_PORT, ENC_CLK_PIN);
}

/* Returns +1 for Clockwise, -1 for Counterclockwise, 0 for no movement */
int Input_PollEncoderStep(void) {
    uint8_t clk = (uint8_t)HAL_GPIO_ReadPin(ENC_PORT, ENC_CLK_PIN);
    int step = 0;
    if (clk != last_clk) {
        if (clk == 0) { /* Falling edge of CLK */
            uint8_t dt = (uint8_t)HAL_GPIO_ReadPin(ENC_PORT, ENC_DT_PIN);
            step = (dt != clk) ? 1 : -1;
        }
        last_clk = clk;
    }
    return step;
}
#endif