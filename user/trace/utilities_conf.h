#ifndef UTILITIES_CONF_H
#define UTILITIES_CONF_H

#include "cmsis_compiler.h"
#include "mem.h"
#include "tiny_vsnprintf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VLEVEL_OFF 0 /* used to set TRACE_SetVerboseLevel() (not as message param) */
#define VLEVEL_ALWAYS 0 /* used as message params, if this level is given trace */
                        /* will be printed even when TRACE_SetVerboseLevel(OFF) */
#define VLEVEL_L 1 /* just essential traces */
#define VLEVEL_M 2 /* functional traces */
#define VLEVEL_H 3 /* all traces */

#define TS_OFF 0 /* Log without TimeStamp */
#define TS_ON 1 /* Log with TimeStamp */

#define T_REG_OFF 0 /* Log without bitmask */

#if defined(__CC_ARM)
#define UTIL_PLACE_IN_SECTION(__x__) __attribute__((section(__x__), zero_init))
#elif defined(__ICCARM__)
#define UTIL_PLACE_IN_SECTION(__x__) __attribute__((section(__x__)))
#else /* __GNUC__ */
#define UTIL_PLACE_IN_SECTION(__x__) __attribute__((section(__x__)))
#endif /* __CC_ARM | __ICCARM__ | __GNUC__ */

#undef ALIGN
#ifdef WIN32
#define ALIGN(n)
#else
#define ALIGN(n) __attribute__((aligned(n)))
#endif /* WIN32 */

#define SEQ_INIT_CRITICAL_SECTION() INIT_CRITICAL_SECTION()
#define SEQ_ENTER_CRITICAL_SECTION() ENTER_CRITICAL_SECTION()
#define SEQ_EXIT_CRITICAL_SECTION() EXIT_CRITICAL_SECTION()
#define SEQ_MEMSET8(dest, value, size) UTIL_MEM_set_8(dest, value, size)
#define INIT_CRITICAL_SECTION()
#define ENTER_CRITICAL_SECTION()            \
    uint32_t primask_bit = __get_PRIMASK(); \
    __disable_irq();
#define EXIT_CRITICAL_SECTION() __set_PRIMASK(primask_bit)

#define TRACE_CONDITIONNAL /*!< not used */
#define TRACE_UNCHUNK_MODE /*!< not used */
#define TRACE_DEBUG(...) /*!< not used */
#define TRACE_INIT_CRITICAL_SECTION() INIT_CRITICAL_SECTION() /*!< init the critical section in trace feature */
#define TRACE_ENTER_CRITICAL_SECTION() ENTER_CRITICAL_SECTION() /*!< enter the critical section in trace feature */
#define TRACE_EXIT_CRITICAL_SECTION() EXIT_CRITICAL_SECTION() /*!< exit the critical section in trace feature */
#define TRACE_TMP_BUF_SIZE (512U) /*!< default trace buffer size */
#define TRACE_TMP_MAX_TIMESTMAP_SIZE (15U) /*!< default trace timestamp size */
#define TRACE_FIFO_SIZE (1024U) /*!< default trace fifo size */
#define TRACE_MEMSET8(dest, value, size) \
    UTIL_MEM_set_8((dest), (value), (size)) /*!< memset utilities interface to trace feature */
#define TRACE_VSNPRINTF(...) tiny_vsnprintf_like(__VA_ARGS__) /*!< vsnprintf utilities interface to trace feature */

#ifdef __cplusplus
}
#endif

#endif
