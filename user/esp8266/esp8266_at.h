#ifndef ESP8266_AT_H
#define ESP8266_AT_H

#include <stdint.h>
#include <stdbool.h>
#include "time_run.h"

enum PowerFlag {
    POWER_OFF,
    POWER_ON,
};

enum WifiInitStatus {
    WIFI_NOT_INIT,
    WIFI_INIT_COMPLETE,
};

enum WifiConnectStatus {
    WIFI_IS_DISCONNECT,
    WIFI_IS_CONNECTED,
};

enum WifiCmdType {
    WIFI_INIT_REST,
    WIFI_INIT_CWMODE,
    WIFI_INIT_NAME_PASSWD,
    WIFI_SET_SNTP_CONFIG,
    WIFI_GET_SNTP_TIME,
    WIFI_GET_VERSION,
};

enum WifiReceiveInfo {
    WIFI_DISCONNECT,
    WIFI_CONNECTED,
    WIFI_GOT_IP,
    WIFI_CONNECT_FAIL,
    WIFI_RETURN_TIME,
    WIFI_GET_TIME_COMPLETE,
};

struct Esp8266GetTimeType {
    enum PowerFlag wifiPowerStatus;
    enum WifiInitStatus wifiInitComplete;
    enum WifiConnectStatus wifiConnectedStatus;
    enum WifiCmdType cmdType;
    enum WifiReceiveInfo rxInfoCtr;
    uint8_t wifiPowerOffTime;
    uint8_t setSntpCtr;
    uint8_t getTimeTimes;
    uint8_t getTimeCtr;
    struct TimeType time;
};

//void WIFI_ReInit(struct Esp8266GetTimeType *wifi);
//bool WIFI_Init(struct Esp8266GetTimeType *wifi);
void WIFI_Power(struct Esp8266GetTimeType *wifi, enum PowerFlag flag);
void WIFI_SendCommand(struct Esp8266GetTimeType *wifi);
void WIFI_ReceiveProcess(struct Esp8266GetTimeType *wifi, uint8_t *buf);

#endif
