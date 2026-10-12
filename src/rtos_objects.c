#include "rtos_objects.h"
#include "sensors.h"
#include "input.h"
#include <stdio.h>
#include <stdarg.h>

QueueHandle_t xSensorDisplayQueue      = NULL;
QueueHandle_t xSensorAlarmQueue        = NULL;
QueueHandle_t xDisplayModeQueue        = NULL;
EventGroupHandle_t xSystemEventGroup   = NULL;
SemaphoreHandle_t xSerialMutex         = NULL;

void RTOS_Objects_Init(void) {
    xSensorDisplayQueue = xQueueCreate(4, sizeof(SensorData));
    xSensorAlarmQueue   = xQueueCreate(4, sizeof(SensorData));
    xDisplayModeQueue   = xQueueCreate(4, sizeof(DisplayMode));
    xSystemEventGroup   = xEventGroupCreate();
    xSerialMutex        = xSemaphoreCreateMutex();

    /* System starts in ACTIVE state */
    if (xSystemEventGroup != NULL) {
        xEventGroupSetBits(xSystemEventGroup, EVENT_ACTIVE);
    }
}

/* Part XI Step 36: Thread-safe Serial logging protected by FreeRTOS Mutex */
void Serial_Printf(const char *fmt, ...) {
    va_list args;
    if (xSerialMutex != NULL) {
        xSemaphoreTake(xSerialMutex, portMAX_DELAY);
    }
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    if (xSerialMutex != NULL) {
        xSemaphoreGive(xSerialMutex);
    }
}