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


#include "vc_config_mgr.h"
#include "vc_main.h"
#include "voice_control_common.h"
#include "voice_control_setting.h"

/** 
* @brief Enumerations of mode.
*/
typedef enum {
	VC_SETTING_STATE_NONE = 0,
	VC_SETTING_STATE_READY
}vc_setting_state_e;

#define VC_SETTING_CONFIG_HANDLE	300000

static vc_setting_state_e g_state = VC_SETTING_STATE_NONE;

static vc_setting_enabled_changed_cb g_callback;

static void* g_user_data;


const char* vc_tag()
{
	return TAG_VCS;
}

void __config_lang_changed_cb(const char* before_lang, const char* current_lang)
{
	SLOG(LOG_DEBUG, TAG_VCS, "Lang changed : before(%s) current(%s)", before_lang, current_lang);
}

void __vc_setting_state_changed_cb(int before_state, int current_state, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_VCS, "Service State changed : Before(%d) Current(%d)", 
		before_state, current_state);
	return;
}

void __vc_setting_enabled_changed_cb(bool enabled)
{
	SLOG(LOG_DEBUG, TAG_VCS, "Service enabled changed : %s", enabled ? "on" : "off");

	if (NULL != g_callback) {
		g_callback(enabled, g_user_data);
	}

	return;
}

int vc_setting_initialize(void)
{
	SLOG(LOG_DEBUG, TAG_VCS, "===== Initialize VC Setting");

	if (VC_SETTING_STATE_READY == g_state) {
		SLOG(LOG_WARN, TAG_VCS, "[WARNING] VC Setting has already been initialized.");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_NONE;
	}

	int ret = vc_config_mgr_initialize(getpid() + VC_SETTING_CONFIG_HANDLE);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Fail to initialize config manager");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_OPERATION_FAILED;
	}
	
	ret = vc_config_mgr_set_lang_cb(getpid() + VC_SETTING_CONFIG_HANDLE, __config_lang_changed_cb);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Fail to initialize config manager");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_OPERATION_FAILED;
	}

	ret = vc_config_mgr_set_enabled_cb(getpid() + VC_SETTING_CONFIG_HANDLE, __vc_setting_enabled_changed_cb);
	
	g_state = VC_SETTING_STATE_READY;

	SLOG(LOG_DEBUG, TAG_VCS, "=====");
	SLOG(LOG_DEBUG, TAG_VCS, " ");

	return VC_ERROR_NONE;
}

//int vc_setting_finalize()
//{
//	SLOG(LOG_DEBUG, TAG_VCS, "===== Finalize VC Setting");
//
//	vc_config_mgr_unset_lang_cb(getpid() + VC_SETTING_CONFIG_HANDLE);
//	vc_config_mgr_finalize(getpid() + VC_SETTING_CONFIG_HANDLE);
//
//	g_state = VC_SETTING_STATE_NONE;
//	
//	SLOG(LOG_DEBUG, TAG_VCS, "=====");
//	SLOG(LOG_DEBUG, TAG_VCS, " ");
//
//	return VC_ERROR_NONE;
//}

int vc_setting_deinitialize()
{
	SLOG(LOG_DEBUG, TAG_VCS, "===== Deinitialize VC Setting");

	vc_config_mgr_unset_lang_cb(getpid() + VC_SETTING_CONFIG_HANDLE);
	vc_config_mgr_finalize(getpid() + VC_SETTING_CONFIG_HANDLE);

	g_state = VC_SETTING_STATE_NONE;
	
	SLOG(LOG_DEBUG, TAG_VCS, "=====");
	SLOG(LOG_DEBUG, TAG_VCS, " ");

	return VC_ERROR_NONE;
}

int vc_setting_foreach_supported_languages(vc_setting_supported_language_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_VCS, "===== Foreach supported languages");

	if (VC_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_STATE;
	}

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Param is NULL");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	int ret = vc_config_mgr_get_language_list((vc_supported_language_cb)callback, user_data);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_VCS, "[SUCCESS] Foreach supported languages");
	}

	SLOG(LOG_DEBUG, TAG_VCS, "=====");
	SLOG(LOG_DEBUG, TAG_VCS, " ");

	return ret;
}

int vc_setting_get_language(char** language)
{
	SLOG(LOG_DEBUG, TAG_VCS, "===== Get default language");

	if (VC_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_STATE;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	int ret = vc_config_mgr_get_default_language(language);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_VCS, "[SUCCESS] Get default language");
	}

	SLOG(LOG_DEBUG, TAG_VCS, "=====");
	SLOG(LOG_DEBUG, TAG_VCS, " ");

	return ret;
}

int vc_setting_set_language(const char* language)
{
	SLOG(LOG_DEBUG, TAG_VCS, "===== Set default language");

	if (VC_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_STATE;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	int ret = vc_config_mgr_set_default_language(language);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_VCS, "[SUCCESS] Set default language");
	}

	SLOG(LOG_DEBUG, TAG_VCS, "=====");
	SLOG(LOG_DEBUG, TAG_VCS, " ");
    
	return ret;
}

int vc_setting_set_auto_language(bool value)
{
	SLOG(LOG_DEBUG, TAG_VCS, "===== Set auto voice");

	if (VC_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_STATE;
	}

	if (value != true && value != false) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Invalid value");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	int ret = vc_config_mgr_set_auto_language(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_VCS, "[SUCCESS] Set auto language (%s)", value ? "on" : "off");
	}

	SLOG(LOG_DEBUG, TAG_VCS, "=====");
	SLOG(LOG_DEBUG, TAG_VCS, " ");

	return ret;
}

int vc_setting_get_auto_language(bool* value)
{
	SLOG(LOG_DEBUG, TAG_VCS, "===== Get auto language");

	if (VC_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_STATE;
	}

	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Param is NULL");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	int ret = vc_config_mgr_get_auto_language(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_VCS, "[SUCCESS] Get auto language (%s)", *value ? "true":"false");
	}

	SLOG(LOG_DEBUG, TAG_VCS, "=====");
	SLOG(LOG_DEBUG, TAG_VCS, " ");

	return 0;
}

int vc_setting_set_enabled(bool value)
{
	SLOG(LOG_DEBUG, TAG_VCS, "===== Set service enabled");

	if (VC_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_STATE;
	}

	if (value != true && value != false) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Invalid value");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	int ret = vc_config_mgr_set_enabled(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Result : %d", ret);
	} else {
		SLOG(LOG_DEBUG, TAG_VCS, "[SUCCESS] Set service enabled (%s)", value ? "on" : "off");
	}

	SLOG(LOG_DEBUG, TAG_VCS, "=====");
	SLOG(LOG_DEBUG, TAG_VCS, " ");

	return ret;
}

int vc_setting_get_enabled(bool* value)
{
	SLOG(LOG_DEBUG, TAG_VCS, "===== Get service enabled");

	if (VC_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_STATE;
	}

	if (NULL == value) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Param is NULL");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	int ret = vc_config_mgr_get_enabled(value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Result : %d", ret);
	} else {
		/* Copy value */
		SLOG(LOG_DEBUG, TAG_VCS, "[SUCCESS] Get service enabled (%s)", *value ? "on" : "off");
	}

	SLOG(LOG_DEBUG, TAG_VCS, "=====");
	SLOG(LOG_DEBUG, TAG_VCS, " ");

	return ret;
}

int vc_setting_set_enabled_changed_cb(vc_setting_enabled_changed_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_VCS, "===== Set service enabled callback");

	if (VC_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_STATE;
	}

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Param is NULL");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	g_callback = callback;
	g_user_data = user_data;

	SLOG(LOG_DEBUG, TAG_VCS, "=====");
	SLOG(LOG_DEBUG, TAG_VCS, " ");

	return 0;
}

int vc_setting_unset_enabled_changed_cb()
{
	SLOG(LOG_DEBUG, TAG_VCS, "===== Unset service enabled callback");

	if (VC_SETTING_STATE_NONE == g_state) {
		SLOG(LOG_ERROR, TAG_VCS, "[ERROR] Not initialized");
		SLOG(LOG_DEBUG, TAG_VCS, "=====");
		SLOG(LOG_DEBUG, TAG_VCS, " ");
		return VC_ERROR_INVALID_STATE;
	}

	g_callback = NULL;
	g_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_VCS, "=====");
	SLOG(LOG_DEBUG, TAG_VCS, " ");

	return 0;
}