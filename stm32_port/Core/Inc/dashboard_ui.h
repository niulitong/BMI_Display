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
	int32_t torque[4];
	int32_t rpm[4];
	int32_t sum_voltage;
	int32_t sum_current;
	int32_t max_temperature;
} dashboard_data_t;

void Dashboard_UI_Init(void);
void Dashboard_UI_Process(void);
void Dashboard_UI_SubmitData(const dashboard_data_t * data);

#ifdef __cplusplus
}
#endif

#endif
