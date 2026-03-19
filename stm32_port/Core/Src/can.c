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

/* USER CODE BEGIN 0 */
CAN_TxHeaderTypeDef TxHeader;
uint8_t CAN_TxData[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 };
uint32_t CAN1_TxMail;
uint32_t CAN1_ID = 0x102;
CAN_RxHeaderTypeDef RxHeader;
uint8_t CAN_RxData[8] = { 0 };
uint16_t CAN1_RX_MSG_ID[4] = {0x401, 0x501, 0x502, 0x50};
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

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**CAN1 GPIO Configuration
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11|GPIO_PIN_12);

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
	CAN_FilterTypeDef CAN_FilterInitStructure = {
			.FilterActivation = ENABLE,                    //enable the filter
			.FilterBank = 0x00,                            //encode the filters family,range:0-13
			.FilterFIFOAssignment = CAN_FILTER_FIFO0,      //报文储存FIFO编号，FIFO0
			.FilterIdHigh = CAN1_RX_MSG_ID[0]<<5,           //first ID
			.FilterIdLow = CAN1_RX_MSG_ID[1]<<5,            //second ID
			.FilterMaskIdHigh = CAN1_RX_MSG_ID[2]<<5,       //third ID
			.FilterMaskIdLow = CAN1_RX_MSG_ID[3]<<5,        //fourth ID
			.FilterMode = CAN_FILTERMODE_IDLIST,           //ID列表模式
			.FilterScale = CAN_FILTERSCALE_16BIT,          //16位，一个过滤器组可设置4个可通过ID
			.SlaveStartFilterBank = 0                      //主从过滤器分界线，单CAN无意义
	};
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

/*
 * @func: CAN1报文接收中断[FIFO0]
 */
void HAL_CAN1_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	if(HAL_CAN_GetRxMessage(hcan, CAN_FILTER_FIFO0, &RxHeader, CAN_RxData)!= HAL_OK)
	{
		// 错误处理
	    Error_Handler();
	}
//	printf("ID:0x%h\r\n",RxHeader.StdId);

	//根据不同的ID，获得不同的信息，具体请看车队的CAN协议
	if(RxHeader.StdId==0x401)
	{

	}
	if(RxHeader.StdId==0x501)//新增电机扭矩与常态化驾驶模式
	{

	}
	if(RxHeader.StdId==0x502)
	{

	}
//	if(RxHeader.StdId==0x50)//IMU 回发与数据处理
//	{
////		User_CAN_Send_sq(0x03,CAN_RxData);
//		if(CAN_RxData[1]==0x50)
//		{
//			User_CAN_Send_sq(0x60,CAN_RxData);
//		}else if(CAN_RxData[1]==0x51)
//		{
//			User_CAN_Send_sq(0x61,CAN_RxData);
//
//		}else if(CAN_RxData[1]==0x52){
//			User_CAN_Send_sq(0x62,CAN_RxData);
//
//		}else if(CAN_RxData[1]==0x53){
//			if(CAN_RxData[2]==0x01)
//			{
//				User_CAN_Send_sq(0x63,CAN_RxData);
//			}else if(CAN_RxData[2]==0x02)
//			{
//				User_CAN_Send_sq(0x64,CAN_RxData);
//			}else if(CAN_RxData[2]==0x03)
//			{
//				User_CAN_Send_sq(0x65,CAN_RxData);
//			}
//
//		}else if(CAN_RxData[1]==0x54){
//			User_CAN_Send_sq(0x66,CAN_RxData);
//		}
//	}

}

/*
 * @func: CAN锟斤拷锟侥斤拷锟斤拷锟叫讹拷[FIFO1],锟斤拷锟�0x50
 */

//void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
//{
//	if(HAL_CAN_GetRxMessage(hcan, CAN_FILTER_FIFO0, &RxHeader, CAN_RxData)!= HAL_OK)
//	{
//	    // 锟斤拷锟斤拷锟斤拷
//	    Error_Handler();
//	}
//	printf("ID:0x%X\r\n",RxHeader.StdId);
//	if(RxHeader.StdId==0x50)
//	{
//		if(CAN_RxData[1]==0x50)
//		{
//			User_CAN_Send_sq(0x60,CAN_RxData);
//		}else if(CAN_RxData[1]==0x51)
//		{
//			User_CAN_Send_sq(0x61,CAN_RxData);
//		}else if(CAN_RxData[1]==0x52){
//			User_CAN_Send_sq(0x62,CAN_RxData);
//		}else if(CAN_RxData[1]==0x53){
//			if(CAN_RxData[2]==0x01)
//			{
//				User_CAN_Send_sq(0x63,CAN_RxData);
//			}else if(CAN_RxData[2]==0x02)
//			{
//				User_CAN_Send_sq(0x64,CAN_RxData);
//			}else if(CAN_RxData[2]==0x03)
//			{
//				User_CAN_Send_sq(0x65,CAN_RxData);
//			}
//
//		}else if(CAN_RxData[1]==0x54){
//			User_CAN_Send_sq(0x66,CAN_RxData);
//		}
//	}
//
//}

/* USER CODE END 1 */
