#include "main.h"
#if defined(SUPPORT_OLED_DISPLAY) && SUPPORT_OLED_DISPLAY
#include "ssd1306.h"
#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"
#include "trace.h"

#define TAG_LOG "ssd1306"

/* Absolute value */
#define ABS(x) ((x) > 0 ? (x) : -(x))

/* Private g_ssd1306 structure */
struct Ssd1306Type {
    uint16_t currentX;
    uint16_t currentY;
    uint8_t inverted;
    uint8_t initialized;
};

#define SSD1306_RIGHT_HORIZONTAL_SCROLL              0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL               0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL  0x2A
#define SSD1306_DEACTIVATE_SCROLL                    0x2E // Stop scroll
#define SSD1306_ACTIVATE_SCROLL                      0x2F // Start scroll
#define SSD1306_SET_VERTICAL_SCROLL_AREA             0xA3 // Set scroll range

#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7

/* Private variable */
static struct Ssd1306Type g_ssd1306;
/* g_ssd1306 data buffer */
static uint8_t g_ssd1306Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

//  _____ ___   _____
// |_   _|__ \ / ____|
//   | |    ) | |
//   | |   / /| |
//  _| |_ / /_| |____
// |_____|____|\_____|

static void SSD1306_I2C_Init(void)
{
    MX_I2C2_Init();
    HAL_I2C_MspInit(&hi2c2);
    uint32_t p = 250000;
    while (p > 0) {
        p--;
    }
}

static void SSD1306_I2C_WriteMulti(uint8_t address, uint8_t reg, uint8_t *data, uint16_t count)
{
    uint8_t i;
    uint8_t dt[256];

    dt[0] = reg;
    for (i = 0; i < count; i++) {
        dt[i + 1] = data[i];
    }
    if (HAL_I2C_Master_Transmit(&hi2c2, address, dt, count + 1, 100) != HAL_OK) {
        LOGE(TAG_LOG, "ssd1306 write multi error!\r\n");
    }
}

static void SSD1306_WriteCommand(uint8_t cmd)
{
    uint8_t dt[2];

    dt[0] = 0x00;
    dt[1] = cmd;
    if (HAL_I2C_Master_Transmit(&hi2c2, SSD1306_I2C_ADDR, dt, 2, 100) != HAL_OK) {
        LOGE(TAG_LOG, "ssd1306 write command error!\r\n");
    }
}

#if 0
static void ssd1306_WriteData(uint8_t in)
{
    uint8_t dt[2];

    dt[0] = 0x40;
    dt[1] = in;
    if (HAL_I2C_Master_Transmit(&hi2c2, SSD1306_I2C_ADDR, dt, 2, 10) != HAL_OK) {
        LOGE(TAG_LOG, "ssd1306 write data error!\r\n");
    }
}
#endif

void SSD1306_ScrollRight(uint8_t startRow, uint8_t endRow)
{
    SSD1306_WriteCommand(SSD1306_RIGHT_HORIZONTAL_SCROLL); // send 0x26
    SSD1306_WriteCommand(0x00);                            // send dummy
    SSD1306_WriteCommand(startRow);                        // start page address
    SSD1306_WriteCommand(0X00);                            // time interval 5 frames
    SSD1306_WriteCommand(endRow);                          // end page address
    SSD1306_WriteCommand(0X00);
    SSD1306_WriteCommand(0XFF);
    SSD1306_WriteCommand(SSD1306_ACTIVATE_SCROLL); // start scroll
}

void SSD1306_ScrollLeft(uint8_t startRow, uint8_t endRow)
{
    SSD1306_WriteCommand(SSD1306_LEFT_HORIZONTAL_SCROLL); // send 0x26
    SSD1306_WriteCommand(0x00);                           // send dummy
    SSD1306_WriteCommand(startRow);                       // start page address
    SSD1306_WriteCommand(0X00);                           // time interval 5 frames
    SSD1306_WriteCommand(endRow);                         // end page address
    SSD1306_WriteCommand(0X00);
    SSD1306_WriteCommand(0XFF);
    SSD1306_WriteCommand(SSD1306_ACTIVATE_SCROLL); // start scroll
}

