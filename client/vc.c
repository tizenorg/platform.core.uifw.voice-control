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

#include <aul.h>
#include <system_info.h>

#include "vc_client.h"
#include "vc_command.h"
#include "vc_config_mgr.h"
#include "vc_dbus.h"
#include "vc_info_parser.h"
#include "vc_main.h"
#include "voice_control.h"
#include "voice_control_authority.h"
#include "voice_control_command.h"
#include "voice_control_command_expand.h"



static Ecore_Timer* g_connect_timer = NULL;

static vc_h g_vc = NULL;

static int g_feature_enabled = -1;

#if 0
static Ecore_Event_Handler* g_focus_in_hander = NULL;
static Ecore_Event_Handler* g_focus_out_hander = NULL;
#endif

Eina_Bool __vc_notify_state_changed(void *data);
Eina_Bool __vc_notify_error(void *data);

static int __vc_get_feature_enabled()
{
	if (0 == g_feature_enabled) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Voice control feature NOT supported");
		return VC_ERROR_NOT_SUPPORTED;
	} else if (-1 == g_feature_enabled) {
		bool vc_supported = false;
		bool mic_supported = false;
		if (0 == system_info_get_platform_bool(VC_FEATURE_PATH, &vc_supported)) {
			if (0 == system_info_get_platform_bool(VC_MIC_FEATURE_PATH, &mic_supported)) {
				if (false == vc_supported || false == mic_supported) {
					SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Voice control feature NOT supported");
					g_feature_enabled = 0;
					return VC_ERROR_NOT_SUPPORTED;
				}

				g_feature_enabled = 1;
			} else {
				SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get feature value");
				return VC_ERROR_NOT_SUPPORTED;
			}
		} else {
			SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get feature value");
			return VC_ERROR_NOT_SUPPORTED;
		}
	}

	return 0;
}

static const char* __vc_get_error_code(vc_error_e err)
{
	switch (err) {
	case VC_ERROR_NONE:			return "VC_ERROR_NONE";			break;
	case VC_ERROR_OUT_OF_MEMORY:		return "VC_ERROR_OUT_OF_MEMORY";	break;
	case VC_ERROR_IO_ERROR:			return "VC_ERROR_IO_ERROR";		break;
	case VC_ERROR_INVALID_PARAMETER:	return "VC_ERROR_INVALID_PARAMETER";	break;
	case VC_ERROR_TIMED_OUT:		return "VC_ERROR_TIMED_OUT";		break;
	case VC_ERROR_RECORDER_BUSY:		return "VC_ERROR_RECORDER_BUSY";	break;
	case VC_ERROR_INVALID_STATE:		return "VC_ERROR_INVALID_STATE";	break;
	case VC_ERROR_INVALID_LANGUAGE:		return "VC_ERROR_INVALID_LANGUAGE";	break;
	case VC_ERROR_ENGINE_NOT_FOUND:		return "VC_ERROR_ENGINE_NOT_FOUND";	break;
	case VC_ERROR_OPERATION_FAILED:		return "VC_ERROR_OPERATION_FAILED";	break;
	default:				return "Invalid error code";		break;
	}
	return NULL;
}

static int __vc_convert_config_error_code(vc_config_error_e code)
{
	if (code == VC_CONFIG_ERROR_NONE)			return VC_ERROR_NONE;
	if (code == VC_CONFIG_ERROR_OUT_OF_MEMORY)		return VC_ERROR_OUT_OF_MEMORY;
	if (code == VC_CONFIG_ERROR_IO_ERROR)			return VC_ERROR_IO_ERROR;
	if (code == VC_CONFIG_ERROR_INVALID_PARAMETER)		return VC_ERROR_INVALID_PARAMETER;
	if (code == VC_CONFIG_ERROR_INVALID_STATE)		return VC_ERROR_INVALID_STATE;
	if (code == VC_CONFIG_ERROR_INVALID_LANGUAGE)		return VC_ERROR_INVALID_LANGUAGE;
	if (code == VC_CONFIG_ERROR_ENGINE_NOT_FOUND)		return VC_ERROR_ENGINE_NOT_FOUND;
	if (code == VC_CONFIG_ERROR_OPERATION_FAILED)		return VC_ERROR_OPERATION_FAILED;

	return VC_ERROR_NONE;
}

