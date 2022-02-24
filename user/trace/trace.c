#include "trace.h"
#include <stdarg.h>
#include <stdio.h>
#include "utilities_conf.h"
#include "trace_uart_if.h"

#if !defined(UTIL_ADV_TRACE_MEMLOCATION)
#define UTIL_ADV_TRACE_MEMLOCATION
#endif

#if defined(UTIL_ADV_TRACE_OVERRUN)
enum TraceOverrunStatus {
    TRACE_OVERRUN_NONE = 0,     /* overrun status none. */
    TRACE_OVERRUN_INDICATION,   /* overrun status an indication shall be sent. */
    TRACE_OVERRUN_TRANSFERT,    /* overrun status data transfer ongoing. */
    TRACE_OVERRUN_EXECUTED,     /* overrun status data transfer complete. */
};
#endif

#if defined(TRACE_UNCHUNK_MODE)
enum TraceUnchunkStatus {
    TRACE_UNCHUNK_NONE = 0, /* unchunk status none. */
    TRACE_UNCHUNK_DETECTED, /* unchunk status an unchunk has been detected. */
    TRACE_UNCHUNK_TRANSFER  /* unchunk status an unchunk transfer is ongoing. */
};
#endif

/* advanced macro to override to enable debug mode */
#ifndef TRACE_DEBUG
#define TRACE_DEBUG(...)
#endif

struct TraceContext {
#if defined(TRACE_UNCHUNK_MODE)
    uint16_t unchunkEnabled;                /* unchunk enable. */
    enum TraceUnchunkStatus unchunkStatus;  /* unchunk transfer status. */
#endif
#if defined(UTIL_ADV_TRACE_OVERRUN)
    enum TraceOverrunStatus overRunStatus;  /* overrun status. */
    cb_overrun *overrunFunc;                /* overrun function */
#endif
#if defined(TRACE_CONDITIONNAL)
    cb_timestamp *timestampFunc;    /* ptr of function used to insert time stamp. */
    uint8_t currentVerboseLevel;    /* verbose level used. */
    uint32_t regionMask;            /* mask of the enabled region. */
#endif
    uint16_t traceReadPointer;      /* read pointer the trace system. */
    uint16_t traceWritePointer;     /* write pointer the trace system. */
    uint16_t traceSentSize;         /* size of the latest transfer. */
    uint16_t traceLock;             /* lock counter of the trace system. */
};

static struct TraceContext g_traceContext;
static UTIL_ADV_TRACE_MEMLOCATION uint8_t g_traceBuffer[TRACE_FIFO_SIZE];

#if defined(TRACE_CONDITIONNAL) && defined(TRACE_UNCHUNK_MODE)
/*
 * temporary buffer used by TRACE_COND_FSend
 * a temporary buffers variable used to evaluate a formatted string size.
 */
static uint8_t sztmp[TRACE_TMP_BUF_SIZE];
#endif
static struct TraceDriverType *g_traceDriver;

/*
 * @brief  Lock the trace buffer.
 * @retval None.
 */
static void TRACE_Lock(void)
{
    TRACE_ENTER_CRITICAL_SECTION();
    g_traceContext.traceLock++;
    TRACE_EXIT_CRITICAL_SECTION();
}

/*
 * @brief  UnLock the trace buffer.
 * @retval None.
 */
static void TRACE_UnLock(void)
{
    TRACE_ENTER_CRITICAL_SECTION();
    g_traceContext.traceLock--;
    TRACE_EXIT_CRITICAL_SECTION();
}

/*
 * @brief  UnLock the trace buffer.
 * @retval None.
 */
static uint32_t TRACE_IsLocked(void)
{
    return (g_traceContext.traceLock == 0u ? 0u : 1u);
}

/*
 * @brief Tx callback called by the low layer level to inform a transfer complete
 * @param Ptr pointer not used only for HAL compatibility
 * @retval none
 */
