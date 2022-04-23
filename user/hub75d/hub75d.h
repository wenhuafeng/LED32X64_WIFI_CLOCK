#ifndef HUB75D_H
#define HUB75D_H

#include <stdint.h>
#include <stdbool.h>
#include "lunar_calendar.h"
#include "time_run.h"

enum DispTime {
    DISP_TIME_OFF = 0,
    DISP_TIME     = (5 * 60),
};

enum DispTH {
    DISP_T,
    DISP_H,
};

enum {
    DISP_OFF = 0,
    DISP_ON  = 1,
};

struct CalendarDecimal {
    uint8_t yearH;
    uint8_t yearL;
    uint8_t week;
    uint8_t monthH;
    uint8_t monthL;
    uint8_t dayH;
    uint8_t dayL;
    uint8_t hourH;
    uint8_t hourL;
    uint8_t minH;
    uint8_t minL;
    uint8_t secH;
    uint8_t secL;
    uint8_t colon;
};

#define WORD_COUNT    16
#define SCAN_ALL_LINE 16
struct RgbType {
    uint8_t r[WORD_COUNT][SCAN_ALL_LINE];
    uint8_t g[WORD_COUNT][SCAN_ALL_LINE];
    uint8_t b[WORD_COUNT][SCAN_ALL_LINE];
};

struct Hub75dType {
    struct CalendarDecimal calendarDecimal;
    struct LunarCalendarType lunarCalendar;
    enum DispTH displayTh;
    uint16_t displayCount;
    int16_t temperature;
    uint16_t humidity;
    struct RgbType rgb;
    struct RgbType rgbScan;
};

void HUB75D_DispScan(struct RgbType *rgb);
void HUB75D_GetCalendar(struct CalendarDecimal *calendar, struct TimeType *time);
bool HUB75D_CtrDec(struct Hub75dType *hub75d);
void HUB75D_Disp(uint16_t *count, enum DispTime time);
void HUB75D_GetScanRgb(struct Hub75dType *hub75d);
void HUB75D_Init(void);

#endif
