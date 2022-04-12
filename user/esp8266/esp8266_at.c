#include "esp8266_at.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "gpio_bit_ctrl.h"
#include "usart.h"
#include "time_run.h"
#include "lunar_calendar.h"
#include "trace_printf.h"
#include "time_stamp.h"

#if (WIFI_MODULE == WIFI_ESP8266)

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
#define WIFI_ON_TIME        (3 * 60) /* 3 min */
#define WIFI_GET_TIME_TIMES (5U)
#define WIFI_SET_SNTP_TIMES (5U)

#define SET_SNTP_DELAY_SEC (2U)
#define GET_TIME_DELAY_SEC (2U)

/* wifi init hander */
#define WIFI_INIT_TOTAL_STEP (6U)

enum WifiCmdType {
    WIFI_INIT_REST,
    WIFI_INIT_CWMODE,
    WIFI_INIT_NAME_PASSWD,
    WIFI_SET_SNTP_CONFIG,
    WIFI_GET_SNTP_TIME,
    WIFI_GET_VERSION,
};

const char *g_wifiCmdTable[WIFI_INIT_TOTAL_STEP] = {
    "AT+RST\r\n",
    "AT+CWMODE=1\r\n",
    "AT+CWJAP_DEF=\"HSG2\",\"13537011631\"\r\n",
    "AT+CIPSNTPCFG=1,8\r\n",
    "AT+CIPSNTPTIME?\r\n",
    "AT+GMR\r\n",
};

/* wifi receive data handler */
#define WIFI_RECEIVE_INFO_SIZE 5

enum WifiReceiveInfo {
    WIFI_DISCONNECT,
    WIFI_CONNECTED,
    WIFI_GOT_IP,
    WIFI_CONNECT_FAIL,
    WIFI_RETURN_TIME,
};

enum WifiInitStatus {
    WIFI_NOT_INIT,
    WIFI_INIT_COMPLETE,
};

enum WifiConnectStatus {
    WIFI_IS_DISCONNECT,
    WIFI_IS_CONNECTED,
};

struct Esp8266GetTimeType {
    enum WifiConnectStatus wifiConnectedStatus;
    uint8_t wifiPowerOffTime;
    uint8_t setSntpCtr;
    uint8_t getTimeTimes;
    uint8_t getTimeCtr;
    enum WifiCmdType cmdType;
    enum WifiReceiveInfo rxInfoCtr;
};
struct Esp8266GetTimeType g_getTime;
static enum WifiInitStatus g_wifiInitComplete = WIFI_NOT_INIT;

typedef void (*WifiReceiveInfoHandler)(char *buf);
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

    TRACE_PRINTF("week:%d\r\n", i);
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

    TRACE_PRINTF("month:%d\r\n", i);
    return i;
}

static bool ProcessClock(char *cRxBuf)
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t week;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    char str[4] = {0};
    struct TimeType time;
    int ret;

    ret = snprintf(str, sizeof(str), "%s", &cRxBuf[WEEK_STR_INDEX]);
    if (ret <= 0) {
        TRACE_PRINTF("%s snprintf error!\r\n", __LINE__);
        return false;
    }
    week = GetWeek(str);

    ret = snprintf(str, sizeof(str), "%s", &cRxBuf[MONTH_STR_INDEX]);
    if (ret <= 0) {
        TRACE_PRINTF("%s snprintf error!\r\n", __LINE__);
        return false;
    }
    month = GetMonth(str);

    day    = AscToHex(cRxBuf[DAY_STR_INDEX_HIGH]) * 10 + AscToHex(cRxBuf[DAY_STR_INDEX_LOW]);
    hour   = AscToHex(cRxBuf[HOUR_STR_INDEX_HIGH]) * 10 + AscToHex(cRxBuf[HOUR_STR_INDEX_LOW]);
    minute = AscToHex(cRxBuf[MIN_STR_INDEX_HIGH]) * 10 + AscToHex(cRxBuf[MIN_STR_INDEX_LOW]);
    second = AscToHex(cRxBuf[SEC_STR_INDEX_HIGH]) * 10 + AscToHex(cRxBuf[SEC_STR_INDEX_LOW]);
    year   = AscToHex(cRxBuf[YEAR_STR_INDEX_THOUSAND]) * 1000 + AscToHex(cRxBuf[YEAR_STR_INDEX_HUNDRED]) * 100 +
             AscToHex(cRxBuf[YEAR_STR_INDEX_TEN]) * 10 + AscToHex(cRxBuf[YEAR_STR_INDEX]);

    if (year < YEAR_MIN || year > YEAR_MAX) {
        return false;
    }
    if (month < MONTH_MIN || month > MONTH_MAX) {
        return false;
    }
    if (day < DAY_MIN || day > DAY_MAX) {
        return false;
    }
    if (week > WEEK_MAX) {
        return false;
    }
    if (hour > HOUR_MAX) {
        return false;
    }
    if (minute > MIN_MAX) {
        return false;
    }
    if (second > SEC_MAX) {
        return false;
    }

    time.year  = year;
    time.month = month;
    time.day   = day;
    time.week  = week;
    time.hour  = hour;
    time.min   = minute;
    time.sec   = second;

    SetTimeData(&time);
    SetClock(&time);
    CalculationLunarCalendar(&time);
    TimeConvertTimestamp(&time);

    return true;
}

