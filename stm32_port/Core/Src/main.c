/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "can.h"
#include "dma.h"
#include "fatfs.h"
#include "rtc.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "../lvgl/lvgl.h"
#include "dashboard_ui.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static lv_display_t * g_lvgl_display;
static uint16_t g_lvgl_draw_buf[480 * 20];
volatile uint32_t g_lvgl_flush_count;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
void SSD1963_Init(void);
void LCD_FillColor(uint16_t color);
void LVGL_Port_Init(void);
void LED_Diag_SetBootStage(uint8_t stage);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define LCD_HOR_RES 480
#define LCD_VER_RES 272

// 1. 基础读写宏 (保持 A16 逻辑)
#define LCD_REG  *(__IO uint16_t *)(0x60000000)
#define LCD_DATA *(__IO uint16_t *)(0x60020000)

static void LCD_SetAddressWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
  LCD_REG = 0x2A;
  LCD_DATA = x1 >> 8;
  LCD_DATA = x1 & 0xFF;
  LCD_DATA = x2 >> 8;
  LCD_DATA = x2 & 0xFF;

  LCD_REG = 0x2B;
  LCD_DATA = y1 >> 8;
  LCD_DATA = y1 & 0xFF;
  LCD_DATA = y2 >> 8;
  LCD_DATA = y2 & 0xFF;

  LCD_REG = 0x2C;
}

static void LCD_WritePixels(const uint16_t * colors, uint32_t pixel_count)
{
  uint32_t index;

  for(index = 0; index < pixel_count; index++) {
    LCD_DATA = colors[index];
  }
}

static void LCD_Backlight_On(void)
{
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_SET);
}

static void LCD_Backlight_Off(void)
{
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);
}

static void SSD1963_Reset_Assert(void)
{
  HAL_GPIO_WritePin(SSD1963_RST_GPIO_Port, SSD1963_RST_Pin, GPIO_PIN_RESET);
}

static void SSD1963_Reset_Release(void)
{
  HAL_GPIO_WritePin(SSD1963_RST_GPIO_Port, SSD1963_RST_Pin, GPIO_PIN_SET);
}

static void SSD1963_HardReset(void)
{
  SSD1963_Reset_Assert();
  HAL_Delay(10);
  SSD1963_Reset_Release();
  HAL_Delay(60);
}

void LED_Diag_SetBootStage(uint8_t stage)
{
  switch(stage) {
    case 0:
      HAL_GPIO_WritePin(GPIOD, LED_RED_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOD, LED_GREEN_Pin, GPIO_PIN_RESET);
      break;

    case 1:
      HAL_GPIO_WritePin(GPIOD, LED_RED_Pin, GPIO_PIN_SET);
      HAL_GPIO_WritePin(GPIOD, LED_GREEN_Pin, GPIO_PIN_SET);
      break;

    case 2:
      HAL_GPIO_WritePin(GPIOD, LED_RED_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOD, LED_GREEN_Pin, GPIO_PIN_SET);
      break;

    default:
      HAL_GPIO_WritePin(GPIOD, LED_RED_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(GPIOD, LED_GREEN_Pin, GPIO_PIN_RESET);
      break;
  }
}

static void lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
  uint32_t width = (uint32_t)(area->x2 - area->x1 + 1);
  uint32_t height = (uint32_t)(area->y2 - area->y1 + 1);

  LCD_SetAddressWindow((uint16_t)area->x1, (uint16_t)area->y1, (uint16_t)area->x2, (uint16_t)area->y2);
  LCD_WritePixels((const uint16_t *)px_map, width * height);
  g_lvgl_flush_count++;
  lv_display_flush_ready(disp);
}

