/**
 ********************************************************************
 * @file    main.c
 * @brief
 *
 * @copyright (c) 2021 DJI. All rights reserved.
 *
 * All information contained herein is, and remains, the property of DJI.
 * The intellectual and technical concepts contained herein are proprietary
 * to DJI and may be covered by U.S. and foreign patents, patents in process,
 * and protected by trade secret or copyright law.  Dissemination of this
 * information, including but not limited to data and other proprietary
 * material(s) incorporated within the information, in any form, is strictly
 * prohibited without the express written consent of DJI.
 *
 * If you receive this source code without DJI’s authorization, you may not
 * further disseminate the information, and you must immediately remove the
 * source code and notify DJI of its removal. DJI reserves the right to pursue
 * legal actions against you for any loss(es) or damage(s) caused by your
 * failure to do so.
 *
 *********************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <dji_platform.h>
#include <dji_logger.h>
#include <dji_core.h>
#include <dji_power_management.h>
#include <dji_high_speed_data_channel.h>
#include <utils/util_misc.h>
#include <errno.h>
#include <signal.h>
// #include <power_management/test_power_management.h>
// #include <upgrade/test_upgrade.h>
// #include <payload_collaboration/test_payload_collaboration.h>
// #include <mop_channel/test_mop_channel.h>
// #include <xport/test_payload_xport.h>
// #include <gimbal_emu/test_payload_gimbal_emu.h>
// #include <fc_subscription/test_fc_subscription.h>
// #include <camera_emu/test_payload_cam_emu_media.h>
// #include <camera_emu/test_payload_cam_emu_base.h>
#include "monitor/sys_monitor.h"
#include "osal/osal.h"
#include "osal/osal_fs.h"
#include "osal/osal_socket.h"
// #include "upgrade_platform_opt/upgrade_platform_opt_linux.h"
#include "../hal/hal_uart.h"
#include "../hal/hal_network.h"
// #include "../hal/hal_usb_bulk.h"
#include "dji_sdk_app_info.h"
#include "dji_aircraft_info.h"
// #include "widget/test_widget.h"
// #include "widget_interaction_test/test_widget_interaction.h"
// #include "data_transmission/test_data_transmission.h"
#include "dji_sdk_config.h"
#include "fc_subscription.h"
#include "widget.h"
#include "payload_xport.h"
#include "../module_sample/camera_emu/test_payload_cam_emu_media.h"
#include "../module_sample/camera_emu/test_payload_cam_emu_base.h"
// #include "../module_sample/utils/util_file.h"

/* Private constants ---------------------------------------------------------*/
#define DJI_LOG_PATH                    "Logs/DJI"
#define DJI_LOG_INDEX_FILE_NAME         "Logs/latest"
#define DJI_LOG_FOLDER_NAME             "Logs"
#define DJI_LOG_PATH_MAX_SIZE           (128)
#define DJI_LOG_FOLDER_NAME_MAX_SIZE    (32)
#define DJI_LOG_MAX_COUNT               (10)
#define DJI_SYSTEM_CMD_STR_MAX_SIZE     (64)
#define DJI_SYSTEM_RESULT_STR_MAX_SIZE  (128)

#define DJI_USE_WIDGET_INTERACTION       0
// #define USER_UTIL_UNUSED(x)                                 ((x) = (x))

/* Private types -------------------------------------------------------------*/
typedef struct {
    pid_t tid;
    char name[16];
    float pcpu;
} T_ThreadAttribute;

/* Private values -------------------------------------------------------------*/
static FILE *s_djiLogFile;
static FILE *s_djiLogFileCnt;
static pthread_t s_monitorThread = 0;

