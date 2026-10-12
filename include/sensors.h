#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>

typedef struct {
    float temperature;
    float humidity;
    int lightLevel;
    bool motionDetected;
} SensorData;

void Sensors_Init(void);
bool DHT22_Read(float *temperature, float *humidity);
int LDR_ReadPercent(void);

#endif /* SENSORS_H */