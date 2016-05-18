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


#ifndef __VOICE_CONTROL_COMMON_H__
#define __VOICE_CONTROL_COMMON_H__

#include <voice_control_command.h>

/**
* @addtogroup CAPI_UIX_VOICE_CONTROL_MODULE
* @{
*/

#ifdef __cplusplus
extern "C"
{
#endif

/**
* @brief Enumerations of error codes.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*/
typedef enum {
	VC_ERROR_NONE			= TIZEN_ERROR_NONE,			/**< Successful */
	VC_ERROR_OUT_OF_MEMORY		= TIZEN_ERROR_OUT_OF_MEMORY,		/**< Out of Memory */
	VC_ERROR_IO_ERROR		= TIZEN_ERROR_IO_ERROR,			/**< I/O error */
	VC_ERROR_INVALID_PARAMETER	= TIZEN_ERROR_INVALID_PARAMETER,	/**< Invalid parameter */
	VC_ERROR_TIMED_OUT		= TIZEN_ERROR_TIMED_OUT,		/**< No answer from service */
	VC_ERROR_RECORDER_BUSY		= TIZEN_ERROR_RESOURCE_BUSY,		/**< Busy recorder */
	VC_ERROR_PERMISSION_DENIED	= TIZEN_ERROR_PERMISSION_DENIED,	/**< Permission denied */
	VC_ERROR_NOT_SUPPORTED		= TIZEN_ERROR_NOT_SUPPORTED,		/**< VC NOT supported */
	VC_ERROR_INVALID_STATE		= TIZEN_ERROR_VOICE_CONTROL | 0x011,	/**< Invalid state */
	VC_ERROR_INVALID_LANGUAGE	= TIZEN_ERROR_VOICE_CONTROL | 0x012,	/**< Invalid language */
	VC_ERROR_ENGINE_NOT_FOUND	= TIZEN_ERROR_VOICE_CONTROL | 0x013,	/**< No available engine */
	VC_ERROR_OPERATION_FAILED	= TIZEN_ERROR_VOICE_CONTROL | 0x014,	/**< Operation failed */
	VC_ERROR_OPERATION_REJECTED	= TIZEN_ERROR_VOICE_CONTROL | 0x015,	/**< Operation rejected */
	VC_ERROR_ITERATION_END		= TIZEN_ERROR_VOICE_CONTROL | 0x016,	/**< List reached end */
	VC_ERROR_EMPTY			= TIZEN_ERROR_VOICE_CONTROL | 0x017,	/**< List empty */
	VC_ERROR_SERVICE_RESET		= TIZEN_ERROR_VOICE_CONTROL | 0x018	/**< Service Damon reset */
} vc_error_e;

/**
* @brief Enumerations of result event.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*/
typedef enum {
	VC_RESULT_EVENT_RESULT_SUCCESS	= 0,	/**< Normal result */
	VC_RESULT_EVENT_REJECTED	= 1	/**< Rejected result */
} vc_result_event_e;

/**
* @brief Enumerations of service state.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*/
typedef enum {
	VC_SERVICE_STATE_NONE		= 0,	/**< 'None' state */
	VC_SERVICE_STATE_READY		= 1,	/**< 'Ready' state */
	VC_SERVICE_STATE_RECORDING	= 2,	/**< 'Recording' state */
	VC_SERVICE_STATE_PROCESSING	= 3	/**< 'Processing' state */
} vc_service_state_e;

/**
* @brief Enumerations of client state.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*/
typedef enum {
	VC_STATE_NONE			= 0,	/**< 'None' state */
	VC_STATE_INITIALIZED		= 1,	/**< 'Initialized' state */
	VC_STATE_READY			= 2	/**< 'Ready' state */
} vc_state_e;


/**
* @brief Called when client gets the recognition result.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*
* @remarks If the duplicated commands are recognized, the event(e.g. #VC_RESULT_EVENT_REJECTED) of command may be rejected \n
*	for selecting command as priority. If you set similar or same commands or the recognized results are multi-results,
*	vc_cmd_list has the multi commands.
*
* @param[in] event The result event (e.g. #VC_RESULT_EVENT_RESULT_SUCCESS, #VC_RESULT_EVENT_REJECTED)
* @param[in] vc_cmd_list The recognized command list
* @param[in] result The spoken text
* @param[in] user_data The user data passed from the callback registration function
*
* @pre An application registers callback function.
*
* @see vc_set_result_cb()
*/
typedef void (*vc_result_cb)(vc_result_event_e event, vc_cmd_list_h vc_cmd_list, const char* result, void *user_data);

/**
* @brief Called when default language is changed.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*
* @param[in] previous Previous language
* @param[in] current Current language
* @param[in] user_data The user data passed from the callback registration function
*
* @pre An application registers this callback to detect changing mode.
*
* @see vc_set_current_language_changed_cb()
*/
typedef void (*vc_current_language_changed_cb)(const char* previous, const char* current, void* user_data);

/**
* @brief Called to retrieve supported language.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*
* @param[in] language A language is specified as an ISO 3166 alpha-2 two letter country-code \n
*		followed by ISO 639-1 for the two-letter language code. \n
*		For example, "ko_KR" for Korean, "en_US" for American English.
* @param[in] user_data The user data passed from the foreach function
*
* @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop.
*
* @pre The function will invoke this callback.
*/
typedef bool (*vc_supported_language_cb)(const char* language, void* user_data);

/**
* @brief Called when the state of voice control client is changed.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*
* @param[in] previous A previous state
* @param[in] current A current state
* @param[in] user_data The user data passed from the callback registration function
*
* @pre An application registers this callback to detect changing state.
*
* @see vc_set_state_changed_cb()
*/
typedef void (*vc_state_changed_cb)(vc_state_e previous, vc_state_e current, void* user_data);

/**
* @brief Called when the state of voice control service is changed.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*
* @param[in] previous A previous state
* @param[in] current A current state
* @param[in] user_data The user data passed from the callback registration function
*
* @pre An application registers this callback to detect changing service state.
*
* @see vc_set_service_state_changed_cb()
*/
typedef void (*vc_service_state_changed_cb)(vc_service_state_e previous, vc_service_state_e current, void* user_data);

/**
* @brief Called when error occurred.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*
* @param[in] reason The error type (e.g. #VC_ERROR_OUT_OF_MEMORY, #VC_ERROR_TIMED_OUT)
* @param[in] user_data The user data passed from the callback registration function
*
* @pre An application registers this callback to detect error.
*
* @see vc_set_error_cb()
*/
typedef void (*vc_error_cb)(vc_error_e reason, void *user_data);


#ifdef __cplusplus
}
#endif

/**
 * @}@}
 */

#endif /* VOICE_CONTROL_COMMON */
