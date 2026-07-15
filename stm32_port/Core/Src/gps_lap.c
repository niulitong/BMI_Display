#include "gps_lap.h"
#include "main.h"
#include "gps.h"
#include "dashboard_ui.h"
#include <math.h>
#include <string.h>

#if defined(__GNUC__)
#pragma GCC push_options
#pragma GCC optimize ("Os")
#endif

#if GPS_LAP_ENABLE

#define GPS_LAP_EARTH_RADIUS_M       6371000.0f
#define GPS_LAP_GATE_HALF_RTK_FIXED_M 6.0f
#define GPS_LAP_GATE_HALF_RTK_FLOAT_M 10.0f
#define GPS_LAP_GATE_HALF_SINGLE_M    15.0f
#define GPS_LAP_LINE_ARM_DISTANCE_M  10.0f
#define GPS_LAP_APPROACH_DISTANCE_M  30.0f
#define GPS_LAP_DIAG_HOLD_MS         2000U
#define GPS_LAP_TRACK_SAMPLE_MAX     512U
#define GPS_LAP_SAMPLE_MIN_DIST_M    1.0f
#define GPS_LAP_SAMPLE_MAX_PERIOD_MS 500U
#define GPS_LAP_UI_UPDATE_PERIOD_MS  200U
#define GPS_LAP_REF_BACKTRACK_SEGMENTS 4U
#define GPS_LAP_REF_LOOKAHEAD_SEGMENTS 32U
#define GPS_LAP_REF_MAX_DISTANCE_DM  300.0f
#define GPS_LAP_MAX_LINE_DISTANCE_M  20000.0f
#define GPS_LAP_SAMPLE_MIN_DM        (-32768.0f)
#define GPS_LAP_SAMPLE_MAX_DM        32767.0f
#define GPS_LAP_ENABLE_GEOMETRY      1U
#define GPS_LAP_ENABLE_LIVE_DELTA    1U

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
	float longitude_scale;
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
static uint16_t g_lap_ref_match_index;
static uint8_t  g_lap_ref_match_valid;
static int32_t  g_lap_ref_match_time_ms;
static GPS_LapLinePos_t g_lap_last_sample_pos;
static uint32_t g_lap_last_sample_tick;
static uint32_t g_lap_ui_last_tick;
static GPS_LapDiagnostic_t g_lap_diag;
static uint32_t g_lap_diag_hold_until;

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

static void submit_lap_times_limited(uint32_t now, uint8_t force)
{
	if((force == 0U) && ((now - g_lap_ui_last_tick) < GPS_LAP_UI_UPDATE_PERIOD_MS)) return;
	g_lap_ui_last_tick = now;
	Dashboard_UI_SubmitLapTimes(g_lap_current_ms / 10,
								g_lap_last_ms / 10,
								g_lap_best_ms / 10,
								g_lap_count);
}

static void update_line_vectors(GPS_LapLine_t * line)
{
	float heading;
	if((finite_f32(line->track) == 0U) || (line->track < 0.0f) || (line->track >= 360.0f)) {
		line->track = 0.0f;
	}
	heading = deg_to_radf32((float)line->track);
	line->forward_e = sinf(heading);
	line->forward_n = cosf(heading);
	line->lateral_e = line->forward_n;
	line->lateral_n = -line->forward_e;
	line->longitude_scale = cosf(deg_to_radf32(line->lat));
}

static GPS_LapLinePos_t project_to_line(const GPS_LapLine_t * line,
										float lat, float lon)
{
	GPS_LapLinePos_t pos;
	float north_m = deg_to_radf32(lat - line->lat) * GPS_LAP_EARTH_RADIUS_M;
	float east_m = deg_to_radf32(lon - line->lon) * GPS_LAP_EARTH_RADIUS_M *
					  line->longitude_scale;

	pos.forward_m = east_m * line->forward_e + north_m * line->forward_n;
	pos.lateral_m = east_m * line->lateral_e + north_m * line->lateral_n;
	return pos;
}

