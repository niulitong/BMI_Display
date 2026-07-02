/**
  ******************************************************************************
  * @file    stm32f4xx_hal_sd.c
  * @brief   SD card HAL module driver.
  ******************************************************************************
  */

#include "stm32f4xx_hal.h"

#ifdef HAL_SD_MODULE_ENABLED

#define SDIO_INIT_CLK_DIV        ((uint8_t)0xFE)
#define SDIO_TRANSFER_CLK_DIV    ((uint8_t)0x05)

#define SD_CMD_TIMEOUT           ((uint32_t)0xFFFFFFFFU)
#define SD_TIMEOUT_MS            3000U
#define SD_VOLTAGE_TRIAL         0x0000FFFFU

#define SD_CMDRESP_NO             ((uint32_t)0x00000000U)
#define SD_CMDRESP_SHORT          SDIO_CMD_WAITRESP_0
#define SD_CMDRESP_LONG           (SDIO_CMD_WAITRESP_0 | SDIO_CMD_WAITRESP_1)

#define SDIO_BLOCKSIZE_512        (SDIO_DCTRL_DBLOCKSIZE_0 | SDIO_DCTRL_DBLOCKSIZE_3)

static HAL_StatusTypeDef SD_PowerON(SD_HandleTypeDef *hsd);
static HAL_StatusTypeDef SD_InitCard(SD_HandleTypeDef *hsd);
static HAL_StatusTypeDef SD_EnableWideBusOperation(SD_HandleTypeDef *hsd, uint32_t WideMode);
static HAL_StatusTypeDef SDMMC_CmdAppCommand(SD_HandleTypeDef *hsd, uint32_t Argument);

HAL_StatusTypeDef HAL_SD_Init(SD_HandleTypeDef *hsd)
{
  uint32_t tmpclk;

  if (hsd == NULL) {
    return HAL_ERROR;
  }

  if (hsd->State == HAL_SD_STATE_BUSY) {
    return HAL_BUSY;
  }

  __HAL_LOCK(hsd);

  hsd->State = HAL_SD_STATE_BUSY;

  hsd->CardState = HAL_SD_CARD_DISCONNECTED;
  hsd->ErrorCode = HAL_SD_ERROR_NONE;

  HAL_SD_MspInit(hsd);

  if (hsd->hdmarx != NULL) {
    __HAL_LINKDMA(hsd, hdmarx, *hsd->hdmarx);
    hsd->hdmarx->XferCpltCallback = NULL;
    hsd->hdmarx->XferErrorCallback = NULL;
    hsd->hdmarx->XferAbortCallback = NULL;
  }

  if (hsd->hdmatx != NULL) {
    __HAL_LINKDMA(hsd, hdmatx, *hsd->hdmatx);
    hsd->hdmatx->XferCpltCallback = NULL;
    hsd->hdmatx->XferErrorCallback = NULL;
    hsd->hdmatx->XferAbortCallback = NULL;
  }

  SDIO->POWER = 0x00;

  tmpclk = hsd->Init.ClockDiv | SDIO_CLKCR_CLKEN;
  tmpclk |= hsd->Init.ClockEdge;
  tmpclk |= hsd->Init.ClockBypass;
  tmpclk |= hsd->Init.ClockPowerSave;
  tmpclk |= hsd->Init.HardwareFlowControl;
  tmpclk |= hsd->Init.BusWide;

  SDIO->CLKCR = tmpclk;

  SDIO->POWER = SDIO_POWER_PWRCTRL_0 | SDIO_POWER_PWRCTRL_1;

  HAL_Delay(2);

  if (SD_PowerON(hsd) != HAL_OK) {
    __HAL_UNLOCK(hsd);
    return HAL_ERROR;
  }

  if (SD_InitCard(hsd) != HAL_OK) {
    __HAL_UNLOCK(hsd);
    return HAL_ERROR;
  }

  if (SD_EnableWideBusOperation(hsd, SDIO_BUS_WIDE_4B) != HAL_OK) {
    __HAL_UNLOCK(hsd);
    return HAL_ERROR;
  }

  hsd->State = HAL_SD_STATE_READY;
  __HAL_UNLOCK(hsd);
  return HAL_OK;
}