static void __vc_lang_changed_cb(const char* before_lang, const char* current_lang)
{
	SLOG(LOG_DEBUG, TAG_VCC, "Lang changed : Before lang(%s) Current lang(%s)",
		 before_lang, current_lang);

	vc_current_language_changed_cb callback;
	void* lang_user_data;
	vc_client_get_current_lang_changed_cb(g_vc, &callback, &lang_user_data);

	if (NULL != callback) {
		vc_client_use_callback(g_vc);
		callback(before_lang, current_lang, lang_user_data);
		vc_client_not_use_callback(g_vc);
		SLOG(LOG_DEBUG, TAG_VCC, "Language changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_VCC, "[WARNING] Language changed callback is null");
	}

	return;
}

static Eina_Bool __notify_auth_changed_cb(void *data)
{
	vc_auth_state_changed_cb callback = NULL;
	void* user_data;

	vc_client_get_auth_state_changed_cb(g_vc, &callback, &user_data);

	vc_auth_state_e before = -1;
	vc_auth_state_e current = -1;

	vc_client_get_before_auth_state(g_vc, &before, &current);

	if (NULL != callback) {
		vc_client_use_callback(g_vc);
		callback(before, current, user_data);
		vc_client_not_use_callback(g_vc);
		SLOG(LOG_DEBUG, TAG_VCC, "Auth state changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_VCC, "[WARNING] Auth state changed callback is null");
	}

	return EINA_FALSE;
}

static int __vc_app_state_changed_cb(int app_state, void *data)
{
	int ret = -1;
	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] app state changed");

	/* Set current pid */
	if (STATUS_VISIBLE == app_state) {
		SLOG(LOG_DEBUG, TAG_VCC, "===== Set foreground");
		ret = vc_dbus_set_foreground(getpid(), true);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to set foreground (true) : %d", ret);
		}

		ret = vc_client_set_is_foreground(g_vc, true);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to save is_foreground (true) : %d", ret);
		}

		/* set authority valid */
		vc_auth_state_e state = VC_AUTH_STATE_NONE;
		if (0 != vc_client_get_auth_state(g_vc, &state)) {
			SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get auth state");
		}
		if (VC_AUTH_STATE_INVALID == state) {
			vc_client_set_auth_state(g_vc, VC_AUTH_STATE_VALID);

			/* notify auth changed cb */
			ecore_timer_add(0, __notify_auth_changed_cb, NULL);
		}
	} else if (STATUS_BG == app_state) {
		SLOG(LOG_DEBUG, TAG_VCC, "===== Set background");
		ret = vc_dbus_set_foreground(getpid(), false);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to set foreground (false) : %d", ret);
		}

		ret = vc_client_set_is_foreground(g_vc, false);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to save is_foreground (false) : %d", ret);
		}

		/* set authority valid */
		vc_auth_state_e state = VC_AUTH_STATE_NONE;
		if (0 != vc_client_get_auth_state(g_vc, &state)) {
			SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get auth state");
		}
		if (VC_AUTH_STATE_VALID == state) {
			vc_client_set_auth_state(g_vc, VC_AUTH_STATE_INVALID);

			/* notify authority changed cb */
			ecore_timer_add(0, __notify_auth_changed_cb, NULL);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, "===== App state is NOT valid");
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return 0;
}

int vc_initialize(void)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Initialize");

	/* check handle */
	if (true == vc_client_is_valid(g_vc)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Already initialized");
		return VC_ERROR_INVALID_STATE;
	}

	if (0 < vc_client_get_count()) {
		SLOG(LOG_DEBUG, TAG_VCC, "[DEBUG] Already initialized");
		return VC_ERROR_INVALID_STATE;
	}

	if (0 != vc_dbus_open_connection()) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to open connection");
		return VC_ERROR_OPERATION_FAILED;
	}

	if (0 != vc_client_create(&g_vc)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to create client!!!!!");
		return VC_ERROR_OUT_OF_MEMORY;
	}

	int ret = vc_config_mgr_initialize(g_vc->handle);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to init config manager : %s",
			 __vc_get_error_code(__vc_convert_config_error_code(ret)));
		vc_client_destroy(g_vc);
		return __vc_convert_config_error_code(ret);
	}

	ret = vc_config_mgr_set_lang_cb(g_vc->handle, __vc_lang_changed_cb);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to set config changed : %d", ret);
		vc_config_mgr_finalize(g_vc->handle);
		vc_client_destroy(g_vc);
		return __vc_convert_config_error_code(ret);
	}

	SLOG(LOG_DEBUG, TAG_VCC, "[Success] pid(%d)", g_vc->handle);

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return VC_ERROR_NONE;
}

static void __vc_internal_unprepare(void)
{
	/* return authority */
	vc_auth_state_e state = VC_AUTH_STATE_NONE;
	if (0 != vc_client_get_auth_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get auth state");
	}

	if (VC_AUTH_STATE_NONE != state) {
		if (0 != vc_auth_disable()) {
			SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to auth disable");
		}
	}

	int ret = vc_dbus_request_finalize(g_vc->handle);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to request finalize : %s", __vc_get_error_code(ret));
	}


#if 0
	ecore_event_handler_del(g_focus_in_hander);
	ecore_event_handler_del(g_focus_out_hander);
#endif

	vc_cmd_parser_delete_file(getpid(), VC_COMMAND_TYPE_FOREGROUND);
	vc_cmd_parser_delete_file(getpid(), VC_COMMAND_TYPE_BACKGROUND);

	return;
}

