#include "sd_log.h"
#include "fatfs.h"
#include "sdio.h"
#include "main.h"

static const char g_test_text[] = "Manba Outlaws\r\nSD OK\r\n";

static void led_red_on(void)  { HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET); }
static void led_red_off(void) { HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET); }

void SD_Log_InitAndWrite(const dashboard_data_t * data)
{
    FRESULT fr;
    FIL     fil;
    UINT    bw;

    (void)data;

    if (!g_sd_ready) {
        led_red_on();                    /* SD init failed */
        return;
    }

    fr = f_mount(&USERFatFS, USERPath, 1);
    if (fr != FR_OK) {
        while (1) {                      /* mount failed: red blink */
            led_red_on();  HAL_Delay(200);
            led_red_off(); HAL_Delay(200);
        }
    }

    fr = f_open(&fil, "0:/manba.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        while (1) {                      /* open failed: red fast blink */
            led_red_on();  HAL_Delay(50);
            led_red_off(); HAL_Delay(50);
        }
    }

    f_write(&fil, g_test_text, sizeof(g_test_text) - 1, &bw);
    f_close(&fil);
    f_mount(NULL, USERPath, 0);

    /* success: green off -> on (visible change) */
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
    HAL_Delay(300);
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
}
