#include "touch.h"

#include "main.h"

#include "dashboard_ui.h"

#include "FreeRTOS.h"
#include "task.h"

__weak uint8_t Touch_Driver_ReadPoint(uint16_t *x, uint16_t *y)
{
    /* Call FT5426 read function */
    return Touch_Ft5426ReadPoint(x, y);
}

uint8_t Touch_Ft5426ReadPoint(uint16_t *x, uint16_t *y)
{
    uint8_t buf[5];
    uint8_t i = 0;
    uint8_t res = 0;

    /* Start I2C communication */
    Touch_I2cStart();
    Touch_I2cSendByte(0x70);  /* FT5426 address 0x38 << 1 */
    if(Touch_I2cWaitAck() != 0) {
        Touch_I2cStop();
        return 0;
    }
    Touch_I2cSendByte(0x02);  /* Register address */
    Touch_I2cWaitAck();
    Touch_I2cStop();

    /* Read data */
    Touch_I2cStart();
    Touch_I2cSendByte(0x71);  /* Read address */
    Touch_I2cWaitAck();

    for(i = 0; i < 4; i++) {
        buf[i] = Touch_I2cReadByte((i == 3) ? 0 : 1);  /* Last byte NACK */
    }
    Touch_I2cStop();

    /* Parse data */
    if((buf[0] & 0x0F) > 0) {  /* Touch detected */
        *x = ((uint16_t)(buf[0] & 0x0F) << 8) | buf[1];
        *y = ((uint16_t)(buf[2] & 0x0F) << 8) | buf[3];
        res = 1;
    } else {
        res = 0;
    }

    return res;
}

void Touch_ServiceTask(void *argument)
{
    uint16_t x = 0U;
    uint16_t y = 0U;
    uint8_t pressed = 0U;
    uint8_t last_pressed = 0U;

    (void)argument;

    /* Initialize FT5426 touch controller */
    if(Touch_Ft5426Init() != 0U) {
        /* Initialization failed, exit task */
        vTaskDelete(NULL);
    }

    for(;;) {
        pressed = Touch_Driver_ReadPoint(&x, &y);

        if((pressed != 0U) || (last_pressed != 0U)) {
            Dashboard_UI_SubmitTouchState(x, y, pressed);
        }

        last_pressed = pressed;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}