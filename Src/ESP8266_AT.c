#include "esp8266_at.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "type_define.h"
#include "main.h"
#include "usart.h"
#include "time.h"

#if (WIFI_MODULE == WIFI_ESP8266)

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

#define GET_TIME_6S     (6)
#define GET_TIME_8S     (8)
#define GET_TIME_10S    (10)
#define GET_TIME_14S    (14)

enum ConnectFlag {
    DISCONNECT,
    CONNECT
};

#define _WIFI_OFF_TIME_ (2 * 60) //2min
#define ESP8266_AT_POWER_PIN_HIGH()                                            \
    do {                                                                       \
        HAL_GPIO_WritePin(WIFI_POWER_GPIO_Port, WIFI_POWER_Pin, GPIO_PIN_SET); \
    } while (0)
#define ESP8266_AT_POWER_PIN_LOW()                              \
    do {                                                        \
        HAL_GPIO_WritePin(WIFI_POWER_GPIO_Port, WIFI_POWER_Pin, \
                          GPIO_PIN_RESET);                      \
    } while (0)

static u8 g_powerOffCtr;
static u8 g_getTimeCtr;
static u8 g_renewInitCtr;
static u8 g_renewInitCtr_1;

static enum ConnectFlag g_connect;
static BOOLEAN g_timeDataOkFlag;

static u8 AscToHex(u8 asc)
{
    u8 hex = asc;

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

uint8_t GetMonth(uint32_t monthString)
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

static u8 ProcessClock(char *cRxBuf)
{
    u16 year;
    u8 month, day, week, hour, minute, second;
    u32 i;
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

    day = AscToHex(cRxBuf[21]) * 10 + AscToHex(cRxBuf[22]);
    hour = AscToHex(cRxBuf[24]) * 10 + AscToHex(cRxBuf[25]);
    minute = AscToHex(cRxBuf[27]) * 10 + AscToHex(cRxBuf[28]);
    second = AscToHex(cRxBuf[30]) * 10 + AscToHex(cRxBuf[31]);
    year = AscToHex(cRxBuf[33]) * 1000 + AscToHex(cRxBuf[34]) * 100 +
           AscToHex(cRxBuf[35]) * 10 + AscToHex(cRxBuf[36]);

    if (year == 1970) {
        status = false;
    } else {
        if ((year < 2000) || (year > 2099) || (month == 0) || (month > 12) ||
            (day == 0) || (day > 31) || (hour > 23) || (minute > 59) || (second > 59)) {
            status = false;
        }
    }

    if (status == true) {
        time.year = year;
        time.month = month;
        time.day = day;
        time.week = week;
        time.hour = hour;
        time.min = minute;
        time.sec = second;

        HAL_UART_DeInit(&huart1);
        g_powerOffCtr = 0x00;
        ESP8266_AT_POWER_PIN_LOW();
        SetClock(&time);
    } else {
        g_getTimeCtr = GET_TIME_10S;
    }

    return status;
}

static void ESP8266_AT_GetTime(void)
{
    printf("AT+CIPSNTPTIME?\r\n");
}

static void ESP8266_AT_SNTP_CFG(void)
{
    printf("AT+CIPSNTPCFG=1,8\r\n");
}

void WIFI_Init(void)
{
    printf("AT+RST\r\n");
    HAL_Delay(1000);

    printf("AT+CWMODE_DEF=1\r\n");
    HAL_Delay(100);

    printf("AT+CWAUTOCONN=1\r\n");
    HAL_Delay(100);

    printf("AT+CWJAP_DEF=\"ABC\",\"WenHuaFeng547566993\"\r\n");
}

void WIFI_PowerOnOff(enum PowerFlag flag)
{
    if (flag == POWER_ON) {
        ESP8266_AT_POWER_PIN_HIGH();
        g_powerOffCtr = _WIFI_OFF_TIME_;
    } else {
        ESP8266_AT_POWER_PIN_LOW();
        g_connect = DISCONNECT;
        g_getTimeCtr = 0x00;
        g_powerOffCtr = 0x00;
        g_renewInitCtr = 0x00;
        g_renewInitCtr_1 = 0x00;
    }
}

void WIFI_CtrDec(void)
{
    if (g_powerOffCtr) {
        g_powerOffCtr--;
        if (g_powerOffCtr == 0x00) {
            // off wifi. 5min rx time fail.
            WIFI_PowerOnOff(POWER_OFF);
        }
    }

    if (g_getTimeCtr) {
        g_getTimeCtr--;
        if (g_getTimeCtr == GET_TIME_10S) {
            ESP8266_AT_SNTP_CFG();
        } else if (g_getTimeCtr == GET_TIME_8S) {
            ESP8266_AT_GetTime();
        } else if (g_getTimeCtr == GET_TIME_6S) {
            ESP8266_AT_GetTime();
        } else if (g_getTimeCtr == 0x00) {
            ESP8266_AT_GetTime();
        }
    }

    if (g_renewInitCtr) {
        g_renewInitCtr--;
        if (g_renewInitCtr == 0x00) {
            g_renewInitCtr_1 = 2;
            WIFI_PowerOnOff(POWER_ON);
        }
    }

    if (g_renewInitCtr_1) {
        g_renewInitCtr_1--;
        if (g_renewInitCtr_1 == 0x00) {
            WIFI_Init();
        }
    }
}

void WIFI_ReceiveProcess(u8 *buf)
{
    char *str;
    char *strPosition;

    str = "WIFI CON"; /* WIFI CONNECTED */
    if (strstr((char *)buf, str) != NULL) {
        g_connect = CONNECT;
        g_getTimeCtr = GET_TIME_14S;
    }
    str = "WIFI DIS"; /* WIFI DISCONNECT */
    if (strstr((char *)buf, str) != NULL) {
        WIFI_PowerOnOff(POWER_OFF);
        g_renewInitCtr = 2;
    }
    str = "+CWJAP:2"; /* WIFI CONNECT FAIL */
    if (strstr((char *)buf, str) != NULL) {
        WIFI_PowerOnOff(POWER_OFF);
        g_renewInitCtr = 2;
    }
    if (g_connect == CONNECT) {
        str = "+CIPSNTPTIME:";
        strPosition = strstr((char *)buf, str);
        if (strPosition != NULL) {
            ProcessClock(strPosition);
            g_timeDataOkFlag = 1;
        }
    }
}

BOOLEAN WIFI_GetTimeDataFlag(void)
{
    return g_timeDataOkFlag;
}

void WIFI_SetTimeDataFlag(BOOLEAN value)
{
    g_timeDataOkFlag = value;
}

#endif
