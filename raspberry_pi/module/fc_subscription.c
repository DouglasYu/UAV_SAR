/** 
 * @file fc_subscription.c
 * @brief User defined functions for fc info subscription.
 * @author Douglas Yu
 */

/* Includes ------------------------------------------------------------------*/
#include <utils/util_misc.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include "dji_logger.h"
#include "dji_platform.h"
#include "fc_subscription.h"

/* Private constants ---------------------------------------------------------*/
#define USER_UTIL_UNUSED(x)                                 ((x) = (x))

/* Private types -------------------------------------------------------------*/

/* Private functions declaration ---------------------------------------------*/
static T_DjiReturnCode DjiTest_FcSubscriptionReceiveQuaternionCallback(const uint8_t *data, uint16_t dataSize,
                                                                       const T_DjiDataTimestamp *timestamp);
static T_DjiReturnCode DjiTest_FcSubscriptionReceiveGpsPositionCallback(const uint8_t *data, uint16_t dataSize,
                                                                       const T_DjiDataTimestamp *timestamp);
static T_DjiReturnCode DjiTest_FcSubscriptionReceiveVelocityCallback(const uint8_t *data, uint16_t dataSize,
                                                                       const T_DjiDataTimestamp *timestamp);

/* Private variables ---------------------------------------------------------*/
FILE* s_quaternionLog;
FILE* s_gpsLog;
FILE* s_velocityLog;
int isRecording = 0;