int vc_deinitialize(void)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Deinitialize");

	if (false == vc_client_is_valid(g_vc)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] NOT initialized");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	vc_state_e state;
	vc_client_get_client_state(g_vc, &state);

	/* check state */
	switch (state) {
	case VC_STATE_READY:
		__vc_internal_unprepare();
		/* no break. need to next step*/
	case VC_STATE_INITIALIZED:
		if (NULL != g_connect_timer) {
			SLOG(LOG_DEBUG, TAG_VCC, "Connect Timer is deleted");
			ecore_timer_del(g_connect_timer);
		}

		vc_config_mgr_unset_lang_cb(g_vc->handle);
		vc_config_mgr_finalize(g_vc->handle);

		/* Free client resources */
		vc_client_destroy(g_vc);
		g_vc = NULL;
		break;
	case VC_STATE_NONE:
		break;
	default:
		break;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "Success: destroy");

	if (0 != vc_dbus_close_connection()) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to close connection");
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return VC_ERROR_NONE;
}

#if 0
static Eina_Bool __vc_x_event_window_focus_in(void *data, int type, void *event)
{
	Ecore_X_Event_Window_Focus_In *e;

	e = event;

	int xid = -1;
	if (0 != vc_client_get_xid(g_vc, &xid)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get current xid");
		return ECORE_CALLBACK_PASS_ON;
	}

	if (e->win == (Ecore_X_Window)xid) {
		SLOG(LOG_DEBUG, TAG_VCC, "Focus in : pid(%d) xid(%d)", getpid(), xid);
		int ret = vc_config_mgr_set_foreground(getpid(), true);
		if (0 != ret) {
			ret = vc_config_convert_error_code((vc_config_error_e)ret);
			SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to set focus in : %s", __vc_get_error_code(ret));
		}
		/* set authority valid */
		vc_auth_state_e state = VC_AUTH_STATE_NONE;
		if (0 != vc_client_get_auth_state(g_vc, &state)) {
			SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get auth state");
		}
		if (VC_AUTH_STATE_INVALID == state) {
			vc_client_set_auth_state(g_vc, VC_AUTH_STATE_VALID);

			/* notify auth changed cb */
			ecore_timer_add(0, __notify_auth_changed_cb, NULL);
		}
	}

	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool __vc_x_event_window_focus_out(void *data, int type, void *event)
{
	Ecore_X_Event_Window_Focus_In *e;

	e = event;

	int xid = -1;
	if (0 != vc_client_get_xid(g_vc, &xid)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get current xid");
		return ECORE_CALLBACK_PASS_ON;
	}

	if (e->win == (Ecore_X_Window)xid) {
		SLOG(LOG_DEBUG, TAG_VCC, "Focus out : pid(%d) xid(%d)", getpid(), xid);
		int ret = vc_config_mgr_set_foreground(getpid(), false);
		if (0 != ret) {
			ret = vc_config_convert_error_code((vc_config_error_e)ret);
			SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to set focus out : %s", __vc_get_error_code(ret));
		}
		/* set authority valid */
		vc_auth_state_e state = VC_AUTH_STATE_NONE;
		if (0 != vc_client_get_auth_state(g_vc, &state)) {
			SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get auth state");
		}
		if (VC_AUTH_STATE_VALID == state) {
			vc_client_set_auth_state(g_vc, VC_AUTH_STATE_INVALID);

			/* notify authority changed cb */
			ecore_timer_add(0, __notify_auth_changed_cb, NULL);
		}
	}

	return ECORE_CALLBACK_PASS_ON;
}
#endif

static Eina_Bool __vc_connect_daemon(void *data)
{
	/* Send hello */
	if (0 != vc_dbus_request_hello()) {
		return EINA_TRUE;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Connect daemon");

	/* request initialization */
	int ret = -1;
	int mgr_pid = -1;
	int service_state = 0;
	ret = vc_dbus_request_initialize(g_vc->handle, &mgr_pid, &service_state);
	if (VC_ERROR_ENGINE_NOT_FOUND == ret) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to initialize : %s", __vc_get_error_code(ret));

		vc_client_set_error(g_vc, VC_ERROR_ENGINE_NOT_FOUND);
		ecore_timer_add(0, __vc_notify_error, g_vc);

		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, "  ");
		return EINA_FALSE;

	} else if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to initialize :%s", __vc_get_error_code(ret));

		vc_client_set_error(g_vc, VC_ERROR_TIMED_OUT);
		ecore_timer_add(0, __vc_notify_error, g_vc);

		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, "  ");
		return EINA_FALSE;
	} else {
		/* Success to connect */
	}

	/* Set service state */
	vc_client_set_service_state(g_vc, (vc_service_state_e)service_state);

	g_connect_timer = NULL;
#if 0
	g_focus_in_hander = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_IN, __vc_x_event_window_focus_in, NULL);
	g_focus_out_hander = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_OUT, __vc_x_event_window_focus_out, NULL);
#else
	ret = aul_add_status_local_cb(__vc_app_state_changed_cb, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to set app stae changed callback");
	}
#endif
	vc_client_set_client_state(g_vc, VC_STATE_READY);
	ecore_timer_add(0, __vc_notify_state_changed, g_vc);

	vc_client_set_mgr_pid(g_vc, mgr_pid);

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, "  ");

	return EINA_FALSE;
}

int vc_prepare(void)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Prepare");

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: Current state is not 'CREATED'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	g_connect_timer = ecore_timer_add(0, __vc_connect_daemon, NULL);

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return VC_ERROR_NONE;
}

int vc_unprepare(void)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Unprepare");

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	__vc_internal_unprepare();

	vc_client_set_client_state(g_vc, VC_STATE_INITIALIZED);
	ecore_timer_add(0, __vc_notify_state_changed, g_vc);

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return VC_ERROR_NONE;
}

int vc_foreach_supported_languages(vc_supported_language_cb callback, void* user_data)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Foreach Supported Language");

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret = -1;
	ret = vc_config_mgr_get_language_list(callback, user_data);
	if (0 != ret) {
		ret = vc_config_convert_error_code((vc_config_error_e)ret);
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get languages : %s", __vc_get_error_code(ret));
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return VC_ERROR_NONE;
}


int vc_get_current_language(char** language)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Get Current Language");

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret = -1;
	ret = vc_config_mgr_get_default_language(language);
	if (0 != ret) {
		ret = vc_config_convert_error_code((vc_config_error_e)ret);
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get current languages : %s", __vc_get_error_code(ret));
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return ret;
}

int vc_get_state(vc_state_e* state)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Get State");

	if (NULL == state) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e temp;
	if (0 != vc_client_get_client_state(g_vc, &temp)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	*state = temp;

	switch (*state) {
	case VC_STATE_NONE:		SLOG(LOG_DEBUG, TAG_VCC, "Current state is 'None'");		break;
	case VC_STATE_INITIALIZED:	SLOG(LOG_DEBUG, TAG_VCC, "Current state is 'Created'");		break;
	case VC_STATE_READY:		SLOG(LOG_DEBUG, TAG_VCC, "Current state is 'Ready'");		break;
	default:			SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid state");		break;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return VC_ERROR_NONE;
}

int vc_get_service_state(vc_service_state_e* state)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Get Service State");

	if (NULL == state) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e temp;
	if (0 != vc_client_get_client_state(g_vc, &temp)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	if (VC_STATE_READY != temp) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* get service state */
	vc_service_state_e service_state;
	if (0 != vc_client_get_service_state(g_vc, &service_state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get service state");
		return VC_ERROR_OPERATION_FAILED;
	}

	*state = service_state;

	switch (*state) {
	case VC_SERVICE_STATE_NONE:		SLOG(LOG_DEBUG, TAG_VCC, "Current service state is 'None'");		break;
	case VC_SERVICE_STATE_READY:		SLOG(LOG_DEBUG, TAG_VCC, "Current service state is 'Ready'");		break;
	case VC_SERVICE_STATE_RECORDING:	SLOG(LOG_DEBUG, TAG_VCC, "Current service state is 'Recording'");	break;
	case VC_SERVICE_STATE_PROCESSING:	SLOG(LOG_DEBUG, TAG_VCC, "Current service state is 'Processing'");	break;
	default:				SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid state");			break;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return VC_ERROR_NONE;
}

#if 0
int vc_set_window_id(int wid)
{
	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Set Window id");

	if (0 >= wid) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Input parameter is invalid");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	if (0 != vc_client_set_xid(g_vc, wid)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check if current xid is top window */
	int ret = -1;
	if ((Ecore_X_Window)wid == ecore_x_window_focus_get()) {
		/* Set current pid */
		ret = vc_config_mgr_set_foreground(getpid(), true);
		if (0 != ret) {
			ret = vc_config_convert_error_code((vc_config_error_e)ret);
			SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to set focus in : %s", __vc_get_error_code(ret));
		}
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return ret;
}

int vc_get_window_id(int* wid)
{
	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Get Window id");

	if (NULL == wid) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	if (0 != vc_client_get_xid(g_vc, wid)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return VC_ERROR_NONE;
}
#endif

/**
* @brief Checks whether the command format is supported.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
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
#if 0
int vc_is_command_format_supported(vc_cmd_format_e format, bool* support)
{
	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Is command format supported");

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check support */
	bool non_fixed_support = false;
	if (0 != vc_config_mgr_get_nonfixed_support(&non_fixed_support)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get nonfixed support info");
	}

	switch (format) {
	case VC_CMD_FORMAT_FIXED:			*support = true;		break;
	case VC_CMD_FORMAT_FIXED_AND_EXTRA:	*support = non_fixed_support;	break;
	case VC_CMD_FORMAT_EXTRA_AND_FIXED:	*support = non_fixed_support;	break;
	default:					*support = false;		break;
	}

	SLOG(LOG_ERROR, TAG_VCC, "[DEBUG] Format(%d) support(%s)", format, *support ? "true" : "false");

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return VC_ERROR_NONE;
}
#endif

int vc_set_command_list(vc_cmd_list_h vc_cmd_list, int type)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Set Command list");

	if (NULL == vc_cmd_list) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check type */
	if ((VC_COMMAND_TYPE_FOREGROUND != type) && (VC_COMMAND_TYPE_BACKGROUND != type)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid command type: input type is %d", type);
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_list_s* list = NULL;
	list = (vc_cmd_list_s*)vc_cmd_list;

	int ret = 0;
	if (0 != vc_cmd_parser_save_file(getpid(), (vc_cmd_type_e)type, list->list)) {
		ret = VC_ERROR_INVALID_PARAMETER;
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to save command list : %s", __vc_get_error_code(ret));
	} else {
		int count = 0;
		do {
			ret = vc_dbus_request_set_command(g_vc->handle, (vc_cmd_type_e)type);
			if (0 != ret) {
				if (VC_ERROR_TIMED_OUT != ret) {
					SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to request set command to daemon : %s", __vc_get_error_code(ret));
					break;
				} else {
					SLOG(LOG_WARN, TAG_VCC, "[WARNING] retry request set command : %s", __vc_get_error_code(ret));
					usleep(10000);
					count++;
					if (VC_RETRY_COUNT == count) {
						SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to request");
						break;
					}
				}
			}
		} while (0 != ret);
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return ret;
}

int vc_unset_command_list(int type)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Unset Command list");

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int count = 0;
	int ret = -1;
	while (0 != ret) {
		ret = vc_dbus_request_unset_command(g_vc->handle, (vc_cmd_type_e)type);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to request unset command to daemon : %s", __vc_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCC, "[WARNING] retry request unset command : %s", __vc_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to request");
					break;
				}
			}
		}
	}

	ret = vc_cmd_parser_delete_file(getpid(), (vc_cmd_type_e)type);
	if (0 != ret) {
		ret = vc_config_convert_error_code((vc_config_error_e)ret);
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] cmd_type(%d), Fail to delete command list : %s", type, __vc_get_error_code(ret));
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return ret;
}

