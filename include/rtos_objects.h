#ifndef RTOS_OBJECTS_H
#define RTOS_OBJECTS_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

/* Part X Step 35: Event Group bit definitions */
#define EVENT_ACTIVE (1UL << 0) /* BIT0: System is in ACTIVE state */
#define EVENT_MOTION (1UL << 1) /* BIT1: PIR motion currently detected */
#define EVENT_ALARM  (1UL << 2) /* BIT2: Temperature alarm is active */

extern QueueHandle_t xSensorDisplayQueue;
extern QueueHandle_t xSensorAlarmQueue;
extern QueueHandle_t xDisplayModeQueue;
extern EventGroupHandle_t xSystemEventGroup;
extern SemaphoreHandle_t xSerialMutex;

void RTOS_Objects_Init(void);
void Serial_Printf(const char *fmt, ...);

#endif /* RTOS_OBJECTS_H */