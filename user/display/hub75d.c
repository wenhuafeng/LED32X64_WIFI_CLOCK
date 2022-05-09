#include "hub75d.h"
#include <stdbool.h>
#include <string.h>
#include "tim.h"
#include "gpio_bit_ctrl.h"
#include "trace.h"

#define LOG_TAG "hub75d"

#define TEMP_HUMI_DISP_TIME (1) /* seconds */

#define HUB_A   PBout(13)
#define HUB_B   PBout(14)
#define HUB_C   PBout(15)
#define HUB_D   PAout(8)
#define HUB_OE  PBout(4)
#define HUB_LAT PBout(3)
#define HUB_CLK PAout(15)
#define HUB_R1  PAout(6)
#define HUB_G1  PAout(5)
#define HUB_B1  PAout(7)
#define HUB_R2  PBout(0)
#define HUB_G2  PBout(1)
#define HUB_B2  PBout(10)

#define HUB75D_DISP_POWER_PIN PBout(11)

#define CHINESE_YEAR_INDEX           10
#define CHINESE_MONTH_INDEX          9
#define CHINESE_DAY_INDEX            8
#define CHINESE_LUNAR_CALENDAR_INDEX 7

#define TEMP_DISP_NULL_INDEX           11
#define TEMP_NEGATIVE_SIGN_INDEX       12
#define TEMP_HUMI_DOT_INDEX            13
#define TEMP_CELSIUS_DEGREE_ICON_INDEX 14
#define TEMP_PERCENT_SIGN_ICON_INDEX   15

#define DATE_TABLE_DISP_NULL_INDEX 11

enum {
    DISP_OFF = 0,
    DISP_ON  = 1,
};

typedef union {
    uint16_t flag;
    struct {
        uint16_t hubA : 1;
        uint16_t hubB : 1;
        uint16_t hubC : 1;
        uint16_t hubD : 1;
        uint16_t r1 : 1;
        uint16_t g1 : 1;
        uint16_t b1 : 1;
        uint16_t r2 : 1;
        uint16_t g2 : 1;
        uint16_t b2 : 1;
    } bit;
} PinFlags;

static uint8_t const g_dateTable[][8] = {
    { 0x00, 0x06, 0x09, 0x09, 0x09, 0x09, 0x09, 0x06 }, /* 0         */
    { 0x00, 0x02, 0x06, 0x02, 0x02, 0x02, 0x02, 0x07 }, /* 1         */
    { 0x00, 0x06, 0x09, 0x01, 0x02, 0x04, 0x08, 0x0f }, /* 2         */
    { 0x00, 0x06, 0x09, 0x01, 0x06, 0x01, 0x09, 0x06 }, /* 3         */
    { 0x00, 0x02, 0x06, 0x0a, 0x0a, 0x0f, 0x02, 0x02 }, /* 4         */
    { 0x00, 0x0f, 0x08, 0x0e, 0x09, 0x01, 0x09, 0x06 }, /* 5         */
    { 0x00, 0x06, 0x09, 0x08, 0x0e, 0x09, 0x09, 0x06 }, /* 6         */
    { 0x00, 0x0f, 0x01, 0x01, 0x02, 0x04, 0x04, 0x04 }, /* 7         */
    { 0x00, 0x06, 0x09, 0x09, 0x06, 0x09, 0x09, 0x06 }, /* 8         */
    { 0x00, 0x06, 0x09, 0x09, 0x07, 0x01, 0x09, 0x06 }, /* 9         */
    { 0x00, 0x00, 0x02, 0x02, 0x00, 0x02, 0x02, 0x00 }, /* 10 :      */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* 11 NULL   */
    { 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03 }, /* 13 -      */
    { 0x00, 0x00, 0x00, 0x02, 0x07, 0x02, 0x00, 0x00 }, /* 14 +      */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00 }, /*  - 15     */
    { 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 }, /*  . 16     */
    { 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00 }, /*  . 17     */
    { 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00 }, /*  - 18     */
    { 0x00, 0x00, 0x80, 0x00, 0x30, 0x40, 0x40, 0x30 }, /* 19 .c 4x7 */
};

static uint8_t const g_chineseWeekDateTable[][8] = {
    { 0x00, 0x3e, 0x22, 0x22, 0x3e, 0x22, 0x22, 0x3e }, /* 0  日  */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00 }, /* 1  一  */
    { 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x7e, 0x00 }, /* 2  二  */
    { 0x00, 0x00, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x7e }, /* 3  三  */
    { 0x00, 0x00, 0x7f, 0x55, 0x55, 0x77, 0x41, 0x7f }, /* 4  四  */
    { 0x00, 0x00, 0x3e, 0x08, 0x3e, 0x12, 0x22, 0x7f }, /* 5  五  */
    { 0x00, 0x08, 0x00, 0x7f, 0x00, 0x14, 0x22, 0x41 }, /* 6  六  */
    { 0x10, 0x7c, 0xa4, 0xb4, 0x48, 0x64, 0xb2, 0x20 }, /* 7  农  */
    { 0x00, 0x0f, 0x09, 0x09, 0x0f, 0x09, 0x09, 0x0f }, /* 8  日  */
    { 0x00, 0x0f, 0x09, 0x0f, 0x09, 0x0f, 0x09, 0x11 }, /* 9  月  */
    { 0x08, 0x1f, 0x24, 0x1f, 0x14, 0x3f, 0x04, 0x04 }, /* 10  年 */
};

static uint8_t const g_tempHumiDigitTable[][8] = {
    { 0x00, 0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70 }, /* 0       */
    { 0x00, 0x20, 0x60, 0x20, 0x20, 0x20, 0x20, 0x70 }, /* 1       */
    { 0x00, 0x70, 0x88, 0x08, 0x10, 0x20, 0x40, 0xf8 }, /* 2       */
    { 0x00, 0x70, 0x88, 0x08, 0x30, 0x08, 0x88, 0x70 }, /* 3       */
    { 0x00, 0x10, 0x30, 0x50, 0x90, 0xf8, 0x10, 0x10 }, /* 4       */
    { 0x00, 0x78, 0x80, 0xf0, 0x08, 0x08, 0x88, 0x70 }, /* 5       */
    { 0x00, 0x70, 0x80, 0x80, 0xf0, 0x88, 0x88, 0x70 }, /* 6       */
    { 0x00, 0xf0, 0x08, 0x10, 0x20, 0x40, 0x40, 0x40 }, /* 7       */
    { 0x00, 0x70, 0x88, 0x88, 0x70, 0x88, 0x88, 0x70 }, /* 8       */
    { 0x00, 0x70, 0x88, 0x88, 0x70, 0x08, 0x88, 0x70 }, /* 9       */
    { 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00 }, /* 10 .    */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* 11 NULL */
    { 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00 }, /* 12 -    */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80 }, /* 13.     */
    { 0x00, 0xc0, 0xc0, 0x30, 0x48, 0x40, 0x48, 0x30 }, /* 14 C    */
    { 0x00, 0xc8, 0xd0, 0x20, 0x58, 0x98, 0x00, 0x00 }, /* 15 %    */
};