#if 0
int vc_get_exclusive_command_option(bool* value)
{
	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Get exclusive command");

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret = vc_client_get_exclusive_cmd(g_vc, value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to set exclusive option : %d", ret);
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return ret;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return ret;
}

int vc_set_exclusive_command_option(bool value)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Set exclusive command");

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret = vc_client_set_exclusive_cmd(g_vc, value);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to set exclusive option : %d", ret);
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return ret;
	}

	/* Check if current xid is top window */
	int count = 0;
	do {
		ret = vc_dbus_request_set_exclusive_command(g_vc->handle, value);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to request set exclusive command to daemon : %s", __vc_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCC, "[WARNING] retry request set exclusive command : %s", __vc_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to request");
					break;
				}
			}
		}
	} while (0 != ret);

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return ret;
}
#endif

#if 0
int vc_request_start(bool stop_by_silence)
{
	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Request start");

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: client state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_client_get_service_state(g_vc, &service_state);
	if (service_state != VC_SERVICE_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: service state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret;
	int count = 0;

	/* Request */
	ret = -1;
	count = 0;
	while (0 != ret) {
		ret = vc_dbus_request_start(g_vc->handle, stop_by_silence);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to start request start : %s", __vc_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCC, "[WARNING] retry start request start : %s", __vc_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to request");
					break;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCC, "[SUCCESS] start interrupt");
		}
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return ret;
}

int vc_request_stop(void)
{
	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Request stop");

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: client state is not 'Ready'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_client_get_service_state(g_vc, &service_state);
	if (service_state != VC_SERVICE_STATE_RECORDING) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: service state is not 'RECORDING'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	/* do request */
	while (0 != ret) {
		ret = vc_dbus_request_stop(g_vc->handle);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_DEBUG, TAG_VCC, "[ERROR] Fail to stop request : %s", __vc_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCC, "[WARNING] retry stop request : %s", __vc_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to request");
					break;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCC, "[SUCCESS] Stop interrupt");
		}
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return ret;
}

int vc_request_cancel(void)
{
	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Request cancel Interrupt");

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: Current state is not 'Ready'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_client_get_service_state(g_vc, &service_state);
	if (service_state != VC_SERVICE_STATE_RECORDING && service_state != VC_SERVICE_STATE_PROCESSING) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: service state is not 'RECORDING' or 'PROCESSING'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = vc_dbus_request_cancel(g_vc->handle);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_DEBUG, TAG_VCC, "[ERROR] Fail to cancel request : %s", __vc_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCC, "[WARNING] retry cancel request : %s", __vc_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to request");
					break;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCC, "[SUCCESS] Cancel interrupt");
		}
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return ret;
}
#endif

Eina_Bool __vc_notify_error(void *data)
{
	vc_h vc = (vc_h)data;

	vc_error_cb callback = NULL;
	void* user_data;
	int reason;

	vc_client_get_error_cb(vc, &callback, &user_data);
	vc_client_get_error(vc, &reason);

	if (NULL != callback) {
		vc_client_use_callback(vc);
		callback(reason, user_data);
		vc_client_not_use_callback(vc);
		SLOG(LOG_DEBUG, TAG_VCC, "Error callback is called");
	} else {
		SLOG(LOG_WARN, TAG_VCC, "[WARNING] Error callback is null");
	}

	return EINA_FALSE;
}

int __vc_cb_error(int pid, int reason)
{
	if (0 != vc_client_get_handle(pid, &g_vc)) {
		SLOG(LOG_ERROR, TAG_VCC, "Handle is not valid : pid(%d)", pid);
		return -1;
	}

	vc_client_set_error(g_vc, reason);
	ecore_timer_add(0, __vc_notify_error, g_vc);

	return 0;
}

Eina_Bool __vc_notify_state_changed(void *data)
{
	vc_h vc = (vc_h)data;

	vc_state_changed_cb changed_callback = NULL;
	void* user_data;

	vc_client_get_state_changed_cb(vc, &changed_callback, &user_data);

	vc_state_e current_state;
	vc_state_e before_state;

	vc_client_get_before_state(vc, &current_state, &before_state);

	if (NULL != changed_callback) {
		vc_client_use_callback(vc);
		changed_callback(before_state, current_state, user_data);
		vc_client_not_use_callback(vc);
		SLOG(LOG_DEBUG, TAG_VCC, "State changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_VCC, "[WARNING] State changed callback is null");
	}

	return EINA_FALSE;
}

static Eina_Bool __vc_notify_result(void *data)
{
	char* temp_text;
	int event;
	vc_cmd_list_h vc_cmd_list = NULL;

	vc_result_cb callback = NULL;
	void* user_data = NULL;

	vc_client_get_result_cb(g_vc, &callback, &user_data);

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Client result callback is NULL");
		return EINA_FALSE;
	}

	if (0 != vc_cmd_list_create(&vc_cmd_list)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to create command list");
		return EINA_FALSE;
	}

	vc_info_parser_get_result(&temp_text, &event, NULL, getpid(),  vc_cmd_list, false);

	SLOG(LOG_DEBUG, TAG_VCC, "Result info : result text(%s) event(%d)", temp_text, event);

	vc_cmd_print_list(vc_cmd_list);

	vc_client_use_callback(g_vc);
	callback(event, vc_cmd_list, temp_text, user_data);
	vc_client_not_use_callback(g_vc);

	SLOG(LOG_DEBUG, TAG_VCC, "Client result callback called");

	vc_cmd_list_destroy(vc_cmd_list, true);

	/* Release result */
	if (NULL != temp_text)	free(temp_text);

	return EINA_FALSE;
}

void __vc_cb_result(void)
{
	ecore_timer_add(0, __vc_notify_result, NULL);

	return;
}

int vc_set_result_cb(vc_result_cb callback, void* user_data)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	if (NULL == callback)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Set result callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Set result callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_client_set_result_cb(g_vc, callback, user_data);

	return 0;
}

int vc_unset_result_cb(void)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Unset result callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Unset result callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_client_set_result_cb(g_vc, NULL, NULL);

	return 0;
}

int __vc_cb_service_state(int state)
{
	vc_service_state_e current_state = (vc_service_state_e)state;
	vc_service_state_e before_state;
	vc_client_get_service_state(g_vc, &before_state);

	if (current_state == before_state) {
		return 0;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "Service State changed : Before(%d) Current(%d)",
		before_state, current_state);

	/* Save service state */
	vc_client_set_service_state(g_vc, current_state);

	vc_service_state_changed_cb callback = NULL;
	void* service_user_data;
	vc_client_get_service_state_changed_cb(g_vc, &callback, &service_user_data);

	if (NULL != callback) {
		vc_client_use_callback(g_vc);
		callback((vc_service_state_e)before_state, (vc_service_state_e)current_state, service_user_data);
		vc_client_not_use_callback(g_vc);
		SLOG(LOG_DEBUG, TAG_VCC, "Service state changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_VCC, "[WARNING] Service state changed callback is null");
	}

	return 0;
}

int vc_set_service_state_changed_cb(vc_service_state_changed_cb callback, void* user_data)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	if (NULL == callback)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Set result callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Set result callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_client_set_service_state_changed_cb(g_vc, callback, user_data);

	return 0;
}

int vc_unset_service_state_changed_cb(void)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Unset result callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Unset result callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_client_set_service_state_changed_cb(g_vc, NULL, NULL);

	return 0;
}

int vc_set_state_changed_cb(vc_state_changed_cb callback, void* user_data)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	if (callback == NULL)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Set state changed callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Set state changed callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_client_set_state_changed_cb(g_vc, callback, user_data);

	return 0;
}

int vc_unset_state_changed_cb(void)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Unset state changed callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Unset state changed callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_client_set_state_changed_cb(g_vc, NULL, NULL);

	return 0;
}

int vc_set_current_language_changed_cb(vc_current_language_changed_cb callback, void* user_data)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	if (NULL == callback)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Set current language changed : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Set current language changed : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_client_set_current_lang_changed_cb(g_vc, callback, user_data);

	return 0;
}

