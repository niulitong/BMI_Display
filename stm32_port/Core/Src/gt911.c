#include "gt911.h"
#include "main.h"
#include "touch.h"

#if TOUCH_TYPE_CAP

extern I2C_HandleTypeDef hi2c1;

#define GT911_HOR_RES  800U
#define GT911_VER_RES  480U
#define GT911_I2C_TIMEOUT  10U

gt911_dev_t g_gt911_dev;

static uint8_t GT911_WriteRegister(uint16_t reg, uint8_t value)
{
    uint8_t buf[2] = {(uint8_t)(reg >> 8), (uint8_t)reg};
    if(HAL_I2C_Master_Transmit(&hi2c1, GT911_I2C_ADDR, buf, sizeof(buf), GT911_I2C_TIMEOUT) != HAL_OK) {
        return 0U;
    }
    if(HAL_I2C_Master_Transmit(&hi2c1, GT911_I2C_ADDR, &value, 1U, GT911_I2C_TIMEOUT) != HAL_OK) {
        return 0U;
    }
    return 1U;
}

static uint8_t GT911_ReadRegister(uint16_t reg, uint8_t cnt, uint8_t *data)
{
    uint8_t addr_buf[2] = {(uint8_t)(reg >> 8), (uint8_t)reg};
    if(HAL_I2C_Master_Transmit(&hi2c1, GT911_I2C_ADDR, addr_buf, sizeof(addr_buf), GT911_I2C_TIMEOUT) != HAL_OK) {
        return 0U;
    }
    if(HAL_I2C_Master_Receive(&hi2c1, GT911_I2C_ADDR, data, cnt, GT911_I2C_TIMEOUT) != HAL_OK) {
        return 0U;
    }
    return 1U;
}

static void GT911_IntOut(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = IPS_INT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(IPS_INT_GPIO_Port, &GPIO_InitStruct);
}

static void GT911_IntIn(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = IPS_INT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(IPS_INT_GPIO_Port, &GPIO_InitStruct);
}

static void GT911_RstLow(void)
{
    HAL_GPIO_WritePin(IPS_RST_GPIO_Port, IPS_RST_Pin, GPIO_PIN_RESET);
}

static void GT911_RstHigh(void)
{
    HAL_GPIO_WritePin(IPS_RST_GPIO_Port, IPS_RST_Pin, GPIO_PIN_SET);
}

static void GT911_Reset(void)
{
    GT911_RstLow();
    HAL_Delay(10);
    GT911_RstHigh();
    HAL_Delay(10);
}

static void GT911_IntSync(uint32_t ms)
{
    GT911_IntOut();
    HAL_GPIO_WritePin(IPS_INT_GPIO_Port, IPS_INT_Pin, GPIO_PIN_RESET);
    HAL_Delay(ms);
    GT911_IntIn();
}

static void GT911_ResetGuitar(void)
{
    GT911_IntOut();
    HAL_GPIO_WritePin(IPS_INT_GPIO_Port, IPS_INT_Pin, GPIO_PIN_SET);
    GT911_RstHigh();
    HAL_Delay(20);
    GT911_RstLow();
    HAL_GPIO_WritePin(IPS_INT_GPIO_Port, IPS_INT_Pin, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(IPS_INT_GPIO_Port, IPS_INT_Pin, GPIO_PIN_RESET);
    HAL_Delay(20);
    GT911_RstHigh();
    HAL_Delay(20);
}

uint8_t GT911_Init(void)
{
    uint8_t id_buf[4];

    GT911_Reset();
    GT911_ResetGuitar();
    GT911_IntSync(50);

    if(GT911_ReadRegister(GT911_ID_ADDR, 4U, id_buf) == 0U) {
        return 0U;
    }
    if(id_buf[0] == '9') {
        return 1U;
    }
    return 0U;
}

uint8_t GT911_GetTouchCount(void)
{
    uint8_t count = 0U;
    GT911_ReadRegister(GT911_READ_ADDR, 1U, &count);
    return (uint8_t)(count & 0x0FU);
}

uint8_t GT911_Scan(void)
{
    uint8_t buf[42];
    uint8_t i;
    uint8_t res = 0U;
    uint8_t temp;
    uint8_t tempsta;

    static uint8_t t = 0U;
    t++;
    if((t % 10U) == 0U || t < 10U) {
        if(GT911_ReadRegister(GT911_READ_ADDR, 42U, buf) == 0U) {
            return 0U;
        }

        if((buf[0] & 0x80U) && ((buf[0] & 0x0FU) < 6U)) {
            GT911_WriteRegister(GT911_READ_ADDR, 0U);
        }

        if((buf[0] & 0x0FU) && ((buf[0] & 0x0FU) < 6U)) {
            temp = 0U;
            for(i = 0U; i < (buf[0] & 0x0FU); i++) {
                switch(buf[1U + (uint32_t)i * 8U]) {
                    case 4: temp |= 1U << 4; break;
                    case 3: temp |= 1U << 3; break;
                    case 2: temp |= 1U << 2; break;
                    case 1: temp |= 1U << 1; break;
                    case 0: temp |= 1U << 0; break;
                    default: break;
                }
            }
            tempsta = g_gt911_dev.sta;
            g_gt911_dev.sta = temp | GT911_TP_PRES_DOWN | GT911_TP_CATH_PRES;
            g_gt911_dev.x[4] = g_gt911_dev.x[0];
            g_gt911_dev.y[4] = g_gt911_dev.y[0];

            for(i = 0U; i < GT911_MAX_TOUCH; i++) {
                if(g_gt911_dev.sta & (1U << i)) {
                    g_gt911_dev.x[i] = (uint16_t)(((uint16_t)buf[3U + (uint32_t)i * 8U] << 8) + buf[2U + (uint32_t)i * 8U]);
                    g_gt911_dev.y[i] = (uint16_t)(((uint16_t)buf[5U + (uint32_t)i * 8U] << 8) + buf[4U + (uint32_t)i * 8U]);
                }
            }

            if(g_gt911_dev.x[0] > GT911_HOR_RES || g_gt911_dev.y[0] > GT911_VER_RES) {
                if((buf[0] & 0x0FU) > 1U) {
                    g_gt911_dev.x[0] = g_gt911_dev.x[1];
                    g_gt911_dev.y[0] = g_gt911_dev.y[1];
                } else {
                    g_gt911_dev.x[0] = g_gt911_dev.x[4];
                    g_gt911_dev.y[0] = g_gt911_dev.y[4];
                    g_gt911_dev.sta = tempsta;
                }
            } else {
                t = 0U;
            }
            res = 1U;
        }
    }

    if((buf[0] & 0x8FU) == 0x80U) {
        if(g_gt911_dev.sta & GT911_TP_PRES_DOWN) {
            g_gt911_dev.sta &= (uint8_t)(~(1U << 7));
        } else {
            g_gt911_dev.x[0] = 0xFFFFU;
            g_gt911_dev.y[0] = 0xFFFFU;
            g_gt911_dev.sta &= 0xE0U;
        }
    }

    if(t > 240U) t = 10U;
    return res;
}

#endif /* TOUCH_TYPE_CAP */
