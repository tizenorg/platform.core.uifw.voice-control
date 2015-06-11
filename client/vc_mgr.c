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

#include "vc_info_parser.h"
#include "vc_config_mgr.h"
#include "vc_command.h"
#include "vc_main.h"
#include "vc_mgr_client.h"
#include "vc_mgr_dbus.h"
#include "voice_control.h"
#include "voice_control_command.h"
#include "voice_control_command_expand.h"
#include "voice_control_common.h"
#include "voice_control_manager.h"


#define VC_MANAGER_CONFIG_HANDLE	100000

static bool g_m_is_daemon_started = false;

static Ecore_Timer* g_m_connect_timer = NULL;

static vc_h g_vc_m = NULL;

static GSList* g_demandable_client_list = NULL;


static Eina_Bool __vc_mgr_notify_state_changed(void *data);
static Eina_Bool __vc_mgr_notify_error(void *data);
static Eina_Bool __vc_mgr_notify_result(void *data);

static const char* __vc_mgr_get_error_code(vc_error_e err)
{
	switch(err) {
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

static void __vc_mgr_lang_changed_cb(const char* before_lang, const char* current_lang)
{
	SECURE_SLOG(LOG_DEBUG, TAG_VCM, "Lang changed : Before lang(%s) Current lang(%s)", 
		before_lang, current_lang);

	vc_current_language_changed_cb callback = NULL;
	void* lang_user_data;
	vc_mgr_client_get_current_lang_changed_cb(g_vc_m, &callback, &lang_user_data);

	if (NULL != callback) {
		vc_mgr_client_use_callback(g_vc_m);
		callback(before_lang, current_lang, lang_user_data);
		vc_mgr_client_not_use_callback(g_vc_m);
		SLOG(LOG_DEBUG, TAG_VCM, "Language changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_VCM, "[WARNING] Language changed callback is null");
	}

	return;
}

static void __vc_mgr_service_state_changed_cb(int before_state, int current_state)
{
	SECURE_SLOG(LOG_DEBUG, TAG_VCM, "Service State changed : Before(%d) Current(%d)", 
		before_state, current_state);

	/* Save service state */
	vc_mgr_client_set_service_state(g_vc_m, (vc_service_state_e)current_state);

	vc_service_state_changed_cb callback = NULL;
	void* service_user_data;
	vc_mgr_client_get_service_state_changed_cb(g_vc_m, &callback, &service_user_data);

	if (NULL != callback) {
		vc_mgr_client_use_callback(g_vc_m);
		callback((vc_service_state_e)before_state, (vc_service_state_e)current_state, service_user_data);
		vc_mgr_client_not_use_callback(g_vc_m);
		SLOG(LOG_DEBUG, TAG_VCM, "Service state changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_VCM, "[WARNING] Service state changed callback is null");
	}

	return;
}

static void __vc_mgr_foreground_changed_cb(int previous, int current)
{
	SLOG(LOG_DEBUG, TAG_VCM, "Foreground changed : Before(%d) Current(%d)", previous, current);

	/* get authorized valid app */
	int pid;
	if (0 != vc_mgr_client_get_valid_authorized_client(g_vc_m, &pid)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to get authorized valid app");
		return;
	}

	/* compare & set valid */
	if (current != pid) {
		SLOG(LOG_DEBUG, TAG_VCM, "Authority(%d) changed to invalid", pid);

		/* set authorized valid */
		if (true == vc_mgr_client_is_authorized_client(g_vc_m, current)) {
			SLOG(LOG_DEBUG, TAG_VCM, "Authority(%d) change to valid", current);
			vc_mgr_client_set_valid_authorized_client(g_vc_m, current);
		} else {
			SLOG(LOG_DEBUG, TAG_VCM, "No valid Authority");
			vc_mgr_client_set_valid_authorized_client(g_vc_m, -1);
		}
	}
}

int vc_mgr_initialize()
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Initialize");

	/* check handle */
	if (true == vc_mgr_client_is_valid(g_vc_m)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Already initialized");
		return VC_ERROR_NONE;
	}

	if (0 != vc_mgr_dbus_open_connection()) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to open connection");
		return VC_ERROR_OPERATION_FAILED;
	}

	if (0 != vc_mgr_client_create(&g_vc_m)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to create client!!!!!");
		return VC_ERROR_OUT_OF_MEMORY;
	}

	int ret = vc_config_mgr_initialize(g_vc_m->handle + VC_MANAGER_CONFIG_HANDLE);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to init config manager : %d", ret);
		vc_mgr_client_destroy(g_vc_m);
		return VC_ERROR_OPERATION_FAILED;
	}

	ret = vc_config_mgr_set_lang_cb(g_vc_m->handle + VC_MANAGER_CONFIG_HANDLE, __vc_mgr_lang_changed_cb);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to set callback : %d", ret);
		vc_config_mgr_finalize(g_vc_m->handle + VC_MANAGER_CONFIG_HANDLE);
		vc_mgr_client_destroy(g_vc_m);
		return VC_ERROR_OPERATION_FAILED;
	}
	
	ret = vc_config_mgr_set_service_state_cb(g_vc_m->handle + VC_MANAGER_CONFIG_HANDLE, __vc_mgr_service_state_changed_cb);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to set service change callback : %d", ret);
		vc_config_mgr_unset_lang_cb(g_vc_m->handle + VC_MANAGER_CONFIG_HANDLE);
		vc_config_mgr_finalize(g_vc_m->handle + VC_MANAGER_CONFIG_HANDLE);
		vc_mgr_client_destroy(g_vc_m);
		return VC_ERROR_OPERATION_FAILED;
	}

	ret = vc_config_mgr_set_foreground_cb(g_vc_m->handle + VC_MANAGER_CONFIG_HANDLE, __vc_mgr_foreground_changed_cb);

	int service_state = -1;
	if (0 != vc_config_mgr_get_service_state(&service_state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to get service state");
		vc_config_mgr_finalize(g_vc_m->handle + VC_MANAGER_CONFIG_HANDLE);
		vc_mgr_client_destroy(g_vc_m);
		return VC_ERROR_OPERATION_FAILED;
	}

	vc_mgr_client_set_service_state(g_vc_m, service_state);

	SLOG(LOG_DEBUG, TAG_VCM, "[Success] pid(%d)", g_vc_m->handle);

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return VC_ERROR_NONE;
}

