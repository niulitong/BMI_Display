/**
  ******************************************************************************
  * @file    stm32f4xx_hal_sd.h
  * @brief   Header file of SD HAL module.
  ******************************************************************************
  */

#ifndef __STM32F4xx_HAL_SD_H
#define __STM32F4xx_HAL_SD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal_def.h"

/** @addtogroup STM32F4xx_HAL_Driver SD
  * @{
  */

/** @defgroup SD_Exported_Types SD Exported Types
  * @{
  */

#define SDIO_TRANSFER_CLK_DIV              ((uint32_t)0x00000000U)

#define SDIO_STATIC_FLAGS                  ((uint32_t)(SDIO_STA_CCRCFAIL | SDIO_STA_DCRCFAIL | \
                                                        SDIO_STA_CTIMEOUT | SDIO_STA_DTIMEOUT | \
                                                        SDIO_STA_TXUNDERR | SDIO_STA_RXOVERR  | \
                                                        SDIO_STA_CMDREND  | SDIO_STA_CMDSENT  | \
                                                        SDIO_STA_DATAEND  | SDIO_STA_STBITERR))

#define SDIO_BUS_WIDE_1B                   ((uint32_t)0x00000000U)
#define SDIO_BUS_WIDE_4B                   SDIO_CLKCR_WIDBUS_0
#define SDIO_BUS_WIDE_8B                   SDIO_CLKCR_WIDBUS_1

#define SDIO_CLOCK_EDGE_RISING             ((uint32_t)0x00000000U)
#define SDIO_CLOCK_EDGE_FALLING            SDIO_CLKCR_NEGEDGE

#define SDIO_CLOCK_BYPASS_DISABLE          ((uint32_t)0x00000000U)
#define SDIO_CLOCK_BYPASS_ENABLE           SDIO_CLKCR_BYPASS

#define SDIO_CLOCK_POWER_SAVE_DISABLE      ((uint32_t)0x00000000U)
#define SDIO_CLOCK_POWER_SAVE_ENABLE       SDIO_CLKCR_PWRSAV

#define SDIO_HARDWARE_FLOW_CONTROL_DISABLE ((uint32_t)0x00000000U)
#define SDIO_HARDWARE_FLOW_CONTROL_ENABLE  SDIO_CLKCR_HWFC_EN

#define SDMMC_MAX_VOLT_TRIAL               ((uint32_t)0x0000FFFFU)
#define SDMMC_ALLZERO                      ((uint32_t)0x00000000U)

#define SD_OCR_ERRORBIT_MASK               ((uint32_t)0xFDFFE008U)
#define SD_OCR_ADDR_OUT_OF_RANGE           ((uint32_t)0x80000000U)
#define SD_OCR_ADDR_MISALIGNED             ((uint32_t)0x40000000U)
#define SD_OCR_BLOCK_LEN_ERR               ((uint32_t)0x20000000U)
#define SD_OCR_ERASE_SEQ_ERR               ((uint32_t)0x10000000U)
#define SD_OCR_BAD_ERASE_PARAM             ((uint32_t)0x08000000U)
#define SD_OCR_WRITE_PROT_VIOLATION        ((uint32_t)0x04000000U)
#define SD_OCR_LOCK_UNLOCK_FAILED          ((uint32_t)0x01000000U)
#define SD_OCR_COM_CRC_FAILED              ((uint32_t)0x00800000U)
#define SD_OCR_ILLEGAL_CMD                 ((uint32_t)0x00400000U)
#define SD_OCR_CARD_ECC_FAILED             ((uint32_t)0x00200000U)
#define SD_OCR_CC_ERROR                    ((uint32_t)0x00100000U)
#define SD_OCR_GENERAL_UNKNOWN_ERROR       ((uint32_t)0x00080000U)
#define SD_OCR_STREAM_READ_UNDERRUN        ((uint32_t)0x00040000U)
#define SD_OCR_STREAM_WRITE_OVERRUN        ((uint32_t)0x00020000U)
#define SD_OCR_CID_CSD_OVERWRITE           ((uint32_t)0x00010000U)
#define SD_OCR_WP_ERASE_SKIP               ((uint32_t)0x00008000U)
#define SD_OCR_CARD_ECC_DISABLED           ((uint32_t)0x00004000U)
#define SD_OCR_ERASE_RESET                 ((uint32_t)0x00002000U)
#define SD_OCR_AKE_SEQ_ERROR               ((uint32_t)0x00000008U)

