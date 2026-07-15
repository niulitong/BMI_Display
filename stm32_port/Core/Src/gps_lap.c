#include "gps_lap.h"
#include "main.h"
#include "gps.h"
#include "dashboard_ui.h"
#include <string.h>

#if defined(__GNUC__)
#pragma GCC push_options
#pragma GCC optimize ("Os")
#endif

#if GPS_LAP_ENABLE

#define GPS_LAP_EARTH_RADIUS_M       6371000.0f
#define GPS_LAP_LINE_HALF_WIDTH_M    6.0f
#define GPS_LAP_LINE_ARM_DISTANCE_M  10.0f
#define GPS_LAP_TRACK_SAMPLE_MAX     256U
#define GPS_LAP_SAMPLE_MIN_DIST_M    1.0f
#define GPS_LAP_SAMPLE_MAX_PERIOD_MS 500U
#define GPS_LAP_UI_UPDATE_PERIOD_MS  200U
#define GPS_LAP_MAX_LINE_DISTANCE_M  20000.0f
#define GPS_LAP_SAMPLE_MIN_DM        (-32768.0f)
#define GPS_LAP_SAMPLE_MAX_DM        32767.0f
#define GPS_LAP_ENABLE_GEOMETRY      0U
#define GPS_LAP_ENABLE_LIVE_DELTA    0U

typedef struct {
	int16_t x_dm;
	int16_t y_dm;
	uint16_t time_cs;
} GPS_LapSample_t;

typedef struct {
	float lat;
	float lon;
	float track;
	float forward_e;
	float forward_n;
	float lateral_e;
	float lateral_n;
} GPS_LapLine_t;

typedef struct {
	float forward_m;
	float lateral_m;
} GPS_LapLinePos_t;

static GPS_LapLine_t g_start_line;
static GPS_LapLine_t g_finish_line;
static uint8_t       g_start_set;
static uint8_t       g_finish_set;

static int32_t  g_lap_current_ms;
static int32_t  g_lap_last_ms;
static int32_t  g_lap_best_ms;
static int32_t  g_lap_delta_ms;
static int32_t  g_lap_count;
static uint32_t g_lap_start_tick;
static uint32_t g_lap_prev_tick;
static uint8_t  g_lap_active;
static uint8_t  g_lap_analysis_enabled;
static uint8_t  g_lap_finish_armed;
static uint8_t  g_lap_prev_valid;
static GPS_LapLinePos_t g_lap_prev_pos;
static GPS_LapSample_t g_lap_current_samples[GPS_LAP_TRACK_SAMPLE_MAX];
static GPS_LapSample_t g_lap_ref_samples[GPS_LAP_TRACK_SAMPLE_MAX];
static uint16_t g_lap_current_sample_count;
static uint16_t g_lap_ref_sample_count;
static uint8_t  g_lap_ref_valid;
static GPS_LapLinePos_t g_lap_last_sample_pos;
static uint32_t g_lap_last_sample_tick;
static uint32_t g_lap_ui_last_tick;

static float deg_to_radf32(float deg)
{
	return deg * 0.01745329251994329577f;
}

static float absf32(float v)
{
	return v < 0.0f ? -v : v;
}

static uint8_t finite_f32(float v)
{
	return (uint8_t)((v == v) && (v <= 3.4e38f) && (v >= -3.4e38f));
}

static uint8_t coord_valid(float lat, float lon)
{
	if((finite_f32(lat) == 0U) || (finite_f32(lon) == 0U)) return 0U;
	if((lat < -90.0f) || (lat > 90.0f)) return 0U;
	if((lon < -180.0f) || (lon > 180.0f)) return 0U;
	if((absf32(lat) < 0.000001f) && (absf32(lon) < 0.000001f)) return 0U;
	return 1U;
}

static uint8_t line_pos_valid(const GPS_LapLinePos_t * pos)
{
	if(pos == NULL) return 0U;
	if((finite_f32(pos->forward_m) == 0U) || (finite_f32(pos->lateral_m) == 0U)) return 0U;
	if(absf32(pos->forward_m) > GPS_LAP_MAX_LINE_DISTANCE_M) return 0U;
	if(absf32(pos->lateral_m) > GPS_LAP_MAX_LINE_DISTANCE_M) return 0U;
	return 1U;
}

static float wrap_pi(float rad)
{
	if(rad > 6.28318531f || rad < -6.28318531f) {
		int32_t turns = (int32_t)(rad / 6.28318531f);
		rad -= (float)turns * 6.28318531f;
	}
	if(rad > 3.14159265f) rad -= 6.28318531f;
	if(rad < -3.14159265f) rad += 6.28318531f;
	return rad;
}

static void submit_lap_times_limited(uint32_t now, uint8_t force)
{
	if((force == 0U) && ((now - g_lap_ui_last_tick) < GPS_LAP_UI_UPDATE_PERIOD_MS)) return;
	g_lap_ui_last_tick = now;
	Dashboard_UI_SubmitLapTimes(g_lap_current_ms / 10,
								g_lap_last_ms / 10,
								g_lap_best_ms / 10,
								g_lap_count);
}

static float sin_approx(float rad)
{
	float x = wrap_pi(rad);
	float x2 = x * x;
	return x * (1.0f - (x2 * 0.16666667f) + (x2 * x2 * 0.00833333f));
}

static float cos_approx(float rad)
{
	return sin_approx(rad + 1.57079633f);
}

static void update_line_vectors(GPS_LapLine_t * line)
{
	float heading;
	if((finite_f32(line->track) == 0U) || (line->track < 0.0f) || (line->track >= 360.0f)) {
		line->track = 0.0f;
	}
	heading = deg_to_radf32((float)line->track);
	line->forward_e = sin_approx(heading);
	line->forward_n = cos_approx(heading);
	line->lateral_e = line->forward_n;
	line->lateral_n = -line->forward_e;
}

static GPS_LapLinePos_t project_to_line(const GPS_LapLine_t * line,
										float lat, float lon)
{
	GPS_LapLinePos_t pos;
	float north_m = deg_to_radf32(lat - line->lat) * GPS_LAP_EARTH_RADIUS_M;
	float east_m = deg_to_radf32(lon - line->lon) * GPS_LAP_EARTH_RADIUS_M *
					  cos_approx(deg_to_radf32(line->lat));

	pos.forward_m = east_m * line->forward_e + north_m * line->forward_n;
	pos.lateral_m = east_m * line->lateral_e + north_m * line->lateral_n;
	return pos;
}

#if GPS_LAP_ENABLE_GEOMETRY
static uint8_t crossed_finish_line(const GPS_LapLinePos_t * prev,
									  const GPS_LapLinePos_t * curr,
									  float * ratio_out)
{
	float denom;
	float ratio;
	float lateral;

	if((prev->forward_m >= 0.0f) || (curr->forward_m < 0.0f)) return 0U;
	denom = prev->forward_m - curr->forward_m;
	if(denom <= 0.001f) return 0U;

	ratio = prev->forward_m / denom;
	if((ratio < 0.0f) || (ratio > 1.0f)) return 0U;
	lateral = prev->lateral_m + ((curr->lateral_m - prev->lateral_m) * ratio);
	if(absf32(lateral) > GPS_LAP_LINE_HALF_WIDTH_M) return 0U;

	*ratio_out = ratio;
	return 1U;
}
#endif

static int16_t clamp_dm_f32(float meter)
{
	float dm = meter * 10.0f;
	if(finite_f32(dm) == 0U) return 0;
	if(dm > GPS_LAP_SAMPLE_MAX_DM) return 32767;
	if(dm < GPS_LAP_SAMPLE_MIN_DM) return -32768;
	return (int16_t)(dm + (dm >= 0.0f ? 0.5f : -0.5f));
}

static uint16_t clamp_time_cs(int32_t ms)
{
	int32_t cs = ms / 10;
	if(cs < 0) return 0U;
	if(cs > 65535) return 65535U;
	return (uint16_t)cs;
}

static GPS_LapSample_t make_sample(const GPS_LapLinePos_t * pos, int32_t time_ms)
{
	GPS_LapSample_t sample;
	sample.x_dm = clamp_dm_f32(pos->forward_m);
	sample.y_dm = clamp_dm_f32(pos->lateral_m);
	sample.time_cs = clamp_time_cs(time_ms);
	return sample;
}

static void clear_current_samples(void)
{
	g_lap_current_sample_count = 0U;
	g_lap_last_sample_tick = 0U;
}

