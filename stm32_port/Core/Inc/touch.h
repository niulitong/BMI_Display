#ifndef TOUCH_H
#define TOUCH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Touch_ServiceTask(void *argument);
uint8_t Touch_Driver_ReadPoint(uint16_t *x, uint16_t *y);

#ifdef __cplusplus
}
#endif

#endif