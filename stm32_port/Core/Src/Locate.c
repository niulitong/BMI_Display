#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "Locate.h"
#include "can.h"
#include "dashboard_ui.h"

extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef hdma_usart3_rx;

uint8_t g_gps_dma_buf[GPS_DMA_BUF_SIZE];

static TaskHandle_t     g_gps_task_handle = NULL;
static uint32_t         g_gps_dma_last_ndtr;
static GNSS_Data_t      gnss_data = {0};

static int32_t  g_gsv_snr_sum;
static int32_t  g_gsv_snr_count;
static int32_t  g_gsv_max_snr;
static uint32_t g_gsv_last_tick;

static float parse_latitude(const char* lat_str, char dir) {
    float lat = (float)atof(lat_str);
    int degrees = (int)(lat / 100);
    float minutes = lat - (float)(degrees * 100);
    float decimal_lat = (float)degrees + minutes / 60.0f;

    if (dir == 'S') decimal_lat = -decimal_lat;
    return decimal_lat;
}

static float parse_longitude(const char* lon_str, char dir) {
    float lon = (float)atof(lon_str);
    int degrees = (int)(lon / 100);
    float minutes = lon - (float)(degrees * 100);
    float decimal_lon = (float)degrees + minutes / 60.0f;

    if (dir == 'W') decimal_lon = -decimal_lon;
    return decimal_lon;
}

static void parse_utc_time(const char* time_str, GNSS_Data_t* data) {
    if (strlen(time_str) >= 6) {
        char hour_str[3] = {0}, min_str[3] = {0}, sec_str[3] = {0};
        char millisec_str[4] = {0};

        strncpy(hour_str, time_str, 2);
        strncpy(min_str, time_str + 2, 2);
        strncpy(sec_str, time_str + 4, 2);

        data->utc_hour = (uint8_t)atoi(hour_str);
        data->utc_min  = (uint8_t)atoi(min_str);
        data->utc_sec  = (uint8_t)atoi(sec_str);

        if (strlen(time_str) > 7) {
            strncpy(millisec_str, time_str + 7, 3);
            data->utc_millisec = (uint16_t)atoi(millisec_str);
        }
    }
}

static uint8_t nmea_checksum(const char* sentence) {
    const char* p = sentence;

    if (*p == '$') p++;

    uint8_t checksum = 0;
    while (*p != '*' && *p != '\0' && *p != '\r' && *p != '\n') {
        checksum ^= (uint8_t)*p;
        p++;
    }

    if (*p == '*') {
        p++;
        char hex_str[3] = {0};
        hex_str[0] = *p++;
        hex_str[1] = *p;
        uint8_t msg_checksum = (uint8_t)strtol(hex_str, NULL, 16);

        return (checksum == msg_checksum) ? 1U : 0U;
    }

    return 0U;
}

static void update_signal_level(void) {
    if (gnss_data.avg_snr >= GNSS_SNR_EXCELLENT) {
        gnss_data.signal_level = GNSS_SIG_LEVEL_EXCELLENT;
    } else if (gnss_data.avg_snr >= GNSS_SNR_GOOD) {
        gnss_data.signal_level = GNSS_SIG_LEVEL_GOOD;
    } else if (gnss_data.avg_snr >= GNSS_SNR_FAIR) {
        gnss_data.signal_level = GNSS_SIG_LEVEL_FAIR;
    } else if (gnss_data.avg_snr >= GNSS_SNR_WEAK) {
        gnss_data.signal_level = GNSS_SIG_LEVEL_WEAK;
    } else {
        gnss_data.signal_level = GNSS_SIG_LEVEL_NONE;
    }
    Dashboard_UI_SubmitSignalLevel((int32_t)gnss_data.signal_level);
}

