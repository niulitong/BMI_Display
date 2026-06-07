#include "main.h"
#include "gps.h"
#include "gps_lap.h"
#include "dashboard_ui.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef hdma_usart3_rx;

uint8_t g_gps_dma_buf[GPS_DMA_BUF_SIZE];

static TaskHandle_t g_gps_task_handle = NULL;
static GPS_Data_t  g_gps_data;
static uint32_t    g_gps_dma_last_pos;
static uint32_t    g_gps_sentence_count;
static uint32_t    g_gps_rmc_count;
static int32_t     g_gps_speed_kmh_int;
static char        g_gps_line_buf[160];
static uint16_t    g_gps_line_len;
static uint8_t     g_gps_line_active;

static int32_t     g_gsv_snr_sum;
static int32_t     g_gsv_snr_count;
static int32_t     g_gsv_max_snr;
static uint32_t    g_gsv_last_tick;

static uint8_t nmea_checksum(const char * sentence)
{
	uint8_t c = 0;
	const char * p = sentence;
	if(*p == '$') p++;
	while(*p != '*' && *p != '\0' && *p != '\r' && *p != '\n') {
		c ^= (uint8_t)*p;
		p++;
	}
	if(*p != '*') return 0;
	p++;
	uint8_t hi;
	uint8_t lo;
	char ch = *p++;
	if(ch >= '0' && ch <= '9') hi = (uint8_t)(ch - '0');
	else if(ch >= 'A' && ch <= 'F') hi = (uint8_t)(ch - 'A' + 10);
	else if(ch >= 'a' && ch <= 'f') hi = (uint8_t)(ch - 'a' + 10);
	else return 0;
	ch = *p;
	if(ch >= '0' && ch <= '9') lo = (uint8_t)(ch - '0');
	else if(ch >= 'A' && ch <= 'F') lo = (uint8_t)(ch - 'A' + 10);
	else if(ch >= 'a' && ch <= 'f') lo = (uint8_t)(ch - 'a' + 10);
	else return 0;
	return ((c == (uint8_t)((hi << 4) | lo)) ? 1U : 0U);
}

static const char * nmea_get_field(const char * buf, int32_t idx,
                                    char * out, uint32_t out_size)
{
	int32_t cur = 0;
	const char * p = buf;
	const char * start = NULL;
	uint32_t len = 0;

	while(*p != '\0' && *p != '*' && *p != '\r' && *p != '\n') {
		if(cur == idx) {
			start = p;
			while(*p != ',' && *p != '*' && *p != '\0' &&
			      *p != '\r' && *p != '\n') {
				len++;
				p++;
			}
			break;
		}
		if(*p == ',') cur++;
		p++;
	}

	if(start == NULL) {
		if(out != NULL && out_size > 0) out[0] = '\0';
		return NULL;
	}
	if(len == 0) {
		if(out != NULL && out_size > 0) out[0] = '\0';
		return start;
	}
	if(out != NULL && out_size > 0U) {
		if(len >= out_size) len = out_size - 1;
		for(uint32_t i = 0; i < len; i++) out[i] = start[i];
		out[len] = '\0';
	}
	return start;
}

static int32_t nmea_field_atoi(const char * buf, int32_t idx)
{
	char fld[16];
	const char * p = nmea_get_field(buf, idx, fld, sizeof(fld));
	if(p == NULL || fld[0] == '\0') return 0;
	int32_t val = 0;
	int32_t sign = 1;
	const char * s = fld;
	if(*s == '-') { sign = -1; s++; }
	while(*s >= '0' && *s <= '9') {
		val = val * 10 + (int32_t)(*s - '0');
		s++;
	}
	return val * sign;
}

static int32_t nmea_field_atoi_tenths(const char * buf, int32_t idx, int32_t * out_frac)
{
	char fld[32];
	const char * p = nmea_get_field(buf, idx, fld, sizeof(fld));
	if(out_frac == NULL) return 0;
	if(p == NULL || fld[0] == '\0') { *out_frac = 0; return 0; }

	int32_t int_part = 0;
	int32_t frac_part = 0;
	int32_t sign = 1;
	const char * s = fld;

	if(*s == '-') { sign = -1; s++; }
	else if(*s == '+') { s++; }

	while(*s >= '0' && *s <= '9') {
		int_part = int_part * 10 + (int32_t)(*s - '0');
		s++;
	}
	if(*s == '.') {
		s++;
		if(*s >= '0' && *s <= '9') {
			frac_part = (int32_t)(*s - '0');
			s++;
		}
	}
	*out_frac = sign * frac_part;
	return sign * int_part;
}

