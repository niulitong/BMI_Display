#include "sd_log.h"
#include "fatfs.h"
#include "sdio.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

static const char g_test_text[] = "Manba Outlaws\r\nSD OK\r\n";

#define SD_ODOMETER_MAGIC       0x4F444F31UL
#define SD_ODOMETER_CHECK_SALT  0x6D2B79F5UL
#define SD_ODOMETER_FILE_A      "0:/ODO_A.DAT"
#define SD_ODOMETER_FILE_B      "0:/ODO_B.DAT"

typedef struct {
    uint32_t magic;
    uint32_t sequence;
    uint32_t total_m;
    uint32_t checksum;
} sd_odometer_record_t;

static TaskHandle_t g_sd_odometer_task_handle;
static StaticTask_t g_sd_odometer_task_control;
static StackType_t g_sd_odometer_task_stack[768U];
static volatile uint32_t g_sd_odometer_pending_m;
static volatile uint8_t g_sd_odometer_save_pending;
static uint32_t g_sd_odometer_sequence;

static uint32_t sd_odometer_checksum(uint32_t sequence, uint32_t total_m)
{
    return sequence ^ total_m ^ (total_m << 13) ^ (total_m >> 7) ^
           SD_ODOMETER_CHECK_SALT;
}

static uint8_t sd_odometer_read_file(const char * path,
                                     sd_odometer_record_t * record)
{
    FIL fil;
    UINT bytes_read = 0U;
    FRESULT fr;

    fr = f_open(&fil, path, FA_READ);
    if(fr != FR_OK) return 0U;
    fr = f_read(&fil, record, sizeof(*record), &bytes_read);
    f_close(&fil);
    if((fr != FR_OK) || (bytes_read != sizeof(*record)) ||
       (record->magic != SD_ODOMETER_MAGIC) ||
       (record->checksum !=
        sd_odometer_checksum(record->sequence, record->total_m))) {
        return 0U;
    }
    return 1U;
}

uint8_t SD_Odometer_Load(uint32_t * total_m)
{
    sd_odometer_record_t record_a;
    sd_odometer_record_t record_b;
    uint8_t valid_a;
    uint8_t valid_b;

    if((total_m == NULL) || (g_sd_ready == 0U)) return 0U;
    if(f_mount(&USERFatFS, USERPath, 1) != FR_OK) return 0U;
    valid_a = sd_odometer_read_file(SD_ODOMETER_FILE_A, &record_a);
    valid_b = sd_odometer_read_file(SD_ODOMETER_FILE_B, &record_b);
    f_mount(NULL, USERPath, 0);

    if((valid_a != 0U) &&
       ((valid_b == 0U) ||
        ((int32_t)(record_a.sequence - record_b.sequence) > 0))) {
        g_sd_odometer_sequence = record_a.sequence;
        *total_m = record_a.total_m;
        return 1U;
    }
    if(valid_b != 0U) {
        g_sd_odometer_sequence = record_b.sequence;
        *total_m = record_b.total_m;
        return 1U;
    }
    g_sd_odometer_sequence = 0U;
    return 0U;
}

static uint8_t sd_odometer_save(uint32_t total_m)
{
    sd_odometer_record_t record;
    const char * path;
    FIL fil;
    UINT bytes_written = 0U;
    FRESULT fr;

    if(g_sd_ready == 0U) return 0U;
    record.magic = SD_ODOMETER_MAGIC;
    record.sequence = g_sd_odometer_sequence + 1U;
    record.total_m = total_m;
    record.checksum = sd_odometer_checksum(record.sequence, record.total_m);
    path = ((record.sequence & 1U) != 0U) ?
           SD_ODOMETER_FILE_B : SD_ODOMETER_FILE_A;

    if(f_mount(&USERFatFS, USERPath, 1) != FR_OK) return 0U;
    fr = f_open(&fil, path, FA_CREATE_ALWAYS | FA_WRITE);
    if(fr == FR_OK) {
        fr = f_write(&fil, &record, sizeof(record), &bytes_written);
        if((fr == FR_OK) && (bytes_written == sizeof(record))) {
            fr = f_sync(&fil);
        }
        f_close(&fil);
    }
    f_mount(NULL, USERPath, 0);
    if((fr != FR_OK) || (bytes_written != sizeof(record))) return 0U;
    g_sd_odometer_sequence = record.sequence;
    return 1U;
}

static void SD_Odometer_Task(void * argument)
{
    uint32_t pending_m;

    (void)argument;
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(1000U));
        if(g_sd_odometer_save_pending == 0U) continue;

        taskENTER_CRITICAL();
        pending_m = g_sd_odometer_pending_m;
        taskEXIT_CRITICAL();
        if(sd_odometer_save(pending_m) != 0U) {
            taskENTER_CRITICAL();
            if(g_sd_odometer_pending_m == pending_m) {
                g_sd_odometer_save_pending = 0U;
            }
            taskEXIT_CRITICAL();
        }
        else {
            /* Missing or faulty cards are retried slowly without blocking GPS. */
            vTaskDelay(pdMS_TO_TICKS(9000U));
        }
    }
}

void SD_Odometer_StartTask(void)
{
    if(g_sd_odometer_task_handle != NULL) return;
    g_sd_odometer_task_handle =
        xTaskCreateStatic(SD_Odometer_Task, "ODO_Save", 768U, NULL,
                          tskIDLE_PRIORITY + 1U, g_sd_odometer_task_stack,
                          &g_sd_odometer_task_control);
}

void SD_Odometer_RequestSave(uint32_t total_m)
{
    taskENTER_CRITICAL();
    g_sd_odometer_pending_m = total_m;
    g_sd_odometer_save_pending = 1U;
    taskEXIT_CRITICAL();
}

static void led_red_on(void)  { HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET); }

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
        led_red_on();
        return;
    }

    fr = f_open(&fil, "0:/manba.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        led_red_on();
        f_mount(NULL, USERPath, 0);
        return;
    }

    f_write(&fil, g_test_text, sizeof(g_test_text) - 1, &bw);
    f_close(&fil);
    f_mount(NULL, USERPath, 0);

    /* success: green off -> on (visible change) */
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
    HAL_Delay(300);
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
}
