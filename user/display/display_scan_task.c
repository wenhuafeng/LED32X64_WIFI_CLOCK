#include "display_scan_task.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "tim.h"
#include "hub75d.h"
#include "display_task.h"
#include "trace.h"

#define LOG_TAG "display_scan_task"

#define DISP_SCAN_TASK_EVENT_ALL (0x00ffffff)

#define DISP_TASK_NAME       "dispScanTask"
#define DISP_TASK_STACK_SIZE (128 * 16)
#define DISP_TASK_PRIORITY   (osPriority_t) osPriorityNormal7

const osThreadAttr_t g_dispScanTaskAttributes = {
    .name       = DISP_TASK_NAME,
    .stack_size = DISP_TASK_STACK_SIZE,
    .priority   = DISP_TASK_PRIORITY,
};

static osEventFlagsId_t g_dispScanEvent = NULL;
static osThreadId_t g_dispScanTaskId    = NULL;
static struct RgbType g_rgbScan         = { 0 };

static void DISP_ScanTask(void *argument)
{
    uint32_t event;

    LOGI(LOG_TAG, "display scan task enter!\r\n");

    while (1) {
        event = osEventFlagsWait(g_dispScanEvent, DISP_SCAN_TASK_EVENT_ALL, osFlagsWaitAny, osWaitForever);

        if ((event & DISP_SCAN_TASK_EVENT_RECEIVED_NEW_DATA) == DISP_SCAN_TASK_EVENT_RECEIVED_NEW_DATA) {
            if (DISP_TaskGetDispScanData(&g_rgbScan) != osOK) {
                LOGI(LOG_TAG, "Get rgb scan data error!\r\n");
            }
        }
    }
}

osStatus_t DISP_ScanTaskInit(void)
{
    if (g_dispScanTaskId != NULL) {
        return osError;
    }

    g_dispScanEvent = osEventFlagsNew(NULL);
    if (g_dispScanEvent == NULL) {
        return osError;
    }

    g_dispScanTaskId = osThreadNew(DISP_ScanTask, NULL, &g_dispScanTaskAttributes);
    if (g_dispScanTaskId == NULL) {
        return osError;
    }

    return osOK;
}

void DISP_ScanTaskSetEvent(uint32_t event)
{
    osEventFlagsSet(g_dispScanEvent, event);
}

void DISP_ScanLed(void)
{
    HUB75D_DispScan(&g_rgbScan);
}

void DISP_ScanTaskSuspend(void)
{
    osStatus_t ret;

    ret = osThreadSuspend(g_dispScanTaskId);
    if (ret != osOK) {
        LOGE(LOG_TAG, "display scan task suspend\r\n");
    } else {
        LOGI(LOG_TAG, "display scan task suspend\r\n");
    }
}

void DISP_ScanTaskResume(void)
{
    osStatus_t ret;

    ret = osThreadResume(g_dispScanTaskId);
    if (ret != osOK) {
        LOGE(LOG_TAG, "display scan task resume\r\n");
    } else {
        LOGI(LOG_TAG, "display scan task resume\r\n");
    }
}
