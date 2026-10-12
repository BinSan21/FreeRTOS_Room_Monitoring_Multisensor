#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sensors.h"
#include "rtos_objects.h"
#include "display.h"
#include "input.h"
#include "alarm.h"
#include <stdio.h>

UART_HandleTypeDef huart1;

int _write(int file, char *ptr, int len) {
    (void)file;
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
    return len;
}

static void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_CFGR_PLLXTPRE_HSE;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void MX_USART1_UART_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

/* Producer: SensorTask (Priority 2) reads sensors and pushes to queue */
static void vSensorTask(void *pvParameters) {
    (void)pvParameters;
    TickType_t lastWakeTime = xTaskGetTickCount();
    SensorData data = {0.0f, 0.0f, 0, false};

    for (;;) {
        data.lightLevel = LDR_ReadPercent();
        if (DHT22_Read(&data.temperature, &data.humidity)) {
            xQueueSend(xSensorDisplayQueue, &data, 0);
            xQueueSend(xSensorAlarmQueue, &data, 0);
        } else {
            printf("[SensorTask] DHT22 read error\r\n");
        }
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(2000));
    }
}

/* Consumer: DisplayTask (Priority 1) updates OLED on new sensor data or encoder change */
static void vDisplayTask(void *pvParameters) {
    (void)pvParameters;
    SensorData latestData = {25.4f, 61.2f, 76, false};
    DisplayMode currentMode = DISPLAY_MODE_TEMPERATURE;

    Display_RenderScreen(&latestData, currentMode);

    for (;;) {
        bool needsRedraw = false;
        if (xQueueReceive(xDisplayModeQueue, &currentMode, pdMS_TO_TICKS(50)) == pdPASS) {
            needsRedraw = true;
        }
        if (xQueueReceive(xSensorDisplayQueue, &latestData, 0) == pdPASS) {
            printf("[Queue -> DisplayTask] Temp: %.2f C | Hum: %.2f %% | Light: %d %%\r\n",
                   latestData.temperature, latestData.humidity, latestData.lightLevel);
            needsRedraw = true;
        }
        if (needsRedraw) {
            Display_RenderScreen(&latestData, currentMode);
        }
    }
}

/* Part VII: InputTask (Priority 3) polls rotary encoder and updates DisplayMode */
static void vInputTask(void *pvParameters) {
    (void)pvParameters;
    DisplayMode mode = DISPLAY_MODE_TEMPERATURE;

    for (;;) {
        int step = Input_PollEncoderStep();
        if (step > 0) {
            mode = nextDisplayMode(mode);
            xQueueSend(xDisplayModeQueue, &mode, 0);
            printf("[InputTask] Encoder CW -> Mode %d\r\n", (int)mode);
        } else if (step < 0) {
            mode = previousDisplayMode(mode);
            xQueueSend(xDisplayModeQueue, &mode, 0);
            printf("[InputTask] Encoder CCW -> Mode %d\r\n", (int)mode);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/* Part VIII: AlarmTask (Priority 2) evaluates temperature and drives buzzer */
static void vAlarmTask(void *pvParameters) {
    (void)pvParameters;
    SensorData data = {0};

    for (;;) {
        if (xQueueReceive(xSensorAlarmQueue, &data, portMAX_DELAY) == pdPASS) {
            AlarmState state = evaluateTemperature(data.temperature);
            if (state == ALARM_STATE_LOW_TEMPERATURE) {
                Alarm_SetBuzzer(true);
                printf("[AlarmTask] LOW TEMP ALARM (%.2f C < 18.0 C) - Buzzer ON\r\n", data.temperature);
            } else if (state == ALARM_STATE_HIGH_TEMPERATURE) {
                Alarm_SetBuzzer(true);
                printf("[AlarmTask] HIGH TEMP ALARM (%.2f C > 30.0 C) - Buzzer ON\r\n", data.temperature);
            } else {
                Alarm_SetBuzzer(false);
            }
        }
    }
}

void vApplicationTickHook(void) {
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    while (1) {
    }
}

void vApplicationMallocFailedHook(void) {
    while (1) {
    }
}

void Diag_AssertFailed(const char *file, int line) {
    (void)file;
    (void)line;
    while (1) {
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();
    Sensors_Init();
    Display_Init();
    Input_Init();
    Alarm_Init();
    RTOS_Objects_Init();

    printf("BCA182 FreeRTOS Multisensor\r\n");
    printf("System starting...\r\n");

    xTaskCreate(vInputTask,   "InputTask",   256, NULL, 3, NULL);
    xTaskCreate(vSensorTask,  "SensorTask",  512, NULL, 2, NULL);
    xTaskCreate(vAlarmTask,   "AlarmTask",   512, NULL, 2, NULL);
    xTaskCreate(vDisplayTask, "DisplayTask", 512, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1) {
    }
}