#ifndef _EMW3060_AT_H_
#define _EMW3060_AT_H_

#include "type_define.h"
#include "gpio_bit_ctrl.h"

enum PowerFlag {
    POWER_OFF,
    POWER_ON
};

void WIFI_ReceiveProcess(u8 *buf);
void WIFI_Init(void);
void WIFI_CtrDec(void);
void WIFI_PowerOnOff(enum PowerFlag flag);
BOOLEAN WIFI_GetTimeDataFlag(void);
void WIFI_SetTimeDataFlag(BOOLEAN value);

#endif