HAL_StatusTypeDef HAL_SD_DeInit(SD_HandleTypeDef *hsd)
{
  if (hsd == NULL) {
    return HAL_ERROR;
  }

  hsd->State = HAL_SD_STATE_RESET;
  SDIO->POWER = 0x00;
  __HAL_RCC_SDIO_CLK_DISABLE();
  __HAL_RCC_SDIO_FORCE_RESET();
  __HAL_RCC_SDIO_RELEASE_RESET();

  return HAL_OK;
}

static HAL_StatusTypeDef SD_PowerON(SD_HandleTypeDef *hsd)
{
  uint32_t SD_OCR = SD_OCR_VDD_27_28 | SD_OCR_VDD_28_29 | SD_OCR_VDD_29_30 |
                    SD_OCR_VDD_30_31 | SD_OCR_VDD_31_32 | SD_OCR_VDD_32_33 |
                    SD_OCR_VDD_33_34 | SD_OCR_VDD_34_35 | SD_OCR_VDD_35_36;
  uint32_t tickstart;
  uint32_t response = 0;
  uint32_t validvoltage = 0;

  SDIO->CLKCR &= ~SDIO_CLKCR_CLKEN;
  SDIO->CLKCR = (SDIO->CLKCR & ~SDIO_CLKCR_CLKDIV) | SDIO_INIT_CLK_DIV;
  SDIO->CLKCR |= SDIO_CLKCR_CLKEN;

  SDIO->POWER = SDIO_POWER_PWRCTRL_0 | SDIO_POWER_PWRCTRL_1;

  SDIO->ARG = 0;
  SDIO->CMD = (SDIO_CMD_CPSMEN | SD_CMDRESP_NO) | SD_CMD_GO_IDLE_STATE;

  tickstart = HAL_GetTick();
  while (!(SDIO->STA & (SDIO_STA_CMDSENT | SDIO_STA_CTIMEOUT))) {
    if ((HAL_GetTick() - tickstart) > 500U) {
      hsd->ErrorCode |= HAL_SD_ERROR_CMD_RSP_TIMEOUT;
      return HAL_ERROR;
    }
  }

  if (SDIO->STA & SDIO_STA_CTIMEOUT) {
    hsd->ErrorCode |= HAL_SD_ERROR_CMD_RSP_TIMEOUT;
    return HAL_ERROR;
  }

  SDIO->ICR = SDIO_STATIC_FLAGS;

  SDIO->ARG = SD_OCR | SD_OCR_HIGH_CAPACITY;
  SDIO->CMD = (SDIO_CMD_CPSMEN | SD_CMDRESP_SHORT) | SD_CMD_APP_CMD;

  tickstart = HAL_GetTick();
  while (!(SDIO->STA & (SDIO_STA_CMDSENT | SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL))) {
    if ((HAL_GetTick() - tickstart) > 500U) {
      hsd->ErrorCode |= HAL_SD_ERROR_CMD_RSP_TIMEOUT;
      SDIO->ICR = SDIO_STATIC_FLAGS;
      return HAL_ERROR;
    }
  }

  if (SDIO->STA & (SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL)) {
    hsd->ErrorCode |= HAL_SD_ERROR_CMD_RSP_TIMEOUT;
    SDIO->ICR = SDIO_STATIC_FLAGS;
    return HAL_ERROR;
  }

  SDIO->ICR = SDIO_STATIC_FLAGS;

  tickstart = HAL_GetTick();
  while (!validvoltage) {
    if ((HAL_GetTick() - tickstart) > SD_TIMEOUT_MS) {
      hsd->ErrorCode |= HAL_SD_ERROR_INVALID_VOLTRANGE;
      return HAL_ERROR;
    }

    SDIO->ARG = SD_OCR | SD_OCR_HIGH_CAPACITY;
    SDIO->CMD = (SDIO_CMD_CPSMEN | SD_CMDRESP_SHORT) | SD_CMD_SD_APP_OP_COND;

    {
      uint32_t t = HAL_GetTick();
      while (!(SDIO->STA & (SDIO_STA_CMDSENT | SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL))) {
        if ((HAL_GetTick() - t) > 200U) break;
      }
    }

    if (SDIO->STA & (SDIO_STA_CMDREND | SDIO_STA_CMDSENT)) {
      response = SDIO->RESP1;
      if ((response & SD_OCR_POWER_UP_BUSY) == SD_OCR_POWER_UP_BUSY) {
        validvoltage = 1;
      }
    }

    SDIO->ICR = SDIO_STATIC_FLAGS;
    HAL_Delay(1);
  }

  if ((response & SD_OCR_HIGH_CAPACITY) == SD_OCR_HIGH_CAPACITY) {
    hsd->CardType = SD_CARD_TYPE_SDHC_SDXC;
  } else {
    hsd->CardType = SD_CARD_TYPE_SDSC;
  }

  hsd->CardState = HAL_SD_CARD_READY;

  return HAL_OK;
}

