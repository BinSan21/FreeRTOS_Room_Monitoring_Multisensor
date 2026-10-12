#include "alarm.h"
#ifdef STM32F103xB
#include "stm32f1xx_hal.h"
#endif

/* Pure decision logic:
 * Normal range is inclusive of boundaries [18.0 C, 30.0 C] per Part XIV Step 43:
 * - Below 18.0 C      -> ALARM_STATE_LOW_TEMPERATURE
 * - Exactly 18.0 C    -> ALARM_STATE_NORMAL
 * - Normal value      -> ALARM_STATE_NORMAL
 * - Exactly 30.0 C    -> ALARM_STATE_NORMAL
 * - Above 30.0 C      -> ALARM_STATE_HIGH_TEMPERATURE
 */
AlarmState evaluateTemperature(float temperature) {
    if (temperature < LOW_TEMPERATURE_LIMIT) {
        return ALARM_STATE_LOW_TEMPERATURE;
    }
    if (temperature > HIGH_TEMPERATURE_LIMIT) {
        return ALARM_STATE_HIGH_TEMPERATURE;
    }
    return ALARM_STATE_NORMAL;
}

#ifdef STM32F103xB
#define BUZZER_PORT GPIOA
#define BUZZER_PIN  GPIO_PIN_8

void Alarm_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = BUZZER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
}

void Alarm_SetBuzzer(bool active) {
    if (active) {
        /* Generate a short audible square-wave burst (~1 kHz) so Wokwi shows the sound icon */
        for (int i = 0; i < 40; i++) {
            HAL_GPIO_TogglePin(BUZZER_PORT, BUZZER_PIN);
            for (volatile int d = 0; d < 4000; d++) {
                __NOP();
            }
        }
        HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
    }
}
#endif