static void parse_GNGGA(const char* sentence) {
    char buffer[256];
    strncpy(buffer, sentence, sizeof(buffer) - 1);

    int field_index = 0;
    char* token = strtok(buffer, ",");

    taskENTER_CRITICAL();
    while (token != NULL && field_index <= 12) {
        switch(field_index) {
            case 1:
                parse_utc_time(token, &gnss_data);
                break;
            case 2:
                if (strlen(token) > 0) {
                    char* dir_token = strtok(NULL, ",");
                    if (dir_token != NULL) {
                        gnss_data.latitude = parse_latitude(token, dir_token[0]);
                    }
                }
                break;
            case 3:
                break;
            case 4:
                if (strlen(token) > 0) {
                    char* dir_token = strtok(NULL, ",");
                    if (dir_token != NULL) {
                        gnss_data.longitude = parse_longitude(token, dir_token[0]);
                    }
                }
                break;
            case 5:
                break;
            case 6:
                gnss_data.fix_quality = (uint8_t)atoi(token);
                break;
            case 7:
                gnss_data.sat_num = (uint8_t)atoi(token);
                break;
            case 8:
                gnss_data.hdop = (float)atof(token);
                break;
            case 9:
                gnss_data.altitude = (float)atof(token);
                break;
            default:
                break;
        }
        field_index++;
        token = strtok(NULL, ",");
    }

    gnss_data.gga_valid = (uint8_t)(gnss_data.fix_quality > 0);
    taskEXIT_CRITICAL();
}

static void parse_GNRMC(const char* sentence) {
    char buffer[256];
    strncpy(buffer, sentence, sizeof(buffer) - 1);

    int field_index = 0;
    char* token = strtok(buffer, ",");

    taskENTER_CRITICAL();
    while (token != NULL && field_index <= 12) {
        switch(field_index) {
            case 1:
                parse_utc_time(token, &gnss_data);
                break;
            case 2:
                gnss_data.rmc_valid = (uint8_t)(token[0] == 'A');
                break;
            case 3:
                if (strlen(token) > 0 && gnss_data.rmc_valid) {
                    char* dir_token = strtok(NULL, ",");
                    if (dir_token != NULL && dir_token[0] != '\0') {
                        gnss_data.latitude = parse_latitude(token, dir_token[0]);
                    }
                }
                break;
            case 4:
                break;
            case 5:
                if (strlen(token) > 0 && gnss_data.rmc_valid) {
                    char* dir_token = strtok(NULL, ",");
                    if (dir_token != NULL && dir_token[0] != '\0') {
                        gnss_data.longitude = parse_longitude(token, dir_token[0]);
                    }
                }
                break;
            case 6:
                break;
            case 7:
                gnss_data.speed_knot = (float)atof(token);
                gnss_data.speed_kmh = gnss_data.speed_knot * 1.852f;
                if(gnss_data.rmc_valid) {
                    CAN_SendGPSSpeed((int32_t)gnss_data.speed_kmh);
                }
                break;
            case 8:
                gnss_data.track_angle = (float)atof(token);
                break;
            default:
                break;
        }
        field_index++;
        token = strtok(NULL, ",");
    }
    taskEXIT_CRITICAL();
}

static void parse_GNGSV(const char* sentence) {
    char buffer[256];
    strncpy(buffer, sentence, sizeof(buffer) - 1);

    int field_index = 0;
    char* token = strtok(buffer, ",");

    while (token != NULL && field_index <= 20) {
        switch(field_index) {
            case 1:
                break;
            case 2:
                break;
            case 3:
                break;
            default:
                if (field_index >= 4) {
                    int sat_idx = (field_index - 4) % 4;
                    if (sat_idx == 3) {
                        int snr = atoi(token);
                        if (snr > 0 && snr <= 99) {
                            g_gsv_snr_sum += snr;
                            g_gsv_snr_count++;
                            if (snr > g_gsv_max_snr) g_gsv_max_snr = snr;
                        }
                    }
                }
                break;
        }
        field_index++;
        token = strtok(NULL, ",");
    }

    taskENTER_CRITICAL();
    if (g_gsv_snr_count > 0) {
        gnss_data.gsv_valid = 1;
        gnss_data.tracked_sats = (uint8_t)g_gsv_snr_count;
        gnss_data.max_snr = (int8_t)g_gsv_max_snr;
        gnss_data.avg_snr = (int8_t)(g_gsv_snr_sum / g_gsv_snr_count);
        g_gsv_last_tick = HAL_GetTick();
        update_signal_level();
    }
    taskEXIT_CRITICAL();
}

