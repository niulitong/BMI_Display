#ifndef __GPS_LAP_H
#define __GPS_LAP_H

#include "gps.h"

#define GPS_LAP_ENABLE  0

#if GPS_LAP_ENABLE

void GPS_Lap_SetStartLine(double lat, double lon, double track);
void GPS_Lap_SetFinishLine(double lat, double lon, double track);
void GPS_Lap_Reset(void);
const char * GPS_Lap_GetDeltaStr(char * buf, uint32_t size);
int32_t GPS_Lap_GetDeltaHundredths(void);
int32_t GPS_Lap_GetCurrentLapHundredths(void);
int32_t GPS_Lap_GetLastLapHundredths(void);
int32_t GPS_Lap_GetBestLapHundredths(void);
int32_t GPS_Lap_GetCurrentLapNum(void);

#else

#define GPS_Lap_SetStartLine(lat,lon,t)  ((void)0)
#define GPS_Lap_SetFinishLine(lat,lon,t) ((void)0)
#define GPS_Lap_Reset()                  ((void)0)
#define GPS_Lap_GetDeltaStr(b,s)         ("0.00s")
#define GPS_Lap_GetDeltaHundredths()     0
#define GPS_Lap_GetCurrentLapHundredths() 0
#define GPS_Lap_GetLastLapHundredths()    0
#define GPS_Lap_GetBestLapHundredths()    0
#define GPS_Lap_GetCurrentLapNum()        0

#endif

void GPS_LapProcess(const GPS_Data_t * data);

#endif
