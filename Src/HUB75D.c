#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "type_define.h"
#include "hub75d.h"
#include "htu21d.h"
#include "esp8266_at.h"
#include "lunar_calendar.h"
#include "main.h"
#include "time.h"

#define TEMP_HUMI_DISP_TIME (4) /* 4 seconds temperature and temperature switch display */

#define HUB75D_DISP_POWER_PIN   PBout(11)

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

enum DispTorH g_dispTorH;

static u8 f1, f0, s1, s0, h1, h0, sd, d1, d0, m1, m0, y1, y0, wk;
static u8 t1, t0, td;
static u8 zr[16], zg[16], zb[16];
static u16 g_displayOffCtr;

static u8 const ZF[] = {
    0x00, 0x06, 0x09, 0x09, 0x09, 0x09, 0x09, 0x06, //0
    0x00, 0x02, 0x06, 0x02, 0x02, 0x02, 0x02, 0x07, //1
    0x00, 0x06, 0x09, 0x01, 0x02, 0x04, 0x08, 0x0f, //2
    0x00, 0x06, 0x09, 0x01, 0x06, 0x01, 0x09, 0x06, //3
    0x00, 0x02, 0x06, 0x0a, 0x0a, 0x0f, 0x02, 0x02, //4
    0x00, 0x0f, 0x08, 0x0e, 0x09, 0x01, 0x09, 0x06, //5
    0x00, 0x06, 0x09, 0x08, 0x0e, 0x09, 0x09, 0x06, //6
    0x00, 0x0f, 0x01, 0x01, 0x02, 0x04, 0x04, 0x04, //7
    0x00, 0x06, 0x09, 0x09, 0x06, 0x09, 0x09, 0x06, //8
    0x00, 0x06, 0x09, 0x09, 0x07, 0x01, 0x09, 0x06, //9
    0x00, 0x00, 0x02, 0x02, 0x00, 0x02, 0x02, 0x00, //10 :
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //11 ��
    0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, //13 -
    0x00, 0x00, 0x00, 0x02, 0x07, 0x02, 0x00, 0x00, //14 +
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, // - 15
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, // . 16
    0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, // .�� 17
    0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, // - 18
    0x00, 0x00, 0x80, 0x00, 0x30, 0x40, 0x40, 0x30, //19 .c 4x7
};

static u8 const HZ[] = {
    0x00, 0x3e, 0x22, 0x22, 0x3e, 0x22, 0x22, 0x3e, //0 ��
    0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, //1 һ
    0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x7e, 0x00, //2 ��
    0x00, 0x00, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x7e, //3 ��
    0x00, 0x00, 0x7f, 0x55, 0x55, 0x77, 0x41, 0x7f, //4 ��
    0x00, 0x00, 0x3e, 0x08, 0x3e, 0x12, 0x22, 0x7f, //5 ��
    0x00, 0x08, 0x00, 0x7f, 0x00, 0x14, 0x22, 0x41, //6 ��
    0x10, 0x7c, 0xa4, 0xb4, 0x48, 0x64, 0xb2, 0x20, //7 ũ
    0x00, 0x0f, 0x09, 0x09, 0x0f, 0x09, 0x09, 0x0f, //8 ��
    0x00, 0x0f, 0x09, 0x0f, 0x09, 0x0f, 0x09, 0x11, //9 ��
    0x08, 0x1f, 0x24, 0x1f, 0x14, 0x3f, 0x04, 0x04, //10 ��
};