void SSD1306_Scrolldiagright(uint8_t startRow, uint8_t endRow)
{
    SSD1306_WriteCommand(SSD1306_SET_VERTICAL_SCROLL_AREA); // sect the area
    SSD1306_WriteCommand(0x00);                             // write dummy
    SSD1306_WriteCommand(SSD1306_HEIGHT);

    SSD1306_WriteCommand(SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(startRow);
    SSD1306_WriteCommand(0X00);
    SSD1306_WriteCommand(endRow);
    SSD1306_WriteCommand(0x01);
    SSD1306_WriteCommand(SSD1306_ACTIVATE_SCROLL);
}

void SSD1306_Scrolldiagleft(uint8_t startRow, uint8_t endRow)
{
    SSD1306_WriteCommand(SSD1306_SET_VERTICAL_SCROLL_AREA); // sect the area
    SSD1306_WriteCommand(0x00);                             // write dummy
    SSD1306_WriteCommand(SSD1306_HEIGHT);

    SSD1306_WriteCommand(SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
    SSD1306_WriteCommand(0x00);
    SSD1306_WriteCommand(startRow);
    SSD1306_WriteCommand(0X00);
    SSD1306_WriteCommand(endRow);
    SSD1306_WriteCommand(0x01);
    SSD1306_WriteCommand(SSD1306_ACTIVATE_SCROLL);
}

void SSD1306_Stopscroll(void)
{
    SSD1306_WriteCommand(SSD1306_DEACTIVATE_SCROLL);
}

/* inverts the display i = 1->inverted, i = 0->normal */
void SSD1306_InvertDisplay(uint8_t i)
{
    uint8_t invert;

    if (i) {
        invert = SSD1306_INVERTDISPLAY;
    } else {
        invert = SSD1306_NORMALDISPLAY;
    }

    SSD1306_WriteCommand(invert);
}

void SSD1306_DrawPixel(uint16_t x, uint16_t y, enum Ssd1306ColorType color)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        /* Error */
        return;
    }

    /* Check if pixels are inverted */
    if (g_ssd1306.inverted) {
        color = (enum Ssd1306ColorType) !color;
    }

    /* Set color */
    if (color == SSD1306_COLOR_WHITE) {
        g_ssd1306Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
    } else {
        g_ssd1306Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

void SSD1306_DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, enum Ssd1306ColorType color)
{
    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte      = 0;

    for (int16_t j = 0; j < h; j++, y++) {
        for (int16_t i = 0; i < w; i++) {
            if (i & 7) {
                byte <<= 1;
            } else {
                byte = (*(const uint8_t *)(&bitmap[j * byteWidth + i / 8]));
            }
            if (byte & 0x80) {
                SSD1306_DrawPixel(x + i, y, color);
            }
        }
    }
}

void SSD1306_Fill(enum Ssd1306ColorType color)
{
    /* Set memory */
    memset(g_ssd1306Buffer, (color == SSD1306_COLOR_BLACK) ? 0x00 : 0xFF, sizeof(g_ssd1306Buffer));
}

void SSD1306_UpdateScreen(void)
{
    uint8_t m;

    for (m = 0; m < 8; m++) {
        SSD1306_WriteCommand(0xB0 + m);
        SSD1306_WriteCommand(0x00);
        SSD1306_WriteCommand(0x10);

        /* Write multi data */
        SSD1306_I2C_WriteMulti(SSD1306_I2C_ADDR, 0x40, &g_ssd1306Buffer[SSD1306_WIDTH * m], SSD1306_WIDTH);
    }
}

bool SSD1306_Init(void)
{
    /* Init I2C */
    SSD1306_I2C_Init();

    /* Check if LCD connected to I2C */
    if (HAL_I2C_IsDeviceReady(&hi2c2, SSD1306_I2C_ADDR, 1, 20000) != HAL_OK) {
        /* Return false */
        return false;
    }

    /* A little delay */
    uint32_t p = 2500;
    while (p > 0) {
        p--;
    }

    /* Init LCD */
    SSD1306_WriteCommand(0xAE); // display off
    SSD1306_WriteCommand(0x20); // Set Memory Addressing Mode
    SSD1306_WriteCommand(0x10); // 00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
    SSD1306_WriteCommand(0xB0); // Set Page Start Address for Page Addressing Mode,0-7
    SSD1306_WriteCommand(0xC8); // Set COM Output Scan Direction
    SSD1306_WriteCommand(0x00); // ---set low column address
    SSD1306_WriteCommand(0x10); // ---set high column address
    SSD1306_WriteCommand(0x40); // --set start line address
    SSD1306_WriteCommand(0x81); // --set contrast control register
    SSD1306_WriteCommand(0xFF);
    SSD1306_WriteCommand(0xA1); // --set segment re-map 0 to 127
    SSD1306_WriteCommand(0xA6); // --set normal display
    SSD1306_WriteCommand(0xA8); // --set multiplex ratio(1 to 64)
    SSD1306_WriteCommand(0x3F); //
    SSD1306_WriteCommand(0xA4); // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content
    SSD1306_WriteCommand(0xD3); // -set display offset
    SSD1306_WriteCommand(0x00); // -not offset
    SSD1306_WriteCommand(0xD5); // --set display clock divide ratio/oscillator frequency
    SSD1306_WriteCommand(0xF0); // --set divide ratio
    SSD1306_WriteCommand(0xD9); // --set pre-charge period
    SSD1306_WriteCommand(0x22); //
    SSD1306_WriteCommand(0xDA); // --set com pins hardware configuration
    SSD1306_WriteCommand(0x12);
    SSD1306_WriteCommand(0xDB); // --set vcomh
    SSD1306_WriteCommand(0x20); // 0x20,0.77xVcc
    SSD1306_WriteCommand(0x8D); // --set DC-DC enable
    SSD1306_WriteCommand(0x14);
    SSD1306_WriteCommand(0xAF); // --turn on g_ssd1306 panel

    SSD1306_WriteCommand(SSD1306_DEACTIVATE_SCROLL);

    /* Clear screen */
    SSD1306_Fill(SSD1306_COLOR_BLACK);

    /* Update screen */
    SSD1306_UpdateScreen();

    /* Set default values */
    g_ssd1306.currentX = 0;
    g_ssd1306.currentY = 0;

    /* initialized OK */
    g_ssd1306.initialized = 1;

    /* Return OK */
    return true;
}

void SSD1306_ToggleInvert(void)
{
    uint16_t i;

    /* Toggle invert */
    g_ssd1306.inverted = !g_ssd1306.inverted;

    /* Do memory toggle */
    for (i = 0; i < sizeof(g_ssd1306Buffer); i++) {
        g_ssd1306Buffer[i] = ~g_ssd1306Buffer[i];
    }
}

void SSD1306_GotoXY(uint16_t x, uint16_t y)
{
    /* Set write pointers */
    g_ssd1306.currentX = x;
    g_ssd1306.currentY = y;
}

char SSD1306_Putc(char ch, struct FontDefineType *font, enum Ssd1306ColorType color)
{
    uint32_t i, b, j;

    /* Check available space in LCD */
    if (SSD1306_WIDTH <= (g_ssd1306.currentX + font->fontWidth) ||
        SSD1306_HEIGHT <= (g_ssd1306.currentY + font->fontHeight)) {
        /* Error */
        return 0;
    }

    /* Go through font */
    for (i = 0; i < font->fontHeight; i++) {
        b = font->data[(ch - 32) * font->fontHeight + i];
        for (j = 0; j < font->fontWidth; j++) {
            if ((b << j) & 0x8000) {
                SSD1306_DrawPixel(g_ssd1306.currentX + j, (g_ssd1306.currentY + i), (enum Ssd1306ColorType)color);
            } else {
                SSD1306_DrawPixel(g_ssd1306.currentX + j, (g_ssd1306.currentY + i), (enum Ssd1306ColorType) !color);
            }
        }
    }

    /* Increase pointer */
    g_ssd1306.currentX += font->fontWidth;

    /* Return character written */
    return ch;
}

char SSD1306_Puts(char *str, struct FontDefineType *font, enum Ssd1306ColorType color)
{
    /* Write characters */
    while (*str) {
        /* Write character by character */
        if (SSD1306_Putc(*str, font, color) != *str) {
            /* Return error */
            return *str;
        }

        /* Increase string pointer */
        str++;
    }

    /* Everything OK, zero should be returned */
    return *str;
}

void SSD1306_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, enum Ssd1306ColorType color)
{
    int16_t dx, dy, sx, sy, err, e2, i, tmp;

    /* Check for overflow */
    if (x0 >= SSD1306_WIDTH) {
        x0 = SSD1306_WIDTH - 1;
    }
    if (x1 >= SSD1306_WIDTH) {
        x1 = SSD1306_WIDTH - 1;
    }
    if (y0 >= SSD1306_HEIGHT) {
        y0 = SSD1306_HEIGHT - 1;
    }
    if (y1 >= SSD1306_HEIGHT) {
        y1 = SSD1306_HEIGHT - 1;
    }

    dx  = (x0 < x1) ? (x1 - x0) : (x0 - x1);
    dy  = (y0 < y1) ? (y1 - y0) : (y0 - y1);
    sx  = (x0 < x1) ? 1 : -1;
    sy  = (y0 < y1) ? 1 : -1;
    err = ((dx > dy) ? dx : -dy) / 2;

    if (dx == 0) {
        if (y1 < y0) {
            tmp = y1;
            y1  = y0;
            y0  = tmp;
        }

        if (x1 < x0) {
            tmp = x1;
            x1  = x0;
            x0  = tmp;
        }

        /* Vertical line */
        for (i = y0; i <= y1; i++) {
            SSD1306_DrawPixel(x0, i, color);
        }

        /* Return from function */
        return;
    }

    if (dy == 0) {
        if (y1 < y0) {
            tmp = y1;
            y1  = y0;
            y0  = tmp;
        }

        if (x1 < x0) {
            tmp = x1;
            x1  = x0;
            x0  = tmp;
        }

        /* Horizontal line */
        for (i = x0; i <= x1; i++) {
            SSD1306_DrawPixel(i, y0, color);
        }

        /* Return from function */
        return;
    }

    while (1) {
        SSD1306_DrawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
}

void SSD1306_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, enum Ssd1306ColorType color)
{
    /* Check input parameters */
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        /* Return error */
        return;
    }

    /* Check width and height */
    if ((x + w) >= SSD1306_WIDTH) {
        w = SSD1306_WIDTH - x;
    }
    if ((y + h) >= SSD1306_HEIGHT) {
        h = SSD1306_HEIGHT - y;
    }

    /* Draw 4 lines */
    SSD1306_DrawLine(x, y, x + w, y, color);         /* Top line */
    SSD1306_DrawLine(x, y + h, x + w, y + h, color); /* Bottom line */
    SSD1306_DrawLine(x, y, x, y + h, color);         /* Left line */
    SSD1306_DrawLine(x + w, y, x + w, y + h, color); /* Right line */
}

