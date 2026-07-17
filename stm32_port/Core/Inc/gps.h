#ifndef __GPS_H
#define __GPS_H

#include <stdint.h>

#define GPS_DMA_BUF_SIZE  512U

#define GPS_SIG_LEVEL_NONE      0
#define GPS_SIG_LEVEL_WEAK      1
#define GPS_SIG_LEVEL_FAIR      2
#define GPS_SIG_LEVEL_GOOD      3
#define GPS_SIG_LEVEL_EXCELLENT 4

#define GPS_SNR_EXCELLENT  42
#define GPS_SNR_GOOD       35
#define GPS_SNR_FAIR       28
#define GPS_SNR_WEAK       20

/* GPHPR heading describes the ANT1-to-ANT2 baseline.  Set this offset to the
 * clockwise angle from that baseline to the vehicle forward direction. */
#define GPS_HEADING_INSTALL_OFFSET_DEG  0.0f

typedef struct {
	uint8_t  valid;
	float    latitude;
	float    longitude;
	float    altitude;
	float    track_angle;
	float    heading_angle;
	float    speed_kmh;
	uint8_t  heading_valid;
	uint8_t  heading_quality;
	uint8_t  fix_quality;
	uint8_t  satellites;
	uint8_t  signal_level;
	int8_t   max_snr;
	int8_t   avg_snr;
	uint8_t  gsv_tracked_sats;
} GPS_Data_t;

extern uint8_t g_gps_dma_buf[GPS_DMA_BUF_SIZE];

void GPS_Init(void);
void GPS_ISR_Notify(void);
void GPS_GetData(GPS_Data_t * out);

#endif
