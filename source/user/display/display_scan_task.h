#ifndef DISPLAY_SCAN_TASK_H
#define DISPLAY_SCAN_TASK_H

#include <stdint.h>
#include "cmsis_os2.h"

#define DISP_SCAN_TASK_EVENT_RECEIVED_NEW_DATA (1 << 0)

osStatus_t DISP_ScanTaskInit(void);
void DISP_ScanTaskSetEvent(uint32_t event);
void DISP_ScanLed(void);
void DISP_ScanTaskSuspend(void);
void DISP_ScanTaskResume(void);

#endif