static u8 const DateD[][8] = {
    0x00, 0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, //0
    0x00, 0x20, 0x60, 0x20, 0x20, 0x20, 0x20, 0x70, //1
    0x00, 0x70, 0x88, 0x08, 0x10, 0x20, 0x40, 0xf8, //2
    0x00, 0x70, 0x88, 0x08, 0x30, 0x08, 0x88, 0x70, //3
    0x00, 0x10, 0x30, 0x50, 0x90, 0xf8, 0x10, 0x10, //4
    0x00, 0x78, 0x80, 0xf0, 0x08, 0x08, 0x88, 0x70, //5
    0x00, 0x70, 0x80, 0x80, 0xf0, 0x88, 0x88, 0x70, //6
    0x00, 0xf0, 0x08, 0x10, 0x20, 0x40, 0x40, 0x40, //7
    0x00, 0x70, 0x88, 0x88, 0x70, 0x88, 0x88, 0x70, //8
    0x00, 0x70, 0x88, 0x88, 0x70, 0x08, 0x88, 0x70, //9
    0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, //10 .
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //11 ��
    0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, //12 -
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, //13.
    0x00, 0xc0, 0xc0, 0x30, 0x48, 0x40, 0x48, 0x30, //14 C
    0x00, 0xc8, 0xd0, 0x20, 0x58, 0x98, 0x00, 0x00, //15 %
};

static u8 const DZF[][16] = {
    /*  --- 8x16 ---*/
    0x3c, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
    0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xe7, 0x7e, 0x3c, //0
    0x1c, 0x7c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c,
    0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x7f, //1
    0x3c, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0x03, 0x03,
    0x07, 0x0e, 0x1c, 0x38, 0x70, 0xe1, 0xff, 0xff, //2
    0x3c, 0x7e, 0xe7, 0xc3, 0x03, 0x03, 0x03, 0x3e,
    0x3e, 0x03, 0x03, 0x03, 0xc3, 0xe7, 0x7e, 0x3c, //3
    0x0c, 0x0c, 0x1c, 0x1c, 0x3c, 0x3c, 0x6c, 0x6c,
    0xcc, 0xcc, 0x8c, 0xff, 0xff, 0x0c, 0x0c, 0x1e, //4
    0x7e, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xfe,
    0x07, 0x03, 0x03, 0x03, 0xc3, 0xe7, 0x7e, 0x3c, //5
    0x3c, 0x7e, 0xe7, 0xc3, 0xc0, 0xc0, 0xc0, 0xfc,
    0xfe, 0xe7, 0xc3, 0xc3, 0xc3, 0xe7, 0x7e, 0x3c, //6
    0x7e, 0xff, 0xc3, 0x83, 0x03, 0x03, 0x06, 0x06,
    0x0c, 0x0c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, //7
    0x3c, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xe7, 0x7e,
    0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xe7, 0x7e, 0x3c, //8
    0x3c, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xe7, 0x7f,
    0x3f, 0x03, 0x03, 0xc3, 0xc3, 0xe7, 0x7e, 0x3c, //9
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //10
    0x00, 0x00, 0x18, 0x24, 0x24, 0x18, 0x00, 0x00,
    0x00, 0x00, 0x18, 0x24, 0x24, 0x18, 0x00, 0x00, //11 SD
    0x00, 0x3f, 0x21, 0x21, 0x21, 0x3f, 0x21, 0x21,
    0x21, 0x3f, 0x21, 0x21, 0x21, 0x21, 0x45, 0x82, //12 ��
    0x00, 0x00, 0x7f, 0x41, 0x41, 0x41, 0x41, 0x7f,
    0x41, 0x41, 0x41, 0x41, 0x41, 0x7f, 0x41, 0x00, //13 ��
    0x00, 0x7f, 0x41, 0x49, 0x5d, 0x49, 0x5d, 0x41,
    0x5d, 0x55, 0x55, 0x5d, 0x55, 0x41, 0x41, 0x82, //14 ��
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x7f,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //15 һ
    0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x7e, 0x00,
    0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x00, //16 ��
    0x00, 0x00, 0x02, 0x7e, 0x00, 0x00, 0x02, 0x7e,
    0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x00, //17 ��
    0x00, 0x00, 0x02, 0x7e, 0x00, 0x00, 0x02, 0x7e,
    0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x00, //18 ��
    0x00, 0x00, 0x7e, 0x08, 0x08, 0x08, 0x0a, 0x7e,
    0x12, 0x12, 0x12, 0x22, 0x22, 0xff, 0x00, 0x00, //19 ��
    0x00, 0x20, 0x18, 0x0c, 0x08, 0xff, 0x00, 0x00,
    0x24, 0x34, 0x24, 0x46, 0x43, 0x83, 0x82, 0x00, //20 ��
    0x00, 0x00, 0x10, 0x10, 0x10, 0x1f, 0xf0, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x0e, 0x00, //21 ��
    0x00, 0x00, 0x04, 0x26, 0x34, 0x24, 0x24, 0x24,
    0x24, 0x66, 0x62, 0x62, 0x83, 0x83, 0x81, 0x00, //22 ��
    0x00, 0x20, 0x20, 0x20, 0x20, 0xfc, 0x24, 0x24,
    0x24, 0x24, 0x44, 0x44, 0x45, 0x85, 0x86, 0x00, //23 ��
    0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0xfe,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, //24 ʮ
    0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0xfe,
    0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x7e, 0x00, //25 إ
    0x00, 0x00, 0x54, 0x54, 0x54, 0x54, 0x54, 0xfe,
    0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x00, //26 ئ
    0x00, 0x00, 0xe1, 0xa1, 0xe2, 0x02, 0x04, 0x04,
    0x08, 0x10, 0x20, 0x20, 0x40, 0x47, 0x85, 0x87, //27 %
    0x00, 0x00, 0xe0, 0xa0, 0xe0, 0x00, 0x1e, 0x23,
    0x41, 0x40, 0x40, 0x40, 0x40, 0x41, 0x23, 0x1e, //28 c
};

