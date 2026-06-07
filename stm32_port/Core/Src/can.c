/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.c
  * @brief   This file provides code for the configuration
  *          of the CAN instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "can.h"

/* USER CODE BEGIN Includes */
#include "cmsis_os2.h"
#include "dashboard_ui.h"
/* USER CODE END Includes */

/* USER CODE BEGIN 0 */
CAN_TxHeaderTypeDef TxHeader;
uint8_t CAN_TxData[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 };
uint32_t CAN1_TxMail;
uint32_t CAN1_ID = 0x102;
static uint8_t g_can_heartbeat_counter;
CAN_RxHeaderTypeDef RxHeader;
uint8_t CAN_RxData[8] = { 0 };
/* DBC Vehicle_CanB.dbc filter ID list */
/* Bank0: BMS(0x401,非DBC), DataLogger(0x305 BO_773), Debug2_Torque(0x502 BO_1282), Debug5_Velocity(0x505 BO_1285) */
uint16_t CAN1_RX_MSG_ID_BANK0[4] = {0x401, 0x305, 0x502, 0x505};
/* Bank1: Debug6_MotorTemp(0x506 BO_1286), Debug9_Status(0x509 BO_1289), Debug8_IGBT(0x508 BO_1288), Debug7_Inverter(0x507 BO_1287) */
uint16_t CAN1_RX_MSG_ID_BANK1[4] = {0x506, 0x509, 0x508, 0x507};
/* Bank2: Debug3_Diag12(0x503 BO_1283), Debug4_Diag34(0x504 BO_1284) */
uint16_t CAN1_RX_MSG_ID_BANK2[4] = {0x503, 0x504, 0x503, 0x504};
/* Bank3: IMU_Raw(0x50 BO_80) -> FIFO1 */
uint16_t CAN1_RX_MSG_ID_BANK3[4] = {0x050, 0x050, 0x050, 0x050};
static dashboard_data_t g_can_dashboard_data = {
  .speed = 24,             /* DBC BO_769 GPS_Speed: GroundSpeed */
  .soc = 24,               /* BMS(非DBC总线) */
  .mode_index = 0,         /* DBC BO_1289 Debug9: ModeFlag [-8,7] */
  .torque = {24, 24, 24, 24},     /* DBC BO_1282 Debug2: ActualTorque (1,0) */
  .motor_enable = {1, 1, 1, 1},   /* DBC BO_1289 Debug9: AMK_bEnable */
  .rpm = {24, 24, 24, 24},        /* DBC BO_1285 Debug5: ActualVelocity (1,0) */
  .sum_voltage = 24,        /* BMS(非DBC总线) */
  .sum_current = 24,        /* BMS(非DBC总线) */
  .max_temperature = 24,    /* BMS(非DBC总线) */
  .motor_temp = {48, 47, 49, 50},  /* DBC BO_1286 Debug6: Motor_temperature (0.1,0) degC */
  .aps_open_pct = 0,        /* DBC BO_773 DataLogger: APS_OpenPct (0.1,0) % */
  .steering_angle = 0,      /* DBC BO_773 DataLogger: SteeringWheelAngle (0.1,0) deg */
  .oil_pressure = 0,        /* DBC BO_773 DataLogger: OilPressure_Kpa (0.001,0) Kpa, raw count */
  .igbt_temp = {0, 0, 0, 0},       /* DBC BO_1288 Debug8: IGBT_temperature (0.1,0) degC */
  .inverter_temp = {0, 0, 0, 0},   /* DBC BO_1287 Debug7: Inverter_temperature (0.1,0) degC */
  .diag_num = {0, 0, 0, 0},        /* DBC BO_1283/1284 Debug3/4: Diagnostic_number */
  .imu_accel = {0, 0, 0},   /* DBC BO_97 IMU_Accel: raw int16, scale=0.00048828125 g */
  .imu_gyro = {0, 0, 0},    /* DBC BO_98 IMU_Gyro: raw int16, scale=0.0610352 deg/s */
  .imu_roll = 0,            /* DBC BO_99 IMU_Roll: raw int16, scale=0.005493 deg */
  .imu_pitch = 0,           /* DBC BO_100 IMU_Pitch: raw int16, scale=0.005493 deg */
  .imu_yaw = 0,             /* DBC BO_101 IMU_Yaw: raw int16, scale=0.005493 deg */
  .imu_mag = {0, 0, 0},     /* DBC BO_102 IMU_Magnetic: raw int16, scale=1 */
  .signal_level = 0,        /* 非DBC */
  .alert_active = 1,        /* 非DBC */
  .odometer_tenths = 412,   /* 非DBC: 里程 0.1km */
  .brake_pct = 10,          /* 非DBC: 制动 0~100% */
};
/* USER CODE END 0 */

CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;

/* CAN1 init function */
void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 6;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_12TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_1TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}
/* CAN2 init function */
void MX_CAN2_Init(void)
{

  /* USER CODE BEGIN CAN2_Init 0 */

  /* USER CODE END CAN2_Init 0 */

  /* USER CODE BEGIN CAN2_Init 1 */

  /* USER CODE END CAN2_Init 1 */
  hcan2.Instance = CAN2;
  hcan2.Init.Prescaler = 6;
  hcan2.Init.Mode = CAN_MODE_NORMAL;
  hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan2.Init.TimeSeg1 = CAN_BS1_12TQ;
  hcan2.Init.TimeSeg2 = CAN_BS2_1TQ;
  hcan2.Init.TimeTriggeredMode = DISABLE;
  hcan2.Init.AutoBusOff = DISABLE;
  hcan2.Init.AutoWakeUp = DISABLE;
  hcan2.Init.AutoRetransmission = DISABLE;
  hcan2.Init.ReceiveFifoLocked = DISABLE;
  hcan2.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN2_Init 2 */

  /* USER CODE END CAN2_Init 2 */

}

static uint32_t HAL_RCC_CAN1_CLK_ENABLED=0;

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspInit 0 */

  /* USER CODE END CAN1_MspInit 0 */
    /* CAN1 clock enable */
    HAL_RCC_CAN1_CLK_ENABLED++;
    if(HAL_RCC_CAN1_CLK_ENABLED==1){
      __HAL_RCC_CAN1_CLK_ENABLE();
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**CAN1 GPIO Configuration
    PB8     ------> CAN1_RX
    PB9     ------> CAN1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* CAN1 interrupt Init */
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
  /* USER CODE BEGIN CAN1_MspInit 1 */

  /* USER CODE END CAN1_MspInit 1 */
  }
  else if(canHandle->Instance==CAN2)
  {
  /* USER CODE BEGIN CAN2_MspInit 0 */

  /* USER CODE END CAN2_MspInit 0 */
    /* CAN2 clock enable */
    __HAL_RCC_CAN2_CLK_ENABLE();
    HAL_RCC_CAN1_CLK_ENABLED++;
    if(HAL_RCC_CAN1_CLK_ENABLED==1){
      __HAL_RCC_CAN1_CLK_ENABLE();
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**CAN2 GPIO Configuration
    PB12     ------> CAN2_RX
    PB13     ------> CAN2_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* CAN2 interrupt Init */
    HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN2_RX1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN2_RX1_IRQn);
    HAL_NVIC_SetPriority(CAN2_SCE_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(CAN2_SCE_IRQn);
  /* USER CODE BEGIN CAN2_MspInit 1 */

  /* USER CODE END CAN2_MspInit 1 */
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspDeInit 0 */

  /* USER CODE END CAN1_MspDeInit 0 */
    /* Peripheral clock disable */
    HAL_RCC_CAN1_CLK_ENABLED--;
    if(HAL_RCC_CAN1_CLK_ENABLED==0){
      __HAL_RCC_CAN1_CLK_DISABLE();
    }

    /**CAN1 GPIO Configuration
    PB8     ------> CAN1_RX
    PB9     ------> CAN1_TX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8|GPIO_PIN_9);

    /* CAN1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_SCE_IRQn);
  /* USER CODE BEGIN CAN1_MspDeInit 1 */

  /* USER CODE END CAN1_MspDeInit 1 */
  }
  else if(canHandle->Instance==CAN2)
  {
  /* USER CODE BEGIN CAN2_MspDeInit 0 */

  /* USER CODE END CAN2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CAN2_CLK_DISABLE();
    HAL_RCC_CAN1_CLK_ENABLED--;
    if(HAL_RCC_CAN1_CLK_ENABLED==0){
      __HAL_RCC_CAN1_CLK_DISABLE();
    }

    /**CAN2 GPIO Configuration
    PB12     ------> CAN2_RX
    PB13     ------> CAN2_TX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12|GPIO_PIN_13);

    /* CAN2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(CAN2_RX0_IRQn);
    HAL_NVIC_DisableIRQ(CAN2_RX1_IRQn);
    HAL_NVIC_DisableIRQ(CAN2_SCE_IRQn);
  /* USER CODE BEGIN CAN2_MspDeInit 1 */

  /* USER CODE END CAN2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/*
 * @func: 锟斤拷锟斤拷CAN锟斤拷锟剿诧拷
 *
 */
void CAN1_Filter_Config(void)
{
	CAN_FilterTypeDef CAN_FilterInitStructure;

	/* Bank 0: BMS(0x401,非DBC), DataLogger(0x305 BO_773), Debug2_Torque(0x502 BO_1282), Debug5_Velocity(0x505 BO_1285) */
	CAN_FilterInitStructure.FilterActivation = ENABLE;
	CAN_FilterInitStructure.FilterBank = 0x00;
	CAN_FilterInitStructure.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	CAN_FilterInitStructure.FilterIdHigh = CAN1_RX_MSG_ID_BANK0[0] << 5;
	CAN_FilterInitStructure.FilterIdLow = CAN1_RX_MSG_ID_BANK0[1] << 5;
	CAN_FilterInitStructure.FilterMaskIdHigh = CAN1_RX_MSG_ID_BANK0[2] << 5;
	CAN_FilterInitStructure.FilterMaskIdLow = CAN1_RX_MSG_ID_BANK0[3] << 5;
	CAN_FilterInitStructure.FilterMode = CAN_FILTERMODE_IDLIST;
	CAN_FilterInitStructure.FilterScale = CAN_FILTERSCALE_16BIT;
	CAN_FilterInitStructure.SlaveStartFilterBank = 0;
	HAL_CAN_ConfigFilter(&hcan1, &CAN_FilterInitStructure);

	/* Bank 1: Debug6_MotorTemp(0x506), Debug9_Status(0x509), Debug8_IGBT(0x508), Debug7_Inverter(0x507) */
	CAN_FilterInitStructure.FilterBank = 0x01;
	CAN_FilterInitStructure.FilterIdHigh = CAN1_RX_MSG_ID_BANK1[0] << 5;
	CAN_FilterInitStructure.FilterIdLow = CAN1_RX_MSG_ID_BANK1[1] << 5;
	CAN_FilterInitStructure.FilterMaskIdHigh = CAN1_RX_MSG_ID_BANK1[2] << 5;
	CAN_FilterInitStructure.FilterMaskIdLow = CAN1_RX_MSG_ID_BANK1[3] << 5;
	HAL_CAN_ConfigFilter(&hcan1, &CAN_FilterInitStructure);

	/* Bank 2: Debug3_Diag12(0x503), Debug4_Diag34(0x504) */
	CAN_FilterInitStructure.FilterBank = 0x02;
	CAN_FilterInitStructure.FilterIdHigh = CAN1_RX_MSG_ID_BANK2[0] << 5;
	CAN_FilterInitStructure.FilterIdLow = CAN1_RX_MSG_ID_BANK2[1] << 5;
	CAN_FilterInitStructure.FilterMaskIdHigh = CAN1_RX_MSG_ID_BANK2[2] << 5;
	CAN_FilterInitStructure.FilterMaskIdLow = CAN1_RX_MSG_ID_BANK2[3] << 5;
	HAL_CAN_ConfigFilter(&hcan1, &CAN_FilterInitStructure);

	/* Bank 3: IMU_Raw(0x50) -> FIFO1 */
	CAN_FilterInitStructure.FilterBank = 0x03;
	CAN_FilterInitStructure.FilterFIFOAssignment = CAN_FILTER_FIFO1;
	CAN_FilterInitStructure.FilterIdHigh = CAN1_RX_MSG_ID_BANK3[0] << 5;
	CAN_FilterInitStructure.FilterIdLow = CAN1_RX_MSG_ID_BANK3[1] << 5;
	CAN_FilterInitStructure.FilterMaskIdHigh = CAN1_RX_MSG_ID_BANK3[2] << 5;
	CAN_FilterInitStructure.FilterMaskIdLow = CAN1_RX_MSG_ID_BANK3[3] << 5;
	HAL_CAN_ConfigFilter(&hcan1, &CAN_FilterInitStructure);
}

/*
 * @func: CAN报文发送[标准格式、数据帧]
 */
void User_CAN_Send()
{
	TxHeader.RTR = CAN_RTR_DATA;
	TxHeader.IDE = CAN_ID_STD;
	TxHeader.StdId = CAN1_ID;
	TxHeader.TransmitGlobalTime = DISABLE;
	TxHeader.DLC = 8;
	HAL_CAN_AddTxMessage(&hcan1, &TxHeader, CAN_TxData, &CAN1_TxMail);
}

/*
 * @func: CAN paper send[standard form、data]
 */
//shaoqi_add
//发送指定ID
void User_CAN_Send_sq(uint32_t CAN_ID_NEW,uint8_t* CAN_TxData_NEW)
{
	TxHeader.RTR = CAN_RTR_DATA;
	TxHeader.IDE = CAN_ID_STD;
	TxHeader.StdId = CAN_ID_NEW;
	TxHeader.TransmitGlobalTime = DISABLE;
	TxHeader.DLC = 8;
	HAL_CAN_AddTxMessage(&hcan1, &TxHeader, CAN_TxData_NEW, &CAN1_TxMail);
}

void CAN1_SendHeartbeat(void)
{
  uint8_t heartbeat_data[8];

  if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 0U) {
    return;
  }

  heartbeat_data[0] = 0xA5;
  heartbeat_data[1] = 0x5A;
  heartbeat_data[2] = g_can_heartbeat_counter++;
  heartbeat_data[3] = 0xF4;
  heartbeat_data[4] = 0x07;
  heartbeat_data[5] = 0x40;
  heartbeat_data[6] = 0x00;
  heartbeat_data[7] = 0x01;

  User_CAN_Send_sq(CAN1_ID, heartbeat_data);
}

void CAN_RequestDriveMode(int32_t mode_index)
{
  uint8_t mode_data[8] = {0};
  static const uint8_t mode_code[4] = {48U, 49U, 50U, 51U};

  if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 0U) {
    return;
  }

  if(mode_index < 0) mode_index = 0;
  if(mode_index > 3) mode_index = 3;
  mode_data[0] = mode_code[mode_index];

  User_CAN_Send_sq(0x310, mode_data);
}

