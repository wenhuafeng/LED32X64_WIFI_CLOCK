#ifndef TRACE_PRINTF_H
#define TRACE_PRINTF_H

#include "trace.h"

#define TRACE_PRINTF(...)                                                   \
    do {                                                              \
        TRACE_COND_FSend(VLEVEL_OFF, T_REG_OFF, TS_OFF, __VA_ARGS__); \
    } while (0)

#endif