static u8 const ZZF[][16] = {
    0x00, 0x00, 0x3e, 0x7f, 0x63, 0x63, 0x63, 0x63,
    0x63, 0x63, 0x63, 0x63, 0x7f, 0x3e, 0x00, 0x00, //0
    0x00, 0x00, 0x1c, 0x3c, 0x1c, 0x1c, 0x1c, 0x1c,
    0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x3e, 0x00, 0x00, //1
    0x00, 0x00, 0x3e, 0x7f, 0x63, 0x63, 0x07, 0x0e,
    0x1c, 0x38, 0x70, 0x60, 0x7f, 0x7f, 0x00, 0x00, //2
    0x00, 0x00, 0x3e, 0x7f, 0x63, 0x63, 0x03, 0x06,
    0x06, 0x03, 0x63, 0x63, 0x7f, 0x3e, 0x00, 0x00, //3
    0x00, 0x00, 0x1e, 0x1e, 0x36, 0x36, 0x36, 0x66,
    0x66, 0x7f, 0x7f, 0x06, 0x06, 0x06, 0x00, 0x00, //4
    0x00, 0x00, 0x3f, 0x7f, 0x60, 0x60, 0x60, 0x7e,
    0x3f, 0x03, 0x03, 0x63, 0x7f, 0x3e, 0x00, 0x00, //5
    0x00, 0x00, 0x3e, 0x7f, 0x63, 0x60, 0x60, 0x7e,
    0x7f, 0x63, 0x63, 0x63, 0x7f, 0x3e, 0x00, 0x00, //6
    0x00, 0x00, 0x3e, 0x7f, 0x63, 0x63, 0x03, 0x06,
    0x06, 0x06, 0x0c, 0x0c, 0x0c, 0x0c, 0x00, 0x00, //7
    0x00, 0x00, 0x3e, 0x7f, 0x63, 0x63, 0x63, 0x3e,
    0x3e, 0x63, 0x63, 0x63, 0x7f, 0x3e, 0x00, 0x00, //8
    0x00, 0x00, 0x3e, 0x7f, 0x63, 0x63, 0x63, 0x7f,
    0x3f, 0x03, 0x03, 0x63, 0x7f, 0x3e, 0x00, 0x00, //9
    0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x00,
    0x00, 0x06, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, //10 :
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //11
};