static HAL_StatusTypeDef SD_SendCmd(uint32_t cmdreg, uint32_t arg, uint32_t timeout_ms)
{
  uint32_t t = HAL_GetTick();
  SDIO->ARG = arg;
  SDIO->CMD = cmdreg;
  while (!(SDIO->STA & (SDIO_STA_CMDSENT | SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL))) {
    if ((HAL_GetTick() - t) > timeout_ms) {
      return HAL_ERROR;
    }
  }
  if (SDIO->STA & (SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL)) {
    return HAL_ERROR;
  }
  SDIO->ICR = SDIO_STATIC_FLAGS;
  return HAL_OK;
}

static HAL_StatusTypeDef SD_InitCard(SD_HandleTypeDef *hsd)
{
  uint16_t rca;
  uint32_t csd_ver, c_size, c_mult, read_bl, blk_nbr, blk_len;

  if (SD_SendCmd((SDIO_CMD_CPSMEN | SD_CMDRESP_LONG) | SD_CMD_ALL_SEND_CID, 0, 500U) != HAL_OK) {
    hsd->ErrorCode |= HAL_SD_ERROR_CMD_RSP_TIMEOUT;
    return HAL_ERROR;
  }

  hsd->CID[0] = SDIO->RESP1;
  hsd->CID[1] = SDIO->RESP2;
  hsd->CID[2] = SDIO->RESP3;
  hsd->CID[3] = SDIO->RESP4;

  hsd->CardState = HAL_SD_CARD_IDENTIFICATION;

  if (SD_SendCmd((SDIO_CMD_CPSMEN | SD_CMDRESP_SHORT) | SD_CMD_SET_REL_ADDR, 0, 500U) != HAL_OK) {
    hsd->ErrorCode |= HAL_SD_ERROR_CMD_RSP_TIMEOUT;
    return HAL_ERROR;
  }

  rca = (uint16_t)(SDIO->RESP1 >> 16);
  hsd->RCA = rca;

  if (SD_SendCmd((SDIO_CMD_CPSMEN | SD_CMDRESP_LONG) | SD_CMD_SEND_CSD, (uint32_t)rca << 16, 500U) != HAL_OK) {
    hsd->ErrorCode |= HAL_SD_ERROR_CMD_RSP_TIMEOUT;
    return HAL_ERROR;
  }

  hsd->CSD[0] = SDIO->RESP1;
  hsd->CSD[1] = SDIO->RESP2;
  hsd->CSD[2] = SDIO->RESP3;
  hsd->CSD[3] = SDIO->RESP4;

  if (SD_SendCmd((SDIO_CMD_CPSMEN | SD_CMDRESP_SHORT) | SD_CMD_SEL_DESEL_CARD, (uint32_t)rca << 16, 500U) != HAL_OK) {
    hsd->ErrorCode |= HAL_SD_ERROR_CMD_RSP_TIMEOUT;
    return HAL_ERROR;
  }

  SDIO->CLKCR &= ~SDIO_CLKCR_CLKEN;
  SDIO->CLKCR = (SDIO->CLKCR & ~SDIO_CLKCR_CLKDIV) | SDIO_TRANSFER_CLK_DIV;
  SDIO->CLKCR |= SDIO_CLKCR_CLKEN;

  hsd->CardState = HAL_SD_CARD_TRANSFER;

  hsd->SdCard.BlockSize = 512;
  hsd->SdCard.LogBlockSize = 9;
  hsd->SdCard.CardType = hsd->CardType;

  csd_ver = (hsd->CSD[0] >> 30) & 0x03;

  if (csd_ver == 1) {
    c_size = ((hsd->CSD[1] & 0x0000003F) << 16) | ((hsd->CSD[2] >> 16) & 0x0000FFFF);
    hsd->SdCard.BlockNbr = (c_size + 1) * 1024;
    hsd->SdCard.LogBlockNbr = hsd->SdCard.BlockNbr;
  } else {
    c_size   = (((hsd->CSD[1] >> 8) & 0x00000003) << 10) |
               (( hsd->CSD[1]        & 0x000000FF) << 2)  |
               (( hsd->CSD[2] >> 30) & 0x00000003);
    c_mult   = (hsd->CSD[2] >> 15) & 0x00000007;
    read_bl  = (hsd->CSD[1] >> 16) & 0x0000000F;
    blk_nbr  = (c_size + 1) * ((uint32_t)1 << (c_mult + 2));
    blk_len  = ((uint32_t)1 << read_bl);
    if (blk_len >= 512) {
      hsd->SdCard.BlockNbr = blk_nbr * (blk_len / 512);
    } else {
      hsd->SdCard.BlockNbr = blk_nbr / (512 / blk_len);
    }
    hsd->SdCard.LogBlockNbr = hsd->SdCard.BlockNbr;
  }

  hsd->State = HAL_SD_STATE_READY;

  return HAL_OK;
}