void SSD1306_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, enum Ssd1306ColorType color)
{
    uint8_t i;

    /* Check input parameters */
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        /* Return error */
        return;
    }

    /* Check width and height */
    if ((x + w) >= SSD1306_WIDTH) {
        w = SSD1306_WIDTH - x;
    }
    if ((y + h) >= SSD1306_HEIGHT) {
        h = SSD1306_HEIGHT - y;
    }

    /* Draw lines */
    for (i = 0; i <= h; i++) {
        /* Draw lines */
        SSD1306_DrawLine(x, y + i, x + w, y + i, color);
    }
}

void SSD1306_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3,
                          enum Ssd1306ColorType color)
{
    /* Draw lines */
    SSD1306_DrawLine(x1, y1, x2, y2, color);
    SSD1306_DrawLine(x2, y2, x3, y3, color);
    SSD1306_DrawLine(x3, y3, x1, y1, color);
}

void SSD1306_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3,
                                enum Ssd1306ColorType color)
{
    int16_t deltax, deltay, x, y, xinc1, xinc2, yinc1, yinc2, den, num, numadd, numpixels, curpixel;

    deltax = ABS(x2 - x1);
    deltay = ABS(y2 - y1);
    x      = x1;
    y      = y1;

    if (x2 >= x1) {
        xinc1 = 1;
        xinc2 = 1;
    } else {
        xinc1 = -1;
        xinc2 = -1;
    }

