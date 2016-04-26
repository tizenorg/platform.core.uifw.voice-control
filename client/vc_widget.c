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


#include "vc_command.h"
#include "vc_config_mgr.h"
#include "vc_info_parser.h"
#include "vc_main.h"
#include "vc_widget_client.h"
#include "vc_widget_dbus.h"
#include "voice_control_command.h"
#include "voice_control_command_expand.h"
#include "voice_control_widget.h"


#define VC_WIDGET_CONFIG_HANDLE	200000

static Ecore_Timer* g_w_connect_timer = NULL;

static Ecore_Timer* g_w_start_timer = NULL;
static Ecore_Timer* g_w_tooltip_timer = NULL;

static vc_h g_vc_w = NULL;

static Eina_Bool __vc_widget_notify_state_changed(void *data);
static Eina_Bool __vc_widget_notify_error(void *data);

const char* vc_tag()
{
	return TAG_VCW;
}

static const char* __vc_widget_get_error_code(vc_error_e err)
{
	switch (err) {
	case VC_ERROR_NONE:		return "VC_ERROR_NONE";			break;
	case VC_ERROR_OUT_OF_MEMORY:	return "VC_ERROR_OUT_OF_MEMORY";	break;
	case VC_ERROR_IO_ERROR:		return "VC_ERROR_IO_ERROR";		break;
	case VC_ERROR_INVALID_PARAMETER:return "VC_ERROR_INVALID_PARAMETER";	break;
	case VC_ERROR_TIMED_OUT:	return "VC_ERROR_TIMED_OUT";		break;
	case VC_ERROR_INVALID_STATE:	return "VC_ERROR_INVALID_STATE";	break;
	case VC_ERROR_ENGINE_NOT_FOUND:	return "VC_ERROR_ENGINE_NOT_FOUND";	break;
	case VC_ERROR_OPERATION_FAILED:	return "VC_ERROR_OPERATION_FAILED";	break;
	default:			return "Invalid error code";		break;
	}
	return NULL;
}

static int __vc_widget_convert_config_error_code(vc_config_error_e code)
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

static void __vc_widget_lang_changed_cb(const char* before_lang, const char* current_lang)
{
	SLOG(LOG_DEBUG, TAG_VCW, "Lang changed : Before lang(%s) Current lang(%s)",
		 before_lang, current_lang);

	vc_current_language_changed_cb callback;
	void* lang_user_data;
	vc_widget_client_get_current_lang_changed_cb(g_vc_w, &callback, &lang_user_data);

	if (NULL != callback) {
		vc_widget_client_use_callback(g_vc_w);
		callback(before_lang, current_lang, lang_user_data);
		vc_widget_client_not_use_callback(g_vc_w);
		SLOG(LOG_DEBUG, TAG_VCW, "Language changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_VCW, "[WARNING] Language changed callback is null");
	}

	return;
}

int vc_widget_initialize()
{
	SLOG(LOG_DEBUG, TAG_VCW, "===== [Widget] Initialize");

	/* check handle */
	if (true == vc_widget_client_is_valid(g_vc_w)) {
		SLOG(LOG_ERROR, TAG_VCW, "[WARNING] Already initialized");
		return VC_ERROR_NONE;
	}

	if (0 == vc_widget_client_get_count()) {
		if (0 != vc_widget_dbus_open_connection()) {
			SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to open connection");
			return VC_ERROR_OPERATION_FAILED;
		}
	} else {
		SLOG(LOG_WARN, TAG_VCW, "[WARN] Already initialized");
		return VC_ERROR_NONE;
	}

	if (0 != vc_widget_client_create(&g_vc_w)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to create client!!!!!");
		return VC_ERROR_OUT_OF_MEMORY;
	}

	int ret = vc_config_mgr_initialize(g_vc_w->handle + VC_WIDGET_CONFIG_HANDLE);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to init config manager : %s",
			 __vc_widget_get_error_code(__vc_widget_convert_config_error_code(ret)));
		vc_widget_client_destroy(g_vc_w);
		return __vc_widget_convert_config_error_code(ret);
	}

	ret = vc_config_mgr_set_lang_cb(g_vc_w->handle + VC_WIDGET_CONFIG_HANDLE, __vc_widget_lang_changed_cb);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to set config changed : %d", ret);
		vc_config_mgr_finalize(g_vc_w->handle + VC_WIDGET_CONFIG_HANDLE);
		vc_widget_client_destroy(g_vc_w);
		return __vc_widget_convert_config_error_code(ret);
	}

	SLOG(LOG_DEBUG, TAG_VCW, "[Success] pid(%d)", g_vc_w->handle);

	SLOG(LOG_DEBUG, TAG_VCW, "=====");
	SLOG(LOG_DEBUG, TAG_VCW, " ");

	return VC_ERROR_NONE;
}

