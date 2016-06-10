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


#ifndef __VOICE_CONTROL_MANAGER_H__
#define __VOICE_CONTROL_MANAGER_H__

#include <voice_control_command.h>
#include <voice_control_command_expand.h>
#include <voice_control_common.h>

/**
* @addtogroup VOICE_CONTROL_MANAGER
* @{
*/

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * @brief Definition of audio-in type.
*/
#define VC_AUDIO_TYPE_BLUETOOTH		"VC_AUDIO_ID_BLUETOOTH"		/**< Bluetooth audio type */

/**
 * @brief Definition of audio-in type.
*/
#define VC_AUDIO_TYPE_MSF		"VC_AUDIO_ID_MSF"		/**< MSF (wifi) audio type */

/**
* @brief Definition for foreground command type.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*/
#define VC_COMMAND_TYPE_FOREGROUND	1

/**
* @brief Definition for background command type.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*/
#define VC_COMMAND_TYPE_BACKGROUND	2

/**
* @brief Definition for widget command type.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*/
#define VC_COMMAND_TYPE_WIDGET		3

/**
* @brief Definition for system command type.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*/
#define VC_COMMAND_TYPE_SYSTEM		4

/**
* @brief Definition for exclusive command type.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*/
#define VC_COMMAND_TYPE_EXCLUSIVE	5

/**
* @brief Definition for none message.
*/
#define VC_RESULT_MESSAGE_NONE			"vc.result.message.none"

/**
* @brief Definition for failed recognition because the speech is too loud to listen.
*/
#define VC_RESULT_MESSAGE_ERROR_TOO_LOUD	"vc.result.message.error.too.loud"

/**
* @brief Enumerations of recognition mode.
*/
typedef enum {
	VC_RECOGNITION_MODE_STOP_BY_SILENCE,		/**< Default mode */
	VC_RECOGNITION_MODE_RESTART_AFTER_REJECT,	/**< Restart recognition after rejected result */
	VC_RECOGNITION_MODE_RESTART_CONTINUOUSLY,	/**< Continuously restart recognition - not support yet*/
	VC_RECOGNITION_MODE_MANUAL			/**< Start and stop manually without silence */
} vc_recognition_mode_e;

/**
* @brief Called when client gets the all recognition results from vc-daemon.
*
* @remark temp_command is valid in callback function.
*
* @param[in] event The result event
* @param[in] vc_cmd_list_h Command list handle
* @param[in] result Command text
* @param[in] msg Engine message (e.g. #VC_RESULT_MESSAGE_NONE, #VC_RESULT_MESSAGE_ERROR_TOO_LOUD)
* @param[in] user_data The user data passed from the callback registration function
*
* @return @c true to release command to client, \n @c false to wait for selecting command.
* @pre An application registers callback function using vc_mgr_set_all_result_cb().
*
* @see vc_mgr_set_all_result_cb()
* @see vc_mgr_unset_all_result_cb()
*/
typedef bool (*vc_mgr_all_result_cb)(vc_result_event_e event, vc_cmd_list_h vc_cmd_list,
				const char* result, const char* msg, void *user_data);

typedef enum {
	VC_PRE_RESULT_EVENT_FINAL_RESULT = 0,
	VC_PRE_RESULT_EVENT_PARTIAL_RESULT,
	VC_PRE_RESULT_EVENT_ERROR
}vc_pre_result_event_e;

// support pre-result
typedef bool(*vc_mgr_pre_result_cb)(vc_pre_result_event_e event, const char* result, void *user_data);

/**
* @brief Called when user speaking is detected.
*
* @param[in] user_data The user data passed from the callback registration function
*
* @pre An application registers callback function using vc_mgr_set_speech_detected_cb().
*
* @see vc_mgr_set_speech_detected_cb()
* @see vc_mgr_unset_speech_detected_cb()
*/
typedef void (*vc_mgr_begin_speech_detected_cb)(void *user_data);