/* Exported functions definition ---------------------------------------------*/
T_DjiReturnCode FcSubscripeData(void){

    T_DjiReturnCode djiStat;
    /** subscribe quaternion (四元数) 
     * see dji_fc_subscription.h for the definitions of structs
    */
    djiStat = DjiFcSubscription_SubscribeTopic(DJI_FC_SUBSCRIPTION_TOPIC_QUATERNION, DJI_DATA_SUBSCRIPTION_TOPIC_1_HZ,
                                               DjiTest_FcSubscriptionReceiveQuaternionCallback);
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Subscribe topic quaternion error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    } else {
        USER_LOG_DEBUG("Subscribe topic quaternion success.");
    }

    /* subscribe velocity */
    djiStat = DjiFcSubscription_SubscribeTopic(DJI_FC_SUBSCRIPTION_TOPIC_VELOCITY, DJI_DATA_SUBSCRIPTION_TOPIC_1_HZ,
                                               DjiTest_FcSubscriptionReceiveVelocityCallback);
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Subscribe topic velocity error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    } else {
        USER_LOG_DEBUG("Subscribe topic velocity success.");
    }

    /* subscribe gps position */
    djiStat = DjiFcSubscription_SubscribeTopic(DJI_FC_SUBSCRIPTION_TOPIC_GPS_POSITION, DJI_DATA_SUBSCRIPTION_TOPIC_1_HZ,
                                               DjiTest_FcSubscriptionReceiveGpsPositionCallback);
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Subscribe topic gps position error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    } else {
        USER_LOG_DEBUG("Subscribe topic gps position success.");
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode FcSubscriptionStartService(void)
{
    T_DjiReturnCode djiStat;

    /* create a folder for recorded data if not exsited */
    char folderName[32];
    int ret;
    if (access(QUATERNION_LOG_PATH, 0) != 0) {
        sprintf(folderName, "mkdir %s", QUATERNION_LOG_PATH);
        ret = system(folderName);
        if (ret != 0) {
            USER_LOG_ERROR("Cannot create folder for quaternion logs.");
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    }
    if (access(GPS_POS_LOG_PATH, 0) != 0) {
        sprintf(folderName, "mkdir %s", GPS_POS_LOG_PATH);
        ret = system(folderName);
        if (ret != 0) {
            USER_LOG_ERROR("Cannot create folder for gps logs.");
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    }
    if (access(VELOCITY_LOG_PATH, 0) != 0) {
        sprintf(folderName, "mkdir %s", VELOCITY_LOG_PATH);
        ret = system(folderName);
        if (ret != 0) {
            USER_LOG_ERROR("Cannot create folder for velocity logs.");
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    }

    /* create 3 new log files */
    char filePath[128];
    time_t currentTime = time(NULL);
    struct tm *localTime = localtime(&currentTime);
    // quaternion logs
    sprintf(filePath, "quaternion_logs/quaternion_%04d%02d%02d_%02d-%02d-%02d.log",
            localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday,
            localTime->tm_hour + 7, localTime->tm_min, localTime->tm_sec);
    s_quaternionLog = fopen(filePath, "wb+");
    if (s_quaternionLog == NULL) {
        USER_LOG_ERROR("Open filepath time error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }
    // gps position logs
    sprintf(filePath, "gps_position_logs/gps_%04d%02d%02d_%02d-%02d-%02d.log",
            localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday,
            localTime->tm_hour + 7, localTime->tm_min, localTime->tm_sec);
    s_gpsLog = fopen(filePath, "wb+");
    if (s_gpsLog == NULL) {
        USER_LOG_ERROR("Open filepath time error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }
    // velocity logs
    sprintf(filePath, "velocity_logs/velocity_%04d%02d%02d_%02d-%02d-%02d.log",
            localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday,
            localTime->tm_hour + 7, localTime->tm_min, localTime->tm_sec);
    s_velocityLog = fopen(filePath, "wb+");
    if (s_velocityLog == NULL) {
        USER_LOG_ERROR("Open filepath time error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    /* turn on the recording flag */
    isRecording = 1;

    /* we don’t need detail gps data tentatively. */
    // djiStat = DjiFcSubscription_SubscribeTopic(DJI_FC_SUBSCRIPTION_TOPIC_GPS_DETAILS, DJI_DATA_SUBSCRIPTION_TOPIC_1_HZ,
    //                                            NULL);
    // if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //     USER_LOG_ERROR("Subscribe topic gps details error.");
    //     return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    // } else {
    //     USER_LOG_DEBUG("Subscribe topic gps details success.");
    // }

    // if (osalHandler->TaskCreate("user_subscription_task", UserFcSubscription_Task,
    //                             FC_SUBSCRIPTION_TASK_STACK_SIZE, NULL, &s_userFcSubscriptionThread) !=
    //     DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //     USER_LOG_ERROR("user data subscription task create error.");
    //     return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    // }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

T_DjiReturnCode FcSubscriptionStopService(void){

    // T_DjiReturnCode djiStat = DjiFcSubscription_DeInit();
    // if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //     USER_LOG_ERROR("Deinit fc subscription error.");
    //     return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    // }

    /* turn off the recording flag */
    isRecording = 0;

    fclose(s_quaternionLog);
    fclose(s_gpsLog);
    fclose(s_velocityLog);
    s_quaternionLog = NULL;
    s_gpsLog = NULL;
    s_velocityLog = NULL;

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

/**
 * @brief: Get the current position of the drone.
 * @note: 
 * @param dronePosition: pointer to RTK data
 * @param timestamp: timestamp of the position data
 * @return: Excution result.
 */
T_DjiReturnCode FcSubscriptionGetData(T_DjiFcSubscriptionRtkPosition* position, T_DjiDataTimestamp timestamp){

    T_DjiReturnCode djiStat;
    E_DjiFcSubscriptionTopic topic;

    topic = DJI_FC_SUBSCRIPTION_TOPIC_RTK_POSITION;
    djiStat = DjiFcSubscription_GetLatestValueOfTopic(topic, (uint8_t*) &position, 
                                                        sizeof(position), &timestamp);
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("get value of RTK position info error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

}

/* Private functions definition-----------------------------------------------*/
static T_DjiReturnCode DjiTest_FcSubscriptionReceiveQuaternionCallback(const uint8_t *data, uint16_t dataSize,
                                                                       const T_DjiDataTimestamp *timestamp)
{
    
    // printf("%d\n", isRecording);
    
    if(isRecording == 0) return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    
    T_DjiFcSubscriptionQuaternion *quaternion = (T_DjiFcSubscriptionQuaternion *) data;
    dji_f64_t pitch, yaw, roll;

    USER_UTIL_UNUSED(dataSize);

    pitch = (dji_f64_t) asinf(-2 * quaternion->q1 * quaternion->q3 + 2 * quaternion->q0 * quaternion->q2) * 57.3;
    roll = (dji_f64_t) atan2f(2 * quaternion->q1 * quaternion->q2 + 2 * quaternion->q0 * quaternion->q3,
                              -2 * quaternion->q2 * quaternion->q2 - 2 * quaternion->q3 * quaternion->q3 + 1) *
           57.3;
    yaw = (dji_f64_t) atan2f(2 * quaternion->q2 * quaternion->q3 + 2 * quaternion->q0 * quaternion->q1,
                             -2 * quaternion->q1 * quaternion->q1 - 2 * quaternion->q2 * quaternion->q2 + 1) * 57.3;

    if (s_userFcSubscriptionDataShow == true) {
        USER_LOG_INFO("receive quaternion data.");
        // USER_LOG_INFO("timestamp: millisecond %u microsecond %u.", timestamp->millisecond,
        //               timestamp->microsecond);
        // USER_LOG_INFO("quaternion: %f %f %f %f.\r\n", quaternion->q0, quaternion->q1, quaternion->q2, quaternion->q3);
        // USER_LOG_INFO("euler angles: pitch = %.2f roll = %.2f yaw = %.2f.", pitch, yaw, roll);
        USER_LOG_INFO("pitch = %.2f roll = %.2f yaw = %.2f.", pitch, yaw, roll);
    }

    fprintf(s_quaternionLog, "%u %u %.2f %.2f %.2f\n", timestamp->microsecond, timestamp->millisecond,
            pitch, yaw, roll);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode DjiTest_FcSubscriptionReceiveGpsPositionCallback(const uint8_t *data, uint16_t dataSize,
                                                                       const T_DjiDataTimestamp *timestamp)
{
    if(isRecording == 0) return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;

    USER_UTIL_UNUSED(dataSize);

    T_DjiFcSubscriptionGpsPosition *gps = (T_DjiFcSubscriptionGpsPosition*) data;

    dji_f64_t longitude, latitude, altitude;

    longitude = gps->x;
    latitude = gps->y;
    altitude = gps->z;

    fprintf(s_gpsLog, "%u %u %.2f %.2f %.2f\n", timestamp->microsecond, timestamp->millisecond,
            longitude, latitude, altitude);

    if(s_userFcSubscriptionDataShow == true){
        USER_LOG_INFO("receive gps data. longitude:%.2f, latitude:%.2f, altitude:%.2f.", longitude, latitude, altitude);
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode DjiTest_FcSubscriptionReceiveVelocityCallback(const uint8_t *data, uint16_t dataSize,
                                                                       const T_DjiDataTimestamp *timestamp)
{
    if(isRecording == 0) return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;

    USER_UTIL_UNUSED(dataSize);

    T_DjiFcSubscriptionVelocity* velocity = (T_DjiFcSubscriptionVelocity*) data;

    dji_f64_t x, y, z;

    x = velocity->data.x;
    y = velocity->data.y;
    z = velocity->data.z;

    fprintf(s_velocityLog, "%u %u %.2f %.2f %.2f\n", timestamp->microsecond, timestamp->millisecond,
            x, y, z);

    if(s_userFcSubscriptionDataShow == true){
        USER_LOG_INFO("receive velocity data. x:%.2f, y:%.2f, z:%.2f.", x, y, z);
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

/* END OF FILE */