static void __vc_widget_internal_unprepare()
{
	int ret = vc_widget_dbus_request_finalize(g_vc_w->handle);
	if (0 != ret) {
		SLOG(LOG_WARN, TAG_VCW, "[ERROR] Fail to request finalize : %s", __vc_widget_get_error_code(ret));
	}

	vc_cmd_parser_delete_file(getpid(), VC_COMMAND_TYPE_WIDGET);

	return;
}

int vc_widget_deinitialize()
{
	SLOG(LOG_DEBUG, TAG_VCW, "===== [Widget] Deinitialize");

	if (false == vc_widget_client_is_valid(g_vc_w)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] NOT initialized");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	vc_state_e state;
	vc_widget_client_get_state(g_vc_w, &state);

	/* check state */
	switch (state) {
	case VC_STATE_READY:
		__vc_widget_internal_unprepare();
		/* no break. need to next step*/
	case VC_STATE_INITIALIZED:
		if (NULL != g_w_connect_timer) {
			SLOG(LOG_DEBUG, TAG_VCW, "Connect Timer is deleted");
			ecore_timer_del(g_w_connect_timer);
		}

		vc_config_mgr_unset_lang_cb(g_vc_w->handle + VC_WIDGET_CONFIG_HANDLE);
		vc_config_mgr_finalize(g_vc_w->handle + VC_WIDGET_CONFIG_HANDLE);

		/* Free resources */
		vc_widget_client_destroy(g_vc_w);
		g_vc_w = NULL;
		break;
	case VC_STATE_NONE:
		break;
	default:
		break;
	}

	SLOG(LOG_DEBUG, TAG_VCW, "Success: destroy");

	if (0 != vc_widget_dbus_close_connection()) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to close connection");
	}

	SLOG(LOG_DEBUG, TAG_VCW, "=====");
	SLOG(LOG_DEBUG, TAG_VCW, " ");

	return VC_ERROR_NONE;
}

static Eina_Bool __vc_widget_connect_daemon(void *data)
{
	/* Send hello */
	if (0 != vc_widget_dbus_request_hello()) {
		return EINA_TRUE;
	}

	SLOG(LOG_DEBUG, TAG_VCW, "===== [Widget] Connect daemon");

	/* request initialization */
	int ret = -1;
	int service_state = 0;
	ret = vc_widget_dbus_request_initialize(g_vc_w->handle, &service_state);

	if (VC_ERROR_ENGINE_NOT_FOUND == ret) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to initialize : %s", __vc_widget_get_error_code(ret));

		vc_widget_client_set_error(g_vc_w, VC_ERROR_ENGINE_NOT_FOUND);
		ecore_timer_add(0, __vc_widget_notify_error, g_vc_w);

		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, "  ");
		return EINA_FALSE;

	} else if (VC_ERROR_NONE != ret) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to initialize : %s", __vc_widget_get_error_code(ret));

		vc_widget_client_set_error(g_vc_w, VC_ERROR_TIMED_OUT);
		ecore_timer_add(0, __vc_widget_notify_error, g_vc_w);

		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, "  ");
		return EINA_FALSE;
	}

	vc_widget_client_set_service_state(g_vc_w, (vc_service_state_e)service_state);

	vc_widget_client_set_state(g_vc_w, VC_STATE_READY);
	ecore_timer_add(0, __vc_widget_notify_state_changed, g_vc_w);

	g_w_connect_timer = NULL;

	SLOG(LOG_DEBUG, TAG_VCW, "=====");
	SLOG(LOG_DEBUG, TAG_VCW, "  ");

	return EINA_FALSE;
}

