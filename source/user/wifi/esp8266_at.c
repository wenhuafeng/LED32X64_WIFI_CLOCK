#include "main.h"
#if defined(WIFI_GET_TIME) && WIFI_GET_TIME
#include "esp8266_at.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "gpio_bit_ctrl.h"
#include "usart.h"
#include "time_run.h"
#include "wifi_uart_if.h"
#include "trace.h"

#define LOG_TAG "esp8266"

#define WEEK_STR_INDEX          13
#define MONTH_STR_INDEX         17
#define DAY_STR_INDEX_HIGH      21
#define DAY_STR_INDEX_LOW       22
#define HOUR_STR_INDEX_HIGH     24
#define HOUR_STR_INDEX_LOW      25
#define MIN_STR_INDEX_HIGH      27
#define MIN_STR_INDEX_LOW       28
#define SEC_STR_INDEX_HIGH      30
#define SEC_STR_INDEX_LOW       31
#define YEAR_STR_INDEX_THOUSAND 33
#define YEAR_STR_INDEX_HUNDRED  34
#define YEAR_STR_INDEX_TEN      35
#define YEAR_STR_INDEX          36

#define YEAR_MIN  2000
#define YEAR_MAX  2099
#define MONTH_MIN 1
#define MONTH_MAX 12
#define DAY_MIN   1
#define DAY_MAX   31
#define WEEK_MAX  6
#define HOUR_MAX  23
#define MIN_MAX   59
#define SEC_MAX   59

#define WIFI_CH_PD          PBout(5)
#define WIFI_OFF            0
#define WIFI_ON_TIME        (2 * 60) /* 3 min */
#define WIFI_GET_TIME_TIMES (5U)
#define WIFI_SET_SNTP_TIMES (5U)

#define SET_SNTP_DELAY_SEC (2U)
#define GET_TIME_DELAY_SEC (2U)

/* wifi init hander */
#define WIFI_INIT_TOTAL_STEP (6U)

char *g_wifiCmdTable[WIFI_INIT_TOTAL_STEP] = {
    "AT+RST\r\n",
    "AT+CWMODE=1\r\n",
    "AT+CWJAP_DEF=\"HSG2\",\"#13537011631$\"\r\n",
    "AT+CIPSNTPCFG=1,8\r\n",
    "AT+CIPSNTPTIME?\r\n",
    "AT+GMR\r\n",
};

/* wifi receive data handler */
#define WIFI_RECEIVE_INFO_SIZE 5

typedef void (*WifiReceiveInfoHandler)(struct Esp8266GetTimeType *wifi, char *buf);
struct WifiRxType {
    enum WifiReceiveInfo info;
    char *str;
    WifiReceiveInfoHandler handler;
};

static uint8_t AscToHex(uint8_t asc)
{
    uint8_t hex = asc;

    if ((asc >= 0x30) && (asc <= 0x39)) {
        hex -= 0x30;
    } else if ((asc >= 0x41) && (asc <= 0x46)) {
        hex -= 0x37;
    } else if ((asc >= 0x61) && (asc <= 0x66)) {
        hex -= 0x57;
    } else {
        hex = 0xff;
    }

    return hex;
}

static uint8_t GetWeek(char *weekString)
{
    uint8_t i;
    uint8_t tableLen;
    char *weekTable[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

    tableLen = sizeof(weekTable) / sizeof(weekTable[0]);
    for (i = 0; i < tableLen; i++) {
        if (strcmp(weekTable[i], weekString) == 0) {
            break;
        }
    }

    LOGI(LOG_TAG, "week:%d\r\n", i);
    return i;
}

static uint8_t GetMonth(char *monthString)
{
    uint8_t i;
    uint8_t tableLen;
    char *monthTable[] = { " ", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    tableLen = sizeof(monthTable) / sizeof(monthTable[0]);
    for (i = 0; i < tableLen; i++) {
        if (strcmp(monthTable[i], monthString) == 0) {
            break;
        }
    }
    LOGI(LOG_TAG, "month:%d\r\n", i);

    return i;
}

static bool ProcessClock(struct Esp8266GetTimeType *wifi, char *cRxBuf)
{
    char str[4] = { 0 };
    struct TimeType time;
    int ret;

    ret = snprintf(str, sizeof(str), "%s", &cRxBuf[WEEK_STR_INDEX]);
    if (ret <= 0) {
        LOGI(LOG_TAG, "%s snprintf error!\r\n", __LINE__);
        return false;
    }
    time.week = GetWeek(str);

    ret = snprintf(str, sizeof(str), "%s", &cRxBuf[MONTH_STR_INDEX]);
    if (ret <= 0) {
        LOGI(LOG_TAG, "%s snprintf error!\r\n", __LINE__);
        return false;
    }
    time.month  = GetMonth(str);
    time.day    = AscToHex(cRxBuf[DAY_STR_INDEX_HIGH]) * 10 + AscToHex(cRxBuf[DAY_STR_INDEX_LOW]);
    time.hour   = AscToHex(cRxBuf[HOUR_STR_INDEX_HIGH]) * 10 + AscToHex(cRxBuf[HOUR_STR_INDEX_LOW]);
    time.minute = AscToHex(cRxBuf[MIN_STR_INDEX_HIGH]) * 10 + AscToHex(cRxBuf[MIN_STR_INDEX_LOW]);
    time.second = AscToHex(cRxBuf[SEC_STR_INDEX_HIGH]) * 10 + AscToHex(cRxBuf[SEC_STR_INDEX_LOW]);
    time.year   = AscToHex(cRxBuf[YEAR_STR_INDEX_THOUSAND]) * 1000 + AscToHex(cRxBuf[YEAR_STR_INDEX_HUNDRED]) * 100 +
                  AscToHex(cRxBuf[YEAR_STR_INDEX_TEN]) * 10 + AscToHex(cRxBuf[YEAR_STR_INDEX]);

    if (time.year < YEAR_MIN || time.year > YEAR_MAX) {
        return false;
    }
    if (time.month < MONTH_MIN || time.month > MONTH_MAX) {
        return false;
    }
    if (time.day < DAY_MIN || time.day > DAY_MAX) {
        return false;
    }
    if (time.week > WEEK_MAX) {
        return false;
    }
    if (time.hour > HOUR_MAX) {
        return false;
    }
    if (time.minute > MIN_MAX) {
        return false;
    }
    if (time.second > SEC_MAX) {
        return false;
    }

    memcpy(&wifi->time, &time, sizeof(struct TimeType));
    wifi->rxInfoCtr = WIFI_GET_TIME_COMPLETE;
    LOGI(LOG_TAG, "get time and date complete\r\n");

    return true;
}

static void WIFI_ReInit(struct Esp8266GetTimeType *wifi)
{
    wifi->wifiInitComplete = WIFI_NOT_INIT;
}

static bool WIFI_Init(struct Esp8266GetTimeType *wifi)
{
    if (wifi->wifiInitComplete == WIFI_INIT_COMPLETE) {
        return true;
    }
    if (wifi->wifiConnectedStatus == WIFI_IS_CONNECTED) {
        return true;
    }
    if (wifi->cmdType > WIFI_INIT_NAME_PASSWD) {
        wifi->wifiInitComplete = WIFI_INIT_COMPLETE;
        return true;
    }

    PrintUsart1(g_wifiCmdTable[wifi->cmdType]);
    LOGI(LOG_TAG, "wifi init:%s\r\n", g_wifiCmdTable[wifi->cmdType]);
    wifi->cmdType++;

    return false;
}

static void WIFI_Disconnect(struct Esp8266GetTimeType *wifi, char *buf)
{
    LOGI(LOG_TAG, "wifi disconnect\r\n");
    PrintUsart1(g_wifiCmdTable[WIFI_INIT_NAME_PASSWD]);
    wifi->wifiConnectedStatus = WIFI_IS_DISCONNECT;
    wifi->rxInfoCtr           = WIFI_DISCONNECT;
}

static void WIFI_Connected(struct Esp8266GetTimeType *wifi, char *buf)
{
    LOGI(LOG_TAG, "wifi connected\r\n");
    wifi->wifiConnectedStatus = WIFI_IS_CONNECTED;
    wifi->rxInfoCtr           = WIFI_CONNECTED;
}

static void WIFI_GotIp(struct Esp8266GetTimeType *wifi, char *buf)
{
    LOGI(LOG_TAG, "wifi got ip\r\n");
    if (wifi->wifiConnectedStatus == WIFI_IS_CONNECTED) {
        wifi->setSntpCtr = SET_SNTP_DELAY_SEC;
    }

    wifi->rxInfoCtr = WIFI_GOT_IP;
}

static void WIFI_ConnectedFail(struct Esp8266GetTimeType *wifi, char *buf)
{
    LOGI(LOG_TAG, "wifi connect fail\r\n");
    PrintUsart1(g_wifiCmdTable[WIFI_INIT_NAME_PASSWD]);
    wifi->rxInfoCtr = WIFI_CONNECT_FAIL;
}

static void WIFI_ReturnTime(struct Esp8266GetTimeType *wifi, char *buf)
{
    bool ret;

    LOGI(LOG_TAG, "wifi return time\r\n");
    wifi->rxInfoCtr = WIFI_RETURN_TIME;

    ret = ProcessClock(wifi, buf);
    if (ret == false) {
        LOGI(LOG_TAG, "process clock error!\r\n");
        if (wifi->getTimeTimes < WIFI_GET_TIME_TIMES) {
            wifi->getTimeTimes++;
            wifi->getTimeCtr = GET_TIME_DELAY_SEC;
        } else {
            wifi->getTimeTimes = 0x00;
            WIFI_Power(wifi, WIFI_POWER_OFF);
            WIFI_ReInit(wifi);
        }
    } else {
        WIFI_Power(wifi, WIFI_POWER_OFF);
    }
}

static const struct WifiRxType g_wifiRxHandlerTable[] = {
    { WIFI_DISCONNECT, "WIFI DISCONNECT", WIFI_Disconnect },
    { WIFI_CONNECTED, "WIFI CONNECTED", WIFI_Connected },
    { WIFI_GOT_IP, "WIFI GOT IP", WIFI_GotIp },
    { WIFI_CONNECT_FAIL, "+CWJAP:3", WIFI_ConnectedFail },
    { WIFI_RETURN_TIME, "+CIPSNTPTIME:", WIFI_ReturnTime },
};

void WIFI_Power(struct Esp8266GetTimeType *wifi, enum PowerFlag flag)
{
    if (wifi == NULL) {
        return;
    }

    if (flag == WIFI_POWER_ON) {
        memset(wifi, 0, sizeof(struct Esp8266GetTimeType));
        WIFI_CH_PD             = 1;
        wifi->wifiPowerOffTime = WIFI_ON_TIME;
        wifi->wifiPowerStatus  = WIFI_POWER_ON;
        LOGI(LOG_TAG, "wifi power on\r\n");
    } else {
        WIFI_CH_PD             = 0;
        wifi->wifiPowerOffTime = WIFI_OFF;
        wifi->wifiPowerStatus  = WIFI_POWER_OFF;
        LOGI(LOG_TAG, "wifi power off\r\n");
    }
}

void WIFI_SendCommand(struct Esp8266GetTimeType *wifi)
{
    if (wifi == NULL) {
        return;
    }

    if (WIFI_Init(wifi) == false) {
        return;
    }

    if (wifi->wifiPowerOffTime != 0x00) {
        wifi->wifiPowerOffTime--;
        if (wifi->wifiPowerOffTime == 0x00) {
            WIFI_Power(wifi, WIFI_POWER_OFF);
        }
    }

    if (wifi->getTimeCtr != 0x00) {
        wifi->getTimeCtr--;
        if (wifi->getTimeCtr == 0x00) {
            PrintUsart1(g_wifiCmdTable[WIFI_GET_SNTP_TIME]);
        }
    }

    if (wifi->setSntpCtr != 0x00) {
        wifi->setSntpCtr--;
        if (wifi->setSntpCtr == 0x00) {
            PrintUsart1(g_wifiCmdTable[WIFI_SET_SNTP_CONFIG]);
            wifi->getTimeCtr = GET_TIME_DELAY_SEC;
        }
    }
}

enum WifiReceiveInfo WIFI_ReceiveProcess(struct Esp8266GetTimeType *wifi, uint8_t *buf)
{
    enum WifiReceiveInfo info;
    char *strPosition;

    if (wifi == NULL || buf == NULL) {
        return WIFI_DISCONNECT;
    }

    for (info = WIFI_DISCONNECT; info < sizeof(g_wifiRxHandlerTable) / sizeof(g_wifiRxHandlerTable[0]); info++) {
        strPosition = strstr((char *)buf, g_wifiRxHandlerTable[info].str);
        if (strPosition != NULL) {
            g_wifiRxHandlerTable[info].handler(wifi, strPosition);
            break;
        }
    }

    return (wifi->rxInfoCtr);
}

#endif