/**
* @brief Initialize voice control manager.
*
* @remarks If the function succeeds, @a vc mgr must be released with vc_mgr_deinitialize().
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_OUT_OF_MEMORY Out of memory
* @retval #VC_ERROR_OPERATION_FAILED Operation fail
*
* @pre The state should be #VC_STATE_NONE.
* @post If this function is called, the state will be #VC_STATE_INITIALIZED.
*
* @see vc_mgr_deinitialize()
*/
int vc_mgr_initialize();

/**
* @brief Deinitialize voice control manager.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @post If this function is called, the state will be #VC_STATE_NONE.
*
* @see vc_mgr_deinitialize()
*/
int vc_mgr_deinitialize();

/**
* @brief Connects the voice control service.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
* @post If this function is called, the state will be #VC_STATE_READY.
*
* @see vc_mgr_unprepare()
*/
int vc_mgr_prepare();

/**
* @brief Disconnects the vc-daemon.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_READY.
* @post If this function is called, the state will be #VC_STATE_INITIALIZED.
*
* @see vc_mgr_prepare()
*/
int vc_mgr_unprepare();

/**
* @brief Retrieves all supported languages using callback function.
*
* @param[in] callback Callback function to invoke
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_OPERATION_FAILED Operation failure
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should NOT be #VC_SERVICE_STATE_NONE.
* @post	This function invokes vc_supported_language_cb() repeatedly for getting languages.
*
* @see vc_supported_language_cb()
* @see vc_mgr_get_current_language()
*/
int vc_mgr_foreach_supported_languages(vc_supported_language_cb callback, void* user_data);

/**
* @brief Gets current language set by user.
*
* @remark If the function succeeds, @a language must be released with free() by you when you no longer need it.
*
* @param[out] language A language is specified as an ISO 3166 alpha-2 two letter country-code \n
*			followed by ISO 639-1 for the two-letter language code. \n
*			For example, "ko_KR" for Korean, "en_US" for American English.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_OUT_OF_MEMORY Out of memory
* @retval #VC_ERROR_OPERATION_FAILED Operation failure
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should NOT be #VC_SERVICE_STATE_NONE.
*
* @see vc_mgr_foreach_supported_languages()
*/
int vc_mgr_get_current_language(char** language);

/**
* @brief Gets current state of voice control manager.
*
* @param[out] state The current state
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
*
* @pre The state should NOT be #VC_SERVICE_STATE_NONE.
*
* @see vc_state_changed_cb()
* @see vc_set_state_changed_cb()
*/
int vc_mgr_get_state(vc_state_e* state);

/**
* @brief Gets current state of voice control service.
*
* @param[out] state The current state
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
*
* @pre The state should be #VC_STATE_READY.
*
* @see vc_mgr_start()
* @see vc_mgr_stop()
* @see vc_mgr_cancel()
* @see vc_set_service_state_changed_cb()
* @see vc_unset_service_state_changed_cb()
*/
int vc_mgr_get_service_state(vc_service_state_e* state);

/**
* @brief Sets demandable client list.
*
* @param[in] rule demandable client list rule path
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_READY.
*
* @see vc_mgr_get_demandable_client_rule()
*/
int vc_mgr_set_demandable_client_rule(const char* rule);

/**
* @brief Gets demandable client list.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_READY.
*
* @see vc_mgr_set_demandable_client_rule()
*/
int vc_mgr_unset_demandable_client_rule();

/**
* @brief Checks whether the command format is supported.
*
* @param[in] format The command format
* @param[out] support The result status @c true = supported, @c false = not supported
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_OPERATION_FAILED Operation failure
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_READY.
*/
int vc_mgr_is_command_format_supported(vc_cmd_format_e format, bool* support);

/**
* @brief Sets system or exclusive commands.
*
* @remarks The command type is valid for VC_COMMAND_TYPE_SYSTEM or VC_COMMAND_TYPE_EXCLUSIVE.
*	The commands should include type, command text, format.
*
* @param[in] vc_cmd_list The command list handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_READY.
*
* @see vc_mgr_unset_command_list()
*/
int vc_mgr_set_command_list(vc_cmd_list_h vc_cmd_list);