static void add_current_sample(const GPS_LapLinePos_t * pos,
							   uint32_t tick,
							   int32_t time_ms,
							   uint8_t force)
{
	float dx;
	float dy;

	if(g_lap_current_sample_count >= GPS_LAP_TRACK_SAMPLE_MAX) return;
	if((force == 0U) && (g_lap_current_sample_count > 0U)) {
		dx = pos->forward_m - g_lap_last_sample_pos.forward_m;
		dy = pos->lateral_m - g_lap_last_sample_pos.lateral_m;
		if(((dx * dx + dy * dy) < (GPS_LAP_SAMPLE_MIN_DIST_M * GPS_LAP_SAMPLE_MIN_DIST_M)) &&
		   ((tick - g_lap_last_sample_tick) < GPS_LAP_SAMPLE_MAX_PERIOD_MS)) {
			return;
		}
	}

	g_lap_current_samples[g_lap_current_sample_count] = make_sample(pos, time_ms);
	g_lap_current_sample_count++;
	g_lap_last_sample_pos = *pos;
	g_lap_last_sample_tick = tick;
}

#if GPS_LAP_ENABLE_GEOMETRY
static void archive_current_samples(void)
{
	if(g_lap_current_sample_count < 2U) {
		g_lap_ref_sample_count = 0U;
		g_lap_ref_valid = 0U;
		return;
	}
	memcpy(g_lap_ref_samples,
	       g_lap_current_samples,
	       (size_t)g_lap_current_sample_count * sizeof(g_lap_current_samples[0]));
	g_lap_ref_sample_count = g_lap_current_sample_count;
	g_lap_ref_valid = 1U;
}
#endif

#if GPS_LAP_ENABLE_LIVE_DELTA
static uint8_t interpolate_ref_time(const GPS_LapLinePos_t * pos, int32_t * time_ms_out)
{
	float px = pos->forward_m * 10.0f;
	float py = pos->lateral_m * 10.0f;
	float best_dist2 = 3.4e38f;
	float best_time_cs = 0.0f;

	if((g_lap_ref_valid == 0U) || (g_lap_ref_sample_count < 2U) || (time_ms_out == NULL)) {
		return 0U;
	}

	for(uint16_t i = 1U; i < g_lap_ref_sample_count; i++) {
		float ax = (float)g_lap_ref_samples[i - 1U].x_dm;
		float ay = (float)g_lap_ref_samples[i - 1U].y_dm;
		float bx = (float)g_lap_ref_samples[i].x_dm;
		float by = (float)g_lap_ref_samples[i].y_dm;
		float vx = bx - ax;
		float vy = by - ay;
		float seg_len2 = (vx * vx) + (vy * vy);
		float t = 0.0f;
		float qx;
		float qy;
		float dx;
		float dy;
		float dist2;
		if(seg_len2 > 0.001f) {
			t = (((px - ax) * vx) + ((py - ay) * vy)) / seg_len2;
			if(t < 0.0f) t = 0.0f;
			if(t > 1.0f) t = 1.0f;
		}
		qx = ax + (vx * t);
		qy = ay + (vy * t);
		dx = px - qx;
		dy = py - qy;
		dist2 = (dx * dx) + (dy * dy);
		if(dist2 < best_dist2) {
			best_dist2 = dist2;
			best_time_cs = (float)g_lap_ref_samples[i - 1U].time_cs +
						   (((float)g_lap_ref_samples[i].time_cs -
							 (float)g_lap_ref_samples[i - 1U].time_cs) * t);
		}
	}

	*time_ms_out = (int32_t)((best_time_cs * 10.0f) + 0.5f);
	return 1U;
}

static void update_live_delta(const GPS_LapLinePos_t * pos, int32_t current_ms)
{
	int32_t ref_ms;
	if(interpolate_ref_time(pos, &ref_ms) == 0U) return;
	g_lap_delta_ms = current_ms - ref_ms;
	Dashboard_UI_SubmitLapDelta(g_lap_delta_ms / 10);
}
#endif

void GPS_Lap_SetStartLine(float lat, float lon, float track)
{
	if(coord_valid(lat, lon) == 0U) {
		g_start_set = 0U;
		return;
	}
	g_start_line.lat = lat;
	g_start_line.lon = lon;
	g_start_line.track = track;
	update_line_vectors(&g_start_line);
	g_start_set = 1U;
}