    if (y2 >= y1) {
        yinc1 = 1;
        yinc2 = 1;
    } else {
        yinc1 = -1;
        yinc2 = -1;
    }

    if (deltax >= deltay) {
        xinc1     = 0;
        yinc2     = 0;
        den       = deltax;
        num       = deltax / 2;
        numadd    = deltay;
        numpixels = deltax;
    } else {
        xinc2     = 0;
        yinc1     = 0;
        den       = deltay;
        num       = deltay / 2;
        numadd    = deltax;
        numpixels = deltay;
    }

    for (curpixel = 0; curpixel <= numpixels; curpixel++) {
        SSD1306_DrawLine(x, y, x3, y3, color);

        num += numadd;
        if (num >= den) {
            num -= den;
            x += xinc1;
            y += yinc1;
        }
        x += xinc2;
        y += yinc2;
    }
}

void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, enum Ssd1306ColorType color)
{
    int16_t f    = 1 - r;
    int16_t ddfX = 1;
    int16_t ddfY = -2 * r;
    int16_t x    = 0;
    int16_t y    = r;

    SSD1306_DrawPixel(x0, y0 + r, color);
    SSD1306_DrawPixel(x0, y0 - r, color);
    SSD1306_DrawPixel(x0 + r, y0, color);
    SSD1306_DrawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddfY += 2;
            f += ddfY;
        }
        x++;
        ddfX += 2;
        f += ddfX;

        SSD1306_DrawPixel(x0 + x, y0 + y, color);
        SSD1306_DrawPixel(x0 - x, y0 + y, color);
        SSD1306_DrawPixel(x0 + x, y0 - y, color);
        SSD1306_DrawPixel(x0 - x, y0 - y, color);

        SSD1306_DrawPixel(x0 + y, y0 + x, color);
        SSD1306_DrawPixel(x0 - y, y0 + x, color);
        SSD1306_DrawPixel(x0 + y, y0 - x, color);
        SSD1306_DrawPixel(x0 - y, y0 - x, color);
    }
}