/**
* @brief Unsets system or exclusive commands.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_READY.
*
* @see vc_mgr_set_command_list()
*/
int vc_mgr_unset_command_list();

/**
* @brief Retrieves all available commands.
*
* @remarks If the function succeeds, @a vc_cmd_list must be released with vc_cmd_list_destroy(vc_cmd_list, true).
*
* @param[in] vc_cmd_list The command list
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failure
*
* @pre The state should be #VC_STATE_READY and the service state should be #VC_SERVICE_STATE_READY.
*/
int vc_mgr_get_current_commands(vc_cmd_list_h* vc_cmd_list);

/**
* @brief Sets audio in type.
*
* @param[in] audio_id audio type (e.g. #VC_AUDIO_TYPE_BLUETOOTH or usb device id)
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failure
*
* @pre The state should be #VC_STATE_READY and the service state should be #VC_SERVICE_STATE_READY.
*
* @see vc_mgr_get_audio_type()
*/
int vc_mgr_set_audio_type(const char* audio_id);

/**
* @brief Gets audio-in type.
*
* @remarks audio_id must be released using free() when it is no longer required.
*
* @param[out] audio_id audio id (e.g. #VC_AUDIO_TYPE_BLUETOOTH or usb device id)
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failure
*
* @pre The state should be #VC_STATE_READY and the service state should be #VC_SERVICE_STATE_READY.
*
* @see vc_mgr_set_audio_type()
*/
int vc_mgr_get_audio_type(char** audio_id);

/**
* @brief Sets recognition mode.
*
* @param[in] mode recognition mode (e.g. #VC_RECOGNITION_MODE_STOP_BY_SILENCE is default value)
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_READY and the service state should be #VC_SERVICE_STATE_READY.
*
* @see vc_mgr_set_recognition_mode()
*/
int vc_mgr_set_recognition_mode(vc_recognition_mode_e mode);

/**
* @brief Gets recognition mode.
*
* @param[out] mode recognition mode
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_READY.
*
* @see vc_mgr_get_recognition_mode()
*/
int vc_mgr_get_recognition_mode(vc_recognition_mode_e* mode);

/**
* @brief Starts recognition.
*
* @remarks The default recognition mode is #VC_RECOGNITION_MODE_STOP_BY_SILENCE. \n
* If you want to use other mode, you can set mode with vc_mgr_set_recognition_mode().
*
* @param[in] exclusive_command_option Exclusive command option
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failure
*
* @pre The state should be #VC_STATE_READY and the service state should be #VC_SERVICE_STATE_READY.
* @post It will invoke vc_service_state_changed_cb(), if you register a callback with vc_service_state_changed_cb(). \n
* If this function succeeds, the service state will be #VC_SERVICE_STATE_RECORDING.
*
* @see vc_mgr_stop()
* @see vc_mgr_cancel()
* @see vc_service_state_changed_cb()
* @see vc_mgr_set_recognition_mode()
* @see vc_mgr_get_recognition_mode()
*/
int vc_mgr_start(bool exclusive_command_option);

/**
* @brief Stop recognition.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failure
*
* @pre The service state should be #VC_SERVICE_STATE_RECORDING.
* @post It will invoke vc_service_state_changed_cb(), if you register a callback with vc_service_state_changed_cb(). \n
* If this function succeeds, the service state will be #VC_SERVICE_STATE_PROCESSING.
*
* @see vc_mgr_start()
* @see vc_mgr_cancel()
* @see vc_service_state_changed_cb()
* @see vc_mgr_result_cb()
*/
int vc_mgr_stop();

