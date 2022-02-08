#ifndef ESP8266_AT_H
#define ESP8266_AT_H

#include <stdint.h>
#include <stdbool.h>

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