int vc_widget_prepare()
{
	SLOG(LOG_DEBUG, TAG_VCW, "===== [Widget] Prepare");

	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'Initialized'");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	g_w_connect_timer = ecore_timer_add(0, __vc_widget_connect_daemon, NULL);

	SLOG(LOG_DEBUG, TAG_VCW, "=====");
	SLOG(LOG_DEBUG, TAG_VCW, " ");

	return VC_ERROR_NONE;
}

int vc_widget_unprepare()
{
	SLOG(LOG_DEBUG, TAG_VCW, "===== [Widget] Unprepare");

	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	__vc_widget_internal_unprepare();

	vc_widget_client_set_state(g_vc_w, VC_STATE_INITIALIZED);
	ecore_timer_add(0, __vc_widget_notify_state_changed, g_vc_w);

	SLOG(LOG_DEBUG, TAG_VCW, "=====");
	SLOG(LOG_DEBUG, TAG_VCW, " ");

	return VC_ERROR_NONE;
}

int vc_widget_foreach_supported_languages(vc_supported_language_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_VCW, "===== [Widget] Foreach Supported Language");

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret = -1;
	ret = vc_config_mgr_get_language_list(callback, user_data);
	if (0 != ret) {
		ret = vc_config_convert_error_code((vc_config_error_e)ret);
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to get languages : %s", __vc_widget_get_error_code(ret));
	}

	SLOG(LOG_DEBUG, TAG_VCW, "=====");
	SLOG(LOG_DEBUG, TAG_VCW, " ");

	return VC_ERROR_NONE;
}

int vc_widget_get_current_language(char** language)
{
	SLOG(LOG_DEBUG, TAG_VCW, "===== [Widget] Get Current Language");

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret = -1;
	ret = vc_config_mgr_get_default_language(language);
	if (0 != ret) {
		ret = vc_config_convert_error_code((vc_config_error_e)ret);
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to get current languages : %s", __vc_widget_get_error_code(ret));
	}

	SLOG(LOG_DEBUG, TAG_VCW, "=====");
	SLOG(LOG_DEBUG, TAG_VCW, " ");

	return ret;
}

