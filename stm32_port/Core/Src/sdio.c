/**
  ******************************************************************************
  * @file    sdio.c
  * @brief   SDIO peripheral init for SD card access.
  ******************************************************************************
  */

#include "sdio.h"

SD_HandleTypeDef hsd;
uint8_t g_sd_ready = 0;

void MX_SDIO_SD_Init(void)
{
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv = 5;

  hsd.hdmarx = NULL;
  hsd.hdmatx = NULL;

  if (HAL_SD_Init(&hsd) == HAL_OK) {
    g_sd_ready = 1;
  }
}