static void __vc_mgr_internal_unprepare()
{
	int ret = vc_mgr_dbus_request_finalize(g_vc_m->handle);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request finalize : %s", __vc_mgr_get_error_code(ret));
	}

	g_m_is_daemon_started = false;

	vc_cmd_parser_delete_file(getpid(), VC_COMMAND_TYPE_SYSTEM);
	vc_cmd_parser_delete_file(getpid(), VC_COMMAND_TYPE_EXCLUSIVE);

	return;
}

int vc_mgr_deinitialize()
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Deinitialize");

	if (false == vc_mgr_client_is_valid(g_vc_m)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] NOT initialized");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	vc_state_e state;
	vc_mgr_client_get_client_state(g_vc_m, &state);

	/* check state */
	switch (state) {
	case VC_STATE_READY:
		__vc_mgr_internal_unprepare();
		/* no break. need to next step*/
	case VC_STATE_INITIALIZED:
		if (NULL != g_m_connect_timer) {
			SLOG(LOG_DEBUG, TAG_VCM, "Connect Timer is deleted");
			ecore_timer_del(g_m_connect_timer);
		}

		vc_config_mgr_unset_service_state_cb(g_vc_m->handle + VC_MANAGER_CONFIG_HANDLE);
		vc_config_mgr_unset_lang_cb(g_vc_m->handle + VC_MANAGER_CONFIG_HANDLE);
		vc_config_mgr_finalize(g_vc_m->handle + VC_MANAGER_CONFIG_HANDLE);

		/* Free client resources */
		vc_mgr_client_destroy(g_vc_m);
		g_vc_m = NULL;
		break;
	case VC_STATE_NONE:
		break;
	default:
		break;
	}

	SLOG(LOG_DEBUG, TAG_VCM, "Success: destroy");

	if (0 != vc_mgr_dbus_close_connection()) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to close connection");
	}

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return VC_ERROR_NONE;
}

static void* __fork_vc_daemon()
{
	int pid, i;
	pid = fork();

	switch(pid) {
	case -1:
		SLOG(LOG_ERROR, TAG_VCM, "Fail to create daemon");
		break;
	case 0:
		setsid();
		for (i = 0;i < _NSIG;i++)
			signal(i, SIG_DFL);

		execl(VC_DAEMON_PATH, VC_DAEMON_PATH, NULL);
		break;
	default:
		break;
	}

	return (void*) 1;
}

static Eina_Bool __vc_mgr_connect_daemon(void *data)
{
	/* Send hello */
	if (0 != vc_mgr_dbus_request_hello()) {
		if (false == g_m_is_daemon_started) {
			g_m_is_daemon_started = true;

			pthread_t thread;
			int thread_id;
			thread_id = pthread_create(&thread, NULL, __fork_vc_daemon, NULL);
			if (thread_id < 0) {
				SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to make thread");
				g_m_connect_timer = NULL;
				return EINA_FALSE;
			}

			pthread_detach(thread);
		}
		return EINA_TRUE;
	}

	g_m_connect_timer = NULL;
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Connect daemon");

	/* request initialization */
	int ret = -1;
	ret = vc_mgr_dbus_request_initialize(g_vc_m->handle);

	if (VC_ERROR_ENGINE_NOT_FOUND == ret) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to initialize : %s", __vc_mgr_get_error_code(ret));

		vc_mgr_client_set_error(g_vc_m, VC_ERROR_ENGINE_NOT_FOUND);
		ecore_timer_add(0, __vc_mgr_notify_error, g_vc_m);

		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, "  ");
		return EINA_FALSE;

	} else if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCM, "[WARNING] Fail to connection. Retry to connect : %s", __vc_mgr_get_error_code(ret));
		return EINA_TRUE;
	} else {
		/* Success to connect */
	}

	SECURE_SLOG(LOG_DEBUG, TAG_VCM, "[SUCCESS] Connected daemon");

	vc_mgr_client_set_client_state(g_vc_m, VC_STATE_READY);

	vc_state_changed_cb changed_callback = NULL;
	void* user_data = NULL;

	vc_mgr_client_get_state_changed_cb(g_vc_m, &changed_callback, &user_data);

	vc_state_e current_state;
	vc_state_e before_state;

	vc_mgr_client_get_before_state(g_vc_m, &current_state, &before_state);

	if (NULL != changed_callback) {
		vc_mgr_client_use_callback(g_vc_m);
		changed_callback(before_state, current_state, user_data);
		vc_mgr_client_not_use_callback(g_vc_m);
		SLOG(LOG_DEBUG, TAG_VCM, "State changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_VCM, "[WARNING] State changed callback is null");
	}

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, "  ");

	return EINA_FALSE;
}

int vc_mgr_prepare()
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Prepare");

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: Current state is not 'CREATED'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	g_m_is_daemon_started = false;

	g_m_connect_timer = ecore_timer_add(0, __vc_mgr_connect_daemon, NULL);

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return VC_ERROR_NONE;
}

int vc_mgr_unprepare()
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Unprepare");

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	__vc_mgr_internal_unprepare();

	vc_mgr_client_set_client_state(g_vc_m, VC_STATE_INITIALIZED);
	ecore_timer_add(0, __vc_mgr_notify_state_changed, g_vc_m);

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return VC_ERROR_NONE;
}

int vc_mgr_foreach_supported_languages(vc_supported_language_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Foreach Supported Language");

	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Input parameter is NULL");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret = -1;
	ret = vc_config_mgr_get_language_list(callback, user_data);
	if (0 != ret) {
		ret = vc_config_convert_error_code((vc_config_error_e)ret);
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to get languages : %s", __vc_mgr_get_error_code(ret));
	}

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return VC_ERROR_NONE;
}

