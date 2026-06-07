#ifndef DASHBOARD_UI_H
#define DASHBOARD_UI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	/* DBC BO_769 GPS_Speed: GroundSpeed (1,0) km/h, Display->ECU */
	int32_t speed;
	/* BMS(非DBC总线): SOC 0~100% */
	int32_t soc;
	/* DBC BO_1289 Debug9: ModeFlag, bit4|4@1- [-8,7] */
	int32_t mode_index;
	/* DBC BO_1282 Debug2: FR/FL/RR/RL_ActualTorque (1,0) 0.1%Mn */
	/* Wheel order: 0=LF(DBC FL), 1=LR(DBC RL), 2=RF(DBC FR), 3=RR(DBC RR) */
	int32_t torque[4];
	/* DBC BO_1289 Debug9: FR/FL/RR/RL_AMK_bEnable, bit20~23 each 1bit */
	/* Wheel order: 0=LF, 1=LR, 2=RF, 3=RR */
	uint8_t motor_enable[4];
	/* DBC BO_1285 Debug5: FR/FL/RR/RL_ActualVelocity (1,0) */
	/* Wheel order: 0=LF, 1=LR, 2=RF, 3=RR */
	int32_t rpm[4];
	/* BMS(非DBC总线): 总电压 */
	int32_t sum_voltage;
	/* BMS(非DBC总线): 总电流 */
	int32_t sum_current;
	/* BMS(非DBC总线): 最高温度 */
	int32_t max_temperature;
	/* DBC BO_1286 Debug6: FR/FL/RR/RL_Motor_temperature (0.1,0) degC */
	/* Wheel order: 0=LF, 1=LR, 2=RF, 3=RR */
	int32_t motor_temp[4];
	/* DBC BO_773 DataLogger: APS_OpenPct (0.1,0) % */
	int32_t aps_open_pct;
	/* DBC BO_773 DataLogger: SteeringWheelAngle (0.1,0) deg */
	int32_t steering_angle;
	/* DBC BO_773 DataLogger: OilPressure_Kpa (0.001,0) Kpa */
	int32_t oil_pressure;
	/* DBC BO_1288 Debug8: FR/FL/RR/RL_IGBT_temperature (0.1,0) degC */
	/* Wheel order: 0=LF, 1=LR, 2=RF, 3=RR */
	int32_t igbt_temp[4];
	/* DBC BO_1287 Debug7: FR/FL/RR/RL_Inverter_temperature (0.1,0) degC */
	/* Wheel order: 0=LF, 1=LR, 2=RF, 3=RR */
	int32_t inverter_temp[4];
	/* DBC BO_1283 Debug3 / BO_1284 Debug4: Diagnostic_number_1~4 */
	/* Wheel order: 0=LF, 1=LR, 2=RF, 3=RR */
	uint32_t diag_num[4];
	/* DBC BO_97 IMU_Accel: IMU_AccelX/Y/Z (0.00048828125,0) g */
	int32_t imu_accel[3];
	/* DBC BO_98 IMU_Gyro: IMU_GyroX/Y/Z (0.0610352,0) deg/s */
	int32_t imu_gyro[3];
	/* DBC BO_99 IMU_Roll: IMU_Angle_Roll (0.005493,0) deg */
	int32_t imu_roll;
	/* DBC BO_100 IMU_Pitch: IMU_Angle_Pitch (0.005493,0) deg */
	int32_t imu_pitch;
	/* DBC BO_101 IMU_Yaw: IMU_Angle_Yaw (0.005493,0) deg */
	int32_t imu_yaw;
	/* DBC BO_102 IMU_Magnetic: IMU_MagX/Y/Z (1,0) */
	int32_t imu_mag[3];
	/* 非DBC: 信号强度等级 */
	int32_t signal_level;
	/* 非DBC: 报警激活标志 */
	uint8_t alert_active;
	/* 非DBC: 里程(0.1km) */
	int32_t odometer_tenths;
	/* 非DBC: 制动百分比 */
	int32_t brake_pct;
} dashboard_data_t;

void Dashboard_UI_Init(void);
void Dashboard_UI_Process(void);
void Dashboard_UI_SubmitData(const dashboard_data_t * data);
void Dashboard_UI_SubmitSpeed(int32_t speed);
void Dashboard_UI_SubmitLapDelta(int32_t delta_hundredths);
void Dashboard_UI_SubmitSignalLevel(int32_t level);
void Dashboard_UI_SubmitTouchState(uint16_t x, uint16_t y, uint8_t pressed);

#ifdef __cplusplus
}
#endif

#endif