void LVGL_Port_Init(void)
{
  lv_init();
  lv_tick_set_cb(HAL_GetTick);

  g_lvgl_display = lv_display_create(LCD_HOR_RES, LCD_VER_RES);
  lv_display_set_color_format(g_lvgl_display, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(g_lvgl_display, lvgl_flush_cb);
  lv_display_set_buffers(g_lvgl_display,
               g_lvgl_draw_buf,
               NULL,
               sizeof(g_lvgl_draw_buf),
               LV_DISPLAY_RENDER_MODE_PARTIAL);

  Dashboard_UI_Init();
  lv_refr_now(g_lvgl_display);
  LED_Diag_SetBootStage(2);
}

// 2. 商家标准的 SSD1963 初始化序列
void SSD1963_Init(void) {
    // 等待屏幕上电稳定
  HAL_Delay(10);

    LCD_REG = 0x01;
    HAL_Delay(10);

    // --- PLL 配置 ---
    LCD_REG = 0xE2;
    LCD_DATA = 0x2D; // 商家源码参数 N=45
    LCD_DATA = 0x02; // M=2
    LCD_DATA = 0x04; // 验证标志

    LCD_REG = 0xE0;
    LCD_DATA = 0x01; // 开启 PLL
    HAL_Delay(10);
    LCD_REG = 0xE0;
    LCD_DATA = 0x03; // 切换到 PLL 输出
    HAL_Delay(12);

    LCD_REG = 0x01;  // 软件复位
    HAL_Delay(10);

    // --- 像素频率设置 ---
    LCD_REG = 0xE6;
    LCD_DATA = 0x00;
    LCD_DATA = 0xFF;
    LCD_DATA = 0xBE;

    // --- LCD 模式与分辨率设置 ---
    LCD_REG = 0xB0;
    LCD_DATA = 0x20; // 24-bit 模式
    LCD_DATA = 0x00; // TFT 模式
    LCD_DATA = (480-1)>>8; // 水平像素
    LCD_DATA = 480-1;
    LCD_DATA = (272-1)>>8; // 垂直像素
    LCD_DATA = 272-1;
    LCD_DATA = 0x00; // RGB 序列

    // --- 时序 Porch 设置 (依据 lcd.h 定义) ---
    LCD_REG = 0xB4; // 水平周期
    LCD_DATA = (532-1)>>8; // HT = 480+43+8+1 = 532
    LCD_DATA = 532-1;
    LCD_DATA = 43>>8;      // HPS = Back Porch
    LCD_DATA = 43;
    LCD_DATA = 1-1;       // HPW = 1
    LCD_DATA = 0x00; LCD_DATA = 0x00; LCD_DATA = 0x00;

    LCD_REG = 0xB6; // 垂直周期
    LCD_DATA = (293-1)>>8; // VT = 272+12+8+1 = 293
    LCD_DATA = 293-1;
    LCD_DATA = 12>>8;      // VPS = Back Porch
    LCD_DATA = 12;
    LCD_DATA = 1-1;       // VPW = 1
    LCD_DATA = 0x00; LCD_DATA = 0x00;

    LCD_REG = 0xF0; // 接口设置
    LCD_DATA = 0x03; // 16-bit(565 format)

    LCD_REG = 0x29; // 开启显示
  HAL_Delay(10);
}

// 3. 读取 ID 测试函数 (用于诊断)
uint16_t SSD1963_ReadID(void) {
    uint16_t id = 0;
    LCD_REG = 0xA1;
    LCD_DATA; // 丢弃第一个哑读 (SSD1963 特性)
    id = LCD_DATA << 8;
    id |= (LCD_DATA & 0xFF);
    return id; // 预期返回值应包含 0x196 关键词
}

void LCD_FillColor(uint16_t color)
{
    uint32_t i;

  LCD_SetAddressWindow(0, 0, LCD_HOR_RES - 1, LCD_VER_RES - 1);
  for (i = 0; i < ((uint32_t)LCD_HOR_RES * LCD_VER_RES); i++)
    {
        LCD_DATA = color;
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_CAN1_Init();
  MX_CAN2_Init();
  MX_RTC_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_FATFS_Init();
  MX_FSMC_Init();
  /* USER CODE BEGIN 2 */
  CAN1_Filter_Config();
  if(HAL_CAN_Start(&hcan1) != HAL_OK) {
    Error_Handler();
  }
  if(HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
    Error_Handler();
  }

  LCD_Backlight_Off();
  SSD1963_Reset_Assert();
  LED_Diag_SetBootStage(0);

  HAL_Delay(50);

  SSD1963_HardReset();
  SSD1963_Init();
  LED_Diag_SetBootStage(1);
  LVGL_Port_Init();
  HAL_Delay(10);
  LCD_Backlight_On();
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM8 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM8)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
