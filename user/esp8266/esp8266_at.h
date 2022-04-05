#ifndef ESP8266_AT_H
#define ESP8266_AT_H

#include <stdint.h>
#include <stdbool.h>

enum PowerFlag {
    POWER_OFF,
    POWER_ON,
};

void WIFI_ReInit(void);
bool WIFI_Init(void);
void WIFI_ReceiveProcess(uint8_t *buf);
void WIFI_GetTime(void);
void WIFI_Power(enum PowerFlag flag);

#endif
