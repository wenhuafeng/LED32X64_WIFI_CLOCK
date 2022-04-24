#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#include "cmsis_os2.h"
#include "hub75d.h"

#define DISP_TASK_EVENT_GET_SCAN_RGB (1 << 0)
#define DISP_TASK_EVENT_GET_TH_DATA  (1 << 1)
#define DISP_TASK_EVENT_DISP_ON      (1 << 2)

extern osStatus_t DISP_TaskInit(void);
extern void DISP_TaskSetEvent(uint32_t event);
osStatus_t DISP_TaskGetDispScanData(struct RgbType *scanRgb);

#endif
