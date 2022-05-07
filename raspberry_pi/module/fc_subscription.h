/** 
 * @file fc_subscription.h
 * @brief header file for "fc_subscription.c"
 * @author Douglas Yu
 */

#include "dji_typedef.h"
#include "dji_fc_subscription.h"

#ifndef FC_SUBSCRIPTION
#define FC_SUBSCRIPTION

#define TELEMETRY_LOG_FOLDER "telemetry_logs"
#define QUATERNION_LOG_PATH "quaternion_logs"
#define GPS_POS_LOG_PATH "gps_position_logs"
#define VELOCITY_LOG_PATH "velocity_logs"

static bool s_userFcSubscriptionDataShow = true;

T_DjiReturnCode FcSubscriptionStartService(void);
T_DjiReturnCode FcSubscriptionStopService(void);
T_DjiReturnCode FcSubscripeData(void);


#endif