void SSD1306_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, enum Ssd1306ColorType color)
{
    int16_t f    = 1 - r;
    int16_t ddfX = 1;
    int16_t ddfY = -2 * r;
    int16_t x    = 0;
    int16_t y    = r;

    SSD1306_DrawPixel(x0, y0 + r, color);
    SSD1306_DrawPixel(x0, y0 - r, color);
    SSD1306_DrawPixel(x0 + r, y0, color);
    SSD1306_DrawPixel(x0 - r, y0, color);
    SSD1306_DrawLine(x0 - r, y0, x0 + r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddfY += 2;
            f += ddfY;
        }
        x++;
        ddfX += 2;
        f += ddfX;

        SSD1306_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
        SSD1306_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, color);

        SSD1306_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, color);
        SSD1306_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, color);
    }
}

void SSD1306_Clear(void)
{
    SSD1306_Fill(SSD1306_COLOR_BLACK);
    SSD1306_UpdateScreen();
}

void SSD1306_On(void)
{
    SSD1306_WriteCommand(0x8D);
    SSD1306_WriteCommand(0x14);
    SSD1306_WriteCommand(0xAF);
}

void SSD1306_Off(void)
{
    SSD1306_WriteCommand(0x8D);
    SSD1306_WriteCommand(0x10);
    SSD1306_WriteCommand(0xAE);
}

#endif
