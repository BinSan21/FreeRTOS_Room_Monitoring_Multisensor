#ifndef MOTION_H
#define MOTION_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void Motion_Init(void);
bool Motion_ReadPIR(void);

#ifdef __cplusplus
}
#endif

#endif /* MOTION_H */