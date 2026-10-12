#include "sensors.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

#define DHT22_PORT GPIOA
#define DHT22_PIN  GPIO_PIN_1

static void delay_us(uint16_t us) {
    TIM2->CNT = 0;
    while (TIM2->CNT < us) {
    }
}

static void dht22_pin_output(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT22_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct);
}

static void dht22_pin_input(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT22_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct);
}

static bool wait_for_pin_state(GPIO_PinState state, uint16_t timeout_us) {
    TIM2->CNT = 0;
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) != state) {
        if (TIM2->CNT > timeout_us) {
            return false;
        }
    }
    return true;
}

void Sensors_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* Configure TIM2 to tick at 1 MHz (1 tick = 1 us) */
    uint32_t timer_clk = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) {
        timer_clk *= 2;
    }
    TIM2->CR1 = 0;
    TIM2->PSC = (timer_clk / 1000000U) - 1U;
    TIM2->ARR = 0xFFFF;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->CR1 |= TIM_CR1_CEN;

    dht22_pin_output();
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);
}

bool DHT22_Read(float *temperature, float *humidity) {
    uint8_t data[5] = {0, 0, 0, 0, 0};

    /* 1. Pull low for at least 1 ms using RTOS delay or TIM2 */
    dht22_pin_output();
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET);
    delay_us(1200);

    taskENTER_CRITICAL();

    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);
    delay_us(30);
    dht22_pin_input();

    /* 2. Wait for DHT22 response: LOW (~80us) -> HIGH (~80us) -> LOW */
    if (!wait_for_pin_state(GPIO_PIN_RESET, 100) ||
        !wait_for_pin_state(GPIO_PIN_SET, 100)   ||
        !wait_for_pin_state(GPIO_PIN_RESET, 100)) {
        taskEXIT_CRITICAL();
        return false;
    }

    /* 3. Read 40 bits */
    for (int i = 0; i < 40; i++) {
        if (!wait_for_pin_state(GPIO_PIN_SET, 100)) {
            taskEXIT_CRITICAL();
            return false;
        }
        TIM2->CNT = 0;
        if (!wait_for_pin_state(GPIO_PIN_RESET, 100)) {
            taskEXIT_CRITICAL();
            return false;
        }
        uint16_t pulse_us = (uint16_t)TIM2->CNT;

        data[i / 8] <<= 1;
        if (pulse_us > 40) {
            data[i / 8] |= 1;
        }
    }

    taskEXIT_CRITICAL();

    /* 4. Verify checksum */
    uint8_t checksum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (checksum != data[4]) {
        return false;
    }

    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];

    *humidity = (float)raw_hum / 10.0f;
    if (raw_temp & 0x8000) {
        raw_temp &= 0x7FFF;
        *temperature = -((float)raw_temp / 10.0f);
    } else {
        *temperature = (float)raw_temp / 10.0f;
    }

    return true;
}