int vc_mgr_get_current_language(char** language)
{
	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	int ret = -1;
	ret = vc_config_mgr_get_default_language(language);
	if (0 != ret) {
		ret = vc_config_convert_error_code((vc_config_error_e)ret);
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to get current languages : %s", __vc_mgr_get_error_code(ret));
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, "[Get current language] language : %s", *language);
	}

	return ret;
}

int vc_mgr_get_state(vc_state_e* state)
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Get State");

	if (NULL == state) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e temp;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &temp)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	*state = temp;

	switch(*state) {
		case VC_STATE_NONE:		SLOG(LOG_DEBUG, TAG_VCM, "Current state is 'None'");		break;
		case VC_STATE_INITIALIZED:	SLOG(LOG_DEBUG, TAG_VCM, "Current state is 'Created'");		break;
		case VC_STATE_READY:		SLOG(LOG_DEBUG, TAG_VCM, "Current state is 'Ready'");		break;
		default:			SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid state");		break;
	}

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return VC_ERROR_NONE;
}

int vc_mgr_get_service_state(vc_service_state_e* state)
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Get Service State");

	if (NULL == state) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e client_state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &client_state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (client_state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Current state is not 'READY'");
		return VC_ERROR_INVALID_STATE;
	}

	/* get service state */
	vc_service_state_e service_state;
	if (0 != vc_mgr_client_get_service_state(g_vc_m, &service_state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to get service state");
		return VC_ERROR_OPERATION_FAILED;
	}

	*state = service_state;

	switch(*state) {
		case VC_SERVICE_STATE_NONE:		SLOG(LOG_DEBUG, TAG_VCM, "Current service state is 'None'");		break;
		case VC_SERVICE_STATE_READY:		SLOG(LOG_DEBUG, TAG_VCM, "Current service state is 'Ready'");		break;
		case VC_SERVICE_STATE_RECORDING:	SLOG(LOG_DEBUG, TAG_VCM, "Current service state is 'Recording'");	break;
		case VC_SERVICE_STATE_PROCESSING:	SLOG(LOG_DEBUG, TAG_VCM, "Current service state is 'Processing'");	break;
		default:				SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid state");			break;
	}

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return VC_ERROR_NONE;
}

int vc_mgr_set_demandable_client_rule(const char* rule)
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Set Demandable client rule");

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret = -1;
	ret = vc_info_parser_set_demandable_client(rule);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] rule is NOT valid");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	if (0 != vc_info_parser_get_demandable_clients(&g_demandable_client_list)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to get demandable clients");
		return VC_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");
	
	return 0;

	/*
	int count = 0;
	ret = -1;
	while (0 != ret) {
		ret = vc_mgr_dbus_request_demandable_client(g_vc_m->handle);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request set client rule to daemon : %s", __vc_mgr_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCM, "[WARNING] retry request set client rule : %s", __vc_mgr_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request");
					break;
				}
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return 0;
	*/
}

int vc_mgr_unset_demandable_client_rule()
{
	vc_info_parser_set_demandable_client(NULL);

	int count = 0;
	int ret = -1;
	while (0 != ret) {
		ret = vc_mgr_dbus_request_demandable_client(g_vc_m->handle);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request unset client rule to daemon : %s", __vc_mgr_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCM, "[WARNING] retry request unset client rule : %s", __vc_mgr_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request");
					break;
				}
			}
		}
	}

	return 0;
}

int vc_mgr_is_command_format_supported(vc_cmd_format_e format, bool* support)
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Is command type supported");

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check support */
	bool non_fixed_support = false;
	if (0 != vc_config_mgr_get_nonfixed_support(&non_fixed_support)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to get nonfixed support info");
	}

	switch (format) {
	case VC_CMD_FORMAT_FIXED:			*support = true;		break;
	case VC_CMD_FORMAT_FIXED_AND_EXTRA:	*support = non_fixed_support;	break;
	case VC_CMD_FORMAT_EXTRA_AND_FIXED:	*support = non_fixed_support;	break;
	default:					*support = false;		break;
	}

	SLOG(LOG_ERROR, TAG_VCM, "[DEBUG] Format(%d) support(%s)", format, *support ? "true" : "false");

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return VC_ERROR_NONE;
}

int vc_mgr_set_command_list(vc_cmd_list_h vc_cmd_list)
{ 
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Set Command list");

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_mgr_client_get_service_state(g_vc_m, &service_state);
	if (service_state != VC_SERVICE_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: service state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	vc_cmd_list_s* list = NULL;
	list = (vc_cmd_list_s*)vc_cmd_list;

	int system_ret = vc_cmd_parser_save_file(getpid(), VC_COMMAND_TYPE_SYSTEM, list->list);
	int exclsive_ret = vc_cmd_parser_save_file(getpid(), VC_COMMAND_TYPE_EXCLUSIVE, list->list);
	int ret = 0;

	if (0 != system_ret && 0 != exclsive_ret) {
		ret = VC_ERROR_INVALID_PARAMETER;
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to save command group : %s", __vc_mgr_get_error_code(ret));
	} else {
		int count = 0;
		do {
			ret = vc_mgr_dbus_request_set_command(g_vc_m->handle);
			if (0 != ret) {
				if (VC_ERROR_TIMED_OUT != ret) {
					SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request set command to daemon : %s", __vc_mgr_get_error_code(ret));
					break;
				} else {
					SLOG(LOG_WARN, TAG_VCM, "[WARNING] retry request set command : %s", __vc_mgr_get_error_code(ret));
					usleep(10000);
					count++;
					if (VC_RETRY_COUNT == count) {
						SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request");
						break;
					}
				}
			}
		} while(0 != ret);
	}

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return ret;
}

int vc_mgr_unset_command_list()
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Unset Command list");

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Current state is not 'READY'");
		return VC_ERROR_INVALID_STATE;
	}

	int count = 0;
	int ret = -1;
	while (0 != ret) {
		ret = vc_mgr_dbus_request_unset_command(g_vc_m->handle);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request unset command to daemon : %s", __vc_mgr_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCM, "[WARNING] retry request unset command : %s", __vc_mgr_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request");
					break;
				}
			}
		}
	}

	vc_cmd_parser_delete_file(getpid(), VC_COMMAND_TYPE_SYSTEM);
	vc_cmd_parser_delete_file(getpid(), VC_COMMAND_TYPE_EXCLUSIVE);

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return 0;
}

