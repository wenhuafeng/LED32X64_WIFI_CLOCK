#ifndef TRACE_H
#define TRACE_H

#include "stdint.h"
#include "utilities_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void cb_timestamp(uint8_t *inData, uint16_t *size);
typedef void cb_overrun(uint8_t **inData, uint16_t *size);

enum TraceStatus {
    UTIL_ADV_TRACE_OK            = 0,  /*!< Operation terminated successfully.*/
    UTIL_ADV_TRACE_INVALID_PARAM = -1, /*!< Invalid Parameter.                */
    UTIL_ADV_TRACE_HW_ERROR      = -2, /*!< Hardware Error.                   */
    UTIL_ADV_TRACE_MEM_FULL      = -3, /*!< Memory fifo full.                 */
    UTIL_ADV_TRACE_UNKNOWN_ERROR = -4, /*!< Unknown Error.                    */
#if defined(TRACE_CONDITIONNAL)
    UTIL_ADV_TRACE_GIVEUP       = -5, /*!< trace give up                     */
    UTIL_ADV_TRACE_REGIONMASKED = -6  /*!< trace region masked               */
#endif
};

struct TraceDriverType {
    enum TraceStatus (*Init)(void (*cb)(void *ptr)); /* Media initialization. */
    enum TraceStatus (*DeInit)(void);                /* Media Un-initialization. */
    enum TraceStatus (*StartRx)(void (*cb)(uint8_t *pdata, uint16_t size,
                                           uint8_t error));  /* Media to start RX process. */
    enum TraceStatus (*Send)(uint8_t *pdata, uint16_t size); /* Media to send data. */
};

enum TraceStatus TRACE_Init(void);
enum TraceStatus TRACE_DeInit(void);
uint8_t TRACE_IsBufferEmpty(void);
enum TraceStatus TRACE_StartRxProcess(void (*UserCallback)(uint8_t *PData, uint16_t Size, uint8_t Error));
enum TraceStatus TRACE_FSend(const char *strFormat, ...);
enum TraceStatus TRACE_Send(const uint8_t *pdata, uint16_t length);
enum TraceStatus TRACE_ZCSend_Allocation(uint16_t Length, uint8_t **pData, uint16_t *FifoSize, uint16_t *WritePos);
enum TraceStatus TRACE_ZCSend_Finalize(void);
void TRACE_PreSendHook(void);
void TRACE_PostSendHook(void);

#if defined(UTIL_ADV_TRACE_OVERRUN)
void TRACE_RegisterOverRunFunction(cb_overrun *cb);
#endif

#if defined(TRACE_CONDITIONNAL)
enum TraceStatus TRACE_COND_FSend(uint32_t VerboseLevel, uint32_t Region, uint32_t TimeStampState,
                                  const char *strFormat, ...);
enum TraceStatus TRACE_COND_ZCSend_Allocation(uint32_t VerboseLevel, uint32_t Region, uint32_t TimeStampState,
                                              uint16_t length, uint8_t **pData, uint16_t *FifoSize, uint16_t *WritePos);
enum TraceStatus TRACE_COND_ZCSend_Finalize(void);
enum TraceStatus TRACE_COND_Send(uint32_t VerboseLevel, uint32_t Region, uint32_t TimeStampState, const uint8_t *pdata,
                                 uint16_t length);
void TRACE_RegisterTimeStampFunction(cb_timestamp *cb);
void TRACE_SetVerboseLevel(uint8_t Level);
uint8_t TRACE_GetVerboseLevel(void);
void TRACE_SetRegion(uint32_t Region);
uint32_t TRACE_GetRegion(void);
void TRACE_ResetRegion(uint32_t Region);
#endif

#ifdef __cplusplus
}
#endif

#endif
