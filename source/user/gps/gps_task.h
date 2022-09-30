#ifndef GPS_TASK_H
#define GPS_TASK_H

#include <stdint.h>
#include "cmsis_os2.h"
#include "time_run.h"

osStatus_t GPS_TaskInit(void);
osStatus_t GPS_TaskGetTimeData(struct TimeType *time);
osStatus_t GPS_TaskSendBuffer(uint8_t *buffer);
void GPS_TaskSetEvent(uint32_t event);
void GPS_TaskSuspend(void);
void GPS_TaskResume(void);

#endif
