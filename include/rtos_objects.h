#ifndef RTOS_OBJECTS_H
#define RTOS_OBJECTS_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

/* Queue used by SensorTask to send SensorData to DisplayTask */
extern QueueHandle_t xSensorDisplayQueue;

/* Queue used by SensorTask to send SensorData to AlarmTask */
extern QueueHandle_t xSensorAlarmQueue;

void RTOS_Objects_Init(void);

#endif /* RTOS_OBJECTS_H */