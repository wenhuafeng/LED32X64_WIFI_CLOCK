#include "hub75d.h"
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "gpio_bit_ctrl.h"
#include "htu21d.h"
#include "lunar_calendar.h"
#include "main.h"
#include "time.h"

#define TEMP_HUMI_DISP_TIME (2) /* seconds */

#define HUB_A PBout(13)
#define HUB_B PBout(14)
#define HUB_C PBout(15)
#define HUB_D PAout(8)
#define HUB_OE PBout(4)
#define HUB_LAT PBout(3)
#define HUB_CLK PAout(15)
#define HUB_R1 PAout(6)
#define HUB_G1 PAout(5)
#define HUB_B1 PAout(7)
#define HUB_R2 PBout(0)
#define HUB_G2 PBout(1)
#define HUB_B2 PBout(10)
#define HUB75D_DISP_POWER_PIN PBout(11)

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

#define WORD_COUNT 16
struct RgbType {
    uint8_t red[WORD_COUNT];
    uint8_t green[WORD_COUNT];
    uint8_t blue[WORD_COUNT];
};

typedef union {
    uint16_t flag;
    struct {
        uint32_t hubA : 1;
        uint32_t hubB : 1;
        uint32_t hubC : 1;
        uint32_t hubD : 1;
        uint32_t hubR1 : 1;
        uint32_t hubG1 : 1;
        uint32_t hubB1 : 1;
        uint32_t hubR2 : 1;
        uint32_t hubG2 : 1;
        uint32_t hubB2 : 1;
    } bit;
} PinFlags;

struct CalendarDecimal g_calendarDecimal;
enum DispTorH g_displayTorH;
static uint16_t g_displayOffCtr;

static uint8_t const DateTable[] = {
    0x00, 0x06, 0x09, 0x09, 0x09, 0x09, 0x09, 0x06, /* 0         */
    0x00, 0x02, 0x06, 0x02, 0x02, 0x02, 0x02, 0x07, /* 1         */
    0x00, 0x06, 0x09, 0x01, 0x02, 0x04, 0x08, 0x0f, /* 2         */
    0x00, 0x06, 0x09, 0x01, 0x06, 0x01, 0x09, 0x06, /* 3         */
    0x00, 0x02, 0x06, 0x0a, 0x0a, 0x0f, 0x02, 0x02, /* 4         */
    0x00, 0x0f, 0x08, 0x0e, 0x09, 0x01, 0x09, 0x06, /* 5         */
    0x00, 0x06, 0x09, 0x08, 0x0e, 0x09, 0x09, 0x06, /* 6         */
    0x00, 0x0f, 0x01, 0x01, 0x02, 0x04, 0x04, 0x04, /* 7         */
    0x00, 0x06, 0x09, 0x09, 0x06, 0x09, 0x09, 0x06, /* 8         */
    0x00, 0x06, 0x09, 0x09, 0x07, 0x01, 0x09, 0x06, /* 9         */
    0x00, 0x00, 0x02, 0x02, 0x00, 0x02, 0x02, 0x00, /* 10 :      */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 11 NULL   */
    0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, /* 13 -      */
    0x00, 0x00, 0x00, 0x02, 0x07, 0x02, 0x00, 0x00, /* 14 +      */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, /*  - 15     */
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, /*  . 16     */
    0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, /*  . 17     */
    0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, /*  - 18     */
    0x00, 0x00, 0x80, 0x00, 0x30, 0x40, 0x40, 0x30, /* 19 .c 4x7 */
};

static uint8_t const HZ[] = {
    0x00, 0x3e, 0x22, 0x22, 0x3e, 0x22, 0x22, 0x3e, /* 0  日  */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, /* 1  一  */
    0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x7e, 0x00, /* 2  二  */
    0x00, 0x00, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x7e, /* 3  三  */
    0x00, 0x00, 0x7f, 0x55, 0x55, 0x77, 0x41, 0x7f, /* 4  四  */
    0x00, 0x00, 0x3e, 0x08, 0x3e, 0x12, 0x22, 0x7f, /* 5  五  */
    0x00, 0x08, 0x00, 0x7f, 0x00, 0x14, 0x22, 0x41, /* 6  六  */
    0x10, 0x7c, 0xa4, 0xb4, 0x48, 0x64, 0xb2, 0x20, /* 7  农  */
    0x00, 0x0f, 0x09, 0x09, 0x0f, 0x09, 0x09, 0x0f, /* 8  日  */
    0x00, 0x0f, 0x09, 0x0f, 0x09, 0x0f, 0x09, 0x11, /* 9  月  */
    0x08, 0x1f, 0x24, 0x1f, 0x14, 0x3f, 0x04, 0x04, /* 10  年 */
};

