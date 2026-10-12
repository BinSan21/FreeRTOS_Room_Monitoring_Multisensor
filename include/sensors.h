#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>

void Sensors_Init(void);
bool DHT22_Read(float *temperature, float *humidity);

#endif /* SENSORS_H */