/* Private functions declaration ---------------------------------------------*/
static T_DjiReturnCode DjiUser_FillInUserInfo(T_DjiUserInfo *userInfo);
static T_DjiReturnCode DjiUser_PrintConsole(const uint8_t *data, uint16_t dataLen);
static T_DjiReturnCode DjiUser_LocalWrite(const uint8_t *data, uint16_t dataLen);
static T_DjiReturnCode DjiUser_LocalWriteFsInit(const char *path);
static void *DjiUser_MonitorTask(void *argument);
// static T_DjiReturnCode DjiTest_HighPowerApplyPinInit();
// static T_DjiReturnCode DjiTest_WriteHighPowerApplyPin(E_DjiPowerManagementPinState pinState);
// static bool DjiUser_CheckNetCableConnectStatus(void);

/* Exported functions definition ---------------------------------------------*/


/* Private functions definition-----------------------------------------------*/
static T_DjiReturnCode DjiUser_PrintConsole(const uint8_t *data, uint16_t dataLen)
{
    USER_UTIL_UNUSED(dataLen);

    printf("%s", data);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode DjiUser_LocalWrite(const uint8_t *data, uint16_t dataLen)
{
    int32_t realLen;

    if (s_djiLogFile == NULL) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    realLen = fwrite(data, 1, dataLen, s_djiLogFile);
    fflush(s_djiLogFile);
    if (realLen == dataLen) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    } else {
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }
}

