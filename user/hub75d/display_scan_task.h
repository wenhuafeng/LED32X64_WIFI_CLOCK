#ifndef DISPLAY_SCAN_TASK_H
#define DISPLAY_SCAN_TASK_H

#include "cmsis_os2.h"

#define DISP_SCAN_TASK_EVENT_SCAN_LED          (1 << 0)
#define DISP_SCAN_TASK_EVENT_RECEIVED_NEW_DATA (1 << 1)

osStatus_t DISP_ScanTaskInit(void);
void DISP_ScanTaskSetEvent(uint32_t event);

#endif