static uint8_t const g_timeDigitTable[][16] = {
    { 0x3c, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xe7, 0x7e, 0x3c }, /* 0     */
    { 0x1c, 0x7c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x7f }, /* 1     */
    { 0x3c, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0x03, 0x03, 0x07, 0x0e, 0x1c, 0x38, 0x70, 0xe1, 0xff, 0xff }, /* 2     */
    { 0x3c, 0x7e, 0xe7, 0xc3, 0x03, 0x03, 0x03, 0x3e, 0x3e, 0x03, 0x03, 0x03, 0xc3, 0xe7, 0x7e, 0x3c }, /* 3     */
    { 0x0c, 0x0c, 0x1c, 0x1c, 0x3c, 0x3c, 0x6c, 0x6c, 0xcc, 0xcc, 0x8c, 0xff, 0xff, 0x0c, 0x0c, 0x1e }, /* 4     */
    { 0x7e, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xfe, 0x07, 0x03, 0x03, 0x03, 0xc3, 0xe7, 0x7e, 0x3c }, /* 5     */
    { 0x3c, 0x7e, 0xe7, 0xc3, 0xc0, 0xc0, 0xc0, 0xfc, 0xfe, 0xe7, 0xc3, 0xc3, 0xc3, 0xe7, 0x7e, 0x3c }, /* 6     */
    { 0x7e, 0xff, 0xc3, 0x83, 0x03, 0x03, 0x06, 0x06, 0x0c, 0x0c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c }, /* 7     */
    { 0x3c, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xe7, 0x7e, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xe7, 0x7e, 0x3c }, /* 8     */
    { 0x3c, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xe7, 0x7f, 0x3f, 0x03, 0x03, 0xc3, 0xc3, 0xe7, 0x7e, 0x3c }, /* 9     */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* 10    */
    { 0x00, 0x00, 0x18, 0x24, 0x24, 0x18, 0x00, 0x00, 0x00, 0x00, 0x18, 0x24, 0x24, 0x18, 0x00, 0x00 }, /* 11 SD */
    { 0x00, 0x3f, 0x21, 0x21, 0x21, 0x3f, 0x21, 0x21, 0x21, 0x3f, 0x21, 0x21, 0x21, 0x21, 0x45, 0x82 }, /* 12 月 */
    { 0x00, 0x00, 0x7f, 0x41, 0x41, 0x41, 0x41, 0x7f, 0x41, 0x41, 0x41, 0x41, 0x41, 0x7f, 0x41, 0x00 }, /* 13 日 */
    { 0x00, 0x7f, 0x41, 0x49, 0x5d, 0x49, 0x5d, 0x41, 0x5d, 0x55, 0x55, 0x5d, 0x55, 0x41, 0x41, 0x82 }, /* 14 周 */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* 15 一 */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x00 }, /* 16 二 */
    { 0x00, 0x00, 0x02, 0x7e, 0x00, 0x00, 0x02, 0x7e, 0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x00 }, /* 17 三 */
    { 0x00, 0x00, 0x02, 0x7e, 0x00, 0x00, 0x02, 0x7e, 0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x00 }, /* 18 四 */
    { 0x00, 0x00, 0x7e, 0x08, 0x08, 0x08, 0x0a, 0x7e, 0x12, 0x12, 0x12, 0x22, 0x22, 0xff, 0x00, 0x00 }, /* 19 五 */
    { 0x00, 0x20, 0x18, 0x0c, 0x08, 0xff, 0x00, 0x00, 0x24, 0x34, 0x24, 0x46, 0x43, 0x83, 0x82, 0x00 }, /* 20 六 */
    { 0x00, 0x00, 0x10, 0x10, 0x10, 0x1f, 0xf0, 0x10, 0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x0e, 0x00 }, /* 21 七 */
    { 0x00, 0x00, 0x04, 0x26, 0x34, 0x24, 0x24, 0x24, 0x24, 0x66, 0x62, 0x62, 0x83, 0x83, 0x81, 0x00 }, /* 22 八 */
    { 0x00, 0x20, 0x20, 0x20, 0x20, 0xfc, 0x24, 0x24, 0x24, 0x24, 0x44, 0x44, 0x45, 0x85, 0x86, 0x00 }, /* 23 九 */
    { 0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0xfe, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00 }, /* 24 十 */
    { 0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0xfe, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x7e, 0x00 }, /* 25 廿 */
    { 0x00, 0x00, 0x54, 0x54, 0x54, 0x54, 0x54, 0xfe, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x00 }, /* 26 卅 */
    { 0x00, 0x00, 0xe1, 0xa1, 0xe2, 0x02, 0x04, 0x04, 0x08, 0x10, 0x20, 0x20, 0x40, 0x47, 0x85, 0x87 }, /* 27 %  */
    { 0x00, 0x00, 0xe0, 0xa0, 0xe0, 0x00, 0x1e, 0x23, 0x41, 0x40, 0x40, 0x40, 0x40, 0x41, 0x23, 0x1e }, /* 28 c  */
};

