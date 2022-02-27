#include "emw3060_at.h"
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "usart.h"
#include "time.h"
#include "trace_printf.h"

#if (WIFI_MODULE == WIFI_EMW3060)

#define GET_TIME_8S (8)
#define GET_TIME_10S (10)
#define GET_TIME_14S (14)
#define GET_TIME_16S (16)

enum ConnectFlag {
    DISCONNECT,
    CONNECT,
};

#define WIFI_OFF_TIME (2 * 60) /* 2min */
#define EMW3060_AT_POWER_PIN_HIGH()                                            \
    do {                                                                       \
        HAL_GPIO_WritePin(WIFI_POWER_GPIO_Port, WIFI_POWER_Pin, GPIO_PIN_SET); \
    } while (0)
#define EMW3060_AT_POWER_PIN_LOW()                                               \
    do {                                                                         \
        HAL_GPIO_WritePin(WIFI_POWER_GPIO_Port, WIFI_POWER_Pin, GPIO_PIN_RESET); \
    } while (0)

#define WIFI_NAME_PASSWD "AT+WJAP=HSG2,13537011631\r\n"

struct Esp8266GetTimeType {
    uint8_t powerOffCtr;
    uint8_t getTimeCtr;
    uint8_t renewInitCtr1;
    uint8_t renewInitCtr2;
    enum ConnectFlag connect;
};
struct Esp8266GetTimeType g_getTime;

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

static bool ProcessClock(char *cRxBuf)
{
    uint16_t year;
    uint8_t month, day, hour, minute, second;
    struct TimeType time;
    bool status = true;

    year = AscToHex(cRxBuf[10]) * 1000 + AscToHex(cRxBuf[11]) * 100 + AscToHex(cRxBuf[12]) * 10 + AscToHex(cRxBuf[13]);
    month = AscToHex(cRxBuf[15]) * 10 + AscToHex(cRxBuf[16]);
    day = AscToHex(cRxBuf[18]) * 10 + AscToHex(cRxBuf[19]);
    hour = AscToHex(cRxBuf[21]) * 10 + AscToHex(cRxBuf[22]);
    minute = AscToHex(cRxBuf[24]) * 10 + AscToHex(cRxBuf[25]);
    second = AscToHex(cRxBuf[27]) * 10 + AscToHex(cRxBuf[28]);

    if (year == 1970) {
        status = false;
    } else {
        if ((year < 2000) || (year > 2099) || (month == 0) || (month > 12) || (day == 0) || (day > 31) || (hour > 23) ||
            (minute > 59) || (second > 59)) {
            status = false;
        }
    }

    if (status == true) {
        time.year = year;
        time.month = month;
        time.day = day;
        time.hour = hour;
        time.min = minute;
        time.sec = second;
        CalculateWeek(time.year, time.month, time.day, &time.week);

        HAL_UART_DeInit(&huart1);
        g_getTime.powerOffCtr = 0x00;
        EMW3060_AT_POWER_PIN_LOW();
        SetClock(&time);
        TRACE_PRINTF("update time: %d-%d-%d %02d:%02d:%02d \r\n", time.year, time.month, time.day, time.hour,
                     time.min, time.sec);
    } else {
        g_getTime.getTimeCtr = GET_TIME_10S;
    }

    return status;
}

static void EMW3060_AT_GetTime(void)
{
    printf("AT+SNTPTIME\r\n");
}

static void EMW3060_AT_SNTP_CFG(void)
{
    printf("AT+SNTPCFG=+8,cn.ntp.org.cn,pool.ntp.org\r\n");
}

void WIFI_Init(void)
{
    printf("AT+FWVER?\r\n");
    HAL_Delay(1000);

    printf("AT+WFVER?\r\n");
    HAL_Delay(100);

    printf("AT+WMAC?\r\n");
    HAL_Delay(100);

    printf(WIFI_NAME_PASSWD);
}

void WIFI_PowerOnOff(enum PowerFlag flag)
{
    if (flag == POWER_ON) {
        EMW3060_AT_POWER_PIN_HIGH();
        g_getTime.powerOffCtr = WIFI_OFF_TIME;
    } else {
        EMW3060_AT_POWER_PIN_LOW();
        g_getTime.connect = DISCONNECT;
        g_getTime.getTimeCtr = 0x00;
        g_getTime.powerOffCtr = 0x00;
        g_getTime.renewInitCtr1 = 0x00;
        g_getTime.renewInitCtr2 = 0x00;
    }
}

void WIFI_CtrDec(void)
{
    if (g_getTime.powerOffCtr) {
        g_getTime.powerOffCtr--;
        if (g_getTime.powerOffCtr == 0x00) {
            WIFI_PowerOnOff(POWER_OFF);
        }
    }

    if (g_getTime.getTimeCtr) {
        g_getTime.getTimeCtr--;
        if (g_getTime.getTimeCtr == GET_TIME_14S) {
            EMW3060_AT_SNTP_CFG();
        } else if (g_getTime.getTimeCtr == GET_TIME_8S) {
            EMW3060_AT_GetTime();
        } else if (g_getTime.getTimeCtr == 0x00) {
            EMW3060_AT_GetTime();
        }
    }

    if (g_getTime.renewInitCtr1) {
        g_getTime.renewInitCtr1--;
        if (g_getTime.renewInitCtr1 == 0x00) {
            g_getTime.renewInitCtr2 = 2;
            WIFI_PowerOnOff(POWER_ON);
        }
    }

    if (g_getTime.renewInitCtr2) {
        g_getTime.renewInitCtr2--;
        if (g_getTime.renewInitCtr2 == 0x00) {
            WIFI_Init();
        }
    }
}

void WIFI_ReceiveProcess(uint8_t *buf)
{
    char *str;
    char *strPosition;

    str = "+WEVENT:STATION_UP"; /* WIFI CONNECTED */
    if (strstr((char *)buf, str) != NULL) {
        g_getTime.connect = CONNECT;
        g_getTime.getTimeCtr = GET_TIME_16S;
    }
    str = "+WEVENT:STATION_DOWN"; /* WIFI DISCONNECT */
    if (strstr((char *)buf, str) != NULL) {
        WIFI_PowerOnOff(POWER_OFF);
        g_getTime.renewInitCtr1 = 2;
    }
    // str = "+CWJAP:2";//"WIFI CONNECT FAIL";
    // if (strstr(buf, str) != NULL) {
    //   WIFI_PowerOnOff(_POWER_OFF_);
    //   EMW3060_AT_RenewInitCtr = 2;
    // }
    if (g_getTime.connect == CONNECT) {
        str = "+SNTPTIME:20";
        strPosition = strstr((char *)buf, str);
        if (strPosition != NULL) {
            ProcessClock(strPosition);
        }
    }
}

#endif