#define SD_CMD_GO_IDLE_STATE               ((uint8_t)0)
#define SD_CMD_SEND_OP_COND                ((uint8_t)1)
#define SD_CMD_ALL_SEND_CID                ((uint8_t)2)
#define SD_CMD_SET_REL_ADDR                ((uint8_t)3)
#define SD_CMD_SET_DSR                     ((uint8_t)4)
#define SD_CMD_SDIO_SEN_OP_COND            ((uint8_t)5)
#define SD_CMD_HS_SWITCH                   ((uint8_t)6)
#define SD_CMD_SEL_DESEL_CARD              ((uint8_t)7)
#define SD_CMD_HS_SEND_EXT_CSD             ((uint8_t)8)
#define SD_CMD_SEND_CSD                    ((uint8_t)9)
#define SD_CMD_SEND_CID                    ((uint8_t)10)
#define SD_CMD_READ_DAT_UNTIL_STOP         ((uint8_t)11)
#define SD_CMD_STOP_TRANSMISSION           ((uint8_t)12)
#define SD_CMD_SEND_STATUS                 ((uint8_t)13)
#define SD_CMD_HS_BUSTEST_READ             ((uint8_t)14)
#define SD_CMD_GO_INACTIVE_STATE           ((uint8_t)15)
#define SD_CMD_SET_BLOCKLEN                ((uint8_t)16)
#define SD_CMD_READ_SINGLE_BLOCK           ((uint8_t)17)
#define SD_CMD_READ_MULT_BLOCK             ((uint8_t)18)
#define SD_CMD_HS_BUSTEST_WRITE            ((uint8_t)19)
#define SD_CMD_WRITE_DAT_UNTIL_STOP        ((uint8_t)20)
#define SD_CMD_SET_BLOCK_COUNT             ((uint8_t)23)
#define SD_CMD_WRITE_SINGLE_BLOCK          ((uint8_t)24)
#define SD_CMD_WRITE_MULT_BLOCK            ((uint8_t)25)
#define SD_CMD_PROG_CID                    ((uint8_t)26)
#define SD_CMD_PROG_CSD                    ((uint8_t)27)
#define SD_CMD_SET_WRITE_PROT              ((uint8_t)28)
#define SD_CMD_CLR_WRITE_PROT              ((uint8_t)29)
#define SD_CMD_SEND_WRITE_PROT             ((uint8_t)30)
#define SD_CMD_SD_ERASE_GRP_START          ((uint8_t)32)
#define SD_CMD_SD_ERASE_GRP_END            ((uint8_t)33)
#define SD_CMD_ERASE_GRP_START             ((uint8_t)35)
#define SD_CMD_ERASE_GRP_END               ((uint8_t)36)
#define SD_CMD_ERASE                       ((uint8_t)38)
#define SD_CMD_FAST_IO                     ((uint8_t)39)
#define SD_CMD_GO_IRQ_STATE                ((uint8_t)40)
#define SD_CMD_LOCK_UNLOCK                 ((uint8_t)42)
#define SD_CMD_APP_CMD                     ((uint8_t)55)
#define SD_CMD_GEN_CMD                     ((uint8_t)56)
#define SD_CMD_NO_CMD                      ((uint8_t)64)

