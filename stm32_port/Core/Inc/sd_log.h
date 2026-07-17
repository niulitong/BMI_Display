#ifndef SD_LOG_H
#define SD_LOG_H

#include "dashboard_ui.h"

void SD_Log_InitAndWrite(const dashboard_data_t * data);
uint8_t SD_Odometer_Load(uint32_t * total_m);
void SD_Odometer_StartTask(void);
void SD_Odometer_RequestSave(uint32_t total_m);

#endif