int vc_mgr_set_audio_type(const char* audio_id)
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Set audio type");

	if (NULL == audio_id) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_mgr_client_get_service_state(g_vc_m, &service_state);
	if (service_state != VC_SERVICE_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: service state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret;
	int count = 0;

	/* Request */
	ret = -1;
	count = 0;
	while (0 != ret) {
		ret = vc_mgr_dbus_request_set_audio_type(g_vc_m->handle, audio_id);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to set audio type : %s", __vc_mgr_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCM, "[WARNING] retry to set audio type : %s", __vc_mgr_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request");
					break;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCM, "[SUCCESS] Set audio type");
			/* Save */
			vc_mgr_client_set_audio_type(g_vc_m, audio_id);
		}
	}

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return ret;
}

int vc_mgr_get_audio_type(char** audio_id)
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Get audio type");

	if (NULL == audio_id) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_mgr_client_get_service_state(g_vc_m, &service_state);
	if (service_state != VC_SERVICE_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: service state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	char* temp = NULL;

	vc_mgr_client_get_audio_type(g_vc_m, &temp);

	if (NULL == temp) {
		/* Not initiallized */
		int ret = -1;
		int count = 0;
		while (0 != ret) {
			ret = vc_mgr_dbus_request_get_audio_type(g_vc_m->handle, &temp);
			if (0 != ret) {
				if (VC_ERROR_TIMED_OUT != ret) {
					SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to get audio type : %s", __vc_mgr_get_error_code(ret));
					break;
				} else {
					SLOG(LOG_WARN, TAG_VCM, "[WARNING] retry to get audio type : %s", __vc_mgr_get_error_code(ret));
					usleep(10000);
					count++;
					if (VC_RETRY_COUNT == count) {
						SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request");
						break;
					}
				}
			} else {
				SLOG(LOG_DEBUG, TAG_VCM, "[SUCCESS] Get audio type : %s", temp);
				/* Save */
				vc_mgr_client_set_audio_type(g_vc_m, temp);
			}
		}
	}

	if (NULL != temp) {
		*audio_id = strdup(temp);
		free(temp);
		temp = NULL;
	}

	return 0;
}

int vc_mgr_get_current_commands(vc_cmd_list_h* vc_cmd_list)
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Foreach current commands");

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_mgr_client_get_service_state(g_vc_m, &service_state);
	if (service_state != VC_SERVICE_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: service state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	if (NULL == vc_cmd_list) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid Parameter");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_list_h temp_list = NULL;
	if (0 != vc_cmd_list_create(&temp_list)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to create list");
		return VC_ERROR_INVALID_PARAMETER;
	}

	*vc_cmd_list = temp_list;

	int fg_pid = 0;
	int mgr_pid = 0;
	int count = 0;
	int ret = -1;

	/* Get foreground pid */
	if (0 != vc_config_mgr_get_foreground(&fg_pid)) {
		/* There is no foreground app for voice control */
		SLOG(LOG_WARN, TAG_VCM, "[Manager WARNING] No foreground pid for voice control");
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, "[Manager] Foreground pid(%d)", fg_pid);
	}

	if (0 != vc_mgr_client_get_pid(g_vc_m, &mgr_pid)) {
		SLOG(LOG_DEBUG, TAG_VCM, "[Manager] Fail to get manager pid");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, "[Manager] Manager pid(%d)", mgr_pid);
	}

	/* Get system command */
	ret = vc_cmd_parser_append_commands(mgr_pid, VC_COMMAND_TYPE_SYSTEM, temp_list);
	if (0 != ret) {
		SLOG(LOG_DEBUG, TAG_VCM, "[Manager] No system commands");
	}

	/* Request */
	ret = -1;
	count = 0;
	while (0 != ret) {
		ret = vc_mgr_dbus_request_set_client_info(g_vc_m->handle);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to set client info : %s", __vc_mgr_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCM, "[WARNING] retry to set client info : %s", __vc_mgr_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request");
					break;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCM, "[SUCCESS] Set client info");
		}
	}

	GSList *iter = NULL;
	GSList* client_info_list = NULL;
	vc_client_info_s *client_info = NULL;
	bool is_fgpid = false;

	if (0 != vc_info_parser_get_client_info(&client_info_list)) {
		SLOG(LOG_DEBUG, TAG_VCM, "[DEBUG] No client");
		return 0;
	}
	
	if (VC_NO_FOREGROUND_PID != fg_pid) {
		iter = g_slist_nth(client_info_list, 0);
		while (NULL != iter) {
			client_info = iter->data;
			if (NULL != client_info) {
				if (fg_pid == client_info->pid) {
					is_fgpid = true;
					break;
				}
			}
			iter = g_slist_next(iter);
		}
	}

	/* Get foreground commands and widget */
	if (true == is_fgpid) {
		/* Get handle */
		SLOG(LOG_DEBUG, TAG_VCM, "[Manager] fore cmd(%d)", client_info->fg_cmd);

		/* Get foreground command */
		if (true == client_info->fg_cmd) {
			ret = vc_cmd_parser_append_commands(fg_pid, VC_COMMAND_TYPE_FOREGROUND, temp_list);
			if (0 != ret) {
				SLOG(LOG_ERROR, TAG_VCM, "[Client Data ERROR] Fail to get the fg command list");
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCM, "[Manager] No foreground commands");
		}

		/* Check exclusive option */
		if (true == client_info->exclusive_cmd) {
			SLOG(LOG_DEBUG, TAG_VCM, "[Manager] Exclusive command is ON");

			/* Set background command for exclusive option */
			if (true == client_info->bg_cmd) {
				SLOG(LOG_DEBUG, TAG_VCM, "[Manager] Set background command");
				ret = vc_cmd_parser_append_commands(client_info->pid, VC_COMMAND_TYPE_BACKGROUND, temp_list);
				if (0 != ret) {
					SLOG(LOG_ERROR, TAG_VCM, "[Client Data ERROR] Fail to get the bg command list : pid(%d)", client_info->pid);
				}
			}

			/* need to release client info */
			iter = g_slist_nth(client_info_list, 0);

			while (NULL != iter) {
				client_info = iter->data;
				if (NULL != client_info) {
					free(client_info);
				}
				client_info_list = g_slist_remove_link(client_info_list, iter);
				iter = g_slist_nth(client_info_list, 0);
			}

			SLOG(LOG_DEBUG, TAG_VCM, "=====");
			SLOG(LOG_DEBUG, TAG_VCM, " ");

			return 0;
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, "[Manager] No foreground app");
	}

	/* Get background commands */
	if (0 < g_slist_length(client_info_list)) {
		iter = g_slist_nth(client_info_list, 0);

		while (NULL != iter) {
			client_info = iter->data;

			if (NULL != client_info) {
				SLOG(LOG_DEBUG, TAG_VCM, "[Manager] Pid(%d) Back cmd(%d)", client_info->pid, client_info->bg_cmd);
				if (true == client_info->bg_cmd) {
					ret = vc_cmd_parser_append_commands(client_info->pid, VC_COMMAND_TYPE_BACKGROUND, temp_list);
					if (0 != ret) {
						SLOG(LOG_ERROR, TAG_VCM, "[Client Data ERROR] Fail to get the bg command list : pid(%d)", client_info->pid);
					} 
				}
				free(client_info);
			}
			client_info_list = g_slist_remove_link(client_info_list, iter);

			iter = g_slist_nth(client_info_list, 0);
		}
	} else {
		/* NO client */
		SLOG(LOG_DEBUG, TAG_VCM, "[Manager] No background commands");
	}

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return 0;
}