static void DispTime(u8 i)
{
    if (h1 == 0) {
        h1 = 10;
    }
    if (f1 >= 4) {
        zr[0] |= DZF[h1][i]; //h1
        zg[0] |= DZF[h1][i]; //h1
        zb[0] |= DZF[h1][i]; //h1
        zr[1] |= (DZF[h0][i] >> 1); //h0
        zr[2] |= (DZF[h0][i] << 7); //h0
        zg[1] |= (DZF[h0][i] >> 1); //h0
        zg[2] |= (DZF[h0][i] << 7); //h0
        zb[1] |= (DZF[h0][i] >> 1); //h0
        zb[2] |= (DZF[h0][i] << 7); //h0
        zr[2] |= DZF[sd][i]; //sd
        zb[2] |= DZF[sd][i]; //sd
        zr[3] |= DZF[f1][i]; //f1
        zg[3] |= DZF[f1][i]; //f1
        zb[3] |= DZF[f1][i]; //f1
        zr[4] |= (DZF[f0][i] >> 1); //f0
        zr[5] |= (DZF[f0][i] << 7); //f0
        zg[4] |= (DZF[f0][i] >> 1); //f0
        zg[5] |= (DZF[f0][i] << 7); //f0
        zb[4] |= (DZF[f0][i] >> 1); //f0
        zb[5] |= (DZF[f0][i] << 7); //f0
        zr[5] |= DZF[sd][i]; //sd
        zb[5] |= DZF[sd][i]; //sd
        zr[6] |= ZZF[s1][i]; //s1
        zg[6] |= ZZF[s1][i]; //s1
        zb[6] |= ZZF[s1][i]; //s1
        zr[7] |= ZZF[s0][i]; //s0
        zg[7] |= ZZF[s0][i]; //s0
        zb[7] |= ZZF[s0][i]; //s0
    } else if (f1 >= 2) {
        zg[0] |= DZF[h1][i]; //h1
        zg[1] |= (DZF[h0][i] >> 1); //h0
        zg[2] |= (DZF[h0][i] << 7); //h0
        zr[2] |= DZF[sd][i]; //sd
        zg[2] |= DZF[sd][i]; //sd
        zg[3] |= DZF[f1][i]; //f1
        zg[4] |= (DZF[f0][i] >> 1); //f0
        zg[5] |= (DZF[f0][i] << 7); //f0
        zr[5] |= DZF[sd][i]; //sd
        zg[5] |= DZF[sd][i]; //sd
        zg[6] |= ZZF[s1][i]; //s1
        zg[7] |= ZZF[s0][i]; //s0
    } else {
        zr[0] |= DZF[h1][i]; //h1
        zg[0] |= DZF[h1][i];
        //zb[0] |= DZF[h1][i];
        zr[1] |= (DZF[h0][i] >> 1); //h0
        zg[1] |= (DZF[h0][i] >> 1);
        //zb[1] |= (DZF[h0][i]>>1);
        zr[2] |= (DZF[h0][i] << 7); //h0
        zg[2] |= (DZF[h0][i] << 7);
        //zb[2] |= (DZF[h0][i]<<7);
        zg[2] |= DZF[sd][i]; //sd
        zr[3] |= DZF[f1][i]; //f1
        zg[3] |= DZF[f1][i];
        //zb[3] |= DZF[f1][i];
        zr[4] |= (DZF[f0][i] >> 1); //f0
        zg[4] |= (DZF[f0][i] >> 1);
        //zb[4] |= (DZF[f0][i]>>1);
        zr[5] |= (DZF[f0][i] << 7); //f0
        zg[5] |= (DZF[f0][i] << 7);
        //zb[5] |= (DZF[f0][i]<<7);
        zg[5] |= DZF[sd][i]; //sd
        zr[6] |= ZZF[s1][i]; //s1
        zg[6] |= ZZF[s1][i];
        //zb[6] |= ZZF[s1][i];
        zr[7] |= ZZF[s0][i]; //s0
        zg[7] |= ZZF[s0][i];
        //zb[7]|= ZZF[s0][i];
    }
}

