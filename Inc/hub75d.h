#ifndef HUB75D_H
#define HUB75D_H

#include "time.h"

enum DispTime {
    DISP_TIME_OFF = 0,
    DISP_TIME_5MIN = (5 * 60)
};

enum DispTorH {
    DISP_T,
    DISP_H
};

enum {
    DISP_OFF = 0,
    DISP_ON = 1
};

void HUB75D_DispScan(void);
void HUB75D_CalculateCalendar(struct TimeType *time);
void HUB75D_CtrDec(void);
void HUB75D_SetDispOffCtr(enum DispTime time);
void HUB75D_DispOnOff(enum DispTime time);

#endif