int vc_mgr_start(bool stop_by_silence, bool exclusive_command_option)
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Request start");

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: Current state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_mgr_client_get_service_state(g_vc_m, &service_state);
	if (service_state != VC_SERVICE_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: service state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	vc_mgr_client_set_exclusive_command(g_vc_m, exclusive_command_option);

	bool start_by_client = false;
	if (0 != vc_mgr_client_get_start_by_client(g_vc_m, &start_by_client)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to get start by client");
	}

	int ret;
	int count = 0;

	/* Request */
	ret = -1;
	count = 0;
	while (0 != ret) {
		ret = vc_mgr_dbus_request_start(g_vc_m->handle, stop_by_silence, exclusive_command_option, start_by_client);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to start request start : %s", __vc_mgr_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCM, "[WARNING] retry start request start : %s", __vc_mgr_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request");
					vc_mgr_client_set_exclusive_command(g_vc_m, false);
					break;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCM, "[SUCCESS] start recognition");
			vc_mgr_client_set_service_state(g_vc_m, VC_SERVICE_STATE_RECORDING);
		}
	}

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return ret;
}

int vc_mgr_stop()
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Request stop");

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: client state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_mgr_client_get_service_state(g_vc_m, &service_state);
	if (service_state != VC_SERVICE_STATE_RECORDING) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: service state is not 'RECORDING'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	/* do request */
	while (0 != ret) {
		ret = vc_mgr_dbus_request_stop(g_vc_m->handle);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_DEBUG, TAG_VCM, "[ERROR] Fail to stop request : %s", __vc_mgr_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCM, "[WARNING] retry stop request : %s", __vc_mgr_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request");
					break;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCM, "[SUCCESS] Stop recognition");
			vc_mgr_client_set_service_state(g_vc_m, VC_SERVICE_STATE_PROCESSING);
		}
	}

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return ret;
}

int vc_mgr_cancel()
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Request cancel");

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_READY) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: client state is not 'READY'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	/* Check service state */
	vc_service_state_e service_state = -1;
	vc_mgr_client_get_service_state(g_vc_m, &service_state);
	if (service_state != VC_SERVICE_STATE_RECORDING && service_state != VC_SERVICE_STATE_PROCESSING) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: service state is not 'RECORDING' or 'PROCESSING'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = vc_mgr_dbus_request_cancel(g_vc_m->handle);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_DEBUG, TAG_VCM, "[ERROR] Fail to cancel request : %s", __vc_mgr_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCM, "[WARNING] retry cancel request : %s", __vc_mgr_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request");
					break;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCM, "[SUCCESS] Cancel recognition");
			vc_mgr_client_set_service_state(g_vc_m, VC_SERVICE_STATE_READY);
		}
	}

	vc_mgr_client_set_exclusive_command(g_vc_m, false);

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return ret;
}

int vc_mgr_get_recording_volume(float* volume)
{
	if (NULL == volume) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_service_state_e service_state = -1;
	if (0 != vc_mgr_client_get_service_state(g_vc_m, &service_state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (VC_SERVICE_STATE_RECORDING != service_state) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: Service state is not 'RECORDING'");
		return VC_ERROR_INVALID_STATE;
	}

	FILE* fp = fopen(VC_RUNTIME_INFO_AUDIO_VOLUME, "rb");
	if (!fp) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to open Volume File");
		return VC_ERROR_OPERATION_FAILED;
	}

	int readlen = fread((void*)volume, sizeof(*volume), 1, fp);
	fclose(fp);

	if (0 == readlen)
		*volume = 0.0f;

	return 0;
}