static void invalidate_expired_gsv(void) {
    if (g_gsv_snr_count > 0) {
        uint32_t elapsed = HAL_GetTick() - g_gsv_last_tick;
        if (elapsed > 3000U) {
            g_gsv_snr_count = 0;
            g_gsv_snr_sum = 0;
            g_gsv_max_snr = 0;
            taskENTER_CRITICAL();
            gnss_data.gsv_valid = 0;
            gnss_data.tracked_sats = 0;
            gnss_data.max_snr = 0;
            gnss_data.avg_snr = 0;
            gnss_data.signal_level = GNSS_SIG_LEVEL_NONE;
            taskEXIT_CRITICAL();
            Dashboard_UI_SubmitSignalLevel((int32_t)GNSS_SIG_LEVEL_NONE);
        }
    }
}

static void parse_gnss_sentence(const char* sentence, uint16_t len) {
    if (len < 6) return;

    if (!nmea_checksum(sentence)) {
        return;
    }

    if (strstr(sentence, "GGA") != NULL) {
        parse_GNGGA(sentence);
    }
    else if (strstr(sentence, "RMC") != NULL) {
        parse_GNRMC(sentence);
    }
    else if (strstr(sentence, "GSV") != NULL) {
        parse_GNGSV(sentence);
    }
}

void parse_BESTNAVA(const char* message) {
    char* p = strchr(message, ';');
    if (p == NULL) return;
    p++;

    char buffer[256];
    strncpy(buffer, p, sizeof(buffer) - 1);

    int field_index = 0;
    char* token = strtok(buffer, ",");

    taskENTER_CRITICAL();
    while (token != NULL && field_index <= 8) {
        switch(field_index) {
            case 1:
                if (strcmp(token, "NARROW_INT") == 0) {
                    gnss_data.fix_quality = 4;
                } else if (strcmp(token, "NARROW_FLOAT") == 0) {
                    gnss_data.fix_quality = 5;
                }
                break;
            case 2:
                gnss_data.latitude = (float)atof(token);
                break;
            case 3:
                gnss_data.longitude = (float)atof(token);
                break;
            case 4:
                gnss_data.altitude = (float)atof(token);
                break;
        }
        field_index++;
        token = strtok(NULL, ",");
    }
    taskEXIT_CRITICAL();
}

static void extract_sentence(uint32_t sentence_start, uint32_t sentence_len) {
    char sentence_buf[256];
    uint32_t j;

    sentence_len++;
    if (sentence_len >= sizeof(sentence_buf)) {
        sentence_len = sizeof(sentence_buf) - 1U;
    }
    for (j = 0U; j < sentence_len; j++) {
        sentence_buf[j] = (char)g_gps_dma_buf[(sentence_start + j) % GPS_DMA_BUF_SIZE];
    }
    sentence_buf[sentence_len] = '\0';

    if (sentence_buf[0] == '$') {
        parse_gnss_sentence(sentence_buf, (uint16_t)sentence_len);
    } else if (sentence_buf[0] == '#') {
        parse_BESTNAVA(sentence_buf);
    }
}

