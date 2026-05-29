#ifndef TOUCH_H
#define TOUCH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Touch_ServiceTask(void *argument);
void Touch_Process(void);
uint8_t Touch_Driver_ReadPoint(uint16_t *x, uint16_t *y);
uint8_t Touch_Xpt2046ReadPoint(uint16_t *x, uint16_t *y);
uint8_t Touch_Xpt2046Init(void);

#ifdef __cplusplus
}
#endif

#endif