/**
* @brief Cancels recognition.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_OUT_OF_MEMORY Not enough memory
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failure
*
* @pre The service state should be #VC_SERVICE_STATE_RECORDING or #VC_SERVICE_STATE_PROCESSING.
* @post It will invoke vc_service_state_changed_cb(), if you register a callback with vc_service_state_changed_cb(). \n
* If this function succeeds, the service state will be #VC_SERVICE_STATE_READY.
*
* @see vc_mgr_start()
* @see vc_mgr_stop()
* @see vc_service_state_changed_cb()
*/
int vc_mgr_cancel();

/**
* @brief Gets the microphone volume during recording.
*
* @param[out] volume Recording volume
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_OUT_OF_MEMORY Not enough memory
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failure
*
* @pre The service state should be #VC_SERVICE_STATE_RECORDING.
*
* @see vc_mgr_start()
*/
int vc_mgr_get_recording_volume(float* volume);

/**
* @brief Select valid result from all results.
*
* @param[in] vc_cmd_list The valid result list
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_OUT_OF_MEMORY Not enough memory
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failure
*
* @pre vc_mgr_all_result_cb() should be called
*
* @see vc_mgr_all_result_cb()
*/
int vc_mgr_set_selected_results(vc_cmd_list_h vc_cmd_list);


int vc_mgr_get_nlp_info(char** info);

int vc_mgr_set_pre_result_cb(vc_mgr_pre_result_cb callback, void* user_data);

int vc_mgr_unset_pre_result_cb();

/**
* @brief Registers a callback function for getting recognition result.
*
* @param[in] callback Callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_mgr_all_result_cb()
* @see vc_mgr_unset_all_result_cb()
*/
int vc_mgr_set_all_result_cb(vc_mgr_all_result_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_mgr_set_all_result_cb()
*/
int vc_mgr_unset_all_result_cb();

/**
* @brief Registers a callback function for getting system or exclusive recognition result.
*
* @param[in] callback Callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_result_cb()
* @see vc_mgr_unset_result_cb()
*/
int vc_mgr_set_result_cb(vc_result_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_mgr_set_result_cb()
*/
int vc_mgr_unset_result_cb();

/**
* @brief Registers a callback function to be called when state is changed.
*
* @param[in] callback Callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_state_changed_cb()
* @see vc_mgr_unset_state_changed_cb()
*/
int vc_mgr_set_state_changed_cb(vc_state_changed_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_mgr_set_state_changed_cb()
*/
int vc_mgr_unset_state_changed_cb();

/**
* @brief Registers a callback function to be called when state is changed.
*
* @param[in] callback Callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_service_state_changed_cb()
* @see vc_mgr_unset_service_state_changed_cb()
*/
int vc_mgr_set_service_state_changed_cb(vc_service_state_changed_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_mgr_set_service_state_changed_cb()
*/
int vc_mgr_unset_service_state_changed_cb();

/**
* @brief Registers a callback function to be called when begin of speech is detected.
*
* @param[in] callback Callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_mgr_begin_speech_detected_cb()
* @see vc_mgr_unset_speech_detected_cb()
*/
int vc_mgr_set_speech_detected_cb(vc_mgr_begin_speech_detected_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_mgr_set_speech_detected_cb()
*/
int vc_mgr_unset_speech_detected_cb();

/**
* @brief Registers a callback function to be called when current language is changed.
*
* @param[in] callback Callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_current_language_changed_cb()
* @see vc_mgr_unset_current_language_changed_cb()
*/
int vc_mgr_set_current_language_changed_cb(vc_current_language_changed_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_mgr_set_current_language_changed_cb()
*/
int vc_mgr_unset_current_language_changed_cb();

/**
* @brief Registers a callback function to be called when an error occurred.
*
* @param[in] callback Callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_error_cb()
* @see vc_mgr_unset_error_cb()
*/
int vc_mgr_set_error_cb(vc_error_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_mgr_set_error_cb()
*/
int vc_mgr_unset_error_cb();


#ifdef __cplusplus
}
#endif

/**
 * @}@}
 */

#endif /* __VOICE_CONTROL_MANAGER_H__ */