static uint8_t const g_timeSecondDigitTable[][16] = {
    { 0x00, 0x00, 0x3e, 0x7f, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x7f, 0x3e, 0x00, 0x00 }, /* 0    */
    { 0x00, 0x00, 0x1c, 0x3c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x3e, 0x00, 0x00 }, /* 1    */
    { 0x00, 0x00, 0x3e, 0x7f, 0x63, 0x63, 0x07, 0x0e, 0x1c, 0x38, 0x70, 0x60, 0x7f, 0x7f, 0x00, 0x00 }, /* 2    */
    { 0x00, 0x00, 0x3e, 0x7f, 0x63, 0x63, 0x03, 0x06, 0x06, 0x03, 0x63, 0x63, 0x7f, 0x3e, 0x00, 0x00 }, /* 3    */
    { 0x00, 0x00, 0x1e, 0x1e, 0x36, 0x36, 0x36, 0x66, 0x66, 0x7f, 0x7f, 0x06, 0x06, 0x06, 0x00, 0x00 }, /* 4    */
    { 0x00, 0x00, 0x3f, 0x7f, 0x60, 0x60, 0x60, 0x7e, 0x3f, 0x03, 0x03, 0x63, 0x7f, 0x3e, 0x00, 0x00 }, /* 5    */
    { 0x00, 0x00, 0x3e, 0x7f, 0x63, 0x60, 0x60, 0x7e, 0x7f, 0x63, 0x63, 0x63, 0x7f, 0x3e, 0x00, 0x00 }, /* 6    */
    { 0x00, 0x00, 0x3e, 0x7f, 0x63, 0x63, 0x03, 0x06, 0x06, 0x06, 0x0c, 0x0c, 0x0c, 0x0c, 0x00, 0x00 }, /* 7    */
    { 0x00, 0x00, 0x3e, 0x7f, 0x63, 0x63, 0x63, 0x3e, 0x3e, 0x63, 0x63, 0x63, 0x7f, 0x3e, 0x00, 0x00 }, /* 8    */
    { 0x00, 0x00, 0x3e, 0x7f, 0x63, 0x63, 0x63, 0x7f, 0x3f, 0x03, 0x03, 0x63, 0x7f, 0x3e, 0x00, 0x00 }, /* 9    */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x00, 0x00, 0x06, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* 10 : */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* 11   */
};

static void DispTime(struct Hub75dType *hub75d, uint8_t index)
{
    uint8_t h1 = hub75d->calendarDecimal.hourH;
    uint8_t h0 = hub75d->calendarDecimal.hourL;
    uint8_t m1 = hub75d->calendarDecimal.minH;
    uint8_t m0 = hub75d->calendarDecimal.minL;
    uint8_t s1 = hub75d->calendarDecimal.secH;
    uint8_t s0 = hub75d->calendarDecimal.secL;
    uint8_t sd = hub75d->calendarDecimal.colon;

    if (h1 == 0) {
        h1 = 10;
    }

    /* 8 * 8 = 64 */
    hub75d->rgb.r[0][index] |= g_timeDigitTable[h1][index];
    hub75d->rgb.g[0][index] |= g_timeDigitTable[h1][index];
    hub75d->rgb.b[0][index] |= g_timeDigitTable[h1][index];

    hub75d->rgb.r[1][index] |= (g_timeDigitTable[h0][index] >> 1);
    hub75d->rgb.g[1][index] |= (g_timeDigitTable[h0][index] >> 1);
    hub75d->rgb.b[1][index] |= (g_timeDigitTable[h0][index] >> 1);

    hub75d->rgb.r[2][index] |= (g_timeDigitTable[h0][index] << 7);
    hub75d->rgb.g[2][index] |= (g_timeDigitTable[h0][index] << 7);
    hub75d->rgb.b[2][index] |= (g_timeDigitTable[h0][index] << 7);

    hub75d->rgb.r[2][index] |= g_timeDigitTable[sd][index];
    hub75d->rgb.b[2][index] |= g_timeDigitTable[sd][index];

    hub75d->rgb.r[3][index] |= g_timeDigitTable[m1][index];
    hub75d->rgb.g[3][index] |= g_timeDigitTable[m1][index];
    hub75d->rgb.b[3][index] |= g_timeDigitTable[m1][index];

    hub75d->rgb.r[4][index] |= (g_timeDigitTable[m0][index] >> 1);
    hub75d->rgb.g[4][index] |= (g_timeDigitTable[m0][index] >> 1);
    hub75d->rgb.b[4][index] |= (g_timeDigitTable[m0][index] >> 1);

    hub75d->rgb.r[5][index] |= (g_timeDigitTable[m0][index] << 7);
    hub75d->rgb.g[5][index] |= (g_timeDigitTable[m0][index] << 7);
    hub75d->rgb.b[5][index] |= (g_timeDigitTable[m0][index] << 7);

    hub75d->rgb.r[5][index] |= g_timeDigitTable[sd][index];
    hub75d->rgb.b[5][index] |= g_timeDigitTable[sd][index];

    hub75d->rgb.r[6][index] |= g_timeSecondDigitTable[s1][index];
    hub75d->rgb.g[6][index] |= g_timeSecondDigitTable[s1][index];
    hub75d->rgb.b[6][index] |= g_timeSecondDigitTable[s1][index];

    hub75d->rgb.r[7][index] |= g_timeSecondDigitTable[s0][index];
    hub75d->rgb.g[7][index] |= g_timeSecondDigitTable[s0][index];
    hub75d->rgb.b[7][index] |= g_timeSecondDigitTable[s0][index];
}

static void DispDate(struct Hub75dType *hub75d, uint8_t index)
{
    uint8_t y1 = hub75d->calendarDecimal.yearH;
    uint8_t y0 = hub75d->calendarDecimal.yearL;
    uint8_t m1 = hub75d->calendarDecimal.monthH;
    uint8_t m0 = hub75d->calendarDecimal.monthL;
    uint8_t d1 = hub75d->calendarDecimal.dayH;
    uint8_t d0 = hub75d->calendarDecimal.dayL;
    uint8_t wk = hub75d->calendarDecimal.week;

    hub75d->rgb.g[8][index] |= (g_dateTable[2][index] << 4);
    hub75d->rgb.g[8][index] |= (g_dateTable[0][index] >> 1);
    hub75d->rgb.g[9][index] |= (g_dateTable[0][index] << 7);
    hub75d->rgb.g[9][index] |= (g_dateTable[y1][index] << 2);
    hub75d->rgb.g[9][index] |= (g_dateTable[y0][index] >> 3);
    for (uint8_t j = 8; j < 10; j++) {
        hub75d->rgb.r[j][index] = hub75d->rgb.g[j][index];
    }

    hub75d->rgb.r[10][index] |= (g_dateTable[y0][index] << 5);
    hub75d->rgb.g[10][index] |= (g_dateTable[y0][index] << 5);
    hub75d->rgb.g[10][index] |= (g_chineseWeekDateTable[CHINESE_YEAR_INDEX][index] >> 2);
    hub75d->rgb.g[11][index] |= (g_chineseWeekDateTable[CHINESE_YEAR_INDEX][index] << 6);

    if (m1 == 0) {
        m1 = DATE_TABLE_DISP_NULL_INDEX;
    }
    hub75d->rgb.r[11][index] |= (g_dateTable[m1][index] << 1);
    hub75d->rgb.g[11][index] |= (g_dateTable[m1][index] << 1);
    hub75d->rgb.r[12][index] |= (g_dateTable[m0][index] << 4);
    hub75d->rgb.g[12][index] |= (g_dateTable[m0][index] << 4);
    hub75d->rgb.g[12][index] |= (g_chineseWeekDateTable[CHINESE_MONTH_INDEX][index] >> 1);
    hub75d->rgb.g[13][index] |= (g_chineseWeekDateTable[CHINESE_MONTH_INDEX][index] << 7);

    if (d1 == 0) {
        d1 = DATE_TABLE_DISP_NULL_INDEX;
    }
    hub75d->rgb.r[13][index] |= (g_dateTable[d1][index] << 2);
    hub75d->rgb.g[13][index] |= (g_dateTable[d1][index] << 2);
    hub75d->rgb.r[13][index] |= (g_dateTable[d0][index] >> 3);
    hub75d->rgb.g[13][index] |= (g_dateTable[d0][index] >> 3);

    hub75d->rgb.r[14][index] |= (g_dateTable[d0][index] << 5);
    hub75d->rgb.g[14][index] |= (g_dateTable[d0][index] << 5);
    hub75d->rgb.g[14][index] |= (g_chineseWeekDateTable[CHINESE_DAY_INDEX][index]);
    hub75d->rgb.r[15][index] |= g_chineseWeekDateTable[wk][index];
}

