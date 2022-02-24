#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include "utilities_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UTIL_MEM_PLACE_IN_SECTION(__x__) UTIL_PLACE_IN_SECTION(__x__)
#define UTIL_MEM_ALIGN ALIGN

void UTIL_MEM_cpy_8(void *dst, const void *src, uint16_t size);
void UTIL_MEM_cpyr_8(void *dst, const void *src, uint16_t size);
void UTIL_MEM_set_8(void *dst, uint8_t value, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif
