#ifndef __LOCATE_H
#define __LOCATE_H

#include <stdint.h>

#define GPS_DMA_BUF_SIZE  512U

#define GNSS_SIG_LEVEL_NONE      0
#define GNSS_SIG_LEVEL_WEAK      1
#define GNSS_SIG_LEVEL_FAIR      2
#define GNSS_SIG_LEVEL_GOOD      3
#define GNSS_SIG_LEVEL_EXCELLENT 4
#define GNSS_SIG_LEVEL_COUNT     5

#define GNSS_SNR_EXCELLENT  42
#define GNSS_SNR_GOOD       35
#define GNSS_SNR_FAIR       28
#define GNSS_SNR_WEAK       20

typedef struct {
    uint8_t  gga_valid;
    float    latitude;
    float    longitude;
    float    altitude;
    uint8_t  fix_quality;
    uint8_t  sat_num;
    float    hdop;

    uint8_t  rmc_valid;
    float    speed_knot;
    float    speed_kmh;
    float    track_angle;
    uint8_t  utc_hour;
    uint8_t  utc_min;
    uint8_t  utc_sec;
    uint16_t utc_millisec;

    uint8_t  gsv_valid;
    uint8_t  tracked_sats;
    int8_t   max_snr;
    int8_t   avg_snr;
    uint8_t  signal_level;
} GNSS_Data_t;

extern uint8_t g_gps_dma_buf[GPS_DMA_BUF_SIZE];

void GNSS_Init(void);
void GNSS_ISR_Notify(void);
void get_gnss_data(GNSS_Data_t* data);
void print_gnss_data(void);
void send_gnss_data_uart3(const GNSS_Data_t* data);
void parse_BESTNAVA(const char* message);

#endif
