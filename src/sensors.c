#include "sensors.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

#define DHT22_PORT GPIOA
#define DHT22_PIN  GPIO_PIN_1
#define LDR_PORT   GPIOA
#define LDR_PIN    GPIO_PIN_0

static ADC_HandleTypeDef hadc1;

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

static void adc1_init(void) {
    __HAL_RCC_ADC1_CLK_ENABLE();

    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6; /* 72MHz / 6 = 12MHz */
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LDR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(LDR_PORT, &GPIO_InitStruct);

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADCEx_Calibration_Start(&hadc1);
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

    adc1_init();
}

int LDR_ReadPercent(void) {
    uint32_t raw = 0;
    if (HAL_ADC_Start(&hadc1) == HAL_OK) {
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            raw = HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);
    }
    if (raw > 4095U) {
        raw = 4095U;
    }
    /* Wokwi photoresistor AO voltage drops as light increases: invert 0..4095 to 0..100% */
    int percent = (int)(((4095U - raw) * 100U + 2047U) / 4095U);
    return percent;
}

bool DHT22_Read(float *temperature, float *humidity) {
    uint8_t data[5] = {0, 0, 0, 0, 0};

    dht22_pin_output();
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET);
    delay_us(1200);

    taskENTER_CRITICAL();

    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);
    dht22_pin_input();
    delay_us(20);

    /* Wait for DHT22 response: LOW (~80us) -> HIGH (~80us) -> LOW (start of bit 0) */
    if (!wait_for_pin_state(GPIO_PIN_RESET, 100) ||
        !wait_for_pin_state(GPIO_PIN_SET, 100)   ||
        !wait_for_pin_state(GPIO_PIN_RESET, 100)) {
        taskEXIT_CRITICAL();
        return false;
    }

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

    uint8_t checksum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];

    if (checksum != data[4] || raw_hum == 0 || raw_hum > 1000) {
        return false;
    }

    *humidity = (float)raw_hum / 10.0f;
    if (raw_temp & 0x8000) {
        raw_temp &= 0x7FFF;
        *temperature = -((float)raw_temp / 10.0f);
    } else {
        *temperature = (float)raw_temp / 10.0f;
    }

    return true;
}