static HAL_StatusTypeDef SD_EnableWideBusOperation(SD_HandleTypeDef *hsd, uint32_t WideMode)
{
  uint32_t count;

  if (SDMMC_CmdAppCommand(hsd, (uint32_t)(hsd->RCA << 16)) != HAL_OK) {
    return HAL_ERROR;
  }

  SDIO->ARG = (uint32_t)0x00000002;
  SDIO->CMD = (SDIO_CMD_CPSMEN | SD_CMDRESP_SHORT) | SD_CMD_APP_SD_SET_BUSWIDTH;

  count = SD_CMD_TIMEOUT;
  while (count && !(SDIO->STA & (SDIO_STA_CMDREND | SDIO_STA_CMDSENT | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL))) {
    count--;
  }

  if ((count == 0) || (SDIO->STA & SDIO_STA_CTIMEOUT) || (SDIO->STA & SDIO_STA_CCRCFAIL)) {
    hsd->ErrorCode |= HAL_SD_ERROR_CMD_RSP_TIMEOUT;
    return HAL_ERROR;
  }

  SDIO->ICR = SDIO_STATIC_FLAGS;

  SDIO->CLKCR = (SDIO->CLKCR & ~SDIO_CLKCR_WIDBUS) | WideMode;
  SDIO->CLKCR |= SDIO_CLKCR_CLKEN;

  return HAL_OK;
}