static void GPS_SentenceProcess(void) {
    uint32_t ndtr;
    uint32_t total_bytes;
    uint32_t i;
    uint32_t sentence_start;
    uint32_t sentence_len;

    ndtr = __HAL_DMA_GET_COUNTER(&hdma_usart3_rx);
    total_bytes = GPS_DMA_BUF_SIZE - ndtr;
    if (total_bytes >= GPS_DMA_BUF_SIZE) {
        g_gps_dma_last_ndtr = GPS_DMA_BUF_SIZE;
        return;
    }
    if (total_bytes == 0U) {
        return;
    }

    if (total_bytes <= g_gps_dma_last_ndtr) {
        g_gps_dma_last_ndtr = total_bytes;
        return;
    }

    for (i = g_gps_dma_last_ndtr; i < total_bytes; i++) {
        uint8_t ch = g_gps_dma_buf[i % GPS_DMA_BUF_SIZE];
        if (ch == '$' || ch == '#') {
            sentence_start = i % GPS_DMA_BUF_SIZE;
            sentence_len = 0U;

            while ((i + sentence_len) < total_bytes &&
                   sentence_len < 128U &&
                   g_gps_dma_buf[(sentence_start + sentence_len) % GPS_DMA_BUF_SIZE] != '\n') {
                sentence_len++;
            }

            if (sentence_len > 0U && sentence_len < 128U &&
                g_gps_dma_buf[(sentence_start + sentence_len) % GPS_DMA_BUF_SIZE] == '\n') {
                extract_sentence(sentence_start, sentence_len);
            }
        }
    }

    g_gps_dma_last_ndtr = total_bytes;
    invalidate_expired_gsv();
}

static void GPS_TaskFunc(void *argument) {
    (void)argument;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        GPS_SentenceProcess();
    }
}

static void GNSS_SendCmd(const char* cmd) {
    char buf[128];
    int len = snprintf(buf, sizeof(buf), "%s\r\n", cmd);
    if (len > 0 && len < (int)sizeof(buf)) {
        HAL_UART_Transmit(&huart3, (uint8_t*)buf, (uint16_t)len, 1000);
    }
}

static void GNSS_WaitReply(uint32_t timeout_ms) {
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < timeout_ms) {
        uint32_t ndtr = __HAL_DMA_GET_COUNTER(&hdma_usart3_rx);
        uint32_t total = GPS_DMA_BUF_SIZE - ndtr;
        if (total >= GPS_DMA_BUF_SIZE) total = 0;

        if (total > 16) {
            uint32_t i;
            for (i = total - 16; i < total; i++) {
                char ch = (char)g_gps_dma_buf[i % GPS_DMA_BUF_SIZE];
                if (ch == 'O') {
                    if (g_gps_dma_buf[(i + 1) % GPS_DMA_BUF_SIZE] == 'K') {
                        return;
                    }
                }
                if (ch == 'E') {
                    if (g_gps_dma_buf[(i + 1) % GPS_DMA_BUF_SIZE] == 'R') {
                        return;
                    }
                }
            }
        }
    }
}

void GNSS_Init(void) {
    g_gps_dma_last_ndtr = GPS_DMA_BUF_SIZE;

    HAL_Delay(500);
    GNSS_SendCmd("GPTHS 1");
    GNSS_WaitReply(300);
    GNSS_SendCmd("SAVECONFIG");
    GNSS_WaitReply(500);

    BaseType_t ret = xTaskCreate(
        GPS_TaskFunc,
        "GPS_Task",
        configMINIMAL_STACK_SIZE + 512,
        NULL,
        tskIDLE_PRIORITY + 3,
        &g_gps_task_handle
    );

    (void)ret;
}

