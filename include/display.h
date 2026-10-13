#ifndef DISPLAY_H
#define DISPLAY_H

#include "sensors.h"
#include "input.h"
#include <stdbool.h>

void Display_Init(void);
void Display_Clear(void);
void Display_SetPower(bool on);
void Display_RenderScreen(const SensorData *data, DisplayMode mode);

#endif /* DISPLAY_H */