int vc_unset_current_language_changed_cb(void)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Unset current language changed : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Unset current language changed : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_client_set_current_lang_changed_cb(g_vc, NULL, NULL);

	return 0;
}

int vc_set_error_cb(vc_error_cb callback, void* user_data)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	if (NULL == callback)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Set error callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Set error callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_client_set_error_cb(g_vc, callback,  user_data);

	return 0;
}

int vc_unset_error_cb(void)
{
	if (0 != __vc_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Unset error callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Unset error callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_client_set_error_cb(g_vc, NULL, NULL);

	return 0;
}

/* Authority */
int vc_auth_enable(void)
{
	/* check state */
	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	if (VC_STATE_READY != state) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Current state is not 'READY'");
		return VC_ERROR_INVALID_STATE;
	}

	/* check already authority */
	vc_auth_state_e auth_state = VC_AUTH_STATE_NONE;
	if (0 != vc_client_get_auth_state(g_vc, &auth_state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	if (VC_AUTH_STATE_NONE != auth_state) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Already authority enabled");
		return VC_ERROR_INVALID_STATE;
	}

	/* request authority */
	int mgr_pid = -1;
	if (0 != vc_client_get_mgr_pid(g_vc, &mgr_pid)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get mgr info");
		return VC_ERROR_OPERATION_FAILED;
	}

	if (0 != vc_dbus_request_auth_enable(g_vc->handle, mgr_pid)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to authority enabled");
		return VC_ERROR_OPERATION_FAILED;
	}

	/* set authority into handle */
	bool is_foreground = false;
	if (0 != vc_client_get_is_foreground(g_vc, &is_foreground)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get is_foreground");
		return VC_ERROR_OPERATION_FAILED;
	}

	if (is_foreground) {
		auth_state = VC_AUTH_STATE_VALID;
	} else {
		auth_state = VC_AUTH_STATE_INVALID;
	}

	if (0 != vc_client_set_auth_state(g_vc, auth_state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to set auth state");
		return VC_ERROR_OPERATION_FAILED;
	}

	ecore_timer_add(0, __notify_auth_changed_cb, NULL);

	SLOG(LOG_DEBUG, TAG_VCC, "[SUCCESS] Auth enable");

	return VC_ERROR_NONE;
}

int vc_auth_disable(void)
{
	/* check state */
	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	if (VC_STATE_READY != state) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Current state is not 'READY'");
		return VC_ERROR_INVALID_STATE;
	}

	/* check autority */
	vc_auth_state_e auth_state = VC_AUTH_STATE_NONE;
	if (0 != vc_client_get_auth_state(g_vc, &auth_state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	if (VC_AUTH_STATE_NONE == auth_state) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] No authority");
		return VC_ERROR_INVALID_STATE;
	}

	if (0 != vc_auth_unset_state_changed_cb()) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to unset state changed cb");
	}

	/* request return authority by dbus */
	int mgr_pid = -1;
	if (0 != vc_client_get_mgr_pid(g_vc, &mgr_pid)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get mgr info");
		return VC_ERROR_OPERATION_FAILED;
	}

	if (0 != vc_dbus_request_auth_disable(g_vc->handle, mgr_pid)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to authority disble");
		return VC_ERROR_OPERATION_FAILED;
	}

	/* unset authority from handle */
	if (0 != vc_client_set_auth_state(g_vc, VC_AUTH_STATE_NONE)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to set auth state");
		return VC_ERROR_OPERATION_FAILED;
	}

	ecore_timer_add(0, __notify_auth_changed_cb, NULL);

	SLOG(LOG_DEBUG, TAG_VCC, "[SUCCESS] Auth disable");

	return VC_ERROR_NONE;
}

