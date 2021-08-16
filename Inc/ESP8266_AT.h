#ifndef ESP8266_AT_H
#define ESP8266_AT_H

#include "gpio_bit_ctrl.h"
#include "TypeDefine.h"

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
