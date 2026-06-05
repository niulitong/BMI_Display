#ifndef TOUCH_H
#define TOUCH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TOUCH_TYPE_CAP  1

void Touch_ServiceTask(void *argument);
void Touch_Process(void);
uint8_t Touch_Xpt2046Init(void);
uint8_t Touch_Xpt2046ReadPoint(uint16_t *x, uint16_t *y);

#ifdef __cplusplus
}
#endif

#endif
