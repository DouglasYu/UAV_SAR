/** 
 * @file widget.c
 * @brief Widget configuration and remote control logic.
 * @author Douglas Yu
 */

/* Includes ------------------------------------------------------------------*/
#include <dji_widget.h>
#include <dji_logger.h>
#include <dji_platform.h>
#include <stdio.h>
#include <time.h>
#include "fc_subscription.h"
#include "utils/util_misc.h"

/* Private constants ---------------------------------------------------------*/
#define FC_SUBSCRIPTION_TASK_FREQ       (1)
#define WIDGET_DIR_PATH_LEN_MAX         (256)
#define WIDGET_TASK_STACK_SIZE          (2048)

/* Private types -------------------------------------------------------------*/

/* Private functions declaration ---------------------------------------------*/
static T_DjiReturnCode DjiTestWidget_SetWidgetValue(E_DjiWidgetType widgetType, uint32_t index, 
                                                    int32_t value, void *userData);
static T_DjiReturnCode DjiTestWidget_GetWidgetValue(E_DjiWidgetType widgetType, uint32_t index, 
                                                    int32_t *value, void *userData);

/* Private variables ---------------------------------------------------------*/
static const T_DjiWidgetHandlerListItem s_widgetHandlerList[] = {
    {0, DJI_WIDGET_TYPE_SWITCH, DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
    {1, DJI_WIDGET_TYPE_SWITCH, DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
    {2, DJI_WIDGET_TYPE_BUTTON, DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL},
    {3, DJI_WIDGET_TYPE_INT_INPUT_BOX, DjiTestWidget_SetWidgetValue, DjiTestWidget_GetWidgetValue, NULL}
};

static char *s_widgetTypeNameArray[] = {
    "Unknown",
    "Button",
    "Switch",
    "Scale",
    "List",
    "Int input box"
};

char lastWavFilePath[100]; /* the path to the newest wav file */
int droneVelocity = 2; /* the velocity for data analysis */

/* the list storing the value of each component */
static int32_t s_widgetValueList[sizeof(s_widgetHandlerList) / sizeof(T_DjiWidgetHandlerListItem)] = {0};
static const uint32_t s_widgetHandlerListCount = sizeof(s_widgetHandlerList) / sizeof(T_DjiWidgetHandlerListItem);
static bool s_isWidgetFileDirPathConfigured = false;
static char s_widgetFileDirPath[DJI_FILE_PATH_SIZE_MAX] = {0};
static T_DjiTaskHandle s_recordThread = NULL;
static T_DjiTaskHandle fc_startSubscriptionThread = NULL;
static T_DjiTaskHandle fc_endSubscriptionThread = NULL;

/* Exported functions definition ---------------------------------------------*/
T_DjiReturnCode My_WidgetStartService(void)
{
    T_DjiReturnCode djiStat;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    // test the user log
    // USER_LOG_ERROR("Hello there!");

    //Step 1 : Init DJI Widget
    djiStat = DjiWidget_Init();
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Dji test widget init error, stat = 0x%08llX", djiStat);
        return djiStat;
    }
    // USER_LOG_INFO("Step 1 Finished!");

    //Step 2 : Set UI Config (Linux environment)
    char curFileDirPath[WIDGET_DIR_PATH_LEN_MAX];
    char tempPath[WIDGET_DIR_PATH_LEN_MAX];
    djiStat = DjiUserUtil_GetCurrentFileDirPath(__FILE__, WIDGET_DIR_PATH_LEN_MAX, curFileDirPath);
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Get file current path error, stat = 0x%08llX", djiStat);
        return djiStat;
    }

    if (s_isWidgetFileDirPathConfigured == true) {
        snprintf(tempPath, WIDGET_DIR_PATH_LEN_MAX, "%swidget_file/en_big_screen", s_widgetFileDirPath);
    } else {
        snprintf(tempPath, WIDGET_DIR_PATH_LEN_MAX, "%swidget_file/en_big_screen", curFileDirPath);
    }

    //set default ui config path
    djiStat = DjiWidget_RegDefaultUiConfigByDirPath(tempPath);
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Add default widget ui config error, stat = 0x%08llX", djiStat);
        return djiStat;
    }

    //set ui config for English language
    djiStat = DjiWidget_RegUiConfigByDirPath(DJI_MOBILE_APP_LANGUAGE_ENGLISH,
                                             DJI_MOBILE_APP_SCREEN_TYPE_BIG_SCREEN,
                                             tempPath);
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Add widget ui config error, stat = 0x%08llX", djiStat);
        return djiStat;
    }

    //set ui config for Chinese language
    if (s_isWidgetFileDirPathConfigured == true) {
        snprintf(tempPath, WIDGET_DIR_PATH_LEN_MAX, "%swidget_file/cn_big_screen", s_widgetFileDirPath);
    } else {
        snprintf(tempPath, WIDGET_DIR_PATH_LEN_MAX, "%swidget_file/cn_big_screen", curFileDirPath);
    }

    djiStat = DjiWidget_RegUiConfigByDirPath(DJI_MOBILE_APP_LANGUAGE_CHINESE,
                                             DJI_MOBILE_APP_SCREEN_TYPE_BIG_SCREEN,
                                             tempPath);
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Add widget ui config error, stat = 0x%08llX", djiStat);
        return djiStat;
    }

    // USER_LOG_INFO("Step 2 Finished!");

    //Step 3 : Set widget handler list
    djiStat = DjiWidget_RegHandlerList(s_widgetHandlerList, s_widgetHandlerListCount);
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Set widget handler list error, stat = 0x%08llX", djiStat);
        return djiStat;
    }

    // USER_LOG_INFO("Step 3 Finished!");

    //Step 4 : Run widget api sample task
    // if (osalHandler->TaskCreate("user_widget_task", DjiTest_WidgetTask, WIDGET_TASK_STACK_SIZE, NULL,
    //                             &s_widgetTestThread) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //     USER_LOG_ERROR("Dji widget test task create error.");
    //     return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    // }

    // USER_LOG_INFO("Step 4 Finished!");

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

