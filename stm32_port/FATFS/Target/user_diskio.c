/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * @file    user_diskio.c
  * @brief   SD card disk I/O driver for FatFS via STM32 HAL SD
  ******************************************************************************
  */
 /* USER CODE END Header */

#ifdef USE_OBSOLETE_USER_CODE_SECTION_0
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
#endif

/* USER CODE BEGIN DECL */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "ff_gen_drv.h"
#include "sdio.h"

/* Private variables ---------------------------------------------------------*/
static volatile DSTATUS Stat = STA_NOINIT;

/* USER CODE END DECL */

/* Private function prototypes -----------------------------------------------*/
DSTATUS USER_initialize (BYTE pdrv);
DSTATUS USER_status (BYTE pdrv);
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
  DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif
#if _USE_IOCTL == 1
  DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff);
#endif

Diskio_drvTypeDef  USER_Driver =
{
  USER_initialize,
  USER_status,
  USER_read,
#if  _USE_WRITE
  USER_write,
#endif
#if  _USE_IOCTL == 1
  USER_ioctl,
#endif
};

/* Private functions ---------------------------------------------------------*/

DSTATUS USER_initialize (
	BYTE pdrv
)
{
  /* USER CODE BEGIN INIT */
  Stat = STA_NOINIT;

  if (g_sd_ready && HAL_SD_GetCardState(&hsd) == HAL_SD_CARD_TRANSFER) {
    Stat &= ~STA_NOINIT;
  }

  return Stat;
  /* USER CODE END INIT */
}

DSTATUS USER_status (
	BYTE pdrv
)
{
  /* USER CODE BEGIN STATUS */
  if (g_sd_ready && HAL_SD_GetCardState(&hsd) == HAL_SD_CARD_TRANSFER) {
    Stat &= ~STA_NOINIT;
  } else {
    Stat = STA_NOINIT;
  }
  return Stat;
  /* USER CODE END STATUS */
}

DRESULT USER_read (
	BYTE pdrv,
	BYTE *buff,
	DWORD sector,
	UINT count
)
{
  /* USER CODE BEGIN READ */
  DRESULT res = RES_ERROR;

  if (HAL_SD_ReadBlocks(&hsd, buff, sector, count, 10000) == HAL_OK) {
    res = RES_OK;
  }

  return res;
  /* USER CODE END READ */
}

#if _USE_WRITE == 1
DRESULT USER_write (
	BYTE pdrv,
	const BYTE *buff,
	DWORD sector,
	UINT count
)
{
  /* USER CODE BEGIN WRITE */
  DRESULT res = RES_ERROR;

  if (HAL_SD_WriteBlocks(&hsd, (uint8_t *)buff, sector, count, 20000) == HAL_OK) {
    res = RES_OK;
  }

  return res;
  /* USER CODE END WRITE */
}
#endif

#if _USE_IOCTL == 1
DRESULT USER_ioctl (
	BYTE pdrv,
	BYTE cmd,
	void *buff
)
{
  /* USER CODE BEGIN IOCTL */
  DRESULT res = RES_ERROR;
  HAL_SD_CardInfoTypeDef CardInfo;

  if (HAL_SD_GetCardInfo(&hsd, &CardInfo) != HAL_OK) {
    return RES_ERROR;
  }

  switch (cmd) {
    case CTRL_SYNC:
      res = RES_OK;
      break;

    case GET_SECTOR_COUNT:
      *(DWORD *)buff = CardInfo.BlockNbr;
      res = RES_OK;
      break;

    case GET_SECTOR_SIZE:
      *(WORD *)buff = CardInfo.BlockSize;
      res = RES_OK;
      break;

    case GET_BLOCK_SIZE:
      *(DWORD *)buff = 1;
      res = RES_OK;
      break;

    default:
      res = RES_PARERR;
      break;
  }

  return res;
  /* USER CODE END IOCTL */
}
#endif
