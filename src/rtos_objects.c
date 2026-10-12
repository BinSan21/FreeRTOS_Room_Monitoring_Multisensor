#include "rtos_objects.h"
#include "sensors.h"
#include "input.h"

QueueHandle_t xSensorDisplayQueue = NULL;
QueueHandle_t xSensorAlarmQueue   = NULL;
QueueHandle_t xDisplayModeQueue   = NULL;

void RTOS_Objects_Init(void) {
    xSensorDisplayQueue = xQueueCreate(4, sizeof(SensorData));
    xSensorAlarmQueue   = xQueueCreate(4, sizeof(SensorData));
    xDisplayModeQueue   = xQueueCreate(4, sizeof(DisplayMode));
}