#ifndef DASHBOARD_UI_H
#define DASHBOARD_UI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int32_t speed;
	int32_t soc;
	int32_t mode_index;
	/* Wheel order: 0=LF, 1=LR, 2=RF, 3=RR */
	int32_t torque[4];
	/* Wheel order: 0=LF, 1=LR, 2=RF, 3=RR */
	uint8_t motor_enable[4];
	/* Wheel order: 0=LF, 1=LR, 2=RF, 3=RR */
	int32_t rpm[4];
	int32_t sum_voltage;
	int32_t sum_current;
	int32_t max_temperature;
	/* Wheel order: 0=LF, 1=LR, 2=RF, 3=RR, unit: 0.1 degC */
	int32_t motor_temp[4];
	int32_t aps_open_pct;
	int32_t steering_angle;
	int32_t oil_pressure;
	int32_t igbt_temp[4];
	int32_t inverter_temp[4];
	uint32_t diag_num[4];
	int32_t imu_accel[3];
	int32_t imu_gyro[3];
	int32_t imu_roll;
	int32_t imu_pitch;
	int32_t imu_yaw;
	int32_t imu_mag[3];
	int32_t signal_level;
	uint8_t alert_active;
	int32_t odometer_tenths;
	int32_t brake_pct;
} dashboard_data_t;

void Dashboard_UI_Init(void);
void Dashboard_UI_Process(void);
void Dashboard_UI_SubmitData(const dashboard_data_t * data);
void Dashboard_UI_SubmitLapDelta(int32_t delta_hundredths);
void Dashboard_UI_SubmitSignalLevel(int32_t level);
void Dashboard_UI_SubmitTouchState(uint16_t x, uint16_t y, uint8_t pressed);

#ifdef __cplusplus
}
#endif

#endif
