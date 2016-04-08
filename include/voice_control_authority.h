/*
* Copyright (c) 2011-2015 Samsung Electronics Co., Ltd All Rights Reserved
*
* Licensed under the Apache License, Version 2.0 (the License);
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an AS IS BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#ifndef __VOICE_CONTROL_AUTHORITY_H__
#define __VOICE_CONTROL_AUTHORITY_H__

#include <voice_control_common.h>

/**
* @addtogroup VOICE_CONTROL_AUTHORITY
* @{
*/

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef EXPORT_API
#define EXPORT_API
#endif // EXPORT_API


/* Authority */

typedef enum {
	VC_AUTH_STATE_NONE	= 0,
	VC_AUTH_STATE_VALID	= 1,
	VC_AUTH_STATE_INVALID	= 2
} vc_auth_state_e;


/**
* @brief Called when authority state of client is changed.
*
* @param[in] previous Previous state
* @param[in] current Current state
* @param[in] user_data The user data passed from the callback registration function
*
* @pre An application registers callback function.
*
* @see vc_set_auth_state_changed_cb()
*/
typedef void (*vc_auth_state_changed_cb)(vc_auth_state_e previous, vc_auth_state_e current, void* user_data);

/**
* @brief Enable authority about start/stop/cancel recognition.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failed
*
* @pre The state should be #VC_STATE_READY.
*
* @see vc_auth_disable()
*/
EXPORT_API int vc_auth_enable();

/**
* @brief Disable authority.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failed
*
* @pre The state should be #VC_STATE_READY.
*
* @see vc_auth_enable()
*/
EXPORT_API int vc_auth_disable();

/**
* @brief Get current authority state.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_READY.
*/
EXPORT_API int vc_auth_get_state(vc_auth_state_e* status);

/**
* @brief Set callback for authority state changing.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failed
*
* @pre This function should be called when the authority state is #VC_AUTH_STATE_VALID or #VC_AUTH_STATE_INVALID.
*
* @see vc_auth_unset_state_changed_cb()
* @see vc_auth_state_changed_cb()
*/
EXPORT_API int vc_auth_set_state_changed_cb(vc_auth_state_changed_cb callback, void* user_data);

/**
* @brief Unset callback for authority state changing.
*
* @remarks This function will be called in vc_auth_disable() automatically.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failed
*
* @pre This function should be called when the authority state is #VC_AUTH_STATE_VALID or #VC_AUTH_STATE_INVALID.
*
* @see vc_auth_set_state_changed_cb()
*/
EXPORT_API int vc_auth_unset_state_changed_cb();

/**
* @brief Start recognition.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failed
*
* @pre The state should be #VC_STATE_READY and the service state should be #VC_SERVICE_STATE_READY. \n
* The authority status should be #VC_AUTHORITY_STATUS_VALID.
* @post It will invoke vc_service_state_changed_cb(), if you register a callback with vc_service_state_changed_cb(). \n
* If this function succeeds, the service state will be #VC_SERVICE_STATE_RECORDING.
*
* @see vc_auth_stop()
* @see vc_auth_cancel()
*/
EXPORT_API int vc_auth_start();

/**
* @brief Stop recognition.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failed
*
* @pre The service state should be #VC_SERVICE_STATE_RECORDING. \n
* The authority status should be #VC_AUTHORITY_STATUS_VALID.
* @post It will invoke vc_service_state_changed_cb(), if you register a callback with vc_service_state_changed_cb(). \n
* If this function succeeds, the service state will be #VC_SERVICE_STATE_PROCESSING.
*
* @see vc_auth_start()
* @see vc_auth_cancel()
*/
EXPORT_API int vc_auth_stop();

/**
* @brief Cancel recognition.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failed
*
* @pre The service state should be #VC_SERVICE_STATE_RECORDING or #VC_SERVICE_STATE_PROCESSING. \n
* The authority status should be #VC_AUTHORITY_STAUS_VALID.
* @post It will invoke vc_service_state_changed_cb(), if you register a callback with vc_service_state_changed_cb(). \n
* If this function succeeds, the service state will be #VC_SERVICE_STATE_READY.
*
* @see vc_auth_start()
* @see vc_auth_stop()
*/
EXPORT_API int vc_auth_cancel();


#ifdef __cplusplus
}
#endif

/**
 * @}@}
 */

#endif /* __VOICE_CONTROL_AUTHORITY_H__ */
