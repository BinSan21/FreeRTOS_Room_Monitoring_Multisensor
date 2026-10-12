#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sensors.h"
#include "rtos_objects.h"
#include "display.h"
#include "input.h"
#include "alarm.h"
#include "motion.h"
#include "system_state.h"
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

/* Part IX: MotionTask (Priority 3) monitors PIR sensor and sets/clears EVENT_MOTION */
static void vMotionTask(void *pvParameters) {
    (void)pvParameters;
    bool lastMotion = false;

    for (;;) {
        bool currentMotion = Motion_ReadPIR();
        if (currentMotion) {
            xEventGroupSetBits(xSystemEventGroup, EVENT_MOTION);
            if (!lastMotion) {
                Serial_Printf("[MotionTask] PIR Motion DETECTED!\r\n");
            }
        } else {
            xEventGroupClearBits(xSystemEventGroup, EVENT_MOTION);
            if (lastMotion) {
                Serial_Printf("[MotionTask] PIR Motion ended\r\n");
            }
        }
        lastMotion = currentMotion;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* Part IX & X: StateTask (Priority 2) manages ACTIVE / INACTIVE state machine */
static void vStateTask(void *pvParameters) {
    (void)pvParameters;
    SystemState currentState = SYSTEM_STATE_ACTIVE;
    TickType_t lastMotionTick = xTaskGetTickCount();

    for (;;) {
        EventBits_t bits = xEventGroupGetBits(xSystemEventGroup);
        bool motionDetected = ((bits & EVENT_MOTION) != 0);

        if (motionDetected) {
            lastMotionTick = xTaskGetTickCount();
        }

        uint32_t elapsedInactiveMs = (uint32_t)((xTaskGetTickCount() - lastMotionTick) * portTICK_PERIOD_MS);
        SystemState nextState = evaluateSystemState(currentState, motionDetected, elapsedInactiveMs);

        if (nextState != currentState) {
            currentState = nextState;
            if (currentState == SYSTEM_STATE_ACTIVE) {
                xEventGroupSetBits(xSystemEventGroup, EVENT_ACTIVE);
                Serial_Printf("[StateTask] Motion detected -> System RESTORED to ACTIVE\r\n");
            } else {
                xEventGroupClearBits(xSystemEventGroup, EVENT_ACTIVE);
                Serial_Printf("[StateTask] Inactivity timeout (%lu ms) -> System ENTERED INACTIVE\r\n",
                              (unsigned long)elapsedInactiveMs);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/* Part VII: InputTask (Priority 3) polls rotary encoder when system is ACTIVE */
static void vInputTask(void *pvParameters) {
    (void)pvParameters;
    DisplayMode mode = DISPLAY_MODE_TEMPERATURE;

    for (;;) {
        EventBits_t bits = xEventGroupGetBits(xSystemEventGroup);
        if ((bits & EVENT_ACTIVE) != 0) {
            int step = Input_PollEncoderStep();
            if (step > 0) {
                mode = nextDisplayMode(mode);
                xQueueSend(xDisplayModeQueue, &mode, 0);
                Serial_Printf("[InputTask] Encoder CW -> Mode %d\r\n", (int)mode);
            } else if (step < 0) {
                mode = previousDisplayMode(mode);
                xQueueSend(xDisplayModeQueue, &mode, 0);
                Serial_Printf("[InputTask] Encoder CCW -> Mode %d\r\n", (int)mode);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/* Part IV & V: SensorTask (Priority 2) samples DHT22 & LDR every 2s using vTaskDelayUntil */
static void vSensorTask(void *pvParameters) {
    (void)pvParameters;
    TickType_t lastWakeTime = xTaskGetTickCount();
    SensorData data = {0.0f, 0.0f, 0, false};

    for (;;) {
        EventBits_t bits = xEventGroupGetBits(xSystemEventGroup);
        data.motionDetected = ((bits & EVENT_MOTION) != 0);
        data.lightLevel = LDR_ReadPercent();

        if (DHT22_Read(&data.temperature, &data.humidity)) {
            xQueueSend(xSensorDisplayQueue, &data, 0);
            xQueueSend(xSensorAlarmQueue, &data, 0);
        } else {
            Serial_Printf("[SensorTask] DHT22 read error\r\n");
        }
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(2000));
    }
}

/* Part VIII: AlarmTask (Priority 2) evaluates temperature and controls buzzer */
static void vAlarmTask(void *pvParameters) {
    (void)pvParameters;
    SensorData data = {0};

    for (;;) {
        if (xQueueReceive(xSensorAlarmQueue, &data, portMAX_DELAY) == pdPASS) {
            EventBits_t bits = xEventGroupGetBits(xSystemEventGroup);
            if ((bits & EVENT_ACTIVE) == 0) {
                Alarm_SetBuzzer(false);
                xEventGroupClearBits(xSystemEventGroup, EVENT_ALARM);
                continue;
            }

            AlarmState state = evaluateTemperature(data.temperature);
            if (state == ALARM_STATE_LOW_TEMPERATURE) {
                xEventGroupSetBits(xSystemEventGroup, EVENT_ALARM);
                Alarm_SetBuzzer(true);
                Serial_Printf("[AlarmTask] LOW TEMP ALARM (%.2f C < 18.0 C) - Buzzer ON\r\n", data.temperature);
            } else if (state == ALARM_STATE_HIGH_TEMPERATURE) {
                xEventGroupSetBits(xSystemEventGroup, EVENT_ALARM);
                Alarm_SetBuzzer(true);
                Serial_Printf("[AlarmTask] HIGH TEMP ALARM (%.2f C > 30.0 C) - Buzzer ON\r\n", data.temperature);
            } else {
                xEventGroupClearBits(xSystemEventGroup, EVENT_ALARM);
                Alarm_SetBuzzer(false);
            }
        }
    }
}

/* Part VI: DisplayTask (Priority 1) owns OLED and blanks screen when INACTIVE */
static void vDisplayTask(void *pvParameters) {
    (void)pvParameters;
    SensorData latestData = {25.4f, 61.2f, 76, false};
    DisplayMode currentMode = DISPLAY_MODE_TEMPERATURE;
    bool oledPowered = true;

    Display_RenderScreen(&latestData, currentMode);

    for (;;) {
        EventBits_t bits = xEventGroupGetBits(xSystemEventGroup);
        bool isActive = ((bits & EVENT_ACTIVE) != 0);

        if (!isActive) {
            if (oledPowered) {
                Display_Clear();
                Display_SetPower(false);
                oledPowered = false;
            }
            /* Drain sensor queue without redrawing while INACTIVE */
            xQueueReceive(xSensorDisplayQueue, &latestData, pdMS_TO_TICKS(100));
            continue;
        }

        bool needsRedraw = false;
        if (!oledPowered) {
            Display_SetPower(true);
            oledPowered = true;
            needsRedraw = true;
        }

        if (xQueueReceive(xDisplayModeQueue, &currentMode, pdMS_TO_TICKS(50)) == pdPASS) {
            needsRedraw = true;
        }
        if (xQueueReceive(xSensorDisplayQueue, &latestData, 0) == pdPASS) {
            Serial_Printf("[Queue -> DisplayTask] Temp: %.2f C | Hum: %.2f %% | Light: %d %% | Motion: %s\r\n",
                          latestData.temperature, latestData.humidity, latestData.lightLevel,
                          latestData.motionDetected ? "YES" : "NO");
            needsRedraw = true;
        }

        /* Keep Motion page responsive immediately when PIR changes */
        bool currentMotion = ((bits & EVENT_MOTION) != 0);
        if (latestData.motionDetected != currentMotion) {
            latestData.motionDetected = currentMotion;
            if (currentMode == DISPLAY_MODE_MOTION) {
                needsRedraw = true;
            }
        }

        if (needsRedraw) {
            Display_RenderScreen(&latestData, currentMode);
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
    Motion_Init();
    RTOS_Objects_Init();

    printf("BCA182 FreeRTOS Multisensor\r\n");
    printf("System starting...\r\n");

    /* Part XII Step 38: Explicit Task Priorities */
    xTaskCreate(vMotionTask,  "MotionTask",  192, NULL, 3, NULL);
    xTaskCreate(vInputTask,   "InputTask",   192, NULL, 3, NULL);
    xTaskCreate(vSensorTask,  "SensorTask",  384, NULL, 2, NULL);
    xTaskCreate(vAlarmTask,   "AlarmTask",   384, NULL, 2, NULL);
    xTaskCreate(vStateTask,   "StateTask",   256, NULL, 2, NULL);
    xTaskCreate(vDisplayTask, "DisplayTask", 384, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1) {
    }
}