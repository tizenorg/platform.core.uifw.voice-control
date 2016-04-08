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


#ifndef __VOICE_CONTROL_WIDGET_H__
#define __VOICE_CONTROL_WIDGET_H__

#include <voice_control_command.h>
#include <voice_control_command_expand.h>
#include <voice_control_common.h>


/**
* @addtogroup VOICE_CONTROL_WIDGET
* @{
*/

#ifdef __cplusplus
extern "C"
{
#endif

/**
* @brief Definitions for widget command type.
* @since_tizen 2.4
*/
#define VC_COMMAND_TYPE_WIDGET		3

/**
* @brief Called when widget should show or hide tooltip.
*
* @param[in] vc_widget The voice control handle
* @param[in] show Show or hide option
* @param[in] user_data The user data passed from the callback registration function
*
* @pre An application registers callback function using vc_widget_set_show_tooltip_cb().
*
* @see vc_widget_set_show_tooltip_cb()
* @see vc_widget_unset_show_tooltip_cb()
*/

#ifndef EXPORT_API
#define EXPORT_API
#endif // EXPORT_API

typedef void (*vc_widget_show_tooltip_cb)(bool show, void* user_data);

/**
* @brief Called when widget send current command list to vc daemon.
*
* @param[in] vc_widget The voice control handle
* @param[out] vc_cmd_list Current command list
* @param[in] user_data The user data passed from the callback registration function
*
* @pre An application registers callback function using vc_widget_set_send_current_command_group_cb().
*
* @see vc_widget_set_send_current_command_list_cb()
* @see vc_widget_unsset_send_current_command_list_cb()
*/
typedef void (*vc_widget_send_current_command_list_cb)(vc_cmd_list_h* vc_cmd_list, void* user_data);


/**
* @brief Initialize voice control for widget.
*
* @param[in] vc_widget The voice control handle
*
* @remarks If the function succeeds, @a vc widget must be released with vc_widget_deinitialize().
*
* @param[out] vc_widget The voice control handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_OUT_OF_MEMORY Out of memory
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_OPERATION_FAILED Operation failed
*
* @post If this function is called, the state will be #VC_STATE_INITIALIZED.
*
* @see vc_widget_deinitialize()
*/
EXPORT_API int vc_widget_initialize();

/**
* @brief Deinitialize voice control for widget.
*
* @param[in] vc_widget The voice control handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @see vc_widget_initialize()
*/
EXPORT_API int vc_widget_deinitialize();

/**
* @brief Connects the voice control service asynchronously.
*
* @param[in] vc_widget The voice control handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
* @post If this function is called, the state will be #VC_STATE_READY.
*
* @see vc_widget_unprepare()
*/
EXPORT_API int vc_widget_prepare();

/**
* @brief Disconnects the voice control service.
*
* @param[in] vc_widget The voice control handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_READY.
* @post If this function is called, the state will be #VC_STATE_INITIALIZED.
*
* @see vc_widget_prepare()
*/
EXPORT_API int vc_widget_unprepare();

/**
* @brief Retrieves all supported languages using callback function.
*
* @param[in] vc_widget The voice control handle
* @param[in] callback Callback function to invoke
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_OPERATION_FAILED Operation failure
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_READY.
* @post	This function invokes vc_supported_language_cb() repeatedly for getting languages.
*
* @see vc_supported_language_cb()
* @see vc_widget_get_current_language()
*/
EXPORT_API int vc_widget_foreach_supported_languages(vc_supported_language_cb callback, void* user_data);

/**
* @brief Gets current language set by user.
*
* @remark If the function succeeds, @a language must be released with free() by you when you no longer need it.
*
* @param[in] vc_widget The voice control handle
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
* @pre The state should be #VC_STATE_READY.
*
* @see vc_widget_foreach_supported_languages()
*/
EXPORT_API int vc_widget_get_current_language(char** language);

/**
* @brief Gets current state of voice control widget.
*
* @param[in] vc_widget The voice control handle
* @param[out] state Current state
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
*
* @see vc_widget_initialize()
* @see vc_widget_deinitialize()
* @see vc_widget_prepare()
* @see vc_widget_unprepare()
* @see vc_widget_set_state_changed_cb()
* @see vc_widget_unset_state_changed_cb()
*/
EXPORT_API int vc_widget_get_state(vc_state_e* state);

/**
* @brief Gets current state of voice control service.
*
* @param[in] vc_widget The voice control handle
* @param[out] state The current state
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
*
* @see vc_widget_start()
* @see vc_widget_stop()
* @see vc_widget_cancel()
* @see vc_set_service_state_changed_cb()
* @see vc_unset_service_state_changed_cb()
*/
EXPORT_API int vc_widget_get_service_state(vc_service_state_e* state);

/**
* @brief Checks whether the command format is supported.
*
* @param[in] vc_widget The voice control handle
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
EXPORT_API int vc_widget_is_command_format_supported(vc_cmd_format_e format, bool* support);


/**
* @brief Sets foreground state of application.
*
* @param[in] vc_widget The voice control handle
* @param[in] value value @c true foreground, \n @c false background.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failed
*
* @pre The state should be #VC_STATE_READY.
*/
EXPORT_API int vc_widget_set_foreground(bool value);

/**
* @brief Cancels recognition.
*
* @param[in] vc_widget The voice control handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_OUT_OF_MEMORY Not enough memory
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failure
*
* @pre The service state should be #VC_SERVICE_STATE_RECORDING or #VC_SERVICE_STATE_PROCESSING.
* @post It will invoke vc_state_changed_cb(), if you register a callback with vc_state_changed_cb(). \n
* If this function succeeds, the state will be #VC_STATE_READY.
*
* @see vc_widget_start()
* @see vc_widget_stop()
* @see vc_state_changed_cb()
*/
EXPORT_API int vc_widget_cancel();

/**
* @brief Registers a callback function for getting recognition result.
*
* @param[in] vc_widget The voice control handle
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
* @see vc_widget_result_cb()
* @see vc_widget_unset_result_cb()
*/
EXPORT_API int vc_widget_set_result_cb(vc_result_cb callback, void* user_data);


/**
* @brief Unregisters the callback function.
*
* @param[in] vc_widget The voice control handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_widget_set_result_cb()
*/
EXPORT_API int vc_widget_unset_result_cb();

/**
* @brief Registers a callback function for showing or hiding tooltip.
*
* @param[in] vc_widget The voice control handle
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
* @see vc_widget_show_tooltip_cb()
* @see vc_widget_unset_show_tooltip_cb()
*/
EXPORT_API int vc_widget_set_show_tooltip_cb(vc_widget_show_tooltip_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @param[in] vc_widget The voice control handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_widget_set_show_tooltip_cb()
*/
EXPORT_API int vc_widget_unset_show_tooltip_cb();

/**
* @brief Registers a callback function for setting current command.
*
* @param[in] vc_widget The voice control handle
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
* @see vc_widget_send_current_command_list_cb()
* @see vc_widget_unset_send_current_command_list_cb()
*/
EXPORT_API int vc_widget_set_send_current_command_list_cb(vc_widget_send_current_command_list_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @param[in] vc_widget The voice control handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_widget_set_send_current_command_list_cb()
*/
EXPORT_API int vc_widget_unsset_send_current_command_list_cb();

/**
* @brief Registers a callback function to be called when service state is changed.
*
* @param[in] vc_widget The voice control handle
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
* @see vc_widget_unset_service_state_changed_cb()
*/
EXPORT_API int vc_widget_set_service_state_changed_cb(vc_service_state_changed_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @param[in] vc_widget The voice control handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_widget_set_service_state_changed_cb()
*/
EXPORT_API int vc_widget_unset_service_state_changed_cb();

/**
* @brief Registers a callback function for getting state changed.
*
* @param[in] vc_widget The voice control handle
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
* @see vc_widget_unset_state_changed_cb()
*/
EXPORT_API int vc_widget_set_state_changed_cb(vc_state_changed_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @param[in] vc_widget The voice control handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_widget_set_state_changed_cb()
*/
EXPORT_API int vc_widget_unset_state_changed_cb();

/**
* @brief Registers a callback function to be called when current language is changed.
*
* @param[in] vc_widget The voice control handle
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
* @see vc_widget_unset_current_language_changed_cb()
*/
EXPORT_API int vc_widget_set_current_language_changed_cb(vc_current_language_changed_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @param[in] vc_widget The voice control handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_widget_set_current_language_changed_cb()
*/
EXPORT_API int vc_widget_unset_current_language_changed_cb();

/**
* @brief Registers a callback function for an error occurred.
*
* @param[in] vc_widget The voice control handle
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
* @see vc_widget_unset_error_cb()
*/
EXPORT_API int vc_widget_set_error_cb(vc_error_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @param[in] vc_widget The voice control handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
*
* @pre The state should be #VC_STATE_INITIALIZED.
*
* @see vc_widget_set_error_cb()
*/
EXPORT_API int vc_widget_unset_error_cb();


#ifdef __cplusplus
}
#endif

/**
 * @}@}
 */

#endif /* __VOICE_CONTROL_WIDGET_H__ */