int vc_mgr_set_selected_results(vc_cmd_list_h vc_cmd_list)
{
	SLOG(LOG_DEBUG, TAG_VCM, "===== [Manager] Select result");

	vc_service_state_e service_state = -1;
	vc_mgr_client_get_service_state(g_vc_m, &service_state);
	if (service_state != VC_SERVICE_STATE_PROCESSING) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Invalid State: service state is not 'PROCESSING'");
		SLOG(LOG_DEBUG, TAG_VCM, "=====");
		SLOG(LOG_DEBUG, TAG_VCM, " ");
		return VC_ERROR_INVALID_STATE;
	}

	if (NULL != vc_cmd_list) {
		int event = 0;
		char* result_text = NULL;

		vc_mgr_client_get_all_result(g_vc_m, &event, &result_text);
		
		vc_info_parser_set_result(result_text, event, NULL, vc_cmd_list, false);

		if (NULL != result_text) {
			free(result_text);
			result_text = NULL;
		}
	}
	
	int ret;
	int count = 0;

	/* Request */
	ret = -1;
	count = 0;
	while (0 != ret) {
		ret = vc_mgr_dbus_send_result_selection(g_vc_m->handle);
		if (0 != ret) {
			if (VC_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to send result selection : %s", __vc_mgr_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_VCM, "[WARNING] retry send result selection : %s", __vc_mgr_get_error_code(ret));
				usleep(10000);
				count++;
				if (VC_RETRY_COUNT == count) {
					SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to request");
					break;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCM, "[SUCCESS] result selection");
		}
	}

	vc_mgr_client_unset_all_result(g_vc_m);

	SLOG(LOG_DEBUG, TAG_VCM, "=====");
	SLOG(LOG_DEBUG, TAG_VCM, " ");

	return 0;
}

static Eina_Bool __vc_mgr_set_select_result(void *data)
{
	vc_mgr_set_selected_results(NULL);
	return EINA_FALSE;
}

static Eina_Bool __vc_mgr_notify_all_result(void *data)
{
	char* temp_text = NULL;
	int event;
	char* temp_message = NULL;
	vc_cmd_list_h vc_cmd_list = NULL;

	vc_mgr_all_result_cb all_callback = NULL;
	void* all_user_data = NULL;

	vc_mgr_client_get_all_result_cb(g_vc_m, &all_callback, &all_user_data);
	if (NULL == all_callback) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] All result callback is NULL");
		return EINA_FALSE;
	}

	if (0 != vc_cmd_list_create(&vc_cmd_list)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to create command list");
		return EINA_FALSE;
	}

	vc_info_parser_get_result(&temp_text, &event, &temp_message, -1, vc_cmd_list, vc_mgr_client_get_exclusive_command(g_vc_m));

	SLOG(LOG_DEBUG, TAG_VCM, "Result info : result text(%s) event(%d) result_message(%s)", temp_text, event, temp_message);

	vc_cmd_print_list(vc_cmd_list);

	bool cb_ret;

	vc_mgr_client_use_callback(g_vc_m);
	cb_ret = all_callback(event, vc_cmd_list, temp_text, temp_message, all_user_data);
	vc_mgr_client_not_use_callback(g_vc_m);

	if (true == vc_mgr_client_get_exclusive_command(g_vc_m)) {
		/* exclusive */
		vc_result_cb callback = NULL;
		void* user_data = NULL;

		vc_mgr_client_get_result_cb(g_vc_m, &callback, &user_data);
		if (NULL == callback) {
			SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Client result callback is NULL");
			return EINA_FALSE;
		}

		vc_mgr_client_use_callback(g_vc_m);
		callback(event, vc_cmd_list, temp_text, user_data);
		vc_mgr_client_not_use_callback(g_vc_m);
		SLOG(LOG_DEBUG, TAG_VCM, "Exclusive result callback called");

		/* Release result */
		if (NULL != temp_text)	free(temp_text);

		/* Release list */
		vc_cmd_list_destroy(vc_cmd_list, true);

		vc_mgr_client_set_exclusive_command(g_vc_m, false);

		return EINA_FALSE;
	}

	int count = 0;
	vc_cmd_list_get_count(vc_cmd_list, &count);
	if (0 < count) {
		if (true == cb_ret) {
			SLOG(LOG_DEBUG, TAG_VCM, "Callback result is true");
			ecore_idler_add(__vc_mgr_set_select_result, NULL);
		} else {
			SLOG(LOG_DEBUG, TAG_VCM, "Callback result is false");
			/* need to select conflicted result */

			vc_mgr_client_set_all_result(g_vc_m, event, temp_text);
		}
	} else {
		ecore_idler_add(__vc_mgr_set_select_result, NULL);
		vc_mgr_client_set_exclusive_command(g_vc_m, false);
		vc_mgr_client_unset_all_result(g_vc_m);
	}

	/* Release result */
	if (NULL != temp_text)	free(temp_text);

	/* Release list */
	vc_cmd_list_destroy(vc_cmd_list, true);

	return EINA_FALSE;
}

static Eina_Bool __vc_mgr_notify_result(void *data)
{
	char* temp_text;
	int event;
	vc_cmd_list_h vc_cmd_list = NULL;

	vc_result_cb callback = NULL;
	void* user_data = NULL;

	vc_mgr_client_get_result_cb(g_vc_m, &callback, &user_data);
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Client result callback is NULL");
		return EINA_FALSE;
	}

	if (0 != vc_cmd_list_create(&vc_cmd_list)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to create command list");
		return EINA_FALSE;
	}

	vc_info_parser_get_result(&temp_text, &event, NULL, getpid(), vc_cmd_list, false);

	SLOG(LOG_DEBUG, TAG_VCM, "Result : result text(%s) event(%d)", temp_text, event);

	vc_cmd_print_list(vc_cmd_list);

	vc_mgr_client_use_callback(g_vc_m);
	callback(event, vc_cmd_list, temp_text, user_data);
	vc_mgr_client_not_use_callback(g_vc_m);
	SLOG(LOG_DEBUG, TAG_VCM, "Result callback called");

	vc_cmd_list_destroy(vc_cmd_list, true);

	/* Release result */
	if (NULL != temp_text)	free(temp_text);

	return EINA_FALSE;
}