void GPS_Lap_SetFinishLine(float lat, float lon, float track)
{
	if(coord_valid(lat, lon) == 0U) {
		g_finish_set = 0U;
		return;
	}
	g_finish_line.lat = lat;
	g_finish_line.lon = lon;
	g_finish_line.track = track;
	update_line_vectors(&g_finish_line);
	g_finish_set = 1U;
}

uint8_t GPS_Lap_StartAtCurrent(const GPS_Data_t * data)
{
	if((data == NULL) || (data->valid == 0U) || !g_finish_set) return 0U;
	if(coord_valid(data->latitude, data->longitude) == 0U) return 0U;
	g_lap_active = 1U;
	g_lap_start_tick = HAL_GetTick();
	g_lap_prev_tick = g_lap_start_tick;
	g_lap_current_ms = 0;
	g_lap_finish_armed = 0U;
	g_lap_prev_pos = project_to_line(&g_finish_line, data->latitude, data->longitude);
	if(line_pos_valid(&g_lap_prev_pos) == 0U) {
		g_lap_active = 0U;
		g_lap_prev_valid = 0U;
		return 0U;
	}
	g_lap_prev_valid = 1U;
	clear_current_samples();
	add_current_sample(&g_lap_prev_pos, g_lap_start_tick, 0, 1U);
	submit_lap_times_limited(g_lap_start_tick, 1U);
	return 1U;
}

void GPS_Lap_Reset(void)
{
	g_lap_current_ms = 0;
	g_lap_last_ms = 0;
	g_lap_best_ms = 0;
	g_lap_delta_ms = 0;
	g_lap_count = 0;
	g_lap_active = 0U;
	g_lap_start_tick = 0;
	g_lap_prev_tick = 0;
	g_lap_finish_armed = 0U;
	g_lap_prev_valid = 0U;
	clear_current_samples();
	g_lap_ref_sample_count = 0U;
	g_lap_ref_valid = 0U;
	g_lap_ui_last_tick = 0U;
}

void GPS_Lap_SetAnalysisActive(uint8_t active)
{
	g_lap_analysis_enabled = active != 0U ? 1U : 0U;
	if(g_lap_analysis_enabled == 0U) {
		g_lap_active = 0U;
		g_lap_start_tick = 0U;
		g_lap_prev_tick = 0U;
		g_lap_finish_armed = 0U;
		g_lap_prev_valid = 0U;
		clear_current_samples();
	}
}

uint8_t GPS_Lap_IsAnalysisActive(void)
{
	return g_lap_analysis_enabled;
}

void GPS_Lap_Tick(void)
{
	uint32_t now;

	if((g_lap_analysis_enabled == 0U) || (g_lap_active == 0U)) return;

	now = HAL_GetTick();
	g_lap_current_ms = (int32_t)(now - g_lap_start_tick);
	submit_lap_times_limited(now, 0U);
}

