#ifndef GT911_H
#define GT911_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define GT911_MAX_TOUCH     5
#define GT911_TP_PRES_DOWN  0x80
#define GT911_TP_CATH_PRES  0x40

#define GT911_I2C_ADDR_5D   (0x5DU << 1)
#define GT911_I2C_ADDR_14   (0x14U << 1)
#define GT911_READ_ADDR     0x814EU
#define GT911_ID_ADDR       0x8140U

typedef struct {
    uint16_t x[GT911_MAX_TOUCH];
    uint16_t y[GT911_MAX_TOUCH];
    uint8_t  sta;
} gt911_dev_t;

extern gt911_dev_t g_gt911_dev;

uint8_t GT911_Init(void);
uint8_t GT911_Scan(void);
uint8_t GT911_GetTouchCount(void);

#endif