#if GPS_LAP_ENABLE_GEOMETRY
static uint8_t finish_crossing(const GPS_LapLinePos_t * prev,
								 const GPS_LapLinePos_t * curr,
								 float * ratio_out,
								 float * lateral_out)
{
	float denom;
	float ratio;
	float lateral;

	if((prev->forward_m >= 0.0f) || (curr->forward_m < 0.0f)) return 0U;
	denom = curr->forward_m - prev->forward_m;
	if(denom <= 0.001f) return 0U;

	ratio = -prev->forward_m / denom;
	if((ratio < 0.0f) || (ratio > 1.0f)) return 0U;
	lateral = prev->lateral_m + ((curr->lateral_m - prev->lateral_m) * ratio);
	*ratio_out = ratio;
	*lateral_out = lateral;
	return 1U;
}

static uint8_t reverse_crossing(const GPS_LapLinePos_t * prev,
								  const GPS_LapLinePos_t * curr,
								  float * ratio_out,
								  float * lateral_out)
{
	float denom;
	float ratio;

	if((prev->forward_m <= 0.0f) || (curr->forward_m > 0.0f)) return 0U;
	denom = prev->forward_m - curr->forward_m;
	if(denom <= 0.001f) return 0U;
	ratio = prev->forward_m / denom;
	if((ratio < 0.0f) || (ratio > 1.0f)) return 0U;
	*ratio_out = ratio;
	*lateral_out = prev->lateral_m + ((curr->lateral_m - prev->lateral_m) * ratio);
	return 1U;
}
#endif

static float gate_half_width_for_fix(uint8_t fix_quality)
{
	if(fix_quality == 4U) return GPS_LAP_GATE_HALF_RTK_FIXED_M;
	if((fix_quality == 2U) || (fix_quality == 5U)) return GPS_LAP_GATE_HALF_RTK_FLOAT_M;
	return GPS_LAP_GATE_HALF_SINGLE_M;
}

static uint8_t diag_hold_active(uint32_t now)
{
	return (uint8_t)(((int32_t)(g_lap_diag_hold_until - now)) > 0);
}

static void update_diagnostic(GPS_LapDiagState_t state,
							  const GPS_LapLinePos_t * pos,
							  const GPS_Data_t * data)
{
	if(pos != NULL) {
		g_lap_diag.forward_m = pos->forward_m;
		g_lap_diag.lateral_m = pos->lateral_m;
	}
	if(data != NULL) {
		g_lap_diag.fix_quality = data->fix_quality;
		g_lap_diag.gate_half_width_m = gate_half_width_for_fix(data->fix_quality);
	}
	g_lap_diag.sample_count = g_lap_current_sample_count;
	g_lap_diag.state = state;
}

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

static void compact_current_samples(void)
{
	uint16_t source;
	uint16_t target = 0U;
	uint16_t last_index;
	uint16_t last_copied = 0U;

	if(g_lap_current_sample_count < GPS_LAP_TRACK_SAMPLE_MAX) return;
	last_index = g_lap_current_sample_count - 1U;
	for(source = 0U; source < g_lap_current_sample_count; source += 2U) {
		g_lap_current_samples[target++] = g_lap_current_samples[source];
		last_copied = source;
	}
	if(last_copied != last_index) {
		g_lap_current_samples[target++] = g_lap_current_samples[last_index];
	}
	g_lap_current_sample_count = target;
}

