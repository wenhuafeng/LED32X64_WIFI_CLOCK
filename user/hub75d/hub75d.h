#ifndef HUB75D_H
#define HUB75D_H

#include <stdbool.h>
#include "time_run.h"

enum DispTime {
    DISP_TIME_OFF = 0,
    DISP_TIME     = (5 * 60),
};

enum DispTorH {
    DISP_T,
    DISP_H,
};

enum {
    DISP_OFF = 0,
    DISP_ON  = 1,
};

void HUB75D_DispScan(void);
void HUB75D_CalculateCalendar(struct TimeType *time);
bool HUB75D_CtrDec(void);
void HUB75D_SetDispOffCtr(enum DispTime time);
void HUB75D_Disp(enum DispTime time);
void HUB75D_GetScanRgb(void);
void HUB75D_Init(void);

#endif
