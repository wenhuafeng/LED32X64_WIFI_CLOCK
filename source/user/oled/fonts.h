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
#include <stdint.h>

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
