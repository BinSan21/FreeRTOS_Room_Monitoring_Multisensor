#ifndef ALARM_H
#define ALARM_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOW_TEMPERATURE_LIMIT  18.0f
#define HIGH_TEMPERATURE_LIMIT 30.0f

typedef enum {
    ALARM_STATE_NORMAL = 0,
    ALARM_STATE_LOW_TEMPERATURE,
    ALARM_STATE_HIGH_TEMPERATURE
} AlarmState;

/* Pure decision logic (Testable independently of STM32/Wokwi for Part XIV) */
AlarmState evaluateTemperature(float temperature);

/* Hardware buzzer driver on PA8 */
void Alarm_Init(void);
void Alarm_SetBuzzer(bool active);

#ifdef __cplusplus
}
#endif

#endif /* ALARM_H */