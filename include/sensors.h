#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>

void Sensors_Init(void);
bool DHT22_Read(float *temperature, float *humidity);
int LDR_ReadPercent(void);

#endif /* SENSORS_H */