static void TRACE_TxCpltCallback(void *Ptr)
{
    uint8_t *ptr = NULL;
    TRACE_ENTER_CRITICAL_SECTION();

#if defined(UTIL_ADV_TRACE_OVERRUN)
    if (g_traceContext.overRunStatus == TRACE_OVERRUN_TRANSFERT) {
        g_traceContext.overRunStatus = TRACE_OVERRUN_EXECUTED;
        TRACE_DEBUG("\n--TRACE_Send overrun complete--\n");
        g_traceContext.traceSentSize = 0u;
    }
#endif

#if defined(TRACE_UNCHUNK_MODE)
    if (TRACE_UNCHUNK_TRANSFER == g_traceContext.unchunkStatus) {
        g_traceContext.unchunkStatus = TRACE_UNCHUNK_NONE;
        g_traceContext.traceReadPointer = 0;
        TRACE_DEBUG("\nTRACE_TxCpltCallback::unchunk complete\n");
    } else {
        g_traceContext.traceReadPointer = (g_traceContext.traceReadPointer + g_traceContext.traceSentSize) % TRACE_FIFO_SIZE;
    }
#else
    g_traceContext.traceReadPointer = (g_traceContext.traceReadPointer + g_traceContext.traceSentSize) % TRACE_FIFO_SIZE;
#endif

#if defined(UTIL_ADV_TRACE_OVERRUN)
    if (g_traceContext.overRunStatus == TRACE_OVERRUN_INDICATION) {
        uint8_t *ptr = NULL;
        g_traceContext.overRunStatus = TRACE_OVERRUN_TRANSFERT;
        TRACE_EXIT_CRITICAL_SECTION();
        g_traceContext.overrunFunc(&ptr, &g_traceContext.traceSentSize);
        TRACE_DEBUG("\n--Driver_Send overrun(%d)--\n", g_traceContext.traceSentSize);
        g_traceDriver->Send(ptr, g_traceContext.traceSentSize);
        return;
    }
#endif

    if ((g_traceContext.traceReadPointer != g_traceContext.traceWritePointer) && (1u == g_traceContext.traceLock)) {
#ifdef TRACE_UNCHUNK_MODE
        if (TRACE_UNCHUNK_DETECTED == g_traceContext.unchunkStatus) {
            g_traceContext.traceSentSize = g_traceContext.unchunkEnabled - g_traceContext.traceReadPointer;
            g_traceContext.unchunkStatus = TRACE_UNCHUNK_TRANSFER;
            g_traceContext.unchunkEnabled = 0;

            TRACE_DEBUG("\nTRACE_TxCpltCallback::unchunk start(%d,%d)\n", g_traceContext.unchunkEnabled,
                        g_traceContext.traceReadPointer);

            if (0u == g_traceContext.traceSentSize) {
                /* this case occurs when an ongoing write aligned the Rd position with chunk position */
                /* in that case the unchunk is forgot */
                g_traceContext.unchunkStatus = TRACE_UNCHUNK_NONE;
                g_traceContext.traceReadPointer = 0;
            }
        }

        if (TRACE_UNCHUNK_NONE == g_traceContext.unchunkStatus) {
#endif
            if (g_traceContext.traceWritePointer > g_traceContext.traceReadPointer) {
                g_traceContext.traceSentSize = g_traceContext.traceWritePointer - g_traceContext.traceReadPointer;
            } else {
                /* traceReadPointer > traceWritePointer */
                g_traceContext.traceSentSize = TRACE_FIFO_SIZE - g_traceContext.traceReadPointer;
            }
#ifdef TRACE_UNCHUNK_MODE
        }
#endif
        ptr = &g_traceBuffer[g_traceContext.traceReadPointer];
        TRACE_EXIT_CRITICAL_SECTION();
        TRACE_DEBUG("\n--TRACE_Send(%d-%d)--\n", g_traceContext.traceReadPointer, g_traceContext.traceSentSize);
        g_traceDriver->Send(ptr, g_traceContext.traceSentSize);
    } else {
        TRACE_EXIT_CRITICAL_SECTION();
        TRACE_PostSendHook();
        TRACE_UnLock();
    }
}

/*
 * @brief  allocate space inside the buffer to push data
 * @param  Size to allocate within fifo
 * @param  Pos position within the fifo
 * @retval write position inside the buffer is -1 no space available.
 */