void CAN_ServiceTask(void *argument)
{
  /* USER CODE BEGIN CAN_ServiceTask */
  (void)argument;

  for(;;)
  {
    /* CAN service task - currently handled in interrupts */
    osDelay(1000);
  }
  /* USER CODE END CAN_ServiceTask */
}

/*
 * @func: CAN1_Filter_Config
 * DBC: Vehicle_CanB.dbc
 * Message-ID mapping: DBC decimal -> HEX
 *   BO_1289 Debug9=0x509, BO_1288 Debug8=0x508, BO_1287 Debug7=0x507,
 *   BO_1286 Debug6=0x506, BO_1285 Debug5=0x505, BO_1282 Debug2=0x502,
 *   BO_1284 Debug4=0x504, BO_1283 Debug3=0x503,
 *   BO_773  DataLogger=0x305, BO_769 GPS_Speed=0x301(Display->ECU),
 *   BO_80   IMU_Raw=0x50
 * Wheel order mapping: DBC {RL,RR,FL,FR}(0,1,2,3) -> Dashboard {LF,LR,RF,RR}(0,1,2,3)
 *   dash[0]=LF <- DBC[2]=FL, dash[1]=LR <- DBC[0]=RL,
 *   dash[2]=RF <- DBC[3]=FR, dash[3]=RR <- DBC[1]=RR
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	if(HAL_CAN_GetRxMessage(hcan, CAN_FILTER_FIFO0, &RxHeader, CAN_RxData)!= HAL_OK)
	{
		Error_Handler();
	}

	switch(RxHeader.StdId)
	{
	  /* BMS 0x401 - 非DBC/VCI协议: SOC(byte6), Volt(byte0-1), Curr(byte4-5), MaxTemp(byte7) */
	  case 0x401:
	  {
	    g_can_dashboard_data.soc = (int32_t)CAN_RxData[6];
	    g_can_dashboard_data.sum_voltage = (int32_t)CAN_RxData[0] + ((int32_t)CAN_RxData[1] * 256);
	    g_can_dashboard_data.sum_current = (int32_t)CAN_RxData[4] + ((int32_t)CAN_RxData[5] * 256);
	    g_can_dashboard_data.max_temperature = (int32_t)CAN_RxData[7];
	    break;
	  }

	  /* DBC BO_773 DataLogger 0x305: SteeringWheelAngle(0|16@1-), APS_OpenPct(16|16@1+), OilPressure_Kpa(32|16@1+) */
	  case 0x305:
	  {
	    /* SG_ SteeringWheelAngle: 0|16@1- (0.1,0) deg, raw=int16*0.1 */
	    g_can_dashboard_data.steering_angle = (int32_t)(int16_t)(CAN_RxData[0] | ((uint16_t)CAN_RxData[1] << 8));
	    /* SG_ APS_OpenPct: 16|16@1+ (0.1,0) %, raw=uint16*0.1, store integer pct */
	    {
	      uint16_t aps_raw = (uint16_t)CAN_RxData[2] | ((uint16_t)CAN_RxData[3] << 8);
	      g_can_dashboard_data.aps_open_pct = (int32_t)(aps_raw / 10U);
	    }
	    /* SG_ OilPressure_Kpa: 32|16@1+ (0.001,0) Kpa, raw count */
	    {
	      uint16_t oil_raw = (uint16_t)CAN_RxData[4] | ((uint16_t)CAN_RxData[5] << 8);
	      g_can_dashboard_data.oil_pressure = (int32_t)oil_raw;
	    }
	    break;
	  }

	  /* DBC BO_1282 Debug2 0x502: FR/FL/RR/RL_ActualTorque (1,0) 0.1%Mn, 16-bit signed each */
	  case 0x502:
	  {
	    /* DBC order: RL(0), RR(1), FL(2), FR(3) -> Dashboard: LF(0), LR(1), RF(2), RR(3) */
	    static const uint8_t dbc_map[4] = {1U, 3U, 0U, 2U};
	    uint32_t i;
	    for(i = 0U; i < 4U; i++) {
	      int16_t raw = (int16_t)(CAN_RxData[i * 2U] | ((uint16_t)CAN_RxData[i * 2U + 1U] << 8));
	      g_can_dashboard_data.torque[dbc_map[i]] = (int32_t)raw;
	    }
	    break;
	  }

	  /* DBC BO_1285 Debug5 0x505: FR/FL/RR/RL_ActualVelocity (1,0), 16-bit signed each */
	  case 0x505:
	  {
	    static const uint8_t dbc_map[4] = {1U, 3U, 0U, 2U};
	    uint32_t i;
	    for(i = 0U; i < 4U; i++) {
	      int16_t raw = (int16_t)(CAN_RxData[i * 2U] | ((uint16_t)CAN_RxData[i * 2U + 1U] << 8));
	      g_can_dashboard_data.rpm[dbc_map[i]] = (int32_t)raw;
	    }
	    break;
	  }

	  /* DBC BO_1286 Debug6 0x506: FR/FL/RR/RL_Motor_temperature (0.1,0) degC, 16-bit signed each */
	  case 0x506:
	  {
	    static const uint8_t dbc_map[4] = {1U, 3U, 0U, 2U};
	    uint32_t i;
	    for(i = 0U; i < 4U; i++) {
	      int16_t raw = (int16_t)(CAN_RxData[i * 2U] | ((uint16_t)CAN_RxData[i * 2U + 1U] << 8));
	      g_can_dashboard_data.motor_temp[dbc_map[i]] = (int32_t)(raw / 10);
	    }
	    break;
	  }

	  /* DBC BO_1289 Debug9 0x509: AMK status flags + ModeFlag, DLC=5 */
	  case 0x509:
	  {
	    /* SG_ FR_AMK_bEnable(20|1), FL_AMK_bEnable(21|1), RR_AMK_bEnable(22|1), RL_AMK_bEnable(23|1) */
	    /* DBC byte2: bit4=FR, bit5=FL, bit6=RR, bit7=RL -> Dashboard: LF=FL, LR=RL, RF=FR, RR=RR */
	    g_can_dashboard_data.motor_enable[0] = (CAN_RxData[2] >> 5) & 0x01U;
	    g_can_dashboard_data.motor_enable[1] = (CAN_RxData[2] >> 7) & 0x01U;
	    g_can_dashboard_data.motor_enable[2] = (CAN_RxData[2] >> 4) & 0x01U;
	    g_can_dashboard_data.motor_enable[3] = (CAN_RxData[2] >> 6) & 0x01U;

	    /* SG_ ModeFlag: 4|4@1- (1,0) [-8,7], signed 4-bit in byte0 high nibble */
	    {
	      int32_t mode_val = (int32_t)((CAN_RxData[0] >> 4) & 0x0FU);
	      if(mode_val & 8) mode_val -= 16;
	      g_can_dashboard_data.mode_index = mode_val;
	    }
	    break;
	  }

	  /* DBC BO_1288 Debug8 0x508: FR/FL/RR/RL_IGBT_temperature (0.1,0) degC, 16-bit signed each */
	  case 0x508:
	  {
	    static const uint8_t dbc_map[4] = {1U, 3U, 0U, 2U};
	    uint32_t i;
	    for(i = 0U; i < 4U; i++) {
	      int16_t raw = (int16_t)(CAN_RxData[i * 2U] | ((uint16_t)CAN_RxData[i * 2U + 1U] << 8));
	      g_can_dashboard_data.igbt_temp[dbc_map[i]] = (int32_t)(raw / 10);
	    }
	    break;
	  }

	  /* DBC BO_1287 Debug7 0x507: FR/FL/RR/RL_Inverter_temperature (0.1,0) degC, 16-bit signed each */
	  case 0x507:
	  {
	    static const uint8_t dbc_map[4] = {1U, 3U, 0U, 2U};
	    uint32_t i;
	    for(i = 0U; i < 4U; i++) {
	      int16_t raw = (int16_t)(CAN_RxData[i * 2U] | ((uint16_t)CAN_RxData[i * 2U + 1U] << 8));
	      g_can_dashboard_data.inverter_temp[dbc_map[i]] = (int32_t)(raw / 10);
	    }
	    break;
	  }

	  /* DBC BO_1284 Debug4 0x504: Diagnostic_number_3(0|32), Diagnostic_number_4(32|32), 32-bit each */
	  case 0x504:
	  {
	    g_can_dashboard_data.diag_num[2] = (uint32_t)CAN_RxData[0]
	      | ((uint32_t)CAN_RxData[1] << 8)
	      | ((uint32_t)CAN_RxData[2] << 16)
	      | ((uint32_t)CAN_RxData[3] << 24);
	    g_can_dashboard_data.diag_num[3] = (uint32_t)CAN_RxData[4]
	      | ((uint32_t)CAN_RxData[5] << 8)
	      | ((uint32_t)CAN_RxData[6] << 16)
	      | ((uint32_t)CAN_RxData[7] << 24);
	    break;
	  }

	  /* DBC BO_1283 Debug3 0x503: Diagnostic_number_1(0|32), Diagnostic_number_2(32|32), 32-bit each */
	  case 0x503:
	  {
	    g_can_dashboard_data.diag_num[0] = (uint32_t)CAN_RxData[0]
	      | ((uint32_t)CAN_RxData[1] << 8)
	      | ((uint32_t)CAN_RxData[2] << 16)
	      | ((uint32_t)CAN_RxData[3] << 24);
	    g_can_dashboard_data.diag_num[1] = (uint32_t)CAN_RxData[4]
	      | ((uint32_t)CAN_RxData[5] << 8)
	      | ((uint32_t)CAN_RxData[6] << 16)
	      | ((uint32_t)CAN_RxData[7] << 24);
	    break;
	  }

	  default:
	    break;
    }

    Dashboard_UI_SubmitData(&g_can_dashboard_data);
}

