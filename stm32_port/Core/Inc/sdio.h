/**
  ******************************************************************************
  * @file    sdio.h
  * @brief   This file contains all the function prototypes for
  *          the sdio.c file
  ******************************************************************************
  */
#ifndef __SDIO_H__
#define __SDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

extern SD_HandleTypeDef hsd;
extern uint8_t g_sd_ready;

void MX_SDIO_SD_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __SDIO_H__ */