int vc_widget_get_state(vc_state_e* state)
{
	SLOG(LOG_DEBUG, TAG_VCW, "===== [Widget] Get State");

	if (NULL == state) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e temp;
	if (0 != vc_widget_client_get_state(g_vc_w, &temp)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	*state = temp;

	switch (*state) {
	case VC_STATE_NONE:		SLOG(LOG_DEBUG, TAG_VCW, "Current state is 'None'");		break;
	case VC_STATE_INITIALIZED:	SLOG(LOG_DEBUG, TAG_VCW, "Current state is 'Created'");		break;
	case VC_STATE_READY:		SLOG(LOG_DEBUG, TAG_VCW, "Current state is 'Ready'");		break;
	default:			SLOG(LOG_ERROR, TAG_VCW, "Invalid state");			break;
	}

	SLOG(LOG_DEBUG, TAG_VCW, "=====");
	SLOG(LOG_DEBUG, TAG_VCW, " ");

	return VC_ERROR_NONE;
}

int vc_widget_get_service_state(vc_service_state_e* state)
{
	SLOG(LOG_DEBUG, TAG_VCW, "===== [Widget] Get Service State");

	if (NULL == state) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e temp;
	if (0 != vc_widget_client_get_state(g_vc_w, &temp)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	if (temp != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* get service state */
	vc_service_state_e service_state;
	if (0 != vc_widget_client_get_service_state(g_vc_w, &service_state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to get service state");
		return VC_ERROR_OPERATION_FAILED;
	}

	*state = service_state;

	switch (*state) {
	case VC_SERVICE_STATE_NONE:		SLOG(LOG_DEBUG, TAG_VCW, "Current service state is 'None'");		break;
	case VC_SERVICE_STATE_READY:		SLOG(LOG_DEBUG, TAG_VCW, "Current service state is 'Ready'");		break;
	case VC_SERVICE_STATE_RECORDING:	SLOG(LOG_DEBUG, TAG_VCW, "Current service state is 'Recording'");	break;
	case VC_SERVICE_STATE_PROCESSING:	SLOG(LOG_DEBUG, TAG_VCW, "Current service state is 'Processing'");	break;
	default:				SLOG(LOG_ERROR, TAG_VCW, "Invalid service state");			break;
	}

	SLOG(LOG_DEBUG, TAG_VCW, "=====");
	SLOG(LOG_DEBUG, TAG_VCW, " ");

	return VC_ERROR_NONE;
}

int vc_widget_set_foreground(bool value)
{
	SLOG(LOG_DEBUG, TAG_VCW, "===== [Widget] Set foreground state");

	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	SLOG(LOG_DEBUG, TAG_VCW, "Set foreground : pid(%d) value(%s)", getpid(), value ? "true" : "false");
	int ret = vc_widget_dbus_set_foreground(getpid(), value);
	if (0 != ret) {
		ret = vc_config_convert_error_code((vc_config_error_e)ret);
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to set foreground : %s", __vc_widget_get_error_code(ret));
	}

	SLOG(LOG_DEBUG, TAG_VCW, "=====");
	SLOG(LOG_DEBUG, TAG_VCW, " ");

	return VC_ERROR_NONE;
}
#if 0
int vc_widget_is_format_supported(vc_cmd_format_e format, bool* support)
{
	SLOG(LOG_DEBUG, TAG_VCW, "===== [Widget] Is command type supported");

	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check support */
	bool non_fixed_support = false;
	if (0 != vc_config_mgr_get_nonfixed_support(&non_fixed_support)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to get nonfixed support info");
	}

	switch (format) {
	case VC_CMD_FORMAT_FIXED:			*support = true;		break;
	case VC_CMD_FORMAT_FIXED_AND_EXTRA:	*support = non_fixed_support;	break;
	case VC_CMD_FORMAT_EXTRA_AND_FIXED:	*support = non_fixed_support;	break;
	default:					*support = false;		break;
	}

	SLOG(LOG_ERROR, TAG_VCW, "[DEBUG] Format(%d) support(%s)", format, *support ? "true" : "false");

	SLOG(LOG_DEBUG, TAG_VCW, "=====");
	SLOG(LOG_DEBUG, TAG_VCW, " ");

	return VC_ERROR_NONE;
}
#endif
/**
* @brief Starts recognition.
*
* @param[in] stop_by_silence Silence detection option
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter.
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failure
*
* @pre The state should be #VC_STATE_READY.
* @post It will invoke vc_state_changed_cb(), if you register a callback with vc_state_changed_cb(). \n
* If this function succeeds, the state will be #VC_STATE_RECORDING.
*
* @see vc_widget_stop()
* @see vc_widget_cancel()
* @see vc_state_changed_cb()
*/
#if 0
int vc_widget_start(bool stop_by_silence, vc_cmd_group_h vc_group)
{
	SLOG(LOG_DEBUG, TAG_VCW, "===== [Widget] Start");

	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_widget_client_get_service_state(g_vc_w, &service_state);
	if (service_state != VC_SERVICE_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: service state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret = vc_widget_client_set_command_group(g_vc_w, vc_group);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to set command to client : %d", ret);
		return ret;
	}

	GSList* list = NULL;
	if (0 > vc_cmd_group_get_cmd_list(vc_group, &list)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to get command list : %d", ret);
		return ret;
	}

	ret = vc_cmd_parser_save_file(getpid(), VC_COMMAND_GROUP_TYPE_WIDGET, list);
	if (0 != ret) {
		ret = vc_config_convert_error_code((vc_config_error_e)ret);
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to save command group : %s", __vc_widget_get_error_code(ret));
	} else {
		int count = 0;
		do {
			ret = vc_widget_dbus_request_start(g_vc_w->handle, stop_by_silence);
			if (0 != ret) {
				if (VC_ERROR_TIMED_OUT != ret) {
					SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to request start : %s", __vc_widget_get_error_code(ret));
					break;
				} else {
					SLOG(LOG_WARN, TAG_VCW, "[WARNING] retry request start : %s", __vc_widget_get_error_code(ret));
					usleep(10000);
					count++;
					if (VC_RETRY_COUNT == count) {
						SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to request");
						break;
					}
				}
			}
		} while (0 != ret);
	}

	SLOG(LOG_DEBUG, TAG_VCW, "=====");
	SLOG(LOG_DEBUG, TAG_VCW, " ");

	return VC_ERROR_NONE;
}
#endif

/**
* @brief Stop interrupt.
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_STATE Invalid state
* @retval #VC_ERROR_OPERATION_FAILED Operation failure
*
* @pre The state should be #VC_STATE_RECORDING.
* @post It will invoke vc_state_changed_cb(), if you register a callback with vc_state_changed_cb(). \n
* If this function succeeds, the state will be #VC_STATE_READY and vc_widget_result_cb() is called.
*
* @see vc_widget_start()
* @see vc_widget_cancel()
* @see vc_state_changed_cb()
*/
#if 0
int vc_widget_stop()
{
	SLOG(LOG_DEBUG, TAG_VCW, "===== [Widget] Stop");

	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_widget_client_get_service_state(g_vc_w, &service_state);
	if (service_state != VC_SERVICE_STATE_RECORDING) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: service state is not 'RECORDING'");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret = vc_widget_client_set_command_group(g_vc_w, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to set command to client : %d", ret);
		return ret;
	}

	ret = vc_cmd_parser_delete_file(getpid(), VC_COMMAND_GROUP_TYPE_WIDGET);
	if (0 != ret) {
		ret = vc_config_convert_error_code((vc_config_error_e)ret);
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to delete command group : %s", __vc_widget_get_error_code(ret));
	}

	int count = 0;
	do {
		ret = vc_widget_dbus_request_stop(g_vc_w->handle);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to request stop : %s", __vc_widget_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCW, "[WARNING] retry request stop : %s", __vc_widget_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to request");
					break;
				}
			}
		}
	} while (0 != ret);

	return 0;
}
#endif