void GNSS_ISR_Notify(void) {
    if (!(__HAL_UART_GET_FLAG(&huart3, UART_FLAG_IDLE))) {
        return;
    }
    __HAL_UART_CLEAR_IDLEFLAG(&huart3);

    if (g_gps_task_handle != NULL) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(g_gps_task_handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void get_gnss_data(GNSS_Data_t* data) {
    taskENTER_CRITICAL();
    memcpy(data, &gnss_data, sizeof(GNSS_Data_t));
    taskEXIT_CRITICAL();
}

void print_gnss_data(void) {
    GNSS_Data_t snapshot;

    taskENTER_CRITICAL();
    memcpy(&snapshot, &gnss_data, sizeof(GNSS_Data_t));
    taskEXIT_CRITICAL();

    printf("\r\n========== GNSS Data ==========\r\n");
    printf("Time: %02d:%02d:%02d.%03d\r\n",
        snapshot.utc_hour, snapshot.utc_min, snapshot.utc_sec, snapshot.utc_millisec);
    printf("Position: %.8f, %.8f\r\n", snapshot.latitude, snapshot.longitude);
    printf("Altitude: %.2f m\r\n", snapshot.altitude);
    printf("Fix Quality: %d ", snapshot.fix_quality);
    switch(snapshot.fix_quality) {
        case 1: printf("(Single Point)\r\n"); break;
        case 2: printf("(DGPS)\r\n"); break;
        case 4: printf("(RTK Fixed)\r\n"); break;
        case 5: printf("(RTK Float)\r\n"); break;
        default: printf("(Invalid)\r\n"); break;
    }
    printf("Satellites: %d\r\n", snapshot.sat_num);
    printf("HDOP: %.2f\r\n", snapshot.hdop);
    printf("Speed: %.2f km/h (%.2f knots)\r\n", snapshot.speed_kmh, snapshot.speed_knot);
    printf("Heading: %.1f deg\r\n", snapshot.track_angle);
    printf("Signal: Lv%d ", snapshot.signal_level);
    switch(snapshot.signal_level) {
        case GNSS_SIG_LEVEL_EXCELLENT: printf("(Excellent)\r\n"); break;
        case GNSS_SIG_LEVEL_GOOD: printf("(Good)\r\n"); break;
        case GNSS_SIG_LEVEL_FAIR: printf("(Fair)\r\n"); break;
        case GNSS_SIG_LEVEL_WEAK: printf("(Weak)\r\n"); break;
        default: printf("(None)\r\n"); break;
    }
    printf("SNR: max=%d avg=%d dB-Hz, tracked=%d\r\n",
        snapshot.max_snr, snapshot.avg_snr, snapshot.tracked_sats);
    printf("================================\r\n");
}

void send_gnss_data_uart3(const GNSS_Data_t* data) {
    char line_buf[128];
    uint16_t len;

    if (data == NULL) {
        return;
    }

    len = (uint16_t)snprintf(line_buf, sizeof(line_buf),
        "UTC Time: %02d:%02d:%02d.%03d\r\n",
        data->utc_hour, data->utc_min, data->utc_sec, data->utc_millisec);
    HAL_UART_Transmit(&huart3, (uint8_t*)line_buf, len, 100U);

    len = (uint16_t)snprintf(line_buf, sizeof(line_buf),
        "Latitude: %.8f\r\n", data->latitude);
    HAL_UART_Transmit(&huart3, (uint8_t*)line_buf, len, 100U);

    len = (uint16_t)snprintf(line_buf, sizeof(line_buf),
        "Longitude: %.8f\r\n", data->longitude);
    HAL_UART_Transmit(&huart3, (uint8_t*)line_buf, len, 100U);

    len = (uint16_t)snprintf(line_buf, sizeof(line_buf),
        "Speed: %.2f km/h\r\n", data->speed_kmh);
    HAL_UART_Transmit(&huart3, (uint8_t*)line_buf, len, 100U);

    len = (uint16_t)snprintf(line_buf, sizeof(line_buf),
        "Heading: %.1f deg\r\n", data->track_angle);
    HAL_UART_Transmit(&huart3, (uint8_t*)line_buf, len, 100U);

    len = (uint16_t)snprintf(line_buf, sizeof(line_buf),
        "--------------------------------\r\n");
    HAL_UART_Transmit(&huart3, (uint8_t*)line_buf, len, 100U);
}