void __vc_mgr_cb_all_result()
{
	if (false == vc_mgr_client_get_exclusive_command(g_vc_m)) {
		__vc_mgr_notify_all_result(NULL);
	} else {
		__vc_mgr_notify_result(0);
	}

	return;
}

void __vc_mgr_cb_system_result()
{
	__vc_mgr_notify_result(NULL);
	return;
}

static Eina_Bool __vc_mgr_speech_detected(void *data)
{
	vc_mgr_begin_speech_detected_cb callback = NULL;
	void* user_data = NULL;

	vc_mgr_client_get_speech_detected_cb(g_vc_m, &callback, &user_data);
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Client speech detected callback is NULL");
		return EINA_FALSE;
	}

	vc_mgr_client_use_callback(g_vc_m);
	callback(user_data);
	vc_mgr_client_not_use_callback(g_vc_m);
	SLOG(LOG_DEBUG, TAG_VCM, "Speech detected callback called");

	return EINA_FALSE;
}

void __vc_mgr_cb_speech_detected()
{
	__vc_mgr_speech_detected(NULL);

	return;
}

int vc_mgr_set_all_result_cb(vc_mgr_all_result_cb callback, void* user_data)
{
	if (NULL == callback)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Set result callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Set result callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_mgr_client_set_all_result_cb(g_vc_m, callback, user_data);

	SLOG(LOG_DEBUG, TAG_VCM, "[SUCCESS] Set all result callback");
	
	return 0;
}

int vc_mgr_unset_all_result_cb()
{
	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Unset result callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Unset result callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_mgr_client_set_all_result_cb(g_vc_m, NULL, NULL);

	return 0;
}

int vc_mgr_set_result_cb(vc_result_cb callback, void* user_data)
{
	if (NULL == callback)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Set result callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Set result callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_mgr_client_set_result_cb(g_vc_m, callback, user_data);
	
	return 0;
}

int vc_mgr_unset_result_cb()
{
	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Unset result callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Unset result callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_mgr_client_set_result_cb(g_vc_m, NULL, NULL);

	return 0;
}

static Eina_Bool __vc_mgr_notify_error(void *data)
{
	vc_h vc_m = (vc_h)data;

	vc_error_cb callback = NULL;
	void* user_data = NULL;
	int reason;

	vc_mgr_client_get_error_cb(vc_m, &callback, &user_data);
	vc_mgr_client_get_error(vc_m, &reason);

	if (NULL != callback) {
		vc_mgr_client_use_callback(vc_m);
		callback(reason, user_data);
		vc_mgr_client_not_use_callback(vc_m);
		SLOG(LOG_DEBUG, TAG_VCM, "Error callback is called");
	} else {
		SLOG(LOG_WARN, TAG_VCM, "[WARNING] Error callback is null");
	}  

	return EINA_FALSE;
}

int __vc_mgr_cb_error(int pid, int reason)
{
	if (0 != vc_mgr_client_get_handle(pid, &g_vc_m)) {
		SLOG(LOG_ERROR, TAG_VCM, "Handle is not valid");
		return -1;
	}

	vc_mgr_client_set_error(g_vc_m, reason);
	__vc_mgr_notify_error(g_vc_m);

	return 0;
}

static Eina_Bool __vc_mgr_notify_state_changed(void *data)
{
	vc_state_changed_cb changed_callback = NULL;
	void* user_data;

	vc_mgr_client_get_state_changed_cb(g_vc_m, &changed_callback, &user_data);

	vc_state_e current_state;
	vc_state_e before_state;

	vc_mgr_client_get_before_state(g_vc_m, &current_state, &before_state);

	if (NULL != changed_callback) {
		vc_mgr_client_use_callback(g_vc_m);
		changed_callback(before_state, current_state, user_data);
		vc_mgr_client_not_use_callback(g_vc_m);
		SLOG(LOG_DEBUG, TAG_VCM, "State changed callback is called");
	} else {
		SLOG(LOG_WARN, TAG_VCM, "[WARNING] State changed callback is null");
	}

	return EINA_FALSE;
}

int vc_mgr_set_state_changed_cb(vc_state_changed_cb callback, void* user_data)
{
	if (callback == NULL)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Set state changed callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Set state changed callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_mgr_client_set_state_changed_cb(g_vc_m, callback, user_data);

	return 0;
}

int vc_mgr_unset_state_changed_cb()
{
	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Unset state changed callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Unset state changed callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_mgr_client_set_state_changed_cb(g_vc_m, NULL, NULL);

	return 0;
}

int vc_mgr_set_service_state_changed_cb(vc_service_state_changed_cb callback, void* user_data)
{
	if (callback == NULL)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Set state changed callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Set state changed callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_mgr_client_set_service_state_changed_cb(g_vc_m, callback, user_data);

	return 0;
}

int vc_mgr_unset_service_state_changed_cb()
{
	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Unset state changed callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Unset state changed callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_mgr_client_set_service_state_changed_cb(g_vc_m, NULL, NULL);
	return 0;
}

int vc_mgr_set_speech_detected_cb(vc_mgr_begin_speech_detected_cb callback, void* user_data)
{
	if (callback == NULL)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Set state changed callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Set speech detected callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_mgr_client_set_speech_detected_cb(g_vc_m, callback, user_data);

	return 0;
}

int vc_mgr_unset_speech_detected_cb()
{
	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Unset state changed callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Unset state changed callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_mgr_client_set_speech_detected_cb(g_vc_m, NULL, NULL);
	return 0;
}

int vc_mgr_set_current_language_changed_cb(vc_current_language_changed_cb callback, void* user_data)
{
	if (NULL == callback)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Set current language changed : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Set current language changed : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_mgr_client_set_current_lang_changed_cb(g_vc_m, callback, user_data);

	return 0;
}

int vc_mgr_unset_current_language_changed_cb()
{
	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Unset current language changed : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Unset current language changed : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_mgr_client_set_current_lang_changed_cb(g_vc_m, NULL, NULL);

	return 0;
}

