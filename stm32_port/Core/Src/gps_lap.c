#include "gps_lap.h"
#include "gps.h"
#include "dashboard_ui.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

#if GPS_LAP_ENABLE

#define GPS_LAP_EARTH_RADIUS_M  6371000.0
#define GPS_LAP_LINE_WIDTH_M    8.0

typedef struct {
	double lat;
	double lon;
	double track;
} GPS_LapLine_t;

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
static uint8_t  g_lap_active;

static double deg_to_rad(double deg)
{
	return deg * 3.14159265358979323846 / 180.0;
}

static double calc_distance_m(double lat1, double lon1,
                               double lat2, double lon2)
{
	double dlat = deg_to_rad(lat2 - lat1);
	double dlon = deg_to_rad(lon2 - lon1);
	double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
	           cos(deg_to_rad(lat1)) * cos(deg_to_rad(lat2)) *
	           sin(dlon / 2.0) * sin(dlon / 2.0);
	double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
	return GPS_LAP_EARTH_RADIUS_M * c;
}

static double track_diff(double a, double b)
{
	double diff = a - b;
	if(diff > 180.0) diff -= 360.0;
	if(diff < -180.0) diff += 360.0;
	return diff;
}

static int32_t is_near_line(const GPS_LapLine_t * line,
                             double lat, double lon,
                             double track)
{
	double dist = calc_distance_m(lat, lon, line->lat, line->lon);
	if(dist > GPS_LAP_LINE_WIDTH_M) return 0;

	double diff = track_diff(track, line->track);
	if(diff < -90.0 || diff > 90.0) return 0;

	return 1;
}

void GPS_Lap_SetStartLine(double lat, double lon, double track)
{
	g_start_line.lat = lat;
	g_start_line.lon = lon;
	g_start_line.track = track;
	g_start_set = 1U;
}

void GPS_Lap_SetFinishLine(double lat, double lon, double track)
{
	g_finish_line.lat = lat;
	g_finish_line.lon = lon;
	g_finish_line.track = track;
	g_finish_set = 1U;
}

void GPS_Lap_Reset(void)
{
	g_lap_current_ms = 0;
	g_lap_last_ms = 0;
	g_lap_delta_ms = 0;
	g_lap_count = 0;
	g_lap_active = 0U;
	g_lap_start_tick = 0;
}

void GPS_LapProcess(const GPS_Data_t * data)
{
	if(!g_start_set || !g_finish_set) return;
	if(data == NULL || data->valid == 0U) return;

	int32_t near = is_near_line(&g_start_line,
	                              data->latitude,
	                              data->longitude,
	                              (double)data->track_angle);

	if(near && !g_lap_active) {
		g_lap_active = 1U;
		g_lap_start_tick = HAL_GetTick();
		return;
	}

	if(!g_lap_active) return;

	near = is_near_line(&g_finish_line,
	                     data->latitude,
	                     data->longitude,
	                     (double)data->track_angle);

	if(!near) return;

	uint32_t now = HAL_GetTick();
	g_lap_current_ms = (int32_t)(now - g_lap_start_tick);

	if(g_lap_count > 0) {
		g_lap_last_ms = g_lap_current_ms;
		g_lap_delta_ms = g_lap_current_ms - g_lap_best_ms;
		if(g_lap_current_ms < g_lap_best_ms || g_lap_best_ms == 0) {
			g_lap_best_ms = g_lap_current_ms;
		}
	} else {
		g_lap_best_ms = g_lap_current_ms;
		g_lap_last_ms = 0;
		g_lap_delta_ms = 0;
	}

	g_lap_count++;
	g_lap_start_tick = now;

	Dashboard_UI_SubmitLapDelta(g_lap_delta_ms / 10);
}

const char * GPS_Lap_GetDeltaStr(char * buf, uint32_t size)
{
	int32_t abs_delta = g_lap_delta_ms < 0 ? -g_lap_delta_ms : g_lap_delta_ms;
	int32_t secs = abs_delta / 1000;
	int32_t cs = (abs_delta % 1000) / 10;
	snprintf(buf, size, "%s%ld.%02lds",
	         g_lap_delta_ms >= 0 ? "+" : "-",
	         (long)secs, (long)cs);
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