/*
 * @func: CAN_SendGPSSpeed - send GPS speed to ECU
 * DBC BO_769 GPS_Speed 0x301: GroundSpeed (0.1,0) km/h, Display->ECU
 * Speed source: GNSS module via gps.c (UM982 KSXT message)
 */
void CAN_SendGPSSpeed(int32_t speed_kmh_tenths)
{
  uint8_t speed_data[8] = {0};

  if(speed_kmh_tenths < 0) speed_kmh_tenths = 0;
  if(speed_kmh_tenths > 3000) speed_kmh_tenths = 3000;

  speed_data[0] = (uint8_t)(speed_kmh_tenths & 0xFFU);
  speed_data[1] = (uint8_t)((speed_kmh_tenths >> 8) & 0xFFU);

  g_can_dashboard_data.speed = (speed_kmh_tenths + 5) / 10;

  if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) > 0U) {
    User_CAN_Send_sq(0x301, speed_data);
  }
}

/*
 * @func: CAN FIFO1 callback - IMU raw data relay/parse
 * DBC BO_80 IMU_Raw 0x50: IMU_Header(0|8), IMU_SubType(8|8), IMU_RawData(16|48)
 * SubType dispatches to DBC parsed messages:
 *   0x50=Time(BO_96), 0x51=Accel(BO_97), 0x52=Gyro(BO_98),
 *   0x53=Angle(BO_99~101 SubCmd 1/2/3), 0x54=Magnetic(BO_102)
 */
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	if(HAL_CAN_GetRxMessage(hcan, CAN_FILTER_FIFO1, &RxHeader, CAN_RxData)!= HAL_OK)
	{
		Error_Handler();
	}

	/* DBC: IMU_Header must be 0x55 */
	if(RxHeader.StdId != 0x50U) return;
	if(CAN_RxData[0] != 0x55U) return;

	switch(CAN_RxData[1])
	{
	  case 0x50U:  /* DBC IMU_SubType=0x50: Time -> relay 0x60 BO_96 */
	    User_CAN_Send_sq(0x60, CAN_RxData);
	    break;
	  case 0x51U:  /* DBC IMU_SubType=0x51: Accel -> relay 0x61 BO_97 */
	  {
	    User_CAN_Send_sq(0x61, CAN_RxData);
	    /* DBC BO_97: IMU_AccelX(0|16), IMU_AccelY(16|16), IMU_AccelZ(32|16), raw int16 */
	    g_can_dashboard_data.imu_accel[0] = (int32_t)(int16_t)(CAN_RxData[2] | ((uint16_t)CAN_RxData[3] << 8));
	    g_can_dashboard_data.imu_accel[1] = (int32_t)(int16_t)(CAN_RxData[4] | ((uint16_t)CAN_RxData[5] << 8));
	    g_can_dashboard_data.imu_accel[2] = (int32_t)(int16_t)(CAN_RxData[6] | ((uint16_t)CAN_RxData[7] << 8));
	    break;
	  }
	  case 0x52U:  /* DBC IMU_SubType=0x52: Gyro -> relay 0x62 BO_98 */
	  {
	    User_CAN_Send_sq(0x62, CAN_RxData);
	    /* DBC BO_98: IMU_GyroX(0|16), IMU_GyroY(16|16), IMU_GyroZ(32|16), raw int16 */
	    g_can_dashboard_data.imu_gyro[0] = (int32_t)(int16_t)(CAN_RxData[2] | ((uint16_t)CAN_RxData[3] << 8));
	    g_can_dashboard_data.imu_gyro[1] = (int32_t)(int16_t)(CAN_RxData[4] | ((uint16_t)CAN_RxData[5] << 8));
	    g_can_dashboard_data.imu_gyro[2] = (int32_t)(int16_t)(CAN_RxData[6] | ((uint16_t)CAN_RxData[7] << 8));
	    break;
	  }
	  case 0x53U:  /* DBC IMU_SubType=0x53: Angle(SubCmd) -> relay 0x63/0x64/0x65 BO_99/100/101 */
	  {
	    if(CAN_RxData[2] == 0x01U)
	    {
	      User_CAN_Send_sq(0x63, CAN_RxData);
	      /* DBC BO_99 IMU_Roll: IMU_Angle_Roll(0|16), TODO: verify byte offset with IMU firmware */
	      g_can_dashboard_data.imu_roll = (int32_t)(int16_t)(CAN_RxData[2] | ((uint16_t)CAN_RxData[3] << 8));
	    }
	    else if(CAN_RxData[2] == 0x02U)
	    {
	      User_CAN_Send_sq(0x64, CAN_RxData);
	      /* DBC BO_100 IMU_Pitch: IMU_Angle_Pitch(0|16), TODO: verify byte offset with IMU firmware */
	      g_can_dashboard_data.imu_pitch = (int32_t)(int16_t)(CAN_RxData[2] | ((uint16_t)CAN_RxData[3] << 8));
	    }
	    else if(CAN_RxData[2] == 0x03U)
	    {
	      User_CAN_Send_sq(0x65, CAN_RxData);
	      /* DBC BO_101 IMU_Yaw: IMU_Angle_Yaw(0|16), TODO: verify byte offset with IMU firmware */
	      g_can_dashboard_data.imu_yaw = (int32_t)(int16_t)(CAN_RxData[2] | ((uint16_t)CAN_RxData[3] << 8));
	    }
	    break;
	  }
	  case 0x54U:  /* DBC IMU_SubType=0x54: Magnetic -> relay 0x66 BO_102 */
	  {
	    User_CAN_Send_sq(0x66, CAN_RxData);
	    /* DBC BO_102: IMU_MagX(0|16), IMU_MagY(16|16), IMU_MagZ(32|16), raw int16 */
	    g_can_dashboard_data.imu_mag[0] = (int32_t)(int16_t)(CAN_RxData[2] | ((uint16_t)CAN_RxData[3] << 8));
	    g_can_dashboard_data.imu_mag[1] = (int32_t)(int16_t)(CAN_RxData[4] | ((uint16_t)CAN_RxData[5] << 8));
	    g_can_dashboard_data.imu_mag[2] = (int32_t)(int16_t)(CAN_RxData[6] | ((uint16_t)CAN_RxData[7] << 8));
	    break;
	  }
	  default:
	    break;
	}
}

/* USER CODE END 1 */
