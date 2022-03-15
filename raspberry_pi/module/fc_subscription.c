/** 
 * @file fc_subscription.c
 * @brief User defined functions for fc info subscription.
 * @author Douglas Yu
 */

/* Includes ------------------------------------------------------------------*/
#include <utils/util_misc.h>
#include "dji_logger.h"
#include "dji_platform.h"
#include "fc_subscription.h"

/* Private constants ---------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/

/* Private functions declaration ---------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Exported functions definition ---------------------------------------------*/

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

    // USER_LOG_INFO("--> Step 1: Init fc subscription module");
    // djiStat = DjiFcSubscription_Init();
    // if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //     USER_LOG_ERROR("init data subscription module error.");
    //     return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    // }

    topic = DJI_FC_SUBSCRIPTION_TOPIC_RTK_POSITION;
    djiStat = DjiFcSubscription_GetLatestValueOfTopic(topic, (uint8_t*) &position, 
                                                        sizeof(position), &timestamp);
    if (djiStat != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("get value of RTK position info error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

}

/* Private functions definition-----------------------------------------------*/


/* END OF FILE */