static void DispLunarCalendar(struct Hub75dType *hub75d, uint8_t index)
{
    uint8_t m1         = hub75d->lunarCalendar.month / 10;
    uint8_t m0         = hub75d->lunarCalendar.month % 10;
    uint8_t d1         = hub75d->lunarCalendar.day / 10;
    uint8_t d0         = hub75d->lunarCalendar.day % 10;
    uint8_t tableIndex = index - (SCAN_ALL_LINE / 2);

    hub75d->rgb.g[8][index] |= g_chineseWeekDateTable[CHINESE_LUNAR_CALENDAR_INDEX][tableIndex];

    if (m1 == 0) {
        m1 = DATE_TABLE_DISP_NULL_INDEX;
    }
    hub75d->rgb.r[9][index] |= (g_dateTable[m1][tableIndex] << 4);
    hub75d->rgb.r[9][index] |= (g_dateTable[m0][tableIndex] >> 1);
    hub75d->rgb.r[10][index] |= (g_dateTable[m0][tableIndex] << 7);
    hub75d->rgb.g[10][index] |= (g_chineseWeekDateTable[CHINESE_MONTH_INDEX][tableIndex] << 1);

    if (d1 == 0) {
        d1 = DATE_TABLE_DISP_NULL_INDEX;
    }
    hub75d->rgb.r[11][index] |= (g_dateTable[d1][tableIndex] << 4);
    hub75d->rgb.r[11][index] |= (g_dateTable[d0][tableIndex] >> 1);
    hub75d->rgb.r[12][index] |= (g_dateTable[d0][tableIndex] << 7);
    hub75d->rgb.g[12][index] |= (g_chineseWeekDateTable[CHINESE_DAY_INDEX][tableIndex] << 2);
}

static void DispTemperatureHumidity(struct Hub75dType *hub75d, uint8_t index)
{
    bool sign = false;
    uint8_t t1, t0, td;
    uint16_t tmp;
    uint8_t tableIndex = index - (SCAN_ALL_LINE / 2);

    if (hub75d->displayTh == DISP_T) {
        if (hub75d->tempHumi.temperature < 0) {
            sign = true;
            tmp  = ~hub75d->tempHumi.temperature + 1;
        } else {
            tmp = hub75d->tempHumi.temperature;
        }
        t1 = tmp / 100;
        t0 = tmp % 100 / 10;
        td = tmp % 10;
        if (sign == false) {
            if (t1 == 0x00) {
                t1 = TEMP_DISP_NULL_INDEX;
            }
        } else {
            if (t1 == 0x00) {
                t1 = TEMP_NEGATIVE_SIGN_INDEX;
            }
        }
    } else {
        tmp = hub75d->tempHumi.humidity;
        t1  = tmp / 100;
        t0  = tmp % 100 / 10;
        td  = tmp % 10;
        if (t1 == 0x00) {
            t1 = TEMP_DISP_NULL_INDEX;
        }
    }

    if (sign == true) {
        if (t1 != 0x00) {
            hub75d->rgb.r[12][index] |= (g_tempHumiDigitTable[TEMP_NEGATIVE_SIGN_INDEX][tableIndex] >> 6);
            hub75d->rgb.r[13][index] |= (g_tempHumiDigitTable[TEMP_NEGATIVE_SIGN_INDEX][tableIndex] << 2);
        }
    }
    hub75d->rgb.r[13][index] |= (g_tempHumiDigitTable[t1][tableIndex] >> 1);
    hub75d->rgb.r[13][index] |= (g_tempHumiDigitTable[t0][tableIndex] >> 7);
    hub75d->rgb.r[14][index] |= (g_tempHumiDigitTable[t0][tableIndex] << 1);
    hub75d->rgb.r[14][index] |= (g_tempHumiDigitTable[TEMP_HUMI_DOT_INDEX][tableIndex] >> 4);
    hub75d->rgb.g[14][index] |= (g_tempHumiDigitTable[TEMP_HUMI_DOT_INDEX][tableIndex] >> 4);
    hub75d->rgb.r[14][index] |= (g_tempHumiDigitTable[td][tableIndex] >> 5);
    hub75d->rgb.r[15][index] |= (g_tempHumiDigitTable[td][tableIndex] << 3);
    if (hub75d->displayTh == DISP_T) {
        hub75d->rgb.g[15][index] |= (g_tempHumiDigitTable[TEMP_CELSIUS_DEGREE_ICON_INDEX][tableIndex] >> 3);
    } else {
        hub75d->rgb.g[15][index] |= (g_tempHumiDigitTable[TEMP_PERCENT_SIGN_ICON_INDEX][tableIndex] >> 3);
    }
}

void HUB75D_GetScanRgb(struct Hub75dType *hub75d)
{
    uint8_t count;

    memset(&hub75d->rgb, 0, sizeof(struct RgbType));
    for (count = 0; count < SCAN_ALL_LINE; count++) {
        DispTime(hub75d, count);
        if (count < (SCAN_ALL_LINE / 2)) {
            DispDate(hub75d, count);
        } else {
            DispLunarCalendar(hub75d, count);
            DispTemperatureHumidity(hub75d, count);
        }
    }
}