static void DispDate(u8 i)
{
    u8 k;

    if (f1 >= 2) {
        zg[8] |= (ZF[2 * 8 + i] << 4); //2		8-5 ǰ4�У�6�п�
        zg[8] |= (ZF[0 * 8 + i] >> 1); //0, ǰ3λ	8-8
        zg[9] |= (ZF[0 * 8 + i] << 7); //0, ��1λ	9-3  4�п�
        zg[9] |= (ZF[y1 * 8 + i] << 2); //y1 ǰ4λ	9-8  5�п�
        zg[9] |= (ZF[y0 * 8 + i] >> 3); //y0 ǰ1λ	10-1 2�п�
        for (k = 8; k < 10; k++) {
            zr[k] = zg[k]; // ��ɫ
        }
        zg[10] |= (ZF[y0 * 8 + i] << 5); //y0 ǰ3λ	10-3  12�п�
        zr[10] |= (ZF[y0 * 8 + i] << 5); //��ɫ

        zg[10] |= (HZ[10 * 8 + i] >> 2); //�� ,		10-8
        zg[11] |= (HZ[10 * 8 + i] << 6); //�� ,		11-2
        if (m1 == 0) {
            m1 = 11;
        }
        zg[11] |= (ZF[m1 * 8 + i] << 1); //m1 3λ	11-7
        zr[11] |= (ZF[m1 * 8 + i] << 1); // ��ɫ
        zg[12] |= (ZF[m0 * 8 + i] << 4); //m9 5	12-4
        zr[12] |= (ZF[m0 * 8 + i] << 4); // ��ɫ

        zg[12] |= (HZ[9 * 8 + i] >> 1); //�� ǰ4λ	12-8 ��5��
        zg[13] |= (HZ[9 * 8 + i] << 7); //�� ��1λ	13-11
        if (d1 == 0) {
            d1 = 11;
        }
        zg[13] |= (ZF[d1 * 8 + i] << 2); //d1 ǰ3λ	13-76   ǰ5��
        zr[13] |= (ZF[d1 * 8 + i] << 2); // ��ɫ
        zg[13] |= (ZF[d0 * 8 + i] >> 3); //d0  		14-5
        zr[13] |= (ZF[d0 * 8 + i] >> 3); // ��ɫ
        zg[14] |= (ZF[d0 * 8 + i] << 5); //d0  		14 3
        zr[14] |= (ZF[d0 * 8 + i] << 5); // ��ɫ

        zg[14] |= (HZ[8 * 8 + i]); //�� ��4λ	14-88
        //for (k=8; k<15; k++) zr[k]=zg[k];	// ��ɫ
        zr[15] |= HZ[(wk - 0) * 8 + i]; //week		15-3~15-8
    } else {
        zg[8] |= (ZF[2 * 8 + i] << 4); //2		8-5 ǰ4�У�6�п�
        zg[8] |= (ZF[0 * 8 + i] >> 1); //0, ǰ3λ	8-8
        zg[9] |= (ZF[0 * 8 + i] << 7); //0, ��1λ	9-3  4�п�
        zg[9] |= (ZF[y1 * 8 + i] << 2); //y1 ǰ4λ	9-8  5�п�
        zg[9] |= (ZF[y0 * 8 + i] >> 3); //y0 ǰ1λ	10-1 2�п�
        zg[10] |= (ZF[y0 * 8 + i] << 5); //y0 ǰ3λ	10-3  12�п�

        zr[10] |= (HZ[10 * 8 + i] >> 2); //�� ,		10-8
        zr[11] |= (HZ[10 * 8 + i] << 6); //�� ,		11-2
        if (m1 == 0) {
            m1 = 11;
        }
        zg[11] |= (ZF[m1 * 8 + i] << 1); //m1 3λ	11-7
        zg[12] |= (ZF[m0 * 8 + i] << 4); //m9 5	12-4

        zr[12] |= (HZ[9 * 8 + i] >> 1); //�� ǰ4λ	12-8 ��5��
        zr[13] |= (HZ[9 * 8 + i] << 7); //�� ��1λ	13-11
        if (d1 == 0) {
            d1 = 11;
        }
        zg[13] |= (ZF[d1 * 8 + i] << 2); //d1 ǰ3λ	13-76   ǰ5��
        zg[13] |= (ZF[d0 * 8 + i] >> 3); //d0  		14-5
        zg[14] |= (ZF[d0 * 8 + i] << 5); //d0  		14 3

        zr[14] |= (HZ[8 * 8 + i]); //�� ��4λ	14-88
        zg[15] |= HZ[(wk - 0) * 8 + i]; //week		15-3~15-8
    } //i<8
}

