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
#define DISP_TASK_PRIORITY   (osPriority_t)osPriorityNormal7

const osThreadAttr_t g_dispScanTaskAttributes = {
    .name       = DISP_TASK_NAME,
    .stack_size = DISP_TASK_STACK_SIZE,
    .priority   = DISP_TASK_PRIORITY,
};

static osEventFlagsId_t g_dispScanEvent;
static struct RgbType g_rgbScan;

static void DISP_ScanTask(void *argument)
{
    uint32_t event;

    HAL_TIM_Base_MspInit(&htim4);
    HAL_TIM_Base_Start_IT(&htim4);
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

void DISP_ScanLed(void)
{
    HUB75D_DispScan(&g_rgbScan);
}
