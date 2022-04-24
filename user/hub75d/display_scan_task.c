#include "display_scan_task.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "hub75d.h"
#include "display_task.h"
#include "trace_printf.h"

#define DISP_SCAN_TASK_EVENT_ALL (0xffffffff)

#define DISP_TASK_NAME       "dispScanTask"
#define DISP_TASK_STACK_SIZE (128 * 4)
#define DISP_TASK_PRIORITY   (osPriority_t)osPriorityNormal7

const osThreadAttr_t g_dispScanTaskAttributes = {
    .name       = DISP_TASK_NAME,
    .stack_size = DISP_TASK_STACK_SIZE,
    .priority   = DISP_TASK_PRIORITY,
};

osEventFlagsId_t g_dispScanEvent;

static void DISP_ScanTask(void *argument)
{
    uint32_t event;
    struct RgbType rgbScan;

    while (1) {
        event = osEventFlagsWait(g_dispScanEvent, DISP_SCAN_TASK_EVENT_ALL, osFlagsWaitAny, osWaitForever);

        if ((event & DISP_SCAN_TASK_EVENT_SCAN_LED) == DISP_SCAN_TASK_EVENT_SCAN_LED) {
            HUB75D_DispScan(&rgbScan);
        }
        if ((event & DISP_SCAN_TASK_EVENT_RECEIVED_NEW_DATA) == DISP_SCAN_TASK_EVENT_RECEIVED_NEW_DATA) {
            if (DISP_TaskGetDispScanData(&rgbScan) != osOK) {
                TRACE_PRINTF("Get rgb scan data error!\r\n");
            }
        }
    }
}

osStatus_t DISP_ScanTaskInit(void)
{
    static osThreadId_t dispScanTaskId = NULL;

    if (dispScanTaskId != NULL) {
        return osError;
    }

    g_dispScanEvent = osEventFlagsNew(NULL);
    if (g_dispScanEvent == NULL) {
        return osError;
    }

    dispScanTaskId = osThreadNew(DISP_ScanTask, NULL, &g_dispScanTaskAttributes);
    if (dispScanTaskId == NULL) {
        return osError;
    }

    return osOK;
}

void DISP_ScanTaskSetEvent(uint32_t event)
{
    osEventFlagsSet(g_dispScanEvent, event);
}