void GPS_LapProcess(const GPS_Data_t * data)
{
#if GPS_LAP_ENABLE_GEOMETRY
	GPS_LapLinePos_t curr_pos;
	GPS_LapLinePos_t cross_pos;
	float ratio;
	uint32_t cross_tick;
	int32_t prev_lap_ms;
#endif
	uint32_t now;

	if(!g_start_set || !g_finish_set) return;
	if(g_lap_analysis_enabled == 0U) return;
	if(data == NULL || data->valid == 0U) return;
	if(coord_valid(data->latitude, data->longitude) == 0U) return;

	if(!g_lap_active) return;

	now = HAL_GetTick();
#if GPS_LAP_ENABLE_GEOMETRY == 0U
	g_lap_current_ms = (int32_t)(now - g_lap_start_tick);
	submit_lap_times_limited(now, 0U);
	return;
#endif
#if GPS_LAP_ENABLE_GEOMETRY
	curr_pos = project_to_line(&g_finish_line, data->latitude, data->longitude);
	if(line_pos_valid(&curr_pos) == 0U) {
		g_lap_prev_valid = 0U;
		return;
	}
	g_lap_current_ms = (int32_t)(now - g_lap_start_tick);

	if(absf32(curr_pos.forward_m) >= GPS_LAP_LINE_ARM_DISTANCE_M) {
		g_lap_finish_armed = 1U;
	}

	if((g_lap_finish_armed != 0U) && (g_lap_prev_valid != 0U) &&
	   (crossed_finish_line(&g_lap_prev_pos, &curr_pos, &ratio) != 0U)) {
		cross_tick = g_lap_prev_tick +
					 (uint32_t)(((float)(now - g_lap_prev_tick) * ratio) + 0.5f);
		cross_pos.forward_m = g_lap_prev_pos.forward_m +
							  ((curr_pos.forward_m - g_lap_prev_pos.forward_m) * ratio);
		cross_pos.lateral_m = g_lap_prev_pos.lateral_m +
							 ((curr_pos.lateral_m - g_lap_prev_pos.lateral_m) * ratio);
		g_lap_current_ms = (int32_t)(cross_tick - g_lap_start_tick);
		add_current_sample(&cross_pos, cross_tick, g_lap_current_ms, 1U);

		prev_lap_ms = g_lap_last_ms;
		if(g_lap_count > 0) {
			g_lap_delta_ms = g_lap_current_ms - prev_lap_ms;
			if(g_lap_current_ms < g_lap_best_ms || g_lap_best_ms == 0) {
				g_lap_best_ms = g_lap_current_ms;
			}
		} else {
			g_lap_best_ms = g_lap_current_ms;
			g_lap_delta_ms = 0;
		}
		g_lap_last_ms = g_lap_current_ms;

		g_lap_count++;
		archive_current_samples();
		clear_current_samples();
		g_lap_start_tick = cross_tick;
		g_lap_current_ms = (int32_t)(now - g_lap_start_tick);
		g_lap_finish_armed = 0U;
		add_current_sample(&cross_pos, cross_tick, 0, 1U);

		Dashboard_UI_SubmitLapTimes(g_lap_current_ms / 10,
									g_lap_last_ms / 10,
									g_lap_best_ms / 10,
									g_lap_count);
		Dashboard_UI_SubmitLapDelta(g_lap_delta_ms / 10);
	} else {
		submit_lap_times_limited(now, 0U);
	}

	g_lap_prev_pos = curr_pos;
	g_lap_prev_tick = now;
	g_lap_prev_valid = 1U;
#endif
}

const char * GPS_Lap_GetDeltaStr(char * buf, uint32_t size)
{
	int32_t abs_delta = g_lap_delta_ms < 0 ? -g_lap_delta_ms : g_lap_delta_ms;
	int32_t secs = abs_delta / 1000;
	int32_t cs = (abs_delta % 1000) / 10;
	uint32_t pos = 0U;
	char tmp[10];
	uint32_t n = 0U;

	if((buf == NULL) || (size == 0U)) return "";
	if(pos + 1U < size) buf[pos++] = g_lap_delta_ms >= 0 ? '+' : '-';
	if(secs == 0) {
		if(pos + 1U < size) buf[pos++] = '0';
	} else {
		while((secs > 0) && (n < sizeof(tmp))) {
			tmp[n++] = (char)('0' + (secs % 10));
			secs /= 10;
		}
		while(n > 0U) {
			if(pos + 1U < size) buf[pos++] = tmp[--n];
			else break;
		}
	}
	if(pos + 1U < size) buf[pos++] = '.';
	if(pos + 1U < size) buf[pos++] = (char)('0' + (cs / 10));
	if(pos + 1U < size) buf[pos++] = (char)('0' + (cs % 10));
	if(pos + 1U < size) buf[pos++] = 's';
	buf[pos < size ? pos : (size - 1U)] = '\0';
	return buf;
}

int32_t GPS_Lap_GetDeltaHundredths(void)  { return g_lap_delta_ms / 10; }
int32_t GPS_Lap_GetCurrentLapHundredths(void) { return g_lap_current_ms / 10; }
int32_t GPS_Lap_GetLastLapHundredths(void)    { return g_lap_last_ms / 10; }
int32_t GPS_Lap_GetBestLapHundredths(void)    { return g_lap_best_ms / 10; }
int32_t GPS_Lap_GetCurrentLapNum(void)         { return g_lap_count; }

#else

void GPS_LapProcess(const GPS_Data_t * data)
{
	(void)data;
}

#endif

#if defined(__GNUC__)
#pragma GCC pop_options
#endif