/* Private functions definition ---------------------------------------------*/

static void* StartDataSubscription(void *arg){
    T_DjiReturnCode djiStat;

    USER_UTIL_UNUSED(arg);
    djiStat = FcSubscriptionStartService();
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Start fcSubscription service error.");
    }
}

static void* EndDataSubscription(void *arg){
    T_DjiReturnCode djiStat;
    
    USER_UTIL_UNUSED(arg);
    djiStat = FcSubscriptionStopService();
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Stop fcSubscription service error.");
    }
}

static void* RecordTask(void *arg){
    USER_UTIL_UNUSED(arg);

    time_t currentTime = time(NULL);
    struct tm *localTime = localtime(&currentTime);
    char command[100];

    int year, month, day, hour, minute, second;
    year = localTime->tm_year + 1900;
    month = localTime->tm_mon + 1;
    day = localTime->tm_mday;
    hour = localTime->tm_hour;
    minute = localTime->tm_min;
    second = localTime->tm_sec;

    char ret[100];
    FILE* fp;

    fp = popen("arecord -l", "r");
    if(fp != NULL){
        fgets(ret, sizeof(ret), fp);
        fgets(ret, sizeof(ret), fp);
    }
    USER_LOG_INFO("Get arecord card number %c.", ret[5]);
    pclose(fp);

    sprintf(command, "arecord -D \"plughw:%c,0\" -f S16_LE -r 48000 -c 2 -d 1200 -t wav ./record_files/record_%04d%02d%02d_%02d-%02d-%02d.wav\n", 
        ret[5], year, month, day, hour, minute, second);
    
    sprintf(lastWavFilePath, "record_%04d%02d%02d_%02d-%02d-%02d.wav", 
        year, month, day, hour, minute, second);
    // sprintf(lastWavFilePath, "404%04d", year + 1900);

    // sprintf(command, "arecord -D \"plughw:1,0\" -f S16_LE -r 48000 -c 2 -d 1200 -t wav ./record.wav\n");

    system(command);
}

static T_DjiReturnCode DjiTestWidget_SetWidgetValue(E_DjiWidgetType widgetType, uint32_t index, int32_t value,
                                                    void *userData)
{
    USER_UTIL_UNUSED(userData);

    T_DjiOsalHandler *osalHandler = NULL;
    T_DjiReturnCode djiStat;
    osalHandler = DjiPlatform_GetOsalHandler();

    /* debug message */
    USER_LOG_INFO("Set widget value, widgetType = %s, widgetIndex = %d ,widgetValue = %d",
                  s_widgetTypeNameArray[widgetType], index, value);
    s_widgetValueList[index] = value;

    /* control logic */
    switch (index)
    {
    /** this switch controls the power of the SAR module */
    case 0:
        if (value == 1){
            /* turn on SAR module power */
            USER_LOG_INFO("SAR power on.");
        } else {
            /* turn off SAR module power */
            USER_LOG_INFO("SAR power off.");
        }
        
        break;

    /* This button controls the recording of coming back signals */
    case 1:
        if(value == 1){
            /* start telemetry subscription */
            USER_LOG_INFO("Start recording.");

            FcSubscriptionStartService();

            /* start recording process */
            fc_startSubscriptionThread = NULL;
            if(s_recordThread != NULL){
                system("sudo killall -9 arecord");
            }
            
            if (s_recordThread == NULL){
                osalHandler->TaskCreate("arecord_task", RecordTask, 4096, NULL, &s_recordThread);
            }
        } else {
            /* stop telemetry subscription */
            USER_LOG_INFO("Stop recording.");

            FcSubscriptionStopService();

            if (s_recordThread != NULL){
                osalHandler->TaskDestroy(s_recordThread);
            }
            s_recordThread = NULL;
            system("sudo killall -9 arecord");

        }
        break;

    /* This button will invoke a python script to process the received data */
    case 2:
        if(value == 1){ 
            USER_LOG_INFO("Data processing begins.");
            char command[100];
            sprintf(command, "python ../main.py ./record_files/%s %d 0 0 ~/UAV_SAR/module_sample/camera_emu/media_file/",
                lastWavFilePath, droneVelocity);
            // USER_LOG_INFO("%s", command);
            // FILE* fp;
            // fp = popen(command, "r");
            // pclose(fp);
            USER_LOG_INFO("Data process ends.");
        }
        break;

    /* this input box is to set the velocity value for data analysis */
    case 3:
        USER_LOG_INFO("Input box value: %d", value);
        droneVelocity = value;
        break;
    
    default:
        break;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode DjiTestWidget_GetWidgetValue(E_DjiWidgetType widgetType, uint32_t index, int32_t *value,
                                                    void *userData)
{
    USER_UTIL_UNUSED(userData);
    USER_UTIL_UNUSED(widgetType);

    *value = s_widgetValueList[index];

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}