static void DispYinli(u8 i)
{
    bool sign;
    u8 j;
    u16 tmp;
    u8 nd1, nd0, nm1, nm0;
    s16 temperature = GetTemperature();
    u16 humidity = GetHumidity();
    struct LunarCalendarType lcData = GetLunarCalendar();

    nm1 = (lcData.month & 0x70) >> 4;
    nm0 = lcData.month & 0x0F;
    nd1 = (lcData.day & 0x70) >> 4;
    nd0 = lcData.day & 0x0F;

    j = i - 8;
    if (f1 >= 2) {
        zg[8] |= HZ[7 * 8 + j]; //ũ��5��   8-77
        if (nm1 == 0) {
            nm1 = 11;
        }
        zr[9] |= (ZF[nm1 * 8 + j] << 4); //nm1  5λ	9-4
        zr[9] |= (ZF[nm0 * 8 + j] >> 1); //nm0 ǰ2λ 9-8
        zr[10] |= (ZF[nm0 * 8 + j] << 7); //nm0 ��3λ 10-1
        zg[10] |= (HZ[9 * 8 + j] << 1); //�� ǰ1λ 10-7
        if (nd1 == 0) {
            nd1 = 11;
        }
        zr[11] |= (ZF[nd1 * 8 + j] << 4); //d1 ǰ4λ 10-8
        zr[11] |= (ZF[nd0 * 8 + j] >> 1); //d0 ��1	11-7
        zr[12] |= (ZF[nd0 * 8 + j] << 7); //d0 ��1	11-7
        zg[12] |= (HZ[8 * 8 + j] << 2); // 1λ	11-8
    } else {
        zr[8] |= HZ[7 * 8 + j]; //ũ��5��   8-77
        if (nm1 == 0) {
            nm1 = 11;
        }
        zg[9] |= (ZF[nm1 * 8 + j] << 4); //nm1  5λ 9-4
        zg[9] |= (ZF[nm0 * 8 + j] >> 1); //nm0 ǰ2λ 9-8
        zg[10] |= (ZF[nm0 * 8 + j] << 7); //nm0 ��3λ 10-1
        zr[10] |= (HZ[9 * 8 + j] << 1); //�� ǰ1λ 10-7
        if (nd1 == 0) {
            nd1 = 11;
        }
        zg[11] |= (ZF[nd1 * 8 + j] << 4); //d1 ǰ4λ 10-8
        zg[11] |= (ZF[nd0 * 8 + j] >> 1); //d0 ��1 11-7
        zg[12] |= (ZF[nd0 * 8 + j] << 7); //d0 ��1 11-7
        zr[12] |= (HZ[8 * 8 + j] << 2); //�� ǰ1λ 11-8
    } //>8

    if (g_dispTorH == DISP_T) {
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
                t1 = 11; //' '
            }
        } else {
            if (t1 == 0x00) {
                t1 = 12; //'-'
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

    if (f1 >= 2) {
        if (sign == true) {
            if (t1 != 0x00) {
                zr[12] |= (DateD[12][j] >> 6); //-	12-6
                zr[13] |= (DateD[12][j] << 2); //-	13-1
            }
        }
        zr[13] |= (DateD[t1][j] >> 1); //t1	13-4
        zr[13] |= (DateD[t0][j] >> 7); //t0 3	13-8
        zr[14] |= (DateD[t0][j] << 1); //t0 2	14-4
        zr[14] |= (DateD[13][j] >> 4); //. `   14-5
        zg[14] |= (DateD[13][j] >> 4); // ��ɫ
        zr[14] |= (DateD[td][j] >> 5); //td 3   14-8
        zr[15] |= (DateD[td][j] << 3); //td 2	15-2
        if (g_dispTorH == DISP_T) {
            zg[15] |= (DateD[14][j] >> 3); // C
        } else {
            zg[15] |= (DateD[15][j] >> 3); // %
        }
    } else {
        if (sign == true) {
            if (t1 != 0x00) {
                zg[12] |= (DateD[12][j] >> 6); //- 12-6
                zg[13] |= (DateD[12][j] << 2); //- 13-1
            }
        }
        zg[13] |= (DateD[t1][j] >> 1); //t1 13-4
        zg[13] |= (DateD[t0][j] >> 7); //t0 3 13-8
        zg[14] |= (DateD[t0][j] << 1); //t0 2 14-4
        zr[14] |= (DateD[13][j] >> 4); //. `   14-5
        zg[14] |= (DateD[td][j] >> 5); //td 3   14-8
        zg[15] |= (DateD[td][j] << 3); //td 2 15-2
        if (g_dispTorH == DISP_T) {
            zr[15] |= (DateD[14][j] >> 3); //C
        } else {
            zr[15] |= (DateD[15][j] >> 3); //%
        }
    }
}

void HUB75D_DispScan(void)
{
    static u8 i;
    u8 k, x;

    DispTime(i);
    if (i < 8) {
        DispDate(i);
    } else {
        DispYinli(i);
    }

    HUB_LAT = 0;
    HUB_OE = 1;
    for (k = 0; k < 8; k++) {
        for (x = 0; x < 8; x++) {
            if (zr[k + 8] & 0x80) {
                HUB_R1 = 1;
            } else {
                HUB_R1 = 0;
            }
            if (zg[k + 8] & 0x80) {
                HUB_G1 = 1;
            } else {
                HUB_G1 = 0;
            }
            if (zb[k + 8] & 0x80) {
                HUB_B1 = 1;
            } else {
                HUB_B1 = 0;
            }

            if (zr[k] & 0x80) {
                HUB_R2 = 1;
            } else {
                HUB_R2 = 0;
            }
            if (zg[k] & 0x80) {
                HUB_G2 = 1;
            } else {
                HUB_G2 = 0;
            }
            if (zb[k] & 0x80) {
                HUB_B2 = 1;
            } else {
                HUB_B2 = 0;
            }

            zr[k + 8] = zr[k + 8] << 1;
            zg[k + 8] = zg[k + 8] << 1;
            zb[k + 8] = zb[k + 8] << 1;
            zr[k] = zr[k] << 1;
            zg[k] = zg[k] << 1;
            zb[k] = zb[k] << 1;
            HUB_CLK = 1;
            HUB_CLK = 0;
        }
    }

    if (i & 0x01) {
        HUB_A = 1;
    } else {
        HUB_A = 0;
    }
    if (i & 0x02) {
        HUB_B = 1;
    } else {
        HUB_B = 0;
    }
    if (i & 0x04) {
        HUB_C = 1;
    } else {
        HUB_C = 0;
    }
    if (i & 0x08) {
        HUB_D = 1;
    } else {
        HUB_D = 0;
    }

    HUB_LAT = 1;
    HUB_OE = 0;

    i++;
    if (i > 15) {
        i = 0x00;
    }
}

void HUB75D_CalculateCalendar(struct TimeType *time)
{
    h0 = time->hour % 10;
    h1 = time->hour / 10;
    f0 = time->min % 10;
    f1 = time->min / 10;
    s0 = time->sec % 10;
    s1 = time->sec / 10;

    d0 = time->day % 10;
    d1 = time->day / 10;
    m0 = time->month % 10;
    m1 = time->month / 10;
    y0 = time->year % 100 % 10;
    y1 = time->year % 100 / 10;
    wk = time->week;
    sd = 11; //":"
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

void HUB75D_CtrDec(void)
{
    static u8 changeCtr;

    changeCtr++;
    if (changeCtr > TEMP_HUMI_DISP_TIME) {
        changeCtr = 0x00;
        if (g_dispTorH == DISP_T) {
            g_dispTorH = DISP_H;
        } else {
            g_dispTorH = DISP_T;
        }
    }

    if (g_displayOffCtr) {
        g_displayOffCtr--;
        if (g_displayOffCtr == 0x00) {
            EnterStandbyMode();
        }
    }
}