static int16_t TRACE_AllocateBufer(uint16_t size, uint16_t *pos)
{
    uint16_t freesize;
    int16_t ret = -1;

    TRACE_ENTER_CRITICAL_SECTION();

    if (g_traceContext.traceWritePointer == g_traceContext.traceReadPointer) {
#ifdef TRACE_UNCHUNK_MODE
        freesize = (uint16_t)(TRACE_FIFO_SIZE - g_traceContext.traceWritePointer);
        if ((size >= freesize) && (g_traceContext.traceReadPointer > size)) {
            g_traceContext.unchunkStatus = TRACE_UNCHUNK_DETECTED;
            g_traceContext.unchunkEnabled = g_traceContext.traceWritePointer;
            freesize = g_traceContext.traceReadPointer;
            g_traceContext.traceWritePointer = 0;
        }
#else
        /* need to add buffer full management*/
        freesize = (int16_t)TRACE_FIFO_SIZE;
#endif
    } else {
#ifdef TRACE_UNCHUNK_MODE
        if (g_traceContext.traceWritePointer > g_traceContext.traceReadPointer) {
            freesize = (uint16_t)(TRACE_FIFO_SIZE - g_traceContext.traceWritePointer);
            if ((size >= freesize) && (g_traceContext.traceReadPointer > size)) {
                g_traceContext.unchunkStatus = TRACE_UNCHUNK_DETECTED;
                g_traceContext.unchunkEnabled = g_traceContext.traceWritePointer;
                freesize = g_traceContext.traceReadPointer;
                g_traceContext.traceWritePointer = 0;
            }
        } else {
            freesize = (uint16_t)(g_traceContext.traceReadPointer - g_traceContext.traceWritePointer);
        }
#else
        if (g_traceContext.traceWritePointer > g_traceContext.traceReadPointer) {
            freesize = TRACE_FIFO_SIZE - g_traceContext.traceWritePointer + g_traceContext.traceReadPointer;
        } else {
            freesize = g_traceContext.traceReadPointer - g_traceContext.traceWritePointer;
        }
#endif
    }

    if (freesize > size) {
        *pos = g_traceContext.traceWritePointer;
        g_traceContext.traceWritePointer = (g_traceContext.traceWritePointer + size) % TRACE_FIFO_SIZE;
        ret = 0;
#if defined(UTIL_ADV_TRACE_OVERRUN)
        if (g_traceContext.overRunStatus == TRACE_OVERRUN_EXECUTED) {
            /* clear the over run */
            g_traceContext.overRunStatus = TRACE_OVERRUN_NONE;
        }
#endif

#ifdef TRACE_UNCHUNK_MODE
        TRACE_DEBUG("\n--TRACE_AllocateBufer(%d-%d-%d::%d-%d)--\n", freesize - size, size,
                    g_traceContext.unchunkEnabled, g_traceContext.traceReadPointer, g_traceContext.traceWritePointer);
#else
        TRACE_DEBUG("\n--TRACE_AllocateBufer(%d-%d::%d-%d)--\n", freesize - size, size,
                    g_traceContext.traceReadPointer, g_traceContext.traceWritePointer);
#endif
    }
#if defined(UTIL_ADV_TRACE_OVERRUN)
    else {
        if ((g_traceContext.overRunStatus == TRACE_OVERRUN_NONE) && (NULL != g_traceContext.overrunFunc)) {
            TRACE_DEBUG(":TRACE_OVERRUN_INDICATION");
            g_traceContext.overRunStatus = TRACE_OVERRUN_INDICATION;
        }
    }
#endif

    TRACE_EXIT_CRITICAL_SECTION();
    return ret;
}

enum TraceStatus TRACE_Init(void)
{
    /* initialize the Ptr for Read/Write */
    (void)TRACE_MEMSET8(&g_traceContext, 0x0, sizeof(struct TraceContext));
    (void)TRACE_MEMSET8(&g_traceBuffer, 0x0, sizeof(g_traceBuffer));

#if defined(TRACE_UNCHUNK_MODE)
    TRACE_DEBUG("\nUNCHUNK_MODE\n");
#endif
    /* Allocate Lock resource */
    TRACE_INIT_CRITICAL_SECTION();

    g_traceDriver = TRACE_UART_GetDriver();

    /* Initialize the Low Level interface */
    return g_traceDriver->Init(TRACE_TxCpltCallback);
}

enum TraceStatus TRACE_DeInit(void)
{
    /* Un-initialize the Low Level interface */
    return g_traceDriver->DeInit();
}

uint8_t TRACE_IsBufferEmpty(void)
{
    /* check of the buffer is empty */
    if (g_traceContext.traceWritePointer == g_traceContext.traceReadPointer) {
        return 1;
    }
    return 0;
}

enum TraceStatus TRACE_StartRxProcess(void (*UserCallback)(uint8_t *inData, uint16_t size, uint8_t error))
{
    /* start the RX process */
    return g_traceDriver->StartRx(UserCallback);
}