HAL_StatusTypeDef HAL_SD_ReadBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks, uint32_t Timeout)
{
  uint32_t count = Timeout;
  uint32_t *pData32;
  uint32_t i;

  if (hsd == NULL || pData == NULL) {
    return HAL_ERROR;
  }

  if (hsd->CardType != SD_CARD_TYPE_SDHC_SDXC) {
    BlockAdd *= 512;
  }

  SDIO->DCTRL = 0;
  SDIO->DLEN = NumberOfBlocks * 512;
  SDIO->DTIMER = SD_DATATIMEOUT;
  SDIO->DCTRL = SDIO_DCTRL_DTEN | SDIO_DCTRL_DTDIR | SDIO_BLOCKSIZE_512;

  if (NumberOfBlocks > 1) {
    SDIO->DCTRL |= SDIO_DCTRL_DTMODE;
    SDIO->ARG = BlockAdd;
    SDIO->CMD = (SDIO_CMD_CPSMEN | SD_CMDRESP_SHORT) | SDIO_CMD_ENCMDCOMPL | SD_CMD_READ_MULT_BLOCK;
  } else {
    SDIO->ARG = BlockAdd;
    SDIO->CMD = (SDIO_CMD_CPSMEN | SD_CMDRESP_SHORT) | SDIO_CMD_ENCMDCOMPL | SD_CMD_READ_SINGLE_BLOCK;
  }

  while (count) {
    if (SDIO->STA & SDIO_STA_RXOVERR) {
      SDIO->ICR = SDIO_STA_RXOVERR;
      hsd->ErrorCode |= HAL_SD_ERROR_RX_OVERRUN;
      return HAL_ERROR;
    }

    if (SDIO->STA & SDIO_STA_DTIMEOUT) {
      SDIO->ICR = SDIO_STA_DTIMEOUT;
      hsd->ErrorCode |= HAL_SD_ERROR_DATA_TIMEOUT;
      return HAL_ERROR;
    }

    if (SDIO->STA & SDIO_STA_DCRCFAIL) {
      SDIO->ICR = SDIO_STA_DCRCFAIL;
      hsd->ErrorCode |= HAL_SD_ERROR_DATA_CRC_FAIL;
      return HAL_ERROR;
    }

    if (SDIO->STA & SDIO_STA_STBITERR) {
      SDIO->ICR = SDIO_STA_STBITERR;
      hsd->ErrorCode |= HAL_SD_ERROR_GENERAL_UNKNOWN_ERR;
      return HAL_ERROR;
    }

    if (SDIO->STA & SDIO_STA_DATAEND) {
      SDIO->ICR = SDIO_STA_DATAEND;
      break;
    }

    if ((SDIO->STA & SDIO_STA_RXFIFOHF) && ((SDIO->FIFOCNT & 0x00FFFFFF) >= 8)) {
      pData32 = (uint32_t *)pData;
      for (i = 0; i < 8; i++) {
        *pData32 = SDIO->FIFO;
        pData32++;
      }
      pData = (uint8_t *)pData32;
    }

    count--;
  }

  while ((SDIO->STA & SDIO_STA_RXDAVL)) {
    *((uint32_t *)pData) = SDIO->FIFO;
    pData += 4;
  }

  SDIO->ICR = SDIO_STATIC_FLAGS;

  if (NumberOfBlocks > 1) {
    SDIO->CMD = (SDIO_CMD_CPSMEN | SD_CMDRESP_SHORT) | SD_CMD_STOP_TRANSMISSION;
    count = 0xFFFF;
    while (count && !(SDIO->STA & (SDIO_STA_CMDSENT | SDIO_STA_CMDREND))) {
      count--;
    }
    SDIO->ICR = SDIO_STATIC_FLAGS;
  }

  if (count == 0 && NumberOfBlocks > 1) {
    hsd->ErrorCode |= HAL_SD_ERROR_TIMEOUT;
    return HAL_ERROR;
  }

  return HAL_OK;
}

HAL_StatusTypeDef HAL_SD_WriteBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks, uint32_t Timeout)
{
  uint32_t count;
  uint32_t *pData32;
  uint32_t totalwords = NumberOfBlocks * 128;
  uint32_t words_sent = 0;
  uint32_t i;

  if (hsd == NULL || pData == NULL) {
    return HAL_ERROR;
  }

  if (hsd->CardType != SD_CARD_TYPE_SDHC_SDXC) {
    BlockAdd *= 512;
  }

  if (NumberOfBlocks > 1) {
    SDMMC_CmdAppCommand(hsd, (uint32_t)(hsd->RCA << 16));
    SDIO->ARG = NumberOfBlocks;
    SDIO->CMD = (SDIO_CMD_CPSMEN | SD_CMDRESP_SHORT) | SD_CMD_SET_BLOCK_COUNT;

    count = SD_CMD_TIMEOUT;
    while (count && !(SDIO->STA & (SDIO_STA_CMDREND | SDIO_STA_CMDSENT | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL))) {
      count--;
    }
    SDIO->ICR = SDIO_STATIC_FLAGS;
  }

  SDIO->DCTRL = 0;
  SDIO->DLEN = NumberOfBlocks * 512;
  SDIO->DTIMER = SD_DATATIMEOUT;
  SDIO->DCTRL = SDIO_DCTRL_DTEN | SDIO_BLOCKSIZE_512;

  if (NumberOfBlocks > 1) {
    SDIO->DCTRL |= SDIO_DCTRL_DTMODE;
    SDIO->ARG = BlockAdd;
    SDIO->CMD = (SDIO_CMD_CPSMEN | SD_CMDRESP_SHORT) | SDIO_CMD_ENCMDCOMPL | SD_CMD_WRITE_MULT_BLOCK;
  } else {
    SDIO->ARG = BlockAdd;
    SDIO->CMD = (SDIO_CMD_CPSMEN | SD_CMDRESP_SHORT) | SDIO_CMD_ENCMDCOMPL | SD_CMD_WRITE_SINGLE_BLOCK;
  }

  count = Timeout;
  while (count) {
    if (SDIO->STA & (SDIO_STA_DCRCFAIL | SDIO_STA_DTIMEOUT | SDIO_STA_STBITERR)) {
      SDIO->ICR = SDIO_STATIC_FLAGS;
      hsd->ErrorCode |= HAL_SD_ERROR_DATA_TIMEOUT;
      return HAL_ERROR;
    }

    if (SDIO->STA & SDIO_STA_TXFIFOHE) {
      pData32 = (uint32_t *)pData;
      for (i = 0; i < 8 && words_sent < totalwords; i++) {
        SDIO->FIFO = *pData32;
        pData32++;
        words_sent++;
      }
      pData = (uint8_t *)pData32;

      if (words_sent >= totalwords) {
        break;
      }
    }

    count--;
  }

  count = Timeout;
  while (count) {
    if (SDIO->STA & SDIO_STA_DATAEND) {
      SDIO->ICR = SDIO_STA_DATAEND;
      break;
    }
    count--;
  }

  if (count == 0) {
    hsd->ErrorCode |= HAL_SD_ERROR_TIMEOUT;
    return HAL_ERROR;
  }

  if (NumberOfBlocks > 1) {
    SDIO->CMD = (SDIO_CMD_CPSMEN | SD_CMDRESP_SHORT) | SD_CMD_STOP_TRANSMISSION;
    count = 0xFFFF;
    while (count && !(SDIO->STA & (SDIO_STA_CMDSENT | SDIO_STA_CMDREND))) {
      count--;
    }
    SDIO->ICR = SDIO_STATIC_FLAGS;
  }

  SDIO->ICR = SDIO_STATIC_FLAGS;

  return HAL_OK;
}

HAL_StatusTypeDef HAL_SD_GetCardInfo(SD_HandleTypeDef *hsd, HAL_SD_CardInfoTypeDef *pCardInfo)
{
  if (hsd == NULL || pCardInfo == NULL) {
    return HAL_ERROR;
  }

  pCardInfo->CardType = hsd->SdCard.CardType;
  pCardInfo->CardVersion = hsd->SdCard.CardVersion;
  pCardInfo->Class = hsd->SdCard.Class;
  pCardInfo->RelCardAdd = hsd->SdCard.RelCardAdd;
  pCardInfo->BlockNbr = hsd->SdCard.BlockNbr;
  pCardInfo->BlockSize = hsd->SdCard.BlockSize;
  pCardInfo->LogBlockNbr = hsd->SdCard.LogBlockNbr;
  pCardInfo->LogBlockSize = hsd->SdCard.LogBlockSize;
  pCardInfo->CardSpeed = hsd->SdCard.CardSpeed;

  return HAL_OK;
}

HAL_SD_CardStateTypeDef HAL_SD_GetCardState(SD_HandleTypeDef *hsd)
{
  if (hsd == NULL) {
    return (HAL_SD_CardStateTypeDef)HAL_SD_CARD_ERROR;
  }
  return (HAL_SD_CardStateTypeDef)hsd->CardState;
}

static HAL_StatusTypeDef SDMMC_CmdAppCommand(SD_HandleTypeDef *hsd, uint32_t Argument)
{
  uint32_t count;

  SDIO->ARG = Argument;
  SDIO->CMD = (SDIO_CMD_CPSMEN | SD_CMDRESP_SHORT) | SD_CMD_APP_CMD;

  count = SD_CMD_TIMEOUT;
  while (count && !(SDIO->STA & (SDIO_STA_CMDREND | SDIO_STA_CMDSENT | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL))) {
    count--;
  }

  if ((count == 0) || (SDIO->STA & SDIO_STA_CTIMEOUT)) {
    SDIO->ICR = SDIO_STATIC_FLAGS;
    hsd->ErrorCode |= HAL_SD_ERROR_CMD_RSP_TIMEOUT;
    return HAL_ERROR;
  }

  SDIO->ICR = SDIO_STATIC_FLAGS;
  return HAL_OK;
}

#endif /* HAL_SD_MODULE_ENABLED */