static void WIFI_Disconnect(char *buf)
{
    TRACE_PRINTF("wifi disconnect\r\n");
    printf(g_wifiCmdTable[WIFI_INIT_NAME_PASSWD]);
    g_getTime.wifiConnectedStatus = WIFI_IS_DISCONNECT;
    g_getTime.rxInfoCtr           = WIFI_DISCONNECT;
}

static void WIFI_Connected(char *buf)
{
    TRACE_PRINTF("wifi connected\r\n");
    g_getTime.wifiConnectedStatus = WIFI_IS_CONNECTED;
    g_getTime.rxInfoCtr           = WIFI_CONNECTED;
}

static void WIFI_GotIp(char *buf)
{
    TRACE_PRINTF("wifi got ip\r\n");
    if (g_getTime.wifiConnectedStatus == WIFI_IS_CONNECTED) {
        g_getTime.setSntpCtr = SET_SNTP_DELAY_SEC;
    }

    g_getTime.rxInfoCtr = WIFI_GOT_IP;
}

static void WIFI_ConnectedFail(char *buf)
{
    TRACE_PRINTF("wifi connect fail\r\n");
    printf(g_wifiCmdTable[WIFI_INIT_NAME_PASSWD]);
    g_getTime.rxInfoCtr = WIFI_CONNECT_FAIL;
}

static void WIFI_ReturnTime(char *buf)
{
    bool ret;
    TRACE_PRINTF("wifi return time\r\n");

    ret = ProcessClock(buf);
    if (ret == false) {
        TRACE_PRINTF("process clock error!\r\n");
        if (g_getTime.getTimeTimes < WIFI_GET_TIME_TIMES) {
            g_getTime.getTimeTimes++;
            g_getTime.getTimeCtr = GET_TIME_DELAY_SEC;
        } else {
            g_getTime.getTimeTimes = 0x00;
            WIFI_Power(POWER_OFF);
            WIFI_ReInit();
        }
    } else {
        WIFI_Power(POWER_OFF);
    }

    g_getTime.rxInfoCtr = WIFI_RETURN_TIME;
}

static const struct WifiRxType g_wifiRxHandlerTable[] = {
    { WIFI_DISCONNECT, "WIFI DISCONNECT", WIFI_Disconnect },
    { WIFI_CONNECTED, "WIFI CONNECTED", WIFI_Connected },
    { WIFI_GOT_IP, "WIFI GOT IP", WIFI_GotIp },
    { WIFI_CONNECT_FAIL, "+CWJAP:3", WIFI_ConnectedFail },
    { WIFI_RETURN_TIME, "+CIPSNTPTIME:", WIFI_ReturnTime },
};

void WIFI_ReInit(void)
{
    g_wifiInitComplete = WIFI_NOT_INIT;
}

bool WIFI_Init(void)
{
    if (g_wifiInitComplete == WIFI_INIT_COMPLETE) {
        return true;
    }
    if (g_getTime.wifiConnectedStatus == WIFI_IS_CONNECTED) {
        return true;
    }
    if (g_getTime.cmdType > WIFI_INIT_NAME_PASSWD) {
        g_wifiInitComplete = WIFI_INIT_COMPLETE;
        return true;
    }

    printf(g_wifiCmdTable[g_getTime.cmdType]);
    TRACE_PRINTF("wifi init:%s\r\n", g_wifiCmdTable[g_getTime.cmdType]);
    g_getTime.cmdType++;

    return false;
}

void WIFI_Power(enum PowerFlag flag)
{
    if (flag == POWER_ON) {
        WIFI_CH_PD                 = 1;
        g_getTime.wifiPowerOffTime = WIFI_ON_TIME;
        TRACE_PRINTF("wifi power on\r\n");
    } else {
        WIFI_CH_PD = 0;
        memset(&g_getTime, 0, sizeof(struct Esp8266GetTimeType));
        TRACE_PRINTF("wifi power off\r\n");
    }
}

void WIFI_GetTime(void)
{
    if (WIFI_Init() == false) {
        return;
    }

    if (g_getTime.wifiPowerOffTime != 0x00) {
        g_getTime.wifiPowerOffTime--;
        if (g_getTime.wifiPowerOffTime == 0x00) {
            WIFI_Power(POWER_OFF);
        }
    }

    if (g_getTime.getTimeCtr != 0x00) {
        g_getTime.getTimeCtr--;
        if (g_getTime.getTimeCtr == 0x00) {
            printf(g_wifiCmdTable[WIFI_GET_SNTP_TIME]);
        }
    }

    if (g_getTime.setSntpCtr != 0x00) {
        g_getTime.setSntpCtr--;
        if (g_getTime.setSntpCtr == 0x00) {
            printf(g_wifiCmdTable[WIFI_SET_SNTP_CONFIG]);
            g_getTime.getTimeCtr = GET_TIME_DELAY_SEC;
        }
    }
}

void WIFI_ReceiveProcess(uint8_t *buf)
{
    enum WifiReceiveInfo info;
    char *strPosition;

    for (info = WIFI_DISCONNECT; info < sizeof(g_wifiRxHandlerTable) / sizeof(g_wifiRxHandlerTable[0]); info++) {
        strPosition = strstr((char *)buf, g_wifiRxHandlerTable[info].str);
        if (strPosition != NULL) {
            g_wifiRxHandlerTable[info].handler(strPosition);
            break;
        }
    }
}

#endif
