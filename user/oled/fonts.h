/**
 * original author:  Tilen Majerle<tilen@majerle.eu>
 * modification for STM32f10x: Alexander Lutsai<s.lyra@ya.ru>
   ----------------------------------------------------------------------
   	Copyright (C) Alexander Lutsai, 2016
    Copyright (C) Tilen Majerle, 2015

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------
 */
#ifndef FONTS_H
#define FONTS_H 120

/* C++ detection */
#ifdef __cplusplus
extern C {
#endif

/*
 * Default fonts library. It is used in all LCD based libraries.
 * par Supported fonts
 * Currently, these fonts are supported:
 *  - 7 x 10 pixels
 *  - 11 x 18 pixels
 *  - 16 x 26 pixels
 */
#include "stm32f1xx_hal.h"
#include "string.h"

struct FontDefineType {
    uint8_t fontWidth;
    uint8_t fontHeight;
    const uint16_t *data;
};

struct FontSizeType {
    uint16_t length;
    uint16_t height;
};

extern struct FontDefineType Font_7x10;
extern struct FontDefineType Font_11x18;
extern struct FontDefineType Font_16x26;

char* FONTS_GetStringSize(char* str, struct FontSizeType* size, struct FontDefineType* font);

/* C++ detection */
#ifdef __cplusplus
}
#endif


#endif