static int32_t nmea_parse_speed_kmh_int(const char * buf, int32_t idx)
{
	int32_t frac;
	int32_t int_part = nmea_field_atoi_tenths(buf, idx, &frac);

	while(frac < 0) frac = -frac;
	int32_t tenths = int_part * 10 + (frac % 10);
	if(int_part < 0) tenths = -tenths;

	int32_t kmh_tenths = (int32_t)(((int64_t)tenths * 1852 + 500) / 1000);
	return (kmh_tenths + 5) / 10;
}

static int32_t nmea_coord_to_fixed(const char * buf, int32_t lat_idx, int32_t dir_idx)
{
	char fld[32];
	nmea_get_field(buf, lat_idx, fld, sizeof(fld));
	if(fld[0] == '\0') return 0;

	int32_t int_part = 0;
	int32_t frac_part = 0;
	int32_t frac_digits = 0;
	const char * s = fld;

	while(*s >= '0' && *s <= '9') {
		int_part = int_part * 10 + (int32_t)(*s - '0');
		s++;
	}
	if(*s == '.') {
		s++;
		while(*s >= '0' && *s <= '9' && frac_digits < 6) {
			frac_part = frac_part * 10 + (int32_t)(*s - '0');
			frac_digits++;
			s++;
		}
	}
	while(frac_digits < 6) {
		frac_part *= 10;
		frac_digits++;
	}

	int32_t degrees = int_part / 100;
	int32_t minutes = int_part % 100;
	int32_t fixed = degrees * 1000000;
	fixed += (int32_t)((((int64_t)minutes * 1000000) + frac_part) / 60);

	char dir_str[4];
	nmea_get_field(buf, dir_idx, dir_str, sizeof(dir_str));
	if(dir_str[0] == 'S' || dir_str[0] == 'W') fixed = -fixed;

	return fixed;
}

static int32_t nmea_parse_track_int(const char * buf, int32_t idx)
{
	int32_t frac;
	int32_t int_part = nmea_field_atoi_tenths(buf, idx, &frac);
	if(frac < 0) frac = -frac;
	return int_part * 10 + (frac % 10);
}

static void assign_signal_level_snr(int8_t avg_snr, uint8_t * out_level)
{
	if(avg_snr >= GPS_SNR_EXCELLENT) {
		*out_level = GPS_SIG_LEVEL_EXCELLENT;
	} else if(avg_snr >= GPS_SNR_GOOD) {
		*out_level = GPS_SIG_LEVEL_GOOD;
	} else if(avg_snr >= GPS_SNR_FAIR) {
		*out_level = GPS_SIG_LEVEL_FAIR;
	} else if(avg_snr >= GPS_SNR_WEAK) {
		*out_level = GPS_SIG_LEVEL_WEAK;
	} else {
		*out_level = GPS_SIG_LEVEL_NONE;
	}
}

static int is_sentence_type(const char * sentence, const char * type)
{
	const char * p = sentence;
	while(*p != '\0') {
		if(p[0] == type[0] && p[1] == type[1] && p[2] == type[2]) return 1;
		p++;
	}
	return 0;
}

static void parse_GNGSV(const char * sentence)
{
	int32_t total_msgs = nmea_field_atoi(sentence, 1);
	int32_t msg_num   = nmea_field_atoi(sentence, 2);
	if(total_msgs <= 0 || msg_num <= 0) {
		g_gsv_snr_sum = 0;
		g_gsv_snr_count = 0;
		g_gsv_max_snr = 0;
		return;
	}
	if(msg_num == 1) {
		g_gsv_snr_sum = 0;
		g_gsv_snr_count = 0;
		g_gsv_max_snr = 0;
	}

	int32_t n_fields = 0;
	{
		const char * p = sentence;
		while(*p != '\0' && *p != '*' && *p != '\r' && *p != '\n') {
			if(*p == ',') n_fields++;
			p++;
		}
	}

	for(int32_t i = 4; i + 3 < n_fields; i += 4) {
		int32_t snr = nmea_field_atoi(sentence, i + 3);
		if(snr > 0 && snr <= 99) {
			g_gsv_snr_sum += snr;
			g_gsv_snr_count++;
			if(snr > g_gsv_max_snr) g_gsv_max_snr = snr;
		}
	}

	if(msg_num >= total_msgs) {
		uint8_t sig_level;
		int8_t avg = (g_gsv_snr_count > 0) ?
		              (int8_t)(g_gsv_snr_sum / g_gsv_snr_count) : (int8_t)0;
		assign_signal_level_snr(avg, &sig_level);

		taskENTER_CRITICAL();
		g_gps_data.avg_snr = avg;
		g_gps_data.max_snr = (int8_t)g_gsv_max_snr;
		g_gps_data.gsv_tracked_sats = (uint8_t)g_gsv_snr_count;
		g_gps_data.signal_level = sig_level;
		taskEXIT_CRITICAL();

		g_gsv_last_tick = HAL_GetTick();
		g_gsv_snr_sum = 0;
		g_gsv_snr_count = 0;
		g_gsv_max_snr = 0;

		Dashboard_UI_SubmitSignalLevel((int32_t)sig_level);
	}
}

static void parse_GNRMC(const char * sentence)
{
	char stat[4];
	nmea_get_field(sentence, 2, stat, sizeof(stat));
	if(stat[0] != 'A') return;

	int32_t speed_kmh = nmea_parse_speed_kmh_int(sentence, 7);
	if(speed_kmh < 0) speed_kmh = 0;
	if(speed_kmh > 300) speed_kmh = 300;

	taskENTER_CRITICAL();
	/* Keep the RMC fast path integer-only while isolating HardFault causes. */
	g_gps_speed_kmh_int = speed_kmh;
	g_gps_rmc_count++;
	taskEXIT_CRITICAL();

	/* Diagnostic: a valid RMC sentence toggles RED once. */
	HAL_GPIO_TogglePin(GPIOD, LED_RED_Pin);
	Dashboard_UI_SubmitSpeed(speed_kmh);
}

static void parse_GNGGA(const char * sentence)
{
	int32_t fix_qual = nmea_field_atoi(sentence, 6);
	int32_t sats     = nmea_field_atoi(sentence, 7);

	taskENTER_CRITICAL();
	g_gps_data.fix_quality = (uint8_t)fix_qual;
	g_gps_data.satellites  = (uint8_t)sats;
	g_gps_data.valid     = (fix_qual > 0) ? 1U : 0U;
	taskEXIT_CRITICAL();
}

static void invalidate_expired_gsv(void)
{
	if(g_gsv_last_tick == 0) return;
	uint32_t elapsed = HAL_GetTick() - g_gsv_last_tick;
	if(elapsed > 5000U) {
		taskENTER_CRITICAL();
		g_gps_data.avg_snr = 0;
		g_gps_data.max_snr = 0;
		g_gps_data.gsv_tracked_sats = 0;
		g_gps_data.signal_level = GPS_SIG_LEVEL_NONE;
		taskEXIT_CRITICAL();
		g_gsv_last_tick = 0;
		Dashboard_UI_SubmitSignalLevel(GPS_SIG_LEVEL_NONE);
	}
}

static void process_sentence(const char * buf)
{
	if(buf[0] != '$') return;
	if(!nmea_checksum(buf)) return;

	g_gps_sentence_count++;

	if(is_sentence_type(buf, "RMC")) {
		parse_GNRMC(buf);
	} else if(is_sentence_type(buf, "GGA")) {
		parse_GNGGA(buf);
	} else if(is_sentence_type(buf, "GSV")) {
		parse_GNGSV(buf);
	}
}

static void GPS_ProcessByte(uint8_t byte)
{
	char ch = (char)byte;

	if(ch == '$') {
		g_gps_line_active = 1U;
		g_gps_line_len = 0U;
	}

	if(g_gps_line_active == 0U) return;

	if(g_gps_line_len >= (uint16_t)(sizeof(g_gps_line_buf) - 1U)) {
		g_gps_line_active = 0U;
		g_gps_line_len = 0U;
		return;
	}

	g_gps_line_buf[g_gps_line_len++] = ch;

	if(ch == '\r' || ch == '\n') {
		g_gps_line_buf[g_gps_line_len] = '\0';
		process_sentence(g_gps_line_buf);
		g_gps_line_active = 0U;
		g_gps_line_len = 0U;
	}
}

static void GPS_SentenceProcess(void)
{
	uint32_t ndtr = __HAL_DMA_GET_COUNTER(&hdma_usart3_rx);
	if(ndtr > GPS_DMA_BUF_SIZE) {
		return;
	}

	uint32_t write_pos = (GPS_DMA_BUF_SIZE - ndtr) % GPS_DMA_BUF_SIZE;
	uint32_t pos = g_gps_dma_last_pos;
	uint32_t guard = 0U;

	while(pos != write_pos && guard < GPS_DMA_BUF_SIZE) {
		GPS_ProcessByte(g_gps_dma_buf[pos]);
		pos++;
		if(pos >= GPS_DMA_BUF_SIZE) pos = 0U;
		guard++;
	}

	g_gps_dma_last_pos = write_pos;
}

static void GPS_SendCmd(const char * cmd)
{
	char buf[128];
	int32_t len = 0;
	while(cmd[len] != '\0') len++;
	if(len >= 125) len = 124;
	for(int32_t i = 0; i < len; i++) buf[i] = cmd[i];
	buf[len] = '\r';
	buf[len + 1] = '\n';
	HAL_UART_Transmit(&huart3, (uint8_t *)buf, (uint16_t)(len + 2), 1000);
}

static void GPS_TaskFunc(void * argument)
{
	(void)argument;
	uint32_t parse_count = 0;
	uint32_t rmc_count   = 0;

	vTaskDelay(pdMS_TO_TICKS(1500));

	GPS_SendCmd("UNLOG COM2");
	vTaskDelay(pdMS_TO_TICKS(200));
	GPS_SendCmd("MODE HEADING2");
	vTaskDelay(pdMS_TO_TICKS(500));
	GPS_SendCmd("GPGGA 1");
	vTaskDelay(pdMS_TO_TICKS(100));
	GPS_SendCmd("GPRMC 1");
	vTaskDelay(pdMS_TO_TICKS(100));
	GPS_SendCmd("GPGSV 1");
	vTaskDelay(pdMS_TO_TICKS(200));
	GPS_SendCmd("SAVECONFIG");
	vTaskDelay(pdMS_TO_TICKS(500));

	for(;;) {
		uint32_t notify_val = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(500));
		uint8_t got_data = (notify_val > 0U) ? 1U : 0U;

		GPS_SentenceProcess();
		invalidate_expired_gsv();

		{
			uint32_t new_total = g_gps_sentence_count;
			if(new_total > parse_count) {
				parse_count = new_total;
				rmc_count = parse_count;
			}
		}

		(void)got_data;
	}
}

void GPS_Init(void)
{
	uint32_t ndtr = __HAL_DMA_GET_COUNTER(&hdma_usart3_rx);
	if(ndtr <= GPS_DMA_BUF_SIZE) {
		g_gps_dma_last_pos = (GPS_DMA_BUF_SIZE - ndtr) % GPS_DMA_BUF_SIZE;
	} else {
		g_gps_dma_last_pos = 0U;
	}
	g_gps_sentence_count = 0;
	g_gps_rmc_count = 0;
	g_gps_speed_kmh_int = 0;
	g_gps_line_len = 0U;
	g_gps_line_active = 0U;
	g_gsv_last_tick = 0;

	BaseType_t ret = xTaskCreate(GPS_TaskFunc, "GPS_Task",
	                             3072U, NULL,
	                             tskIDLE_PRIORITY + 2,
	                             &g_gps_task_handle);
	(void)ret;
}

void GPS_ISR_Notify(void)
{
	if(!(__HAL_UART_GET_FLAG(&huart3, UART_FLAG_IDLE))) return;
	__HAL_UART_CLEAR_IDLEFLAG(&huart3);

	if(g_gps_task_handle != NULL && xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		vTaskNotifyGiveFromISR(g_gps_task_handle, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

void GPS_GetData(GPS_Data_t * out)
{
	if(out == NULL) return;
	taskENTER_CRITICAL();
	*out = g_gps_data;
	taskEXIT_CRITICAL();
}