static void add_current_sample(const GPS_LapLinePos_t * pos,
							   uint32_t tick,
							   int32_t time_ms,
							   uint8_t force)
{
	float dx;
	float dy;

	if(g_lap_current_sample_count >= GPS_LAP_TRACK_SAMPLE_MAX) {
		compact_current_samples();
	}
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
	uint16_t best_index = 1U;
	uint16_t start_index = 1U;
	uint16_t end_index;
	int32_t match_time_ms;

	if((g_lap_ref_valid == 0U) || (g_lap_ref_sample_count < 2U) || (time_ms_out == NULL)) {
		return 0U;
	}
	end_index = g_lap_ref_sample_count;
	if(g_lap_ref_match_valid != 0U) {
		start_index = (g_lap_ref_match_index > GPS_LAP_REF_BACKTRACK_SEGMENTS) ?
					  (g_lap_ref_match_index - GPS_LAP_REF_BACKTRACK_SEGMENTS) : 1U;
		end_index = g_lap_ref_match_index + GPS_LAP_REF_LOOKAHEAD_SEGMENTS + 1U;
		if(end_index > g_lap_ref_sample_count) end_index = g_lap_ref_sample_count;
	}

	for(uint16_t i = start_index; i < end_index; i++) {
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
			best_index = i;
			best_time_cs = (float)g_lap_ref_samples[i - 1U].time_cs +
						   (((float)g_lap_ref_samples[i].time_cs -
							 (float)g_lap_ref_samples[i - 1U].time_cs) * t);
		}
	}
	if(best_dist2 > (GPS_LAP_REF_MAX_DISTANCE_DM * GPS_LAP_REF_MAX_DISTANCE_DM)) {
		return 0U;
	}

	match_time_ms = (int32_t)((best_time_cs * 10.0f) + 0.5f);
	if((g_lap_ref_match_valid != 0U) && (match_time_ms < g_lap_ref_match_time_ms)) {
		*time_ms_out = g_lap_ref_match_time_ms;
		return 1U;
	}
	g_lap_ref_match_index = best_index;
	g_lap_ref_match_time_ms = match_time_ms;
	g_lap_ref_match_valid = 1U;
	*time_ms_out = match_time_ms;
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
	g_lap_diag_hold_until = 0U;
	update_diagnostic(GPS_LAP_DIAG_WAIT_ARM, &g_lap_prev_pos, data);
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
	g_lap_ref_match_index = 1U;
	g_lap_ref_match_valid = 0U;
	g_lap_ref_match_time_ms = 0;
	g_lap_ui_last_tick = 0U;
	g_lap_diag_hold_until = 0U;
	(void)memset(&g_lap_diag, 0, sizeof(g_lap_diag));
	g_lap_diag.state = GPS_LAP_DIAG_INACTIVE;
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
		g_lap_ref_match_index = 1U;
		g_lap_ref_match_valid = 0U;
		g_lap_ref_match_time_ms = 0;
		g_lap_diag_hold_until = 0U;
		g_lap_diag.state = GPS_LAP_DIAG_INACTIVE;
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

void GPS_Lap_GetDiagnostic(GPS_LapDiagnostic_t * out)
{
	uint32_t primask;
	if(out == NULL) return;
	primask = __get_PRIMASK();
	__disable_irq();
	*out = g_lap_diag;
	if(primask == 0U) __enable_irq();
}

