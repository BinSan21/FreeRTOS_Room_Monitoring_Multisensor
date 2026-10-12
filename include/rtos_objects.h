#ifndef RTOS_OBJECTS_H
#define RTOS_OBJECTS_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

extern QueueHandle_t xSensorDisplayQueue;
extern QueueHandle_t xSensorAlarmQueue;
extern QueueHandle_t xDisplayModeQueue;

void RTOS_Objects_Init(void);

#endif /* RTOS_OBJECTS_H */