int vc_mgr_set_error_cb(vc_error_cb callback, void* user_data)
{
	if (NULL == callback)
		return VC_ERROR_INVALID_PARAMETER;

	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Set error callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Set error callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_mgr_client_set_error_cb(g_vc_m, callback,  user_data);

	return 0;
}

int vc_mgr_unset_error_cb()
{
	vc_state_e state;
	if (0 != vc_mgr_client_get_client_state(g_vc_m, &state)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Unset error callback : A handle is not available");
		return VC_ERROR_INVALID_STATE;
	}

	/* check state */
	if (state != VC_STATE_INITIALIZED) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Unset error callback : Current state is not 'Initialized'");
		return VC_ERROR_INVALID_STATE;
	}

	vc_mgr_client_set_error_cb(g_vc_m, NULL, NULL);

	return 0;
}

static bool __vc_mgr_check_demandable_client(int pid)
{
	if (0 == g_slist_length(g_demandable_client_list)) {
		SLOG(LOG_WARN, TAG_VCM, "[WARNING] No demandable clients");
		return false;
	}

	char appid[128] = {'\0', };
	aul_app_get_appid_bypid(pid, appid, sizeof(appid));

	if (0 >= strlen(appid)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] No appid");
		return false;
	}
	SLOG(LOG_DEBUG, TAG_VCM, "[CHECK] Appid - %s", appid);

	GSList *iter = NULL;
	vc_demandable_client_s* temp_client;
	iter = g_slist_nth(g_demandable_client_list, 0);

	while (NULL != iter) {
		temp_client = iter->data;

		if (NULL != temp_client) {
			if (NULL != temp_client->appid) {
				if (!strcmp(temp_client->appid, appid)) {
					SLOG(LOG_DEBUG, TAG_VCM, "pid(%d) is available", pid);
					return true;
				}
			}
		}

		iter = g_slist_next(iter);
	}

	return false;
}

/* Authority */
int __vc_mgr_request_auth_enable(int pid)
{
	if (false == __vc_mgr_check_demandable_client(pid)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Not demandable client");
		return VC_ERROR_INVALID_PARAMETER;
	}

	/* check already authorized */
	if (true == vc_mgr_client_is_authorized_client(g_vc_m, pid)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Already authorized");
		return VC_ERROR_INVALID_PARAMETER;
	}

	/* add authorized list */
	if (0 != vc_mgr_client_add_authorized_client(g_vc_m, pid)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to add authorized client");
		return VC_ERROR_OPERATION_FAILED;
	}

	/* foreground check */
	int fore_pid = 0;
	if (0 != vc_config_mgr_get_foreground(&fore_pid)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to get foreground");
		return VC_ERROR_OPERATION_FAILED;
	}

	if (pid == fore_pid) {
		vc_mgr_client_set_valid_authorized_client(g_vc_m, pid);
	}
	
	return 0;
}

int __vc_mgr_request_auth_disable(int pid)
{
	/* check authorized */
	if (false == vc_mgr_client_is_authorized_client(g_vc_m, pid)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] No authorized");
		return VC_ERROR_INVALID_PARAMETER;
	}

	/* remove authorized list */
	if (0 != vc_mgr_client_remove_authorized_client(g_vc_m, pid)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to remove authorized client");
		return VC_ERROR_OPERATION_FAILED;
	}

	/* check authority valid */
	if (true == vc_mgr_client_is_valid_authorized_client(g_vc_m, pid)) {
		SLOG(LOG_DEBUG, TAG_VCM, "Valid authorized client is removed");
		if (0 != vc_mgr_client_set_valid_authorized_client(g_vc_m, -1)) {
			SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to set valid authorized client");
			return VC_ERROR_OPERATION_FAILED;
		}
	}

	return 0;
}

static Eina_Bool __request_auth_start(void* data)
{
	SLOG(LOG_DEBUG, TAG_VCM, "Request Start");

	if (0 != vc_mgr_client_set_start_by_client(g_vc_m, true)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to set start by client");
	}

	if (0 != vc_mgr_start(true, false)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Request start is failed");
		/* TODO - Error handling? */
	}

	if (0 != vc_mgr_client_set_start_by_client(g_vc_m, false)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to set start by client");
	}


	return EINA_FALSE;
}

int __vc_mgr_request_auth_start(int pid)
{
	/* check authorized */
	if (false == vc_mgr_client_is_valid_authorized_client(g_vc_m, pid)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] No valid authorized client");
		return VC_ERROR_INVALID_PARAMETER;
	}

	/* add timer for start recording */
	ecore_timer_add(0, __request_auth_start, NULL);
	
	return 0;
}

static Eina_Bool __request_auth_stop(void* data)
{
	SLOG(LOG_DEBUG, TAG_VCM, "Request Stop");

	if (0 != vc_mgr_stop()) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Request stop is failed");
		/* TODO - Error handling? */
	}

	return EINA_FALSE;
}

int __vc_mgr_request_auth_stop(int pid)
{
	/* check authorized */
	if (false == vc_mgr_client_is_valid_authorized_client(g_vc_m, pid)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] No valid authorized client");
		return VC_ERROR_INVALID_PARAMETER;
	}

	/* add timer for start recording */
	ecore_timer_add(0, __request_auth_stop, NULL);

	return 0;
}

static Eina_Bool __request_auth_cancel(void* data)
{
	SLOG(LOG_DEBUG, TAG_VCM, "Request Cancel");

	if (0 != vc_mgr_cancel()) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Request cancel is failed");
		/* TODO - Error handling? */
	}

	return EINA_FALSE;
}

int __vc_mgr_request_auth_cancel(int pid)
{
	/* check authorized */
	if (false == vc_mgr_client_is_valid_authorized_client(g_vc_m, pid)) {
		SLOG(LOG_ERROR,  TAG_VCM, "[ERROR] No valid authorized client");
		return VC_ERROR_INVALID_PARAMETER;
	}

	/* add timer for start recording */
	ecore_timer_add(0, __request_auth_cancel, NULL);

	return 0;
}