/*
 * send the data of the trace to low layer
 * Status based on @ref enum TraceStatus
 */
static enum TraceStatus TRACE_SendData(void)
{
    enum TraceStatus ret = UTIL_ADV_TRACE_OK;
    uint8_t *ptr = NULL;

    TRACE_ENTER_CRITICAL_SECTION();

    if (TRACE_IsLocked() == 0u) {
        TRACE_Lock();

        if (g_traceContext.traceReadPointer != g_traceContext.traceWritePointer) {
#ifdef TRACE_UNCHUNK_MODE
            if (TRACE_UNCHUNK_DETECTED == g_traceContext.unchunkStatus) {
                g_traceContext.traceSentSize = (uint16_t)(g_traceContext.unchunkEnabled - g_traceContext.traceReadPointer);
                g_traceContext.unchunkStatus = TRACE_UNCHUNK_TRANSFER;
                g_traceContext.unchunkEnabled = 0;

                TRACE_DEBUG("\nTRACE_TxCpltCallback::unchunk start(%d,%d)\n", g_traceContext.unchunkEnabled,
                            g_traceContext.traceReadPointer);

                if (0u == g_traceContext.traceSentSize) {
                    g_traceContext.unchunkStatus = TRACE_UNCHUNK_NONE;
                    g_traceContext.traceReadPointer = 0;
                }
            }

            if (TRACE_UNCHUNK_NONE == g_traceContext.unchunkStatus) {
#endif
                if (g_traceContext.traceWritePointer > g_traceContext.traceReadPointer) {
                    g_traceContext.traceSentSize = g_traceContext.traceWritePointer - g_traceContext.traceReadPointer;
                } else /* traceReadPointer > traceWritePointer */
                {
                    g_traceContext.traceSentSize = TRACE_FIFO_SIZE - g_traceContext.traceReadPointer;
                }
#ifdef TRACE_UNCHUNK_MODE
            }
#endif
            ptr = &g_traceBuffer[g_traceContext.traceReadPointer];

            TRACE_EXIT_CRITICAL_SECTION();
            TRACE_PreSendHook();

            TRACE_DEBUG("\n--TRACE_Send(%d-%d)--\n", g_traceContext.traceReadPointer, g_traceContext.traceSentSize);
            ret = g_traceDriver->Send(ptr, g_traceContext.traceSentSize);
        } else {
            TRACE_UnLock();
            TRACE_EXIT_CRITICAL_SECTION();
        }
    } else {
        TRACE_EXIT_CRITICAL_SECTION();
    }

    return ret;
}

enum TraceStatus TRACE_Send(const uint8_t *inData, uint16_t length)
{
    enum TraceStatus ret;
    uint16_t writepos;
    uint32_t idx;

    TRACE_Lock();

    /* if allocation is ok, write data into the buffer */
    if (TRACE_AllocateBufer(length, &writepos) != -1) {
        /* initialize the Ptr for Read/Write */
        for (idx = 0u; idx < length; idx++) {
            g_traceBuffer[writepos] = inData[idx];
            writepos = (uint16_t)((writepos + 1u) % TRACE_FIFO_SIZE);
        }
        TRACE_UnLock();

        ret = TRACE_SendData();
    } else {
        TRACE_UnLock();
        ret = UTIL_ADV_TRACE_MEM_FULL;
    }

    return ret;
}

#if defined(TRACE_CONDITIONNAL)
enum TraceStatus TRACE_COND_FSend(uint32_t verboseLevel, uint32_t region,
                                  uint32_t timeStampState, const char *strFormat, ...)
{
    va_list vaArgs;
#if defined(TRACE_UNCHUNK_MODE)
    uint8_t buf[TRACE_TMP_MAX_TIMESTMAP_SIZE];
    uint16_t timestamp_size = 0u;
    uint16_t writepos;
    uint16_t idx;
#else
    uint8_t buf[TRACE_TMP_BUF_SIZE + TRACE_TMP_MAX_TIMESTMAP_SIZE];
#endif
    uint16_t buff_size = 0u;

    /* check verbose level */
    if (!(g_traceContext.currentVerboseLevel >= verboseLevel)) {
        return UTIL_ADV_TRACE_GIVEUP;
    }

    if ((region & g_traceContext.regionMask) != region) {
        return UTIL_ADV_TRACE_REGIONMASKED;
    }

#if defined(TRACE_UNCHUNK_MODE)
    if ((g_traceContext.timestampFunc != NULL) && (timeStampState != 0u)) {
        g_traceContext.timestampFunc(buf, &timestamp_size);
    }

    va_start(vaArgs, strFormat);
    buff_size = (uint16_t)TRACE_VSNPRINTF((char *)sztmp, TRACE_TMP_BUF_SIZE, strFormat, vaArgs);

    TRACE_Lock();

    /* if allocation is ok, write data into the buffer */
    if (TRACE_AllocateBufer((buff_size + timestamp_size), &writepos) != -1) {
#if defined(UTIL_ADV_TRACE_OVERRUN)
        TRACE_ENTER_CRITICAL_SECTION();
        if (g_traceContext.overRunStatus == TRACE_OVERRUN_EXECUTED) {
            /* clear the over run */
            g_traceContext.overRunStatus = TRACE_OVERRUN_NONE;
        }
        TRACE_EXIT_CRITICAL_SECTION();
#endif

        /* copy the timestamp */
        for (idx = 0u; idx < timestamp_size; idx++) {
            g_traceBuffer[writepos] = buf[idx];
            writepos = writepos + 1u;
        }

        /* copy the data */
        (void)TRACE_VSNPRINTF((char *)(&g_traceBuffer[writepos]), TRACE_TMP_BUF_SIZE, strFormat,
                                       vaArgs);
        va_end(vaArgs);

        TRACE_UnLock();

        return TRACE_SendData();
    }

    va_end(vaArgs);
    TRACE_UnLock();
#if defined(UTIL_ADV_TRACE_OVERRUN)
    TRACE_ENTER_CRITICAL_SECTION();
    if ((g_traceContext.overRunStatus == TRACE_OVERRUN_NONE) && (NULL != g_traceContext.overrunFunc)) {
        TRACE_DEBUG("TRACE_Send:TRACE_OVERRUN_INDICATION");
        g_traceContext.overRunStatus = TRACE_OVERRUN_INDICATION;
    }
    TRACE_EXIT_CRITICAL_SECTION();
#endif

    return UTIL_ADV_TRACE_MEM_FULL;

#else
    if ((g_traceContext.timestampFunc != NULL) && (TimeStampState != 0u)) {
        g_traceContext.timestampFunc(buf, &buff_size);
    }

    va_start(vaArgs, strFormat);
    buff_size += (uint16_t)TRACE_VSNPRINTF((char *)(buf + buff_size), TRACE_TMP_BUF_SIZE, strFormat,
                                                    vaArgs);
    va_end(vaArgs);

    return TRACE_Send(buf, buff_size);
#endif
}
#endif

enum TraceStatus TRACE_FSend(const char *strFormat, ...)
{
    uint8_t buf[TRACE_TMP_BUF_SIZE];
    va_list vaArgs;

    va_start(vaArgs, strFormat);
    uint16_t bufSize = (uint16_t)TRACE_VSNPRINTF((char *)buf, TRACE_TMP_BUF_SIZE, strFormat, vaArgs);
    va_end(vaArgs);

    return TRACE_Send(buf, bufSize);
}

#if defined(TRACE_CONDITIONNAL)
enum TraceStatus TRACE_COND_ZCSend_Allocation(uint32_t verboseLevel, uint32_t region,
                                                              uint32_t timeStampState, uint16_t length, uint8_t **inData,
                                                              uint16_t *fifoSize, uint16_t *writePos)
{
    enum TraceStatus ret = UTIL_ADV_TRACE_OK;
    uint16_t writepos;
    uint8_t timestamp_ptr[TRACE_TMP_MAX_TIMESTMAP_SIZE];
    uint16_t timestamp_size = 0u;

    /* check verbose level */
    if (!(g_traceContext.currentVerboseLevel >= verboseLevel)) {
        return UTIL_ADV_TRACE_GIVEUP;
    }

    if ((region & g_traceContext.regionMask) != region) {
        return UTIL_ADV_TRACE_REGIONMASKED;
    }

    if ((g_traceContext.timestampFunc != NULL) && (timeStampState != 0u)) {
        g_traceContext.timestampFunc(timestamp_ptr, &timestamp_size);
    }

    TRACE_Lock();

    /* if allocation is ok, write data into the buffer */
    if (TRACE_AllocateBufer(length + timestamp_size, &writepos) != -1) {
        /* fill time stamp information */
        for (uint16_t index = 0u; index < timestamp_size; index++) {
            g_traceBuffer[writepos] = timestamp_ptr[index];
            writepos = (uint16_t)((writepos + 1u) % TRACE_FIFO_SIZE);
        }

        /*user fill */
        *inData = g_traceBuffer;
        *fifoSize = (uint16_t)TRACE_FIFO_SIZE;
        *writePos = writepos;
    } else {
        TRACE_UnLock();
        ret = UTIL_ADV_TRACE_MEM_FULL;
    }
    return ret;
}

enum TraceStatus TRACE_ZCSend_Finalize(void)
{
    TRACE_UnLock();
    return TRACE_SendData();
}

enum TraceStatus TRACE_COND_ZCSend_Finalize(void)
{
    return TRACE_ZCSend_Finalize();
}
#endif

enum TraceStatus TRACE_ZCSend_Allocation(uint16_t length, uint8_t **inData, uint16_t *fifoSize,
                                                         uint16_t *writePos)
{
    enum TraceStatus ret = UTIL_ADV_TRACE_OK;
    uint16_t writepos;

    TRACE_Lock();

    /* if allocation is ok, write data into the buffer */
    if (TRACE_AllocateBufer(length, &writepos) != -1) {
        /*user fill */
        *inData = g_traceBuffer;
        *fifoSize = TRACE_FIFO_SIZE;
        *writePos = (uint16_t)writepos;
    } else {
        TRACE_UnLock();
        ret = UTIL_ADV_TRACE_MEM_FULL;
    }

    return ret;
}

#if defined(TRACE_CONDITIONNAL)
enum TraceStatus TRACE_COND_Send(uint32_t verboseLevel, uint32_t region, uint32_t timeStampState,
                                 const uint8_t *inData, uint16_t length)
{
    enum TraceStatus ret;
    uint16_t writepos;
    uint32_t idx;
    uint8_t timestamp_ptr[TRACE_TMP_MAX_TIMESTMAP_SIZE];
    uint16_t timestamp_size = 0u;

    /* check verbose level */
    if (!(g_traceContext.currentVerboseLevel >= verboseLevel)) {
        return UTIL_ADV_TRACE_GIVEUP;
    }

    if ((region & g_traceContext.regionMask) != region) {
        return UTIL_ADV_TRACE_REGIONMASKED;
    }

    if ((g_traceContext.timestampFunc != NULL) && (timeStampState != 0u)) {
        g_traceContext.timestampFunc(timestamp_ptr, &timestamp_size);
    }

    TRACE_Lock();

    /* if allocation is ok, write data into the buffer */
    if (TRACE_AllocateBufer(length + timestamp_size, &writepos) != -1) {
        /* fill time stamp information */
        for (idx = 0; idx < timestamp_size; idx++) {
            g_traceBuffer[writepos] = timestamp_ptr[idx];
            writepos = (uint16_t)((writepos + 1u) % TRACE_FIFO_SIZE);
        }

        for (idx = 0u; idx < length; idx++) {
            g_traceBuffer[writepos] = inData[idx];
            writepos = (uint16_t)((writepos + 1u) % TRACE_FIFO_SIZE);
        }

        TRACE_UnLock();
        ret = TRACE_SendData();
    } else {
        TRACE_UnLock();
        ret = UTIL_ADV_TRACE_MEM_FULL;
    }

    return ret;
}
#endif

#if defined(UTIL_ADV_TRACE_OVERRUN)
void TRACE_RegisterOverRunFunction(cb_overrun *cb)
{
    g_traceContext.overrunFunc = *cb;
}
#endif

#if defined(TRACE_CONDITIONNAL)
void TRACE_RegisterTimeStampFunction(cb_timestamp *cb)
{
    g_traceContext.timestampFunc = *cb;
}

void TRACE_SetVerboseLevel(uint8_t level)
{
    g_traceContext.currentVerboseLevel = level;
}

uint8_t TRACE_GetVerboseLevel(void)
{
    return g_traceContext.currentVerboseLevel;
}

void TRACE_SetRegion(uint32_t region)
{
    g_traceContext.regionMask |= region;
}

uint32_t TRACE_GetRegion(void)
{
    return g_traceContext.regionMask;
}

void TRACE_ResetRegion(uint32_t region)
{
    g_traceContext.regionMask &= ~region;
}
#endif

__WEAK void TRACE_PreSendHook(void)
{
}

__WEAK void TRACE_PostSendHook(void)
{
}