#define SD_CMD_APP_SD_SET_BUSWIDTH         ((uint8_t)6)
#define SD_CMD_SD_APP_STATUS               ((uint8_t)13)
#define SD_CMD_SD_APP_SEND_NUM_WRITE_BLOCKS ((uint8_t)22)
#define SD_CMD_SD_APP_OP_COND              ((uint8_t)41)
#define SD_CMD_SD_APP_SET_CLR_CARD_DETECT  ((uint8_t)42)
#define SD_CMD_SD_APP_SEND_SCR             ((uint8_t)51)

#define SD_OCR_VDD_165_195                 ((uint32_t)0x00000080U)
#define SD_OCR_VDD_20_21                   ((uint32_t)0x00000100U)
#define SD_OCR_VDD_21_22                   ((uint32_t)0x00000200U)
#define SD_OCR_VDD_22_23                   ((uint32_t)0x00000400U)
#define SD_OCR_VDD_23_24                   ((uint32_t)0x00000800U)
#define SD_OCR_VDD_24_25                   ((uint32_t)0x00001000U)
#define SD_OCR_VDD_25_26                   ((uint32_t)0x00002000U)
#define SD_OCR_VDD_26_27                   ((uint32_t)0x00004000U)
#define SD_OCR_VDD_27_28                   ((uint32_t)0x00008000U)
#define SD_OCR_VDD_28_29                   ((uint32_t)0x00010000U)
#define SD_OCR_VDD_29_30                   ((uint32_t)0x00020000U)
#define SD_OCR_VDD_30_31                   ((uint32_t)0x00040000U)
#define SD_OCR_VDD_31_32                   ((uint32_t)0x00080000U)
#define SD_OCR_VDD_32_33                   ((uint32_t)0x00100000U)
#define SD_OCR_VDD_33_34                   ((uint32_t)0x00200000U)
#define SD_OCR_VDD_34_35                   ((uint32_t)0x00400000U)
#define SD_OCR_VDD_35_36                   ((uint32_t)0x00800000U)
#define SD_OCR_SDHC_CAPACITY               ((uint32_t)0x40000000U)

#define SD_OCR_POWER_UP_BUSY               ((uint32_t)0x80000000U)

#define HAL_SD_STATE_RESET                 ((uint32_t)0x00000000U)
#define HAL_SD_STATE_READY                 ((uint32_t)0x00000001U)
#define HAL_SD_STATE_BUSY                  ((uint32_t)0x00000002U)
#define HAL_SD_STATE_ERROR                 ((uint32_t)0x00000003U)

typedef enum {
    HAL_SD_CARD_READY                  = 0x00000001U,
    HAL_SD_CARD_IDENTIFICATION         = 0x00000002U,
    HAL_SD_CARD_STANDBY                = 0x00000003U,
    HAL_SD_CARD_TRANSFER               = 0x00000004U,
    HAL_SD_CARD_SENDING                = 0x00000005U,
    HAL_SD_CARD_RECEIVING              = 0x00000006U,
    HAL_SD_CARD_PROGRAMMING            = 0x00000007U,
    HAL_SD_CARD_DISCONNECTED           = 0x00000008U,
    HAL_SD_CARD_ERROR                  = 0x000000FFU
} HAL_SD_CardStateTypeDef;

#define SD_RESPONSE_NO_ERROR               ((uint32_t)0x00000000U)
#define SD_RESPONSE_OCR_ERROR              ((uint32_t)0x00000001U)
#define SD_RESPONSE_FAILURE                ((uint32_t)0x000000FFU)

#define SD_CARD_TYPE_SDSC                  0x00000000U
#define SD_CARD_TYPE_SDHC_SDXC             0x00000001U
#define SD_CARD_TYPE_SECURED               0x00000003U

#define SDIO_TRANSFER_OK                   0
#define SDIO_TRANSFER_BUSY                 1

#define SD_CMD_RSP_TIMEOUT                 0xFFFFFFFFU
#define SD_DATATIMEOUT                     0xFFFFFFFFU
#define SD_OCR_VOLTAGE_WINDOW              0x00FF8000U
#define SD_OCR_HIGH_CAPACITY               0x40000000U
#define SD_OCR_CAPACITY_VDD_WINDOW         0x40FF8000U

#define SDIO_SECURE_DIGITAL_IO_CARD        0x00010000U
#define SDIO_SECURE_DIGITAL_IO_COMBO_CARD  0x00020000U
#define SDIO_HIGH_CAPACITY_SD_CARD         0x00040000U

typedef struct {
    uint32_t CardType;
    uint32_t CardVersion;
    uint32_t Class;
    uint32_t RelCardAdd;
    uint32_t BlockNbr;
    uint32_t BlockSize;
    uint32_t LogBlockNbr;
    uint32_t LogBlockSize;
    uint32_t CardSpeed;
} HAL_SD_CardInfoTypeDef;

typedef struct {
    uint32_t ClockEdge;
    uint32_t ClockBypass;
    uint32_t ClockPowerSave;
    uint32_t BusWide;
    uint32_t HardwareFlowControl;
    uint8_t ClockDiv;
} SD_InitTypeDef;

typedef struct {
    uint32_t CSD[4];
    uint32_t CID[4];
    uint64_t CardCapacity;
    uint32_t CardBlockSize;
    uint16_t RCA;
    uint8_t CardType;
    uint32_t RelativeCardAddress;
    uint32_t CardState;
} HAL_SD_CardCSDTypeDef;

typedef struct {
    uint32_t CardState;
    HAL_SD_CardCSDTypeDef CSD;
    HAL_SD_CardInfoTypeDef CardInfo;
} HAL_SD_CardStatusTypeDef;

typedef struct {
    SDIO_TypeDef *Instance;
    SD_InitTypeDef Init;
    HAL_LockTypeDef Lock;
    uint32_t State;
    HAL_SD_CardInfoTypeDef SdCard;
    uint32_t CardType;
    uint32_t CardState;
    uint32_t CSD[4];
    uint32_t CID[4];
    uint16_t RCA;
    uint32_t ErrorCode;
    uint32_t Context;
    DMA_HandleTypeDef *hdmarx;
    DMA_HandleTypeDef *hdmatx;
} SD_HandleTypeDef;

typedef struct {
    uint8_t ManufacturerID;
    uint16_t OEM_AppliID;
    uint32_t ProdName1;
    uint8_t ProdName2;
    uint8_t ProdRev;
    uint32_t ProdSN;
    uint8_t Reserved1;
    uint16_t ManufactDate;
    uint8_t CID_CRC;
    uint8_t Reserved2;
} HAL_SD_CardCIDTypeDef;

#define HAL_SD_ERROR_NONE                  ((uint32_t)0x00000000U)
#define HAL_SD_ERROR_CMD_CRC_FAIL          ((uint32_t)0x00000001U)
#define HAL_SD_ERROR_DATA_CRC_FAIL         ((uint32_t)0x00000002U)
#define HAL_SD_ERROR_CMD_RSP_TIMEOUT       ((uint32_t)0x00000004U)
#define HAL_SD_ERROR_DATA_TIMEOUT          ((uint32_t)0x00000008U)
#define HAL_SD_ERROR_TX_UNDERRUN           ((uint32_t)0x00000010U)
#define HAL_SD_ERROR_RX_OVERRUN            ((uint32_t)0x00000020U)
#define HAL_SD_ERROR_ADDR_MISALIGNED       ((uint32_t)0x00000040U)
#define HAL_SD_ERROR_BLOCK_LEN_ERR         ((uint32_t)0x00000080U)
#define HAL_SD_ERROR_ERASE_SEQ_ERR         ((uint32_t)0x00000100U)
#define HAL_SD_ERROR_BAD_ERASE_PARAM       ((uint32_t)0x00000200U)
#define HAL_SD_ERROR_WRITE_PROT_VIOLATION  ((uint32_t)0x00000400U)
#define HAL_SD_ERROR_LOCK_UNLOCK_FAILED    ((uint32_t)0x00000800U)
#define HAL_SD_ERROR_COM_CRC_FAILED        ((uint32_t)0x00001000U)
#define HAL_SD_ERROR_ILLEGAL_CMD           ((uint32_t)0x00002000U)
#define HAL_SD_ERROR_CARD_ECC_FAILED       ((uint32_t)0x00004000U)
#define HAL_SD_ERROR_CC_ERR                ((uint32_t)0x00008000U)
#define HAL_SD_ERROR_GENERAL_UNKNOWN_ERR   ((uint32_t)0x00010000U)
#define HAL_SD_ERROR_STREAM_READ_UNDERRUN  ((uint32_t)0x00020000U)
#define HAL_SD_ERROR_STREAM_WRITE_OVERRUN  ((uint32_t)0x00040000U)
#define HAL_SD_ERROR_CID_CSD_OVERWRITE     ((uint32_t)0x00080000U)
#define HAL_SD_ERROR_WP_ERASE_SKIP         ((uint32_t)0x00100000U)
#define HAL_SD_ERROR_CARD_ECC_DISABLED     ((uint32_t)0x00200000U)
#define HAL_SD_ERROR_ERASE_RESET           ((uint32_t)0x00400000U)
#define HAL_SD_ERROR_AKE_SEQ_ERR           ((uint32_t)0x00800000U)
#define HAL_SD_ERROR_INVALID_VOLTRANGE     ((uint32_t)0x01000000U)
#define HAL_SD_ERROR_ADDR_OUT_OF_RANGE     ((uint32_t)0x02000000U)
#define HAL_SD_ERROR_REQUEST_NOT_APPLICABLE ((uint32_t)0x04000000U)
#define HAL_SD_ERROR_INVALID_PARAMETER     ((uint32_t)0x08000000U)
#define HAL_SD_ERROR_UNSUPPORTED_FEATURE   ((uint32_t)0x10000000U)
#define HAL_SD_ERROR_BUSY                  ((uint32_t)0x20000000U)
#define HAL_SD_ERROR_DMA                   ((uint32_t)0x40000000U)
#define HAL_SD_ERROR_TIMEOUT               ((uint32_t)0x80000000U)

/**
  * @}
  */

/** @defgroup SD_Exported_Functions SD Exported Functions
  * @{
  */
HAL_StatusTypeDef HAL_SD_Init(SD_HandleTypeDef *hsd);
HAL_StatusTypeDef HAL_SD_DeInit(SD_HandleTypeDef *hsd);
HAL_StatusTypeDef HAL_SD_ReadBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks, uint32_t Timeout);
HAL_StatusTypeDef HAL_SD_WriteBlocks(SD_HandleTypeDef *hsd, uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks, uint32_t Timeout);
HAL_StatusTypeDef HAL_SD_ReadBlocks_DMA(SD_HandleTypeDef *hsd, uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks);
HAL_StatusTypeDef HAL_SD_WriteBlocks_DMA(SD_HandleTypeDef *hsd, uint8_t *pData, uint32_t BlockAdd, uint32_t NumberOfBlocks);
HAL_StatusTypeDef HAL_SD_Erase(SD_HandleTypeDef *hsd, uint32_t BlockStartAdd, uint32_t BlockEndAdd);
HAL_StatusTypeDef HAL_SD_GetCardCID(SD_HandleTypeDef *hsd, HAL_SD_CardCIDTypeDef *pCID);
HAL_StatusTypeDef HAL_SD_GetCardCSD(SD_HandleTypeDef *hsd, HAL_SD_CardCSDTypeDef *pCSD);
HAL_StatusTypeDef HAL_SD_GetCardStatus(SD_HandleTypeDef *hsd, HAL_SD_CardStatusTypeDef *pCardStatus);
HAL_StatusTypeDef HAL_SD_GetCardInfo(SD_HandleTypeDef *hsd, HAL_SD_CardInfoTypeDef *pCardInfo);
HAL_SD_CardStateTypeDef HAL_SD_GetCardState(SD_HandleTypeDef *hsd);
/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_HAL_SD_H */