static uint8_t const DateD[][8] = {
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

static uint8_t const Time8Mul16Table[][16] = {
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

static uint8_t const TimeSecondTable[][16] = {
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

static inline void DispTime(struct CalendarDecimal *caleDeci, struct RgbType *rgb, uint8_t i)
{
    uint8_t h1 = caleDeci->hourH;
    uint8_t h0 = caleDeci->hourL;
    uint8_t m1 = caleDeci->minH;
    uint8_t m0 = caleDeci->minL;
    uint8_t s1 = caleDeci->secH;
    uint8_t s0 = caleDeci->secL;
    uint8_t sd = caleDeci->colon;

    if (h1 == 0) {
        h1 = 10;
    }

    rgb->red[0] |= Time8Mul16Table[h1][i];
    rgb->green[0] |= Time8Mul16Table[h1][i];
    rgb->blue[0] |= Time8Mul16Table[h1][i];
    rgb->red[1] |= (Time8Mul16Table[h0][i] >> 1);
    rgb->red[2] |= (Time8Mul16Table[h0][i] << 7);
    rgb->green[1] |= (Time8Mul16Table[h0][i] >> 1);
    rgb->green[2] |= (Time8Mul16Table[h0][i] << 7);
    rgb->blue[1] |= (Time8Mul16Table[h0][i] >> 1);
    rgb->blue[2] |= (Time8Mul16Table[h0][i] << 7);

    rgb->red[2] |= Time8Mul16Table[sd][i];
    rgb->blue[2] |= Time8Mul16Table[sd][i];

    rgb->red[3] |= Time8Mul16Table[m1][i];
    rgb->green[3] |= Time8Mul16Table[m1][i];
    rgb->blue[3] |= Time8Mul16Table[m1][i];
    rgb->red[4] |= (Time8Mul16Table[m0][i] >> 1);
    rgb->red[5] |= (Time8Mul16Table[m0][i] << 7);
    rgb->green[4] |= (Time8Mul16Table[m0][i] >> 1);
    rgb->green[5] |= (Time8Mul16Table[m0][i] << 7);
    rgb->blue[4] |= (Time8Mul16Table[m0][i] >> 1);
    rgb->blue[5] |= (Time8Mul16Table[m0][i] << 7);

    rgb->red[5] |= Time8Mul16Table[sd][i];
    rgb->blue[5] |= Time8Mul16Table[sd][i];

    rgb->red[6] |= TimeSecondTable[s1][i];
    rgb->green[6] |= TimeSecondTable[s1][i];
    rgb->blue[6] |= TimeSecondTable[s1][i];
    rgb->red[7] |= TimeSecondTable[s0][i];
    rgb->green[7] |= TimeSecondTable[s0][i];
    rgb->blue[7] |= TimeSecondTable[s0][i];
}

static inline void DispDate(struct CalendarDecimal *caleDeci, struct RgbType *rgb, uint8_t i)
{
    uint8_t y1 = caleDeci->yearH;
    uint8_t y0 = caleDeci->yearL;
    uint8_t m1 = caleDeci->monthH;
    uint8_t m0 = caleDeci->monthL;
    uint8_t d1 = caleDeci->dayH;
    uint8_t d0 = caleDeci->dayL;
    uint8_t wk = caleDeci->week;

    rgb->green[8] |= (DateTable[2 * 8 + i] << 4);
    rgb->green[8] |= (DateTable[0 * 8 + i] >> 1);
    rgb->green[9] |= (DateTable[0 * 8 + i] << 7);
    rgb->green[9] |= (DateTable[y1 * 8 + i] << 2);
    rgb->green[9] |= (DateTable[y0 * 8 + i] >> 3);
    for (uint8_t j = 8; j < 10; j++) {
        rgb->red[j] = rgb->green[j];
    }
    rgb->green[10] |= (DateTable[y0 * 8 + i] << 5);
    rgb->red[10] |= (DateTable[y0 * 8 + i] << 5);
    rgb->green[10] |= (HZ[10 * 8 + i] >> 2);
    rgb->green[11] |= (HZ[10 * 8 + i] << 6);

    if (m1 == 0) {
        m1 = 11;
    }
    rgb->green[11] |= (DateTable[m1 * 8 + i] << 1);
    rgb->red[11] |= (DateTable[m1 * 8 + i] << 1);
    rgb->green[12] |= (DateTable[m0 * 8 + i] << 4);
    rgb->red[12] |= (DateTable[m0 * 8 + i] << 4);
    rgb->green[12] |= (HZ[9 * 8 + i] >> 1);
    rgb->green[13] |= (HZ[9 * 8 + i] << 7);

    if (d1 == 0) {
        d1 = 11;
    }
    rgb->green[13] |= (DateTable[d1 * 8 + i] << 2);
    rgb->red[13] |= (DateTable[d1 * 8 + i] << 2);
    rgb->green[13] |= (DateTable[d0 * 8 + i] >> 3);
    rgb->red[13] |= (DateTable[d0 * 8 + i] >> 3);
    rgb->green[14] |= (DateTable[d0 * 8 + i] << 5);
    rgb->red[14] |= (DateTable[d0 * 8 + i] << 5);
    rgb->green[14] |= (HZ[8 * 8 + i]);
    rgb->red[15] |= HZ[(wk - 0) * 8 + i];
}

static inline void DispTemperatureHumidity(struct RgbType *rgb, uint8_t i)
{
    bool sign = false;
    uint8_t j = i - 8;
    uint8_t t1, t0, td;
    uint16_t tmp;
    int16_t temperature = HTU21D_GetTemperature();
    uint16_t humidity = HTU21D_GetHumidity();

    if (g_displayTorH == DISP_T) {
        if (temperature < 0) {
            sign = true;
            tmp = ~temperature + 1;
        } else {
            tmp = temperature;
        }
        t1 = tmp / 100;
        t0 = tmp % 100 / 10;
        td = tmp % 10;
        if (sign == false) {
            if (t1 == 0x00) {
                t1 = 11; /* ' ' */
            }
        } else {
            if (t1 == 0x00) {
                t1 = 12; /* '-' */
            }
        }
    } else {
        tmp = humidity;
        t1 = tmp / 100;
        t0 = tmp % 100 / 10;
        td = tmp % 10;
        if (t1 == 0x00) {
            t1 = 11;
        }
    }

    if (sign == true) {
        if (t1 != 0x00) {
            rgb->red[12] |= (DateD[12][j] >> 6);
            rgb->red[13] |= (DateD[12][j] << 2);
        }
    }
    rgb->red[13] |= (DateD[t1][j] >> 1);
    rgb->red[13] |= (DateD[t0][j] >> 7);
    rgb->red[14] |= (DateD[t0][j] << 1);
    rgb->red[14] |= (DateD[13][j] >> 4);
    rgb->green[14] |= (DateD[13][j] >> 4);
    rgb->red[14] |= (DateD[td][j] >> 5);
    rgb->red[15] |= (DateD[td][j] << 3);
    if (g_displayTorH == DISP_T) {
        rgb->green[15] |= (DateD[14][j] >> 3); /* C */
    } else {
        rgb->green[15] |= (DateD[15][j] >> 3); /* % */
    }
}

static inline void DispLunarCalendar(struct RgbType *rgb, uint8_t i)
{
    uint8_t j = i - 8;
    struct LunarCalendarType *lcData = GetLunarCalendar();
    uint8_t m1 = lcData->month / 10;
    uint8_t m0 = lcData->month % 10;
    uint8_t d1 = lcData->day / 10;
    uint8_t d0 = lcData->day % 10;

    rgb->green[8] |= HZ[7 * 8 + j];
    if (m1 == 0) {
        m1 = 11;
    }
    rgb->red[9] |= (DateTable[m1 * 8 + j] << 4);
    rgb->red[9] |= (DateTable[m0 * 8 + j] >> 1);
    rgb->red[10] |= (DateTable[m0 * 8 + j] << 7);
    rgb->green[10] |= (HZ[9 * 8 + j] << 1);
    if (d1 == 0) {
        d1 = 11;
    }
    rgb->red[11] |= (DateTable[d1 * 8 + j] << 4);
    rgb->red[11] |= (DateTable[d0 * 8 + j] >> 1);
    rgb->red[12] |= (DateTable[d0 * 8 + j] << 7);
    rgb->green[12] |= (HZ[8 * 8 + j] << 2);
}

static inline void SetScanPin(struct RgbType *rgb, uint8_t count)
{
    PinFlags flags = { 0 };

    HUB_LAT = 0;
    HUB_OE = 1;
    for (uint8_t i = 0; i < 8; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            flags.flag = 0x00;
            if (rgb->red[i + 8] & 0x80) {
                flags.bit.hubR1 = 1;
            }
            if (rgb->green[i + 8] & 0x80) {
                flags.bit.hubG1 = 1;
            }
            if (rgb->blue[i + 8] & 0x80) {
                flags.bit.hubB1 = 1;
            }

            if (rgb->red[i] & 0x80) {
                flags.bit.hubR2 = 1;
            }
            if (rgb->green[i] & 0x80) {
                flags.bit.hubG2 = 1;
            }
            if (rgb->blue[i] & 0x80) {
                flags.bit.hubB2 = 1;
            }

            rgb->red[i + 8] = rgb->red[i + 8] << 1;
            rgb->green[i + 8] = rgb->green[i + 8] << 1;
            rgb->blue[i + 8] = rgb->blue[i + 8] << 1;
            rgb->red[i] = rgb->red[i] << 1;
            rgb->green[i] = rgb->green[i] << 1;
            rgb->blue[i] = rgb->blue[i] << 1;

            HUB_R1 = flags.bit.hubR1;
            HUB_G1 = flags.bit.hubG1;
            HUB_B1 = flags.bit.hubB1;
            HUB_R2 = flags.bit.hubR2;
            HUB_G2 = flags.bit.hubG2;
            HUB_B2 = flags.bit.hubB2;
            HUB_CLK = 1;
            HUB_CLK = 0;
        }
    }

    if (count & 0x01) {
        flags.bit.hubA = 1;
    }
    if (count & 0x02) {
        flags.bit.hubB = 1;
    }
    if (count & 0x04) {
        flags.bit.hubC = 1;
    }
    if (count & 0x08) {
        flags.bit.hubD = 1;
    }

    HUB_A = flags.bit.hubA;
    HUB_B = flags.bit.hubB;
    HUB_C = flags.bit.hubC;
    HUB_D = flags.bit.hubD;
    HUB_LAT = 1;
    HUB_OE = 0;
}

void inline HUB75D_DispScan(void)
{
    static uint8_t count = 0;
    struct RgbType rgb;

    memset(&rgb, 0, sizeof(struct RgbType));
    DispTime(&g_calendarDecimal, &rgb, count);
    if (count < 8) {
        DispDate(&g_calendarDecimal, &rgb, count);
    } else {
        DispLunarCalendar(&rgb, count);
        DispTemperatureHumidity(&rgb, count);
    }
    SetScanPin(&rgb, count);

    count++;
    if (count > 15) {
        count = 0x00;
    }
}

void HUB75D_CalculateCalendar(struct TimeType *time)
{
    g_calendarDecimal.hourL = time->hour % 10;
    g_calendarDecimal.hourH = time->hour / 10;
    g_calendarDecimal.minL = time->min % 10;
    g_calendarDecimal.minH = time->min / 10;
    g_calendarDecimal.secL = time->sec % 10;
    g_calendarDecimal.secH = time->sec / 10;

    g_calendarDecimal.dayL = time->day % 10;
    g_calendarDecimal.dayH = time->day / 10;
    g_calendarDecimal.monthL = time->month % 10;
    g_calendarDecimal.monthH = time->month / 10;
    g_calendarDecimal.yearL = time->year % 100 % 10;
    g_calendarDecimal.yearH = time->year % 100 / 10;
    g_calendarDecimal.week = time->week;
    g_calendarDecimal.colon = 11; /* ":" */
}

void HUB75D_SetDispOffCtr(enum DispTime time)
{
    g_displayOffCtr = time;
}

void HUB75D_DispOnOff(enum DispTime time)
{
    HUB75D_SetDispOffCtr(time);
    if (time == DISP_TIME_OFF) {
        HUB75D_DISP_POWER_PIN = DISP_OFF;
        HUB_A = 0;
        HUB_B = 0;
        HUB_C = 0;
        HUB_D = 0;
        HUB_OE = 0;
        HUB_LAT = 0;
        HUB_CLK = 0;
        HUB_R1 = 0;
        HUB_G1 = 0;
        HUB_B1 = 0;
        HUB_R2 = 0;
        HUB_G2 = 0;
        HUB_B2 = 0;
    } else {
        HUB75D_DISP_POWER_PIN = DISP_ON;
    }
}

bool HUB75D_CtrDec(void)
{
    bool standby = false;
    static uint8_t changeCtr;

    changeCtr++;
    if (changeCtr > TEMP_HUMI_DISP_TIME) {
        changeCtr = 0x00;
        if (g_displayTorH == DISP_T) {
            g_displayTorH = DISP_H;
        } else {
            g_displayTorH = DISP_T;
        }
    }

    if (g_displayOffCtr) {
        g_displayOffCtr--;
        if (g_displayOffCtr == 0x00) {
            standby = true;
        }
    }

    return standby;
}