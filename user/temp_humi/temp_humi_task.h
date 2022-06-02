#ifndef TEMP_HUMI_TASK_H
#define TEMP_HUMI_TASK_H

#include "cmsis_os2.h"
#include "htu21d.h"

osStatus_t TH_TaskInit(void);
osStatus_t TH_TaskGetThData(struct Htu21dDataType *th);
void TH_TaskSetEvent(uint32_t event);
void TH_TaskSuspend(void);
void TH_TaskResume(void);

#endif
