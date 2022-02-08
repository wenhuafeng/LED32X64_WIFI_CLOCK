#ifndef _EMW3060_AT_H_
#define _EMW3060_AT_H_

#include <stdint.h>
#include <stdbool.h>
#include "gpio_bit_ctrl.h"

enum PowerFlag {
    POWER_OFF,
    POWER_ON
};

void WIFI_ReceiveProcess(uint8_t *buf);
void WIFI_Init(void);
void WIFI_CtrDec(void);
void WIFI_PowerOnOff(enum PowerFlag flag);
bool WIFI_GetTimeDataFlag(void);
void WIFI_SetTimeDataFlag(bool value);

#endif
