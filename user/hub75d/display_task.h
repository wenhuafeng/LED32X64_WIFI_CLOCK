#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#include "cmsis_os2.h"

extern osStatus_t DISP_TaskInit(void);
extern void DISP_TaskSetEvent(uint32_t event);

#endif
