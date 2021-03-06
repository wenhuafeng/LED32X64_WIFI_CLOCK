#ifndef SSD1306_H
#define SSD1306_H

/* C++ detection */
#ifdef __cplusplus
extern C {
#endif

/*
 * This SSD1306 LCD uses I2C for communication
 * Library features functions for drawing lines, rectangles and circles.
 * It also allows you to draw texts and characters using appropriate functions provided in library.
 * Default pinout
 * SSD1306    |STM32F10x    |DESCRIPTION
 * VCC        |3.3V         |
 * GND        |GND          |
 * SCL        |PB6          |Serial clock line
 * SDA        |PB7          |Serial data line
 */

#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "fonts.h"

/* I2C address */
#ifndef SSD1306_I2C_ADDR
#define SSD1306_I2C_ADDR 0x78
#endif

/* SSD1306 width in pixels */
#ifndef SSD1306_WIDTH
#define SSD1306_WIDTH 128
#endif

/* SSD1306 LCD height in pixels */
#ifndef SSD1306_HEIGHT
#define SSD1306_HEIGHT 64
#endif

/**
 * @brief  SSD1306 color enumeration
 */
enum Ssd1306ColorType {
    SSD1306_COLOR_BLACK = 0x00, /*!< Black color, no pixel */
    SSD1306_COLOR_WHITE = 0x01  /*!< Pixel is set. Color depends on LCD */
};

bool SSD1306_Init(void);
void SSD1306_UpdateScreen(void);

/**
 * @brief  Toggles pixels invertion inside internal RAM
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  None
 * @retval None
 */
void SSD1306_ToggleInvert(void);

/**
 * @brief  Draws pixel at desired location
 * @note   @ref SSD1306_UpdateScreen() must called after that in order to see updated LCD screen
 * @param  x: X location. This parameter can be a value between 0 and SSD1306_WIDTH - 1
 * @param  y: Y location. This parameter can be a value between 0 and SSD1306_HEIGHT - 1
 * @param  color: Color to be used for screen fill. This parameter can be a value of @ref enum Ssd1306ColorType enumeration
 * @retval None
 */
void SSD1306_DrawPixel(uint16_t x, uint16_t y, enum Ssd1306ColorType color);

/**
 * @brief  Sets cursor pointer to desired location for strings
 * @param  x: X location. This parameter can be a value between 0 and SSD1306_WIDTH - 1
 * @param  y: Y location. This parameter can be a value between 0 and SSD1306_HEIGHT - 1
 * @retval None
 */
void SSD1306_GotoXY(uint16_t x, uint16_t y);

/**
 * @brief  Puts character to internal RAM
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  ch: Character to be written
 * @param  *Font: Pointer to @ref FontDef_t structure with used font
 * @param  color: Color used for drawing. This parameter can be a value of @ref enum Ssd1306ColorType enumeration
 * @retval Character written
 */
char SSD1306_Putc(char ch, struct FontDefineType* font, enum Ssd1306ColorType color);

/**
 * @brief  Puts string to internal RAM
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  *str: String to be written
 * @param  *Font: Pointer to @ref FontDef_t structure with used font
 * @param  color: Color used for drawing. This parameter can be a value of @ref enum Ssd1306ColorType enumeration
 * @retval Zero on success or character value when function failed
 */
char SSD1306_Puts(char* str, struct FontDefineType* font, enum Ssd1306ColorType color);

/**
 * @brief  Draws line on LCD
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x0: Line X start point. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y0: Line Y start point. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  x1: Line X end point. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y1: Line Y end point. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  c: Color to be used. This parameter can be a value of @ref enum Ssd1306ColorType enumeration
 * @retval None
 */
void SSD1306_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, enum Ssd1306ColorType color);

/**
 * @brief  Draws rectangle on LCD
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: Top left X start point. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y: Top left Y start point. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  w: Rectangle width in units of pixels
 * @param  h: Rectangle height in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref enum Ssd1306ColorType enumeration
 * @retval None
 */
void SSD1306_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, enum Ssd1306ColorType color);

/**
 * @brief  Draws filled rectangle on LCD
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: Top left X start point. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y: Top left Y start point. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  w: Rectangle width in units of pixels
 * @param  h: Rectangle height in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref enum Ssd1306ColorType enumeration
 * @retval None
 */
void SSD1306_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, enum Ssd1306ColorType color);

/**
 * @brief  Draws triangle on LCD
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x1: First coordinate X location. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y1: First coordinate Y location. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  x2: Second coordinate X location. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y2: Second coordinate Y location. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  x3: Third coordinate X location. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y3: Third coordinate Y location. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  c: Color to be used. This parameter can be a value of @ref enum Ssd1306ColorType enumeration
 * @retval None
 */
void SSD1306_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, enum Ssd1306ColorType color);

/**
 * @brief  Draws circle to STM buffer
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: X location for center of circle. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y: Y location for center of circle. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  r: Circle radius in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref enum Ssd1306ColorType enumeration
 * @retval None
 */
void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, enum Ssd1306ColorType color);

/**
 * @brief  Draws filled circle to STM buffer
 * @note   @ref SSD1306_UpdateScreen() must be called after that in order to see updated LCD screen
 * @param  x: X location for center of circle. Valid input is 0 to SSD1306_WIDTH - 1
 * @param  y: Y location for center of circle. Valid input is 0 to SSD1306_HEIGHT - 1
 * @param  r: Circle radius in units of pixels
 * @param  c: Color to be used. This parameter can be a value of @ref enum Ssd1306ColorType enumeration
 * @retval None
 */
void SSD1306_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, enum Ssd1306ColorType color);

/**
 * @brief  Draws the Bitmap
 * @param  X:  X location to start the Drawing
 * @param  Y:  Y location to start the Drawing
 * @param  *bitmap : Pointer to the bitmap
 * @param  W : width of the image
 * @param  H : height of the image
 * @param  color : 1-> white/blue, 0-> black
 */
void SSD1306_DrawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, enum Ssd1306ColorType color);

void SSD1306_ScrollRight(uint8_t startRow, uint8_t endRow);
void SSD1306_ScrollLeft(uint8_t startRow, uint8_t endRow);
void SSD1306_Scrolldiagright(uint8_t startRow, uint8_t endRow);
void SSD1306_Scrolldiagleft(uint8_t startRow, uint8_t endRow);
void SSD1306_Stopscroll(void);
void SSD1306_InvertDisplay(uint8_t i);
void SSD1306_Clear(void);
void SSD1306_On(void);
void SSD1306_Off(void);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif
