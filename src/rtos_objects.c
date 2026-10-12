#include "rtos_objects.h"
#include "sensors.h"

QueueHandle_t xSensorDisplayQueue = NULL;
QueueHandle_t xSensorAlarmQueue = NULL;

void RTOS_Objects_Init(void) {
    /* Depth 1 with xQueueOverwrite or Depth 4 FIFO queue for SensorData */
    xSensorDisplayQueue = xQueueCreate(4, sizeof(SensorData));
    xSensorAlarmQueue   = xQueueCreate(4, sizeof(SensorData));
}