int vc_widget_cancel()
{
	SLOG(LOG_DEBUG, TAG_VCW, "===== [Widget] Cancel Recognition");

	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_widget_client_get_service_state(g_vc_w, &service_state);
	if (service_state != VC_SERVICE_STATE_RECORDING && service_state != VC_SERVICE_STATE_PROCESSING) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: service state is not 'RECORDING' or 'PROCESSING'");
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int count = 0;
	int ret = -1;

	do {
		ret = vc_widget_dbus_request_cancel(g_vc_w->handle);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to request cancel : %s", __vc_widget_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCW, "[WARNING] retry request cancel : %s", __vc_widget_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to request");
					break;
				}
			}
		}
	} while (0 != ret);

	SLOG(LOG_DEBUG, TAG_VCW, "=====");

	return 0;
}

static Eina_Bool __vc_widget_notify_error(void *data)
{
	vc_error_cb callback = NULL;
	void* user_data;
	int reason;

	vc_widget_client_get_error_cb(g_vc_w, &callback, &user_data);
	vc_widget_client_get_error(g_vc_w, &reason);

	if (NULL != callback) {
		vc_widget_client_use_callback(g_vc_w);
		callback(reason, user_data);
		vc_widget_client_not_use_callback(g_vc_w);
		SLOG(LOG_DEBUG, TAG_VCW, "[Error] callback is called");
	} else {
		SLOG(LOG_WARN, TAG_VCW, "[WARNING] Error callback is null");
	}

	return EINA_FALSE;
}

int __vc_widget_cb_error(int pid, int reason)
{
	if (0 != vc_widget_client_get_handle(pid, &g_vc_w)) {
		SLOG(LOG_ERROR, TAG_VCW, "Handle is not valid : pid(%d)", pid);
		return -1;
	}

	vc_widget_client_set_error(g_vc_w, reason);
	ecore_timer_add(0, __vc_widget_notify_error, g_vc_w);

	return 0;
}

static Eina_Bool __vc_widget_start_recording(void *data)
{
	if (NULL != g_w_start_timer) {
		ecore_timer_del(g_w_start_timer);
		g_w_start_timer = NULL;
	}

	vc_widget_send_current_command_list_cb send_command_list_cb = NULL;
	void* send_command_user_data = NULL;
	vc_cmd_list_h vc_cmd_list = NULL;

	vc_widget_client_get_send_command_list_cb(g_vc_w, &send_command_list_cb, &send_command_user_data);

	if (NULL != send_command_list_cb) {
		vc_widget_client_use_callback(g_vc_w);
		send_command_list_cb(&vc_cmd_list, send_command_user_data);
		vc_widget_client_not_use_callback(g_vc_w);
		SLOG(LOG_DEBUG, TAG_VCW, "client result callback called");

	} else {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] User show tooltip callback is NULL");
	}

	bool widget_command = false;
	int ret;

	if (NULL != vc_cmd_list) {
		vc_cmd_list_s* list = NULL;
		list = (vc_cmd_list_s*)vc_cmd_list;

		ret = vc_cmd_parser_save_file(getpid(), VC_COMMAND_TYPE_WIDGET, list->list);
		if (0 == ret) {
			/* widget command is valid */
			widget_command = true;
			SLOG(LOG_DEBUG, TAG_VCW, "Widget command is valid");
		} else {
			ret = VC_ERROR_OPERATION_FAILED;
			SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to save command group : %s", __vc_widget_get_error_code(ret));
		}
	}

	ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = vc_widget_dbus_request_start_recording(g_vc_w->handle, widget_command);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to request start recording to daemon : %s", __vc_widget_get_error_code(ret));
				return EINA_FALSE;
			} else {
				SLOG(LOG_WARN, TAG_VCW, "[WARNING] retry start recording : %s", __vc_widget_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to request");
					return EINA_FALSE;
				}
			}
		}
	}

	return EINA_FALSE;
}

static Eina_Bool __vc_widget_notify_tooltip(void *data)
{
	if (NULL != g_w_tooltip_timer) {
		ecore_timer_del(g_w_tooltip_timer);
		g_w_tooltip_timer = NULL;
	}

	vc_widget_show_tooltip_cb callback;
	void* user_data;
	bool show;

	vc_widget_client_get_show_tooltip_cb(g_vc_w, &callback, &user_data);
	vc_widget_client_get_show_tooltip(g_vc_w, &show);

	if (NULL != callback) {
		vc_widget_client_use_callback(g_vc_w);
		callback(show, user_data);
		vc_widget_client_not_use_callback(g_vc_w);
		SLOG(LOG_DEBUG, TAG_VCW, "client result callback called");
	} else {
		SLOG(LOG_WARN, TAG_VCW, "[WARNING] Show tooltip callback is NULL");
	}

	if (true == show) {
		g_w_start_timer = ecore_timer_add(0, __vc_widget_start_recording, NULL);
	}

	return EINA_FALSE;
}

void __vc_widget_cb_show_tooltip(int pid, bool show)
{
	if (0 != vc_widget_client_get_handle(pid, &g_vc_w)) {
		SLOG(LOG_ERROR, TAG_VCW, "Handle is not valid : pid(%d)", pid);
		return;
	}

	vc_widget_client_set_show_tooltip(g_vc_w, show);
	g_w_tooltip_timer = ecore_timer_add(0, __vc_widget_notify_tooltip, NULL);

	return;
}

static Eina_Bool __vc_widget_notify_result(void *data)
{
	char* temp_text;
	int event;
	vc_cmd_list_h vc_cmd_list = NULL;

	vc_result_cb callback = NULL;
	void* user_data = NULL;

	vc_widget_client_get_result_cb(g_vc_w, &callback, &user_data);
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Client result callback is NULL");
		return EINA_FALSE;
	}

	if (0 != vc_cmd_list_create(&vc_cmd_list)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to create command list");
		return EINA_FALSE;
	}

	vc_info_parser_get_result(&temp_text, &event, NULL, getpid(), vc_cmd_list, false);

	SLOG(LOG_DEBUG, TAG_VCW, "Result info : result text(%s) event(%d)", temp_text, event);

	vc_cmd_print_list(vc_cmd_list);

	vc_widget_client_use_callback(g_vc_w);
	callback(event, vc_cmd_list, temp_text, user_data);
	vc_widget_client_not_use_callback(g_vc_w);

	SLOG(LOG_DEBUG, TAG_VCW, "Widget result callback called");

	/* Release result */
	if (NULL != temp_text)	free(temp_text);

	vc_cmd_list_destroy(vc_cmd_list, true);

	return EINA_FALSE;
}

void __vc_widget_cb_result()
{
	ecore_timer_add(0, __vc_widget_notify_result, NULL);

	return;
}

static Eina_Bool __vc_widget_notify_state_changed(void *data)
{
	vc_state_changed_cb changed_callback = NULL;
	void* user_data;

	vc_widget_client_get_state_changed_cb(g_vc_w, &changed_callback, &user_data);

	vc_state_e current_state;
	vc_state_e before_state;

	vc_widget_client_get_before_state(g_vc_w, &current_state, &before_state);

	if (NULL != changed_callback) {
		vc_widget_client_use_callback(g_vc_w);
		changed_callback(before_state, current_state, user_data);
		vc_widget_client_not_use_callback(g_vc_w);
		SLOG(LOG_DEBUG, TAG_VCW, "State changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_VCW, "[WARNING] State changed callback is null");
	}

	return EINA_FALSE;
}

int vc_widget_set_result_cb(vc_result_cb callback, void* user_data)
{
	if (NULL == callback)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_widget_client_set_result_cb(g_vc_w, callback, user_data);

	return 0;
}

int vc_widget_unset_result_cb()
{
	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_widget_client_set_result_cb(g_vc_w, NULL, NULL);

	return 0;
}

int vc_widget_set_show_tooltip_cb(vc_widget_show_tooltip_cb callback, void* user_data)
{
	if (NULL == callback)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_widget_client_set_show_tooltip_cb(g_vc_w, callback, user_data);

	return 0;
}

int vc_widget_unset_show_tooltip_cb()
{
	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_widget_client_set_show_tooltip_cb(g_vc_w, NULL, NULL);

	return 0;
}

int vc_widget_set_send_current_command_list_cb(vc_widget_send_current_command_list_cb callback, void* user_data)
{
	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_widget_client_set_send_command_list_cb(g_vc_w, callback, user_data);

	return 0;
}

int vc_widget_unsset_send_current_command_list_cb()
{
	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_widget_client_set_send_command_list_cb(g_vc_w, NULL, NULL);

	return 0;
}

int __vc_widget_cb_service_state(int state)
{
	vc_service_state_e current_state = (vc_service_state_e)state;
	vc_service_state_e before_state;
	vc_widget_client_get_service_state(g_vc_w, &before_state);

	if (current_state == before_state) {
		return 0;
	}

	SLOG(LOG_DEBUG, TAG_VCW, "Service State changed : Before(%d) Current(%d)",
		before_state, current_state);

	/* Save service state */
	vc_widget_client_set_service_state(g_vc_w, current_state);

	vc_service_state_changed_cb callback = NULL;
	void* service_user_data = NULL;
	vc_widget_client_get_service_state_changed_cb(g_vc_w, &callback, &service_user_data);

	if (NULL != callback) {
		vc_widget_client_use_callback(g_vc_w);
		callback((vc_service_state_e)before_state, (vc_service_state_e)current_state, service_user_data);
		vc_widget_client_not_use_callback(g_vc_w);
		SLOG(LOG_DEBUG, TAG_VCW, "Service state changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_VCW, "[WARNING] Service state changed callback is null");
	}

	return 0;
}

int vc_widget_set_service_state_changed_cb(vc_service_state_changed_cb callback, void* user_data)
{
	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_widget_client_set_service_state_changed_cb(g_vc_w, callback, user_data);

	return 0;
}

int vc_widget_unset_service_state_changed_cb()
{
	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_widget_client_set_service_state_changed_cb(g_vc_w, NULL, NULL);

	return 0;
}

int vc_widget_set_state_changed_cb(vc_state_changed_cb callback, void* user_data)
{
	if (NULL == callback)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_widget_client_set_state_changed_cb(g_vc_w, callback, user_data);

	return 0;
}

int vc_widget_unset_state_changed_cb()
{
	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_widget_client_set_state_changed_cb(g_vc_w, NULL, NULL);

	return 0;
}

int vc_widget_set_current_language_changed_cb(vc_current_language_changed_cb callback, void* user_data)
{
	if (NULL == callback)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_widget_client_set_current_lang_changed_cb(g_vc_w, callback, user_data);

	return 0;
}

int vc_widget_unset_current_language_changed_cb()
{
	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_widget_client_set_current_lang_changed_cb(g_vc_w, NULL, NULL);

	return 0;
}

int vc_widget_set_error_cb(vc_error_cb callback, void* user_data)
{
	if (NULL == callback)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_widget_client_set_error_cb(g_vc_w, callback,  user_data);

	return 0;
}

int vc_widget_unset_error_cb()
{
	vc_state_e state;
	if (0 != vc_widget_client_get_state(g_vc_w, &state)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Invalid State: Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_widget_client_set_error_cb(g_vc_w, NULL, NULL);

	return 0;
}

