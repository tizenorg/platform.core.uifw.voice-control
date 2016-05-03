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


#ifndef __VOICE_CONTROL_SETTING_H__
#define __VOICE_CONTROL_SETTING_H__

#include <stdbool.h>


/**
* @addtogroup VOICE_CONTROL_SETTING_MODULE
* @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Defines of audio-in type.
*/
#define VC_SETTING_LANGUAGE_AUTO	"auto"

/**
* @brief Called when voice control service enabled is changed.
*
* @param[in] enabled Service enabled
* @param[in] user_data The user data passed from the callback registration function
*
* @pre An application registers this callback to detect changing service enabled.
*
* @see vc_setting_set_enabled_changed_cb()
*/
typedef void (*vc_setting_enabled_changed_cb)(bool enabled, void* user_data);

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
typedef bool(*vc_setting_supported_language_cb)(const char* language, void* user_data);

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
* @see vc_setting_set_current_language_changed_cb()
*/
typedef void (*vc_setting_current_language_changed_cb)(const char* previous, const char* current, void* user_data);

/**
* @brief Initialize voice control setting
*
* @remarks If the function succeeds, @a vc mgr must be released with vc_setting_finalize().
*
* @return 0 on success, otherwise a negative error value.
* @retval #VC_ERROR_NONE Success.
* @retval #VC_ERROR_INVALID_STATE VC setting has Already been initialized.
* @retval #VC_ERROR_OPERATION_FAILED Operation failure.
*
* @see vc_setting_finalize()
*/
int vc_setting_initialize(void);

/**
* @brief Deinitialize vc setting
*
* @return 0 on success, otherwise a negative error value.
* @retval #VC_ERROR_NONE Success.
* @retval #VC_ERROR_INVALID_STATE VC Not initialized.
* @retval #VC_ERROR_OPERATION_FAILED Operation failure.
*
* @see vc_setting_initialize()
*/
int vc_setting_deinitialize(void);

/**
* @brief Get supported languages of current engine
*
* @param[in] callback callback function
* @param[in] user_data User data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value.
* @retval #VC_ERROR_NONE Success.
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #VC_ERROR_INVALID_STATE VC Not initialized.
* @retval #VC_ERROR_OPERATION_FAILED Operation failure.
*
* @post	This function invokes vc_setting_supported_language_cb() repeatedly for getting supported languages.
*
* @see vc_setting_supported_language_cb()
*/
int vc_setting_foreach_supported_languages(vc_setting_supported_language_cb callback, void* user_data);

/**
* @brief Get the default language.
*
* @remark If the function is success, @a language must be released with free() by you.
*
* @param[out] language current language
*
* @return 0 on success, otherwise a negative error value.
* @retval #VC_ERROR_NONE Success.
* @retval #VC_ERROR_OUT_OF_MEMORY Out of memory.
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #VC_ERROR_INVALID_STATE VC Not initialized.
* @retval #VC_ERROR_OPERATION_FAILED Operation failure.
*
* @see vc_setting_set_language()
*/
int vc_setting_get_language(char** language);

/**
* @brief Set the default language.
*
* @param[in] language language
*
* @return 0 on success, otherwise a negative error value.
* @retval #VC_ERROR_NONE Success.
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #VC_ERROR_INVALID_STATE VC Not initialized.
* @retval #VC_ERROR_OPERATION_FAILED Operation failure.
*
* @see vc_setting_get_language()
*/
int vc_setting_set_language(const char* language);

/**
* @brief Set a automatic option of language.
*
* @param[in] value The automatic option
*
* @return 0 on success, otherwise a negative error value.
* @retval #VC_ERROR_NONE Success.
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #VC_ERROR_INVALID_STATE VC Not initialized.
* @retval #VC_ERROR_OPERATION_FAILED Operation failure.
*
* @see vc_setting_get_auto_language()
*/
int vc_setting_set_auto_language(bool value);

/**
* @brief Get a automatic option of voice.
*
* @param[out] value The automatic option
*
* @return 0 on success, otherwise a negative error value.
* @retval #VC_ERROR_NONE Success.
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #VC_ERROR_INVALID_STATE VC Not initialized.
* @retval #VC_ERROR_OPERATION_FAILED Operation failure.
*
* @see vc_setting_set_auto_language()
*/
int vc_setting_get_auto_language(bool* value);

/**
* @brief Set voice control service enabled.
*
* @param[in] value The enabled option
*
* @return 0 on success, otherwise a negative error value.
* @retval #VC_ERROR_NONE Success.
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #VC_ERROR_INVALID_STATE VC Not initialized.
* @retval #VC_ERROR_OPERATION_FAILED Operation failure.
*
* @see vc_setting_get_enabled()
*/
int vc_setting_set_enabled(bool value);

/**
* @brief Get voice control service enabled.
*
* @param[out] value The enabled option
*
* @return 0 on success, otherwise a negative error value.
* @retval #VC_ERROR_NONE Success.
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #VC_ERROR_INVALID_STATE VC Not initialized.
* @retval #VC_ERROR_OPERATION_FAILED Operation failure.
*
* @see vc_setting_set_enabled()
*/
int vc_setting_get_enabled(bool* value);

/**
* @brief Sets a callback function to be called when service enabled is changed.
*
* @param[in] callback Callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value.
* @retval #VC_ERROR_NONE Success.
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #VC_ERROR_INVALID_STATE VC Not initialized.
* @retval #VC_ERROR_OPERATION_FAILED Operation failure.
*
* @pre vc_setting_initialize() should be successful.
*
* @see vc_setting_unset_enabled_changed_cb()
*/
int vc_setting_set_enabled_changed_cb(vc_setting_enabled_changed_cb callback, void* user_data);

/**
* @brief Unsets the callback function.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Success.
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #VC_ERROR_INVALID_STATE VC Not initialized.
* @retval #VC_ERROR_OPERATION_FAILED Operation failure.
*
* @pre vc_setting_initialize() should be successful.
*
* @see vc_setting_set_enabled_changed_cb()
*/
int vc_setting_unset_enabled_changed_cb();

/**
* @brief Registers a callback function to be called when current language is changed.
*
* @param[in] callback Callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Success.
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #VC_ERROR_INVALID_STATE VC Not initialized.
*
* @pre vc_setting_initialize() should be successful.
*
* @see vc_setting_unset_current_language_changed_cb()
*/
int vc_setting_set_current_language_changed_cb(vc_setting_current_language_changed_cb callback, void* user_data);

/**
* @brief Unregisters the callback function.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Success.
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #VC_ERROR_INVALID_STATE VC Not initialized.
*
* @pre vc_setting_initialize() should be successful.
*
* @see vc_setting_set_current_language_changed_cb()
*/
int vc_setting_unset_current_language_changed_cb();


#ifdef __cplusplus
}
#endif

/**
* @}
*/

#endif /* __VOICE_CONTROL_SETTING_H__ */
