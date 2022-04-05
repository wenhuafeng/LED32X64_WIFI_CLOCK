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

#define WIFI_CH_PD PBout(5)

#define MON 0x004d6f6e
#define TUE 0x00547565
#define WED 0x00576564
#define THU 0x00546875
#define FRI 0x00467269
#define SAT 0x00536174
#define SUN 0x0053756e

#define JAN 0x004a616e
#define FEB 0x00466562
#define MAR 0x004d6172
#define APR 0x00417072
#define MAY 0x004d6179
#define JUN 0x004a756e
#define JUL 0x004a756c
#define AUG 0x00417567
#define SEP 0x00536570
#define OCT 0x004f6374
#define NOV 0x004e6f76
#define DEC 0x00446563

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
    "AT+CIPSNTPTIME?\r\n", "AT+GMR\r\n",
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

static uint8_t GetWeek(uint32_t weekString)
{
    uint8_t i;
    uint8_t tableLen;
    uint32_t weekTable[] = { SUN, MON, TUE, WED, THU, FRI, SAT };

    tableLen = sizeof(weekTable) / sizeof(uint32_t);
    for (i = 0; i < tableLen; i++) {
        if (weekTable[i] == weekString) {
            break;
        }
    }

    return i;
}

static uint8_t GetMonth(uint32_t monthString)
{
    uint8_t i;
    uint8_t tableLen;
    uint32_t monthTable[] = { 0, JAN, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC };

    tableLen = sizeof(monthTable) / sizeof(uint32_t);
    for (i = 0; i < tableLen; i++) {
        if (monthTable[i] == monthString) {
            break;
        }
    }

    return i;
}

static bool ProcessClock(char *cRxBuf)
{
    uint16_t year;
    uint8_t month, day, week, hour, minute, second;
    uint32_t i;
    struct TimeType time;
    bool status = true;

    i = cRxBuf[13];
    i = i << 16;
    i |= cRxBuf[14] << 8 | cRxBuf[15];
    week = GetWeek(i);

    i = cRxBuf[17];
    i = i << 16;
    i |= cRxBuf[18] << 8 | cRxBuf[19];
    month = GetMonth(i);

    day    = AscToHex(cRxBuf[21]) * 10 + AscToHex(cRxBuf[22]);
    hour   = AscToHex(cRxBuf[24]) * 10 + AscToHex(cRxBuf[25]);
    minute = AscToHex(cRxBuf[27]) * 10 + AscToHex(cRxBuf[28]);
    second = AscToHex(cRxBuf[30]) * 10 + AscToHex(cRxBuf[31]);
    year = AscToHex(cRxBuf[33]) * 1000 + AscToHex(cRxBuf[34]) * 100 + AscToHex(cRxBuf[35]) * 10 + AscToHex(cRxBuf[36]);

    if (year == 1970) {
        status = false;
    } else {
        if ((year < 2000) || (year > 2099) || (month == 0) || (month > 12) || (day == 0) || (day > 31) || (hour > 23) ||
            (minute > 59) || (second > 59)) {
            status = false;
        }
    }

    if (status == true) {
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
    }

    return status;
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
        WIFI_CH_PD = 1;
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
