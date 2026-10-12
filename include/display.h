#ifndef DISPLAY_H
#define DISPLAY_H

#include "sensors.h"
#include <stdbool.h>

void Display_Init(void);
void Display_Clear(void);
void Display_SetPower(bool on);
void Display_RenderTemperature(float temperature);

#endif /* DISPLAY_H */