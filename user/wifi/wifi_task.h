#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#include <stdint.h>
#include "cmsis_os2.h"
#include "time_run.h"

osStatus_t WIFI_TaskInit(void);
osStatus_t WIFI_TaskGetTimeData(struct TimeType *time);
osStatus_t WIFI_TaskSendBuffer(uint8_t *buffer);
void WIFI_TaskSetEvent(uint32_t event);
void WIFI_TaskSuspend(void);
void WIFI_TaskResume(void);

#endif