void HUB75D_DispScan(struct RgbType *rgb)
{
    static uint8_t index = 0;
    PinFlags flags       = { 0 };
    uint8_t move;

    HUB_LAT = 0;
    for (uint8_t i = 0; i < 8; i++) {
        move = 0x80;
        for (uint8_t loop = 0; loop < 8; loop++) {
            flags.flag = 0x00;
            if ((rgb->r[i + 8][index] & move) != 0) {
                flags.bit.r1 = 1;
            }
            if ((rgb->g[i + 8][index] & move) != 0) {
                flags.bit.g1 = 1;
            }
            if ((rgb->b[i + 8][index] & move) != 0) {
                flags.bit.b1 = 1;
            }

            if ((rgb->r[i][index] & move) != 0) {
                flags.bit.r2 = 1;
            }
            if ((rgb->g[i][index] & move) != 0) {
                flags.bit.g2 = 1;
            }
            if ((rgb->b[i][index] & move) != 0) {
                flags.bit.b2 = 1;
            }
            move >>= 1;

            HUB_R1  = flags.bit.r1;
            HUB_G1  = flags.bit.g1;
            HUB_B1  = flags.bit.b1;
            HUB_R2  = flags.bit.r2;
            HUB_G2  = flags.bit.g2;
            HUB_B2  = flags.bit.b2;
            HUB_CLK = 0;
            HUB_CLK = 1;
        }
    }

    if (index & 0x01) {
        flags.bit.hubA = 1;
    }
    if (index & 0x02) {
        flags.bit.hubB = 1;
    }
    if (index & 0x04) {
        flags.bit.hubC = 1;
    }
    if (index & 0x08) {
        flags.bit.hubD = 1;
    }

    HUB_A   = flags.bit.hubA;
    HUB_B   = flags.bit.hubB;
    HUB_C   = flags.bit.hubC;
    HUB_D   = flags.bit.hubD;
    HUB_LAT = 1;

    index++;
    if (index >= SCAN_ALL_LINE) {
        index = 0x00;
    }
}

void HUB75D_GetCalendar(struct CalendarDecimal *calendar, struct TimeType *time)
{
    calendar->hourL  = time->hour % 10;
    calendar->hourH  = time->hour / 10;
    calendar->minL   = time->minute % 10;
    calendar->minH   = time->minute / 10;
    calendar->secL   = time->second % 10;
    calendar->secH   = time->second / 10;
    calendar->dayL   = time->day % 10;
    calendar->dayH   = time->day / 10;
    calendar->monthL = time->month % 10;
    calendar->monthH = time->month / 10;
    calendar->yearL  = time->year % 100 % 10;
    calendar->yearH  = time->year % 100 / 10;
    calendar->week   = time->week;
    calendar->colon  = 11; /* ":" */
}

void HUB75D_Disp(uint16_t *count, enum DispTime time)
{
    *count = time;
    if (time == DISP_TIME_OFF) {
        HUB75D_DISP_POWER_PIN = DISP_OFF;

        HUB_A   = 0;
        HUB_B   = 0;
        HUB_C   = 0;
        HUB_D   = 0;
        HUB_OE  = 0;
        HUB_LAT = 0;
        HUB_CLK = 0;
        HUB_R1  = 0;
        HUB_G1  = 0;
        HUB_B1  = 0;
        HUB_R2  = 0;
        HUB_G2  = 0;
        HUB_B2  = 0;
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
    } else {
        HUB75D_DISP_POWER_PIN = DISP_ON;
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    }

    LOGI(LOG_TAG, "display time: %d\r\n\r\n", time);
}

bool HUB75D_CtrDec(struct Hub75dType *hub75d)
{
    bool ret                 = false;
    static uint8_t changeCtr = 0;

    changeCtr++;
    if (changeCtr > TEMP_HUMI_DISP_TIME) {
        changeCtr = 0x00;
        if (hub75d->displayTh == DISP_T) {
            hub75d->displayTh = DISP_H;
        } else {
            hub75d->displayTh = DISP_T;
        }
    }

    if (hub75d->displayCount != DISP_TIME_OFF) {
        hub75d->displayCount--;
        if (hub75d->displayCount == 0x00) {
            ret = true;
        }
    }

    return ret;
}

void HUB75D_Init(void)
{
}