void GPS_LapProcess(const GPS_Data_t * data)
{
#if GPS_LAP_ENABLE_GEOMETRY
	GPS_LapLinePos_t curr_pos;
	GPS_LapLinePos_t cross_pos;
	float ratio;
	float cross_lateral = 0.0f;
	float gate_half_width;
	uint32_t cross_tick;
	int32_t reference_lap_ms;
	uint8_t new_best_lap;
	uint8_t lap_crossed = 0U;
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
	gate_half_width = gate_half_width_for_fix(data->fix_quality);
	update_diagnostic(g_lap_diag.state, &curr_pos, data);

	if(absf32(curr_pos.forward_m) >= GPS_LAP_LINE_ARM_DISTANCE_M) {
		g_lap_finish_armed = 1U;
	}

	if((g_lap_finish_armed != 0U) && (g_lap_prev_valid != 0U)) {
		if(finish_crossing(&g_lap_prev_pos, &curr_pos, &ratio, &cross_lateral) != 0U) {
			cross_pos.forward_m = 0.0f;
			cross_pos.lateral_m = cross_lateral;
			if(absf32(cross_lateral) <= gate_half_width) {
				lap_crossed = 1U;
			} else {
				g_lap_diag_hold_until = now + GPS_LAP_DIAG_HOLD_MS;
				update_diagnostic(GPS_LAP_DIAG_GATE_MISS, &cross_pos, data);
			}
		} else if(reverse_crossing(&g_lap_prev_pos, &curr_pos, &ratio, &cross_lateral) != 0U) {
			cross_pos.forward_m = 0.0f;
			cross_pos.lateral_m = cross_lateral;
			if(absf32(cross_lateral) <= gate_half_width) {
				g_lap_diag_hold_until = now + GPS_LAP_DIAG_HOLD_MS;
				update_diagnostic(GPS_LAP_DIAG_REVERSE_PASS, &cross_pos, data);
			}
		}
	}

	if(lap_crossed != 0U) {
		cross_tick = g_lap_prev_tick +
					 (uint32_t)(((float)(now - g_lap_prev_tick) * ratio) + 0.5f);
		cross_pos.forward_m = g_lap_prev_pos.forward_m +
							  ((curr_pos.forward_m - g_lap_prev_pos.forward_m) * ratio);
		cross_pos.lateral_m = g_lap_prev_pos.lateral_m +
							 ((curr_pos.lateral_m - g_lap_prev_pos.lateral_m) * ratio);
		g_lap_current_ms = (int32_t)(cross_tick - g_lap_start_tick);
		add_current_sample(&cross_pos, cross_tick, g_lap_current_ms, 1U);

		reference_lap_ms = g_lap_best_ms;
		new_best_lap = (uint8_t)((reference_lap_ms == 0) ||
								 (g_lap_current_ms < reference_lap_ms));
		g_lap_delta_ms = (reference_lap_ms > 0) ?
						 (g_lap_current_ms - reference_lap_ms) : 0;
		g_lap_last_ms = g_lap_current_ms;
		if(new_best_lap != 0U) {
			g_lap_best_ms = g_lap_current_ms;
			archive_current_samples();
		}

		g_lap_count++;
		clear_current_samples();
		g_lap_ref_match_index = 1U;
		g_lap_ref_match_valid = g_lap_ref_valid;
		g_lap_ref_match_time_ms = 0;
		g_lap_start_tick = cross_tick;
		g_lap_current_ms = (int32_t)(now - g_lap_start_tick);
		g_lap_finish_armed = 0U;
		add_current_sample(&cross_pos, cross_tick, 0, 1U);

		Dashboard_UI_SubmitLapTimes(g_lap_current_ms / 10,
									g_lap_last_ms / 10,
									g_lap_best_ms / 10,
									g_lap_count);
		Dashboard_UI_SubmitLapDelta(g_lap_delta_ms / 10);
		g_lap_diag_hold_until = now + GPS_LAP_DIAG_HOLD_MS;
		update_diagnostic(GPS_LAP_DIAG_CROSSED, &cross_pos, data);
	} else {
		add_current_sample(&curr_pos, now, g_lap_current_ms, 0U);
		update_live_delta(&curr_pos, g_lap_current_ms);
		submit_lap_times_limited(now, 0U);
		if(diag_hold_active(now) == 0U) {
			GPS_LapDiagState_t state = GPS_LAP_DIAG_WAIT_ARM;
			if(g_lap_finish_armed != 0U) {
				state = ((curr_pos.forward_m < 0.0f) &&
						 (absf32(curr_pos.forward_m) <= GPS_LAP_APPROACH_DISTANCE_M) &&
						 (absf32(curr_pos.lateral_m) <= (gate_half_width * 2.0f))) ?
						GPS_LAP_DIAG_APPROACHING : GPS_LAP_DIAG_ARMED;
			}
			update_diagnostic(state, &curr_pos, data);
		}
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