int vc_auth_get_state(vc_auth_state_e* state)
{
	/* check state */
	vc_state_e vc_state;
	if (0 != vc_client_get_client_state(g_vc, &vc_state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	if (VC_STATE_READY != vc_state) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Current state is not 'READY'");
		return VC_ERROR_INVALID_STATE;
	}

	/* get autority */
	vc_auth_state_e temp = VC_AUTH_STATE_NONE;
	if (0 != vc_client_get_auth_state(g_vc, &temp)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	*state = temp;

	SLOG(LOG_DEBUG, TAG_VCC, "[SUCCESS] Current auth state is %d", *state);

	return VC_ERROR_NONE;
}

int vc_auth_set_state_changed_cb(vc_auth_state_changed_cb callback, void* user_data)
{
	/* check parameter */
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] NULL Parameter");
		return VC_ERROR_INVALID_PARAMETER;
	}

	/* check auth */
	vc_auth_state_e auth_state;
	if (0 != vc_client_get_auth_state(g_vc, &auth_state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	if (VC_AUTH_STATE_NONE == auth_state) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Auth is not enabled");
		return VC_ERROR_INVALID_STATE;
	}

	/* set cb into handle */
	if (0 != vc_client_set_auth_state_changed_cb(g_vc, callback, user_data)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to set auth state changed cb");
		return VC_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG,  TAG_VCC, "[SUCCESS] Set auth state changed cb");

	return VC_ERROR_NONE;
}

int vc_auth_unset_state_changed_cb(void)
{
	/* check auth */
	vc_auth_state_e auth_state;
	if (0 != vc_client_get_auth_state(g_vc, &auth_state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	if (VC_AUTH_STATE_NONE == auth_state) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Auto is not enabled");
		return VC_ERROR_INVALID_STATE;
	}

	/* unset cb from handle */
	if (0 != vc_client_unset_auth_state_changed_cb(g_vc)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to unset auth state changed cb");
		return VC_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_VCC, "[SUCCESS] Unset auth state changed cb");

	return VC_ERROR_NONE;
}

int vc_auth_start(void)
{
	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Request start");

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: client state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_client_get_service_state(g_vc, &service_state);
	if (service_state != VC_SERVICE_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: service state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check authority */
	vc_auth_state_e auth_state = VC_AUTH_STATE_NONE;
	if (0 != vc_client_get_auth_state(g_vc, &auth_state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get auth state");
		return VC_ERROR_OPERATION_FAILED;
	}

	if (VC_AUTH_STATE_VALID != auth_state) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Not auth valid");
		return VC_ERROR_OPERATION_FAILED;
	}

	/* get mgr_pid */
	int mgr_pid = -1;
	if (0 != vc_client_get_mgr_pid(g_vc, &mgr_pid)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get mgr info");
		return VC_ERROR_OPERATION_FAILED;
	}

	int ret;
	int count = 0;
	/* Request */
	ret = -1;
	count = 0;
	while (0 != ret) {
		ret = vc_dbus_request_auth_start(g_vc->handle, mgr_pid);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to request auth start : %s", __vc_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCC, "[WARNING] retry request auth start : %s", __vc_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to request");
					break;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCC, "[SUCCESS] request auth start");
		}
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return ret;
}

int vc_auth_stop(void)
{
	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Request stop");

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: client state is not 'Ready'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_client_get_service_state(g_vc, &service_state);
	if (service_state != VC_SERVICE_STATE_RECORDING) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: service state is not 'RECORDING'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check authority */
	vc_auth_state_e auth_state = VC_AUTH_STATE_NONE;
	if (0 != vc_client_get_auth_state(g_vc, &auth_state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get auth state");
		return VC_ERROR_OPERATION_FAILED;
	}

	if (VC_AUTH_STATE_VALID != auth_state) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Not auth valid");
		return VC_ERROR_OPERATION_FAILED;
	}

	/* get mgr_pid */
	int mgr_pid = -1;
	if (0 != vc_client_get_mgr_pid(g_vc, &mgr_pid)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get mgr info");
		return VC_ERROR_OPERATION_FAILED;
	}

	int ret = -1;
	int count = 0;
	/* do request */
	while (0 != ret) {
		ret = vc_dbus_request_auth_stop(g_vc->handle, mgr_pid);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_DEBUG, TAG_VCC, "[ERROR] Fail to request auth stop: %s", __vc_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCC, "[WARNING] retry request auth stop: %s", __vc_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to request");
					break;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCC, "[SUCCESS] request auth stop");
		}
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return ret;
}

int vc_auth_cancel(void)
{
	SLOG(LOG_DEBUG, TAG_VCC, "===== [Client] Request cancel");

	vc_state_e state;
	if (0 != vc_client_get_client_state(g_vc, &state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: Current state is not 'Ready'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_client_get_service_state(g_vc, &service_state);
	if (service_state != VC_SERVICE_STATE_RECORDING && service_state != VC_SERVICE_STATE_PROCESSING) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Invalid State: service state is not 'RECORDING' or 'PROCESSING'");
		SLOG(LOG_DEBUG, TAG_VCC, "=====");
		SLOG(LOG_DEBUG, TAG_VCC, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check authority */
	vc_auth_state_e auth_state = VC_AUTH_STATE_NONE;
	if (0 != vc_client_get_auth_state(g_vc, &auth_state)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get auth state");
		return VC_ERROR_OPERATION_FAILED;
	}

	if (VC_AUTH_STATE_VALID != auth_state) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Not auth valid");
		return VC_ERROR_OPERATION_FAILED;
	}

	/* get mgr_pid */
	int mgr_pid = -1;
	if (0 != vc_client_get_mgr_pid(g_vc, &mgr_pid)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to get mgr info");
		return VC_ERROR_OPERATION_FAILED;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = vc_dbus_request_auth_cancel(g_vc->handle, mgr_pid);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_DEBUG, TAG_VCC, "[ERROR] Fail to request auth cancel: %s", __vc_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCC, "[WARNING] retry request auth cancel: %s", __vc_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to request");
					break;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCC, "[SUCCESS] request auth cancel");
		}
	}

	SLOG(LOG_DEBUG, TAG_VCC, "=====");
	SLOG(LOG_DEBUG, TAG_VCC, " ");

	return ret;
}