static T_DjiReturnCode DjiUser_FillInUserInfo(T_DjiUserInfo *userInfo)
{
    memset(userInfo->appName, 0, sizeof(userInfo->appName));
    memset(userInfo->appId, 0, sizeof(userInfo->appId));
    memset(userInfo->appKey, 0, sizeof(userInfo->appKey));
    memset(userInfo->appLicense, 0, sizeof(userInfo->appLicense));
    memset(userInfo->developerAccount, 0, sizeof(userInfo->developerAccount));
    memset(userInfo->baudRate, 0, sizeof(userInfo->baudRate));

    if (strlen(USER_APP_NAME) >= sizeof(userInfo->appName) ||
        strlen(USER_APP_ID) > sizeof(userInfo->appId) ||
        strlen(USER_APP_KEY) > sizeof(userInfo->appKey) ||
        strlen(USER_APP_LICENSE) > sizeof(userInfo->appLicense) ||
        strlen(USER_DEVELOPER_ACCOUNT) >= sizeof(userInfo->developerAccount) ||
        strlen(USER_BAUD_RATE) > sizeof(userInfo->baudRate)) {
        USER_LOG_ERROR("Length of user information string is beyond limit. Please check.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
    }

    if (!strcmp(USER_APP_NAME, "your_app_name") ||
        !strcmp(USER_APP_ID, "your_app_id") ||
        !strcmp(USER_APP_KEY, "your_app_key") ||
        !strcmp(USER_BAUD_RATE, "your_app_license") ||
        !strcmp(USER_DEVELOPER_ACCOUNT, "your_developer_account") ||
        !strcmp(USER_BAUD_RATE, "your_baud_rate")) {
        USER_LOG_ERROR(
            "Please fill in correct user information to 'samples/sample_c/platform/linux/manifold2/application/dji_sdk_app_info.h' file.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
    }

    strncpy(userInfo->appName, USER_APP_NAME, sizeof(userInfo->appName) - 1);
    memcpy(userInfo->appId, USER_APP_ID, USER_UTIL_MIN(sizeof(userInfo->appId), strlen(USER_APP_ID)));
    memcpy(userInfo->appKey, USER_APP_KEY, USER_UTIL_MIN(sizeof(userInfo->appKey), strlen(USER_APP_KEY)));
    memcpy(userInfo->appLicense, USER_APP_LICENSE,
           USER_UTIL_MIN(sizeof(userInfo->appLicense), strlen(USER_APP_LICENSE)));
    memcpy(userInfo->baudRate, USER_BAUD_RATE, USER_UTIL_MIN(sizeof(userInfo->baudRate), strlen(USER_BAUD_RATE)));
    strncpy(userInfo->developerAccount, USER_DEVELOPER_ACCOUNT, sizeof(userInfo->developerAccount) - 1);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode DjiUser_LocalWriteFsInit(const char *path)
{
    T_DjiReturnCode djiReturnCode = DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    char filePath[DJI_LOG_PATH_MAX_SIZE];
    char systemCmd[DJI_SYSTEM_CMD_STR_MAX_SIZE];
    char folderName[DJI_LOG_FOLDER_NAME_MAX_SIZE];
    time_t currentTime = time(NULL);
    struct tm *localTime = localtime(&currentTime);
    uint16_t logFileIndex = 0;
    uint16_t currentLogFileIndex;
    uint8_t ret;

    if (localTime == NULL) {
        printf("Get local time error.\r\n");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // Create a folder for log file
    if (access(DJI_LOG_FOLDER_NAME, F_OK) != 0) {
        sprintf(folderName, "mkdir %s", DJI_LOG_FOLDER_NAME);
        ret = system(folderName);
        if (ret != 0) {
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    }

    // try to open log count file
    s_djiLogFileCnt = fopen(DJI_LOG_INDEX_FILE_NAME, "rb+");
    if (s_djiLogFileCnt == NULL) {
        s_djiLogFileCnt = fopen(DJI_LOG_INDEX_FILE_NAME, "wb+");
        if (s_djiLogFileCnt == NULL) {
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    } else {
        ret = fseek(s_djiLogFileCnt, 0, SEEK_SET);
        if (ret != 0) {
            printf("Seek log count file error, ret: %d, errno: %d.\r\n", ret, errno);
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }

        ret = fread((uint16_t *) &logFileIndex, 1, sizeof(uint16_t), s_djiLogFileCnt);
        if (ret != sizeof(uint16_t)) {
            printf("Read log file index error.\r\n");
        }
    }

    currentLogFileIndex = logFileIndex;
    logFileIndex++;

    ret = fseek(s_djiLogFileCnt, 0, SEEK_SET);
    if (ret != 0) {
        printf("Seek log file error, ret: %d, errno: %d.\r\n", ret, errno);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // write a new index to log count file
    ret = fwrite((uint16_t *) &logFileIndex, 1, sizeof(uint16_t), s_djiLogFileCnt);
    if (ret != sizeof(uint16_t)) {
        printf("Write log file index error.\r\n");
        fclose(s_djiLogFileCnt);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    fclose(s_djiLogFileCnt);

    // create a new log file
    sprintf(filePath, "%s_%04d_%04d%02d%02d_%02d-%02d-%02d.log", path, currentLogFileIndex,
            localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday,
            localTime->tm_hour, localTime->tm_min, localTime->tm_sec);

    s_djiLogFile = fopen(filePath, "wb+");
    if (s_djiLogFile == NULL) {
        USER_LOG_ERROR("Open filepath time error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // Remove excessive log files
    if (logFileIndex >= DJI_LOG_MAX_COUNT) {
        sprintf(systemCmd, "rm -rf %s_%04d*.log", path, currentLogFileIndex - DJI_LOG_MAX_COUNT);
        ret = system(systemCmd);
        if (ret != 0) {
            printf("Remove file error, ret:%d.\r\n", ret);
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    }

    return djiReturnCode;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#pragma GCC diagnostic ignored "-Wreturn-type"

// static void *DjiUser_MonitorTask(void *argument)
// {
//     unsigned int i = 0;
//     unsigned int threadCount = 0;
//     pid_t *tidList = NULL;
//     T_ThreadAttribute *threadAttribute = NULL;
//     T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

//     USER_UTIL_UNUSED(argument);

//     while (1) {
//         threadCount = Monitor_GetThreadCountOfProcess(getpid());
//         tidList = osalHandler->Malloc(threadCount * sizeof(pid_t));
//         if (tidList == NULL) {
//             USER_LOG_ERROR("malloc fail.");
//             goto delay;
//         }
//         Monitor_GetTidListOfProcess(getpid(), tidList, threadCount);

//         threadAttribute = osalHandler->Malloc(threadCount * sizeof(T_ThreadAttribute));
//         if (threadAttribute == NULL) {
//             USER_LOG_ERROR("malloc fail.");
//             goto freeTidList;
//         }
//         for (i = 0; i < threadCount; ++i) {
//             threadAttribute[i].tid = tidList[i];
//         }

//         USER_LOG_DEBUG("thread pcpu:");
//         USER_LOG_DEBUG("tid\tname\tpcpu");
//         for (i = 0; i < threadCount; ++i) {
//             threadAttribute[i].pcpu = Monitor_GetPcpuOfThread(getpid(), tidList[i]);
//             Monitor_GetNameOfThread(getpid(), tidList[i], threadAttribute[i].name, sizeof(threadAttribute[i].name));
//             USER_LOG_DEBUG("%d\t%15s\t%f %%.", threadAttribute[i].tid, threadAttribute[i].name,
//                            threadAttribute[i].pcpu);
//         }

//         USER_LOG_DEBUG("heap used: %d B.", Monitor_GetHeapUsed(getpid()));
//         USER_LOG_DEBUG("stack used: %d B.", Monitor_GetStackUsed(getpid()));

//         osalHandler->Free(threadAttribute);
// freeTidList:
//         osalHandler->Free(tidList);

// delay:
//         sleep(10);
//     }
// }

static T_DjiReturnCode DjiTest_HighPowerApplyPinInit()
{
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode DjiTest_WriteHighPowerApplyPin(E_DjiPowerManagementPinState pinState)
{
    //attention: please pull up the HWPR pin state by hardware.
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

#pragma GCC diagnostic pop

static bool DjiUser_CheckNetCableConnectStatus(void)
{
    FILE *fp;
    char *ret = NULL;
    char systemCmd[DJI_SYSTEM_CMD_STR_MAX_SIZE] = {0};
    char lineBuf[DJI_SYSTEM_RESULT_STR_MAX_SIZE] = {0};

    sprintf(systemCmd, "ifconfig %s | grep RUNNING", LINUX_NETWORK_DEV);
    fp = popen(systemCmd, "r");
    if (fp == NULL) {
        return false;
    }

    ret = fgets(lineBuf, sizeof(lineBuf), fp);
    if (ret == NULL) {
        return false;
    }

    return true;
}

int main(int argc, char **argv)
{
    T_DjiReturnCode returnCode;
    T_DjiUserInfo userInfo;
    T_DjiAircraftInfoBaseInfo aircraftInfoBaseInfo;
    // DjiTestDataTransmissionConfig dataTransmissionConfig;
    T_DjiOsalHandler osalHandler = {
        .TaskCreate = Osal_TaskCreate,
        .TaskDestroy = Osal_TaskDestroy,
        .TaskSleepMs = Osal_TaskSleepMs,
        .MutexCreate= Osal_MutexCreate,
        .MutexDestroy = Osal_MutexDestroy,
        .MutexLock = Osal_MutexLock,
        .MutexUnlock = Osal_MutexUnlock,
        .SemaphoreCreate = Osal_SemaphoreCreate,
        .SemaphoreDestroy = Osal_SemaphoreDestroy,
        .SemaphoreWait = Osal_SemaphoreWait,
        .SemaphoreTimedWait = Osal_SemaphoreTimedWait,
        .SemaphorePost = Osal_SemaphorePost,
        .Malloc = Osal_Malloc,
        .Free = Osal_Free,
        .GetTimeMs = Osal_GetTimeMs,
        .GetTimeUs = Osal_GetTimeUs,
    };
    T_DjiLoggerConsole printConsole = {
        .func = DjiUser_PrintConsole,
        .consoleLevel = DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,
        .isSupportColor = true,
    };

    T_DjiLoggerConsole localRecordConsole = {
        .consoleLevel = DJI_LOGGER_CONSOLE_LOG_LEVEL_DEBUG,
        .func = DjiUser_LocalWrite,
        .isSupportColor = false,
    };

    T_DjiHalUartHandler uartHandler = {
        .UartInit = HalUart_Init,
        .UartDeInit = HalUart_DeInit,
        .UartWriteData = HalUart_WriteData,
        .UartReadData = HalUart_ReadData,
        .UartGetStatus = HalUart_GetStatus,
    };

    T_DjiHalNetworkHandler networkHandler = {
        .NetworkInit = HalNetWork_Init,
        .NetworkDeInit = HalNetWork_DeInit,
    };

    T_DjiFileSystemHandler fileSystemHandler = {
        .FileOpen = Osal_FileOpen,
        .FileClose = Osal_FileClose,
        .FileWrite = Osal_FileWrite,
        .FileRead = Osal_FileRead,
        .FileSync = Osal_FileSync,
        .FileSeek = Osal_FileSeek,
        .DirOpen = Osal_DirOpen,
        .DirClose = Osal_DirClose,
        .DirRead = Osal_DirRead,
        .Mkdir = Osal_Mkdir,
        .Unlink = Osal_Unlink,
        .Rename = Osal_Rename,
        .Stat = Osal_Stat,
    };

    T_DjiSocketHandler socketHandler = {
        .Socket = Osal_Socket,
        .Bind = Osal_Bind,
        .Close = Osal_Close,
        .UdpSendData = Osal_UdpSendData,
        .UdpRecvData = Osal_UdpRecvData,
        .TcpListen = Osal_TcpListen,
        .TcpAccept = Osal_TcpAccept,
        .TcpConnect = Osal_TcpConnect,
        .TcpSendData = Osal_TcpSendData,
        .TcpRecvData = Osal_TcpRecvData,
    };
    
    // T_DjiHalUsbBulkHandler usbBulkHandler = {
    //     .UsbBulkInit = HalUsbBulk_Init,
    //     .UsbBulkDeInit = HalUsbBulk_DeInit,
    //     .UsbBulkWriteData = HalUsbBulk_WriteData,
    //     .UsbBulkReadData = HalUsbBulk_ReadData,
    //     .UsbBulkGetDeviceInfo = HalUsbBulk_GetDeviceInfo,
    // };

    returnCode = DjiPlatform_RegOsalHandler(&osalHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("register osal handler error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiPlatform_RegHalUartHandler(&uartHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("register hal uart handler error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiPlatform_RegHalNetworkHandler(&networkHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("register hal network handler error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    // returnCode = DjiPlatform_RegHalUsbBulkHandler(&usbBulkHandler);
    // if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //     printf("register hal usb bulk handler error");
    //     return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    // }

    returnCode = DjiPlatform_RegSocketHandler(&socketHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("register osal socket handler error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiPlatform_RegFileSystemHandler(&fileSystemHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("register osal filesystem handler error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    /* prepare for log writing */
    if (DjiUser_LocalWriteFsInit(DJI_LOG_PATH) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("file system init error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    /* two consoles. one for terminal printing, one for log file writing. */
    returnCode = DjiLogger_AddConsole(&printConsole);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("add printf console error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiLogger_AddConsole(&localRecordConsole);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("add printf console error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiUser_FillInUserInfo(&userInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        osalHandler.TaskSleepMs(1000);
        USER_LOG_ERROR("fill user info error, please check user info config");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiCore_Init(&userInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("core init error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    /* Get base info of the drone. See the struct definition for more info */
    returnCode = DjiAircraftInfo_GetBaseInfo(&aircraftInfoBaseInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("get aircraft base info error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiCore_SetAlias("UAV_SAR_M300RTK");
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("set alias error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    /* Main sequence starts */

    /* The following test sequence is commented out cause we dont need them tentatively */
    // if (aircraftInfoBaseInfo.mountPosition == DJI_MOUNT_POSITION_EXTENSION_PORT) {
        // dataTransmissionConfig.isEnableLowSpeedDataChannel = true;
        // dataTransmissionConfig.isEnableHighSpeedDataChannel = false;

        // returnCode = DjiTest_DataTransmissionStartService(dataTransmissionConfig);
        // if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        //     USER_LOG_ERROR("data transmission init error");
        // }

        // returnCode = DjiTest_WidgetInteractionStartService();
        // if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        //     USER_LOG_ERROR("widget interaction sample init error");
        // }
//     } else {
// #ifdef CONFIG_MODULE_SAMPLE_POWER_MANAGEMENT_ON
//         T_DjiTestApplyHighPowerHandler applyHighPowerHandler = {
//             .pinInit = DjiTest_HighPowerApplyPinInit,
//             .pinWrite = DjiTest_WriteHighPowerApplyPin,
//         };

//         returnCode = DjiTest_RegApplyHighPowerHandler(&applyHighPowerHandler);
//         if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
//             USER_LOG_ERROR("regsiter apply high power handler error");
//         }

//         returnCode = DjiTest_PowerManagementStartService();
//         if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
//             USER_LOG_ERROR("power management init error");
//         }
// #endif

        returnCode = DjiTest_CameraEmuBaseStartService();
        if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
            USER_LOG_ERROR("camera emu common init error");
        }

        if (DjiUser_CheckNetCableConnectStatus() == true) {
            returnCode = DjiTest_CameraEmuMediaStartService();
            if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
                USER_LOG_ERROR("camera emu media init error");
            }
        }

// #ifdef CONFIG_MODULE_SAMPLE_FC_SUBSCRIPTION_ON
//         returnCode = DjiTest_FcSubscriptionStartService();
//         if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
//             USER_LOG_ERROR("data subscription sample init error\n");
//         }
// #endif

// #ifdef CONFIG_MODULE_SAMPLE_GIMBAL_ON
//         if (aircraftInfoBaseInfo.djiAdapterType == DJI_SDK_ADAPTER_TYPE_SKYPORT_V2 ||
//             aircraftInfoBaseInfo.djiAdapterType == DJI_SDK_ADAPTER_TYPE_NONE) {
//             if (DjiTest_GimbalStartService() != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
//                 USER_LOG_ERROR("psdk gimbal init error");
//             }
//         }
// #endif

// #ifdef CONFIG_MODULE_SAMPLE_XPORT_ON
//         if (aircraftInfoBaseInfo.djiAdapterType == DJI_SDK_ADAPTER_TYPE_XPORT) {
//             if (DjiTest_XPortStartService() != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
//                 USER_LOG_ERROR("psdk xport init error");
//             }
//         }
// #endif

// #ifdef CONFIG_MODULE_SAMPLE_WIDGET_ON
// #if DJI_USE_WIDGET_INTERACTION
//         returnCode = DjiTest_WidgetInteractionStartService();
//         if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
//             USER_LOG_ERROR("widget interaction test init error");
//         }
// #else
        // returnCode = DjiTest_WidgetStartService();
//         if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
//             USER_LOG_ERROR("widget sample init error");
//         }
// #endif
// #endif

// #ifdef CONFIG_MODULE_SAMPLE_DATA_TRANSMISSION_ON
//         dataTransmissionConfig.isEnableLowSpeedDataChannel = true;
//         dataTransmissionConfig.isEnableHighSpeedDataChannel = DjiUser_CheckNetCableConnectStatus();
//         returnCode = DjiTest_DataTransmissionStartService(dataTransmissionConfig);
//         if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
//             USER_LOG_ERROR("widget sample init error");
//         }
// #endif

// #ifdef CONFIG_MODULE_SAMPLE_MOP_CHANNEL_ON
//         returnCode = DjiTest_MopChannelStartService();
//         if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
//             USER_LOG_ERROR("mop channel sample init error");
//         }
// #endif

// #ifdef CONFIG_MODULE_SAMPLE_PAYLOAD_COLLABORATION_ON
//         returnCode = DjiTest_PayloadCollaborationStartService();
//         if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
//             USER_LOG_ERROR("Payload collaboration sample init error\n");
//         }
// #endif

// #ifdef CONFIG_MODULE_SAMPLE_UPGRADE_ON
//         T_DjiTestUpgradePlatformOpt linuxUpgradePlatformOpt = {
//             .rebootSystem = DjiUpgradePlatformLinux_RebootSystem,
//             .cleanUpgradeProgramFileStoreArea = DjiUpgradePlatformLinux_CleanUpgradeProgramFileStoreArea,
//             .createUpgradeProgramFile = DjiUpgradePlatformLinux_CreateUpgradeProgramFile,
//             .writeUpgradeProgramFile = DjiUpgradePlatformLinux_WriteUpgradeProgramFile,
//             .readUpgradeProgramFile = DjiUpgradePlatformLinux_ReadUpgradeProgramFile,
//             .closeUpgradeProgramFile = DjiUpgradePlatformLinux_CloseUpgradeProgramFile,
//             .replaceOldProgram = DjiUpgradePlatformLinux_ReplaceOldProgram,
//             .setUpgradeRebootState = DjiUpgradePlatformLinux_SetUpgradeRebootState,
//             .getUpgradeRebootState = DjiUpgradePlatformLinux_GetUpgradeRebootState,
//             .cleanUpgradeRebootState = DjiUpgradePlatformLinux_CleanUpgradeRebootState,
//         };
//         T_DjiTestUpgradeConfig testUpgradeConfig = {
//             .firmwareVersion = {1, 0, 0, 0},
//             .transferType = DJI_FIRMWARE_TRANSFER_TYPE_FTP,
//             .needReplaceProgramBeforeReboot = true
//         };
//         if (DjiTest_UpgradeStartService(&linuxUpgradePlatformOpt, testUpgradeConfig) !=
//             DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
//             USER_LOG_ERROR("psdk upgrade init error");
//         }
// #endif
//     }

    /* Create a thread to monitor system status. We dont need it for now. */
    // if (pthread_create(&s_monitorThread, NULL, DjiUser_MonitorTask, NULL) != 0) {
    //     USER_LOG_ERROR("create monitor task fail.");
    // }

    // if (pthread_setname_np(s_monitorThread, "monitor task") != 0) {
    //     USER_LOG_ERROR("set name for monitor task fail.");
    // }

    /* ----------------- User defined sequence starts! ---------------------- */
    /* start widget service */
    // returnCode = My_WidgetStartService();
    // if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //     USER_LOG_ERROR("Cannot start widget service.");
    //     return returnCode;
    // }

    /* tests */
    printf("Mount position: %d\n", aircraftInfoBaseInfo.mountPosition);

    /* start fc subscription */
    // returnCode = FcSubscriptionStartService();
    // if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //     USER_LOG_ERROR("Cannot start FC subscription service.");
    // }

    returnCode = DjiCore_ApplicationStart();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("start sdk application error");
    }

    T_DjiReturnCode djiStat;
    djiStat = DjiFcSubscription_Init();
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("init data subscription module error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }
    FcSubscripeData();

    /* rotate the xport */
    returnCode = DjiTest_XPortStartService();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Cannot start X-port service.");
    }
    
    /* start widget settings */
    returnCode = My_WidgetStartService();
    // returnCode = DjiTest_WidgetStartService();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Cannot start widget service.");
    }

    /* enable camera module */
    // if (DjiUser_CheckNetCableConnectStatus() == true) {
    // returnCode = DjiTest_CameraEmuMediaStartService();
    //     if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //         USER_LOG_ERROR("camera emu media init error");
    //     }
    // }

    while (1) {
        sleep(1);
    }
}

/****************** (C) COPYRIGHT DJI Innovations *****END OF FILE****/