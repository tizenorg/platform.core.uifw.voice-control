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

#include <dirent.h>
#include <sound_manager.h>

#include "vc_info_parser.h"
#include "vcd_main.h"
#include "vcd_server.h"
#include "vcd_client_data.h"

#include "vcd_engine_agent.h"
#include "vcd_config.h"
#include "vcd_recorder.h"
#include "vcd_dbus.h"

#include "voice_control_command_expand.h"

/*
* VC Server static variable
*/
static bool	g_is_engine;

/*
* VC Server Internal Functions
*/
static Eina_Bool __stop_by_silence(void *data)
{
	SLOG(LOG_DEBUG, TAG_VCD, "===== Silence Detected ");

	vcd_server_mgr_stop();

	SLOG(LOG_DEBUG, TAG_VCD, "=====");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");
	return EINA_FALSE;
}

static Eina_Bool __cancel_by_interrupt(void *data)
{
	SLOG(LOG_DEBUG, TAG_VCD, "===== Cancel by interrupt");

	vcd_server_mgr_cancel();

	SLOG(LOG_DEBUG, TAG_VCD, "=====");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");
	return EINA_FALSE;
}

static Eina_Bool __restart_engine(void *data)
{
	SLOG(LOG_DEBUG, TAG_VCD, "===== Restart by no result");

	/* Restart recognition */
	int ret = vcd_engine_recognize_start(true);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to restart recognition : result(%d)", ret);
		return EINA_FALSE;
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Server Success] Start engine");

	if (VCD_RECOGNITION_MODE_RESTART_AFTER_REJECT == vcd_client_get_recognition_mode()) {
		vcd_config_set_service_state(VCD_STATE_RECORDING);
		vcdc_send_service_state(VCD_STATE_RECORDING);
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Server Success] Restart recognition");

	SLOG(LOG_DEBUG, TAG_VCD, "=====");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");
	return EINA_FALSE;
}

static int __server_recorder_callback(const void* data, const unsigned int length)
{
	vcd_state_e state = vcd_config_get_service_state();
	if (VCD_STATE_RECORDING != state) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Skip by engine processing");
		return 0;
	}

	vcp_speech_detect_e speech_detected = VCP_SPEECH_DETECT_NONE;
	int ret;

	ret = vcd_engine_recognize_audio(data, length, &speech_detected);

	if (0 > ret) {
		/* Error */
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to set recording data to engine(%d)", ret);
		ecore_timer_add(0, __cancel_by_interrupt, NULL);
		return 0;
	}

	if (VCP_SPEECH_DETECT_BEGIN == speech_detected) {
		if (-1 != vcd_client_manager_get_pid()) {
			/* Manager client is available */
			if (0 != vcdc_send_speech_detected(vcd_client_manager_get_pid())) {
				SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Fail to send speech detected");
			}
		}
	} else if (VCP_SPEECH_DETECT_END == speech_detected) {
		if (VCD_RECOGNITION_MODE_STOP_BY_SILENCE == vcd_client_get_recognition_mode()) {
			/* silence detected */
			ecore_timer_add(0, __stop_by_silence, NULL);
		} else if (VCD_RECOGNITION_MODE_RESTART_AFTER_REJECT == vcd_client_get_recognition_mode()) {
			/* Stop engine recognition */
			int ret = vcd_engine_recognize_stop();
			if (0 != ret) {
				SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to stop recognition : %d", ret);
			}
			vcd_config_set_service_state(VCD_STATE_PROCESSING);
			vcdc_send_service_state(VCD_STATE_PROCESSING);

			SLOG(LOG_DEBUG, TAG_VCD, "[Server] Stop engine only by silence");
		} else if (VCD_RECOGNITION_MODE_RESTART_CONTINUOUSLY == vcd_client_get_recognition_mode()) {
			/* Stop engine recognition */
			int ret = vcd_engine_recognize_stop();
			if (0 != ret) {
				SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to stop recognition : %d", ret);
			}
		}
	}

	return 0;
}

void __server_recorder_interrupt_callback()
{
	SLOG(LOG_DEBUG, TAG_VCD, "===== Cancel by sound interrupt");

	ecore_timer_add(0, __cancel_by_interrupt, NULL);

	SLOG(LOG_DEBUG, TAG_VCD, "=====");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");
}

static void __config_lang_changed_cb(const char* current_lang, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_VCD, "===== Change language ");

	/* Current state is recording */
	vcd_state_e state = vcd_config_get_service_state();
	if (VCD_STATE_RECORDING == state || VCD_STATE_PROCESSING == state) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Current state is 'Recording'. Cancel recognition");
		vcd_server_mgr_cancel();
	}

	int ret;
	ret = vcd_engine_set_current_language(current_lang);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to set language of engine : %d", ret);
	}

	SLOG(LOG_DEBUG, TAG_VCD, "=====");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return;
}

static void __config_foreground_changed_cb(int previous, int current, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_VCD, "===== Change foreground");

	SLOG(LOG_DEBUG, TAG_VCD, "Foreground pid(%d)", current);

	if (VC_NO_FOREGROUND_PID != current) {
		/* Foreground app is changed */
		vcd_state_e state = vcd_config_get_service_state();
		if (VCD_STATE_RECORDING == state) {
			SLOG(LOG_DEBUG, TAG_VCD, "[Server] Foreground pid(%d) is changed. Cancel recognition", current);
			ecore_timer_add(0, __cancel_by_interrupt, NULL);
		}
	}

	SLOG(LOG_DEBUG, TAG_VCD, "=====");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return;
}

static Eina_Bool __vcd_send_selected_result(void *data)
{
	GSList* pid_list = NULL;
	if (0 != vc_info_parser_get_result_pid_list(&pid_list)) {
		SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Fail to get pid list. No result");
	} else {
		if (0 < g_slist_length(pid_list)) {
			GSList* iter = NULL;
			vc_cmd_s* temp_cmd = NULL;
			int ret = 0;
			int count = 0;

			iter = g_slist_nth(pid_list, 0);
			while (NULL != iter) {
				temp_cmd = iter->data;

				if (NULL != temp_cmd) {
					count = 0;
					do {
						/* send result noti */
						ret = vcdc_send_result(temp_cmd->pid, temp_cmd->type);
						if (0 != ret) {
							SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Fail to send result");
							if (VCD_ERROR_TIMED_OUT != ret) {
								break;
							}
						} else {
							SLOG(LOG_DEBUG, TAG_VCD, "[Server] Send result : pid(%d) type(%d)", temp_cmd->pid, temp_cmd->type);
						}
						count++;

						if (100 == count)	break;
						/* While is retry code */
					} while (0 != ret);
					free(temp_cmd);
				}

				pid_list = g_slist_remove_link(pid_list, iter);
				iter = g_slist_nth(pid_list, 0);
			}
		}
	}

	if (VCD_RECOGNITION_MODE_RESTART_CONTINUOUSLY != vcd_client_get_recognition_mode()) {
		vcd_config_set_service_state(VCD_STATE_READY);
		vcdc_send_service_state(VCD_STATE_READY);
	}

	return EINA_FALSE;
}

static void __vcd_server_result_cb(vcp_result_event_e event, int* result_id, int count, const char* all_result,
								   const char* non_fixed_result, const char* msg, void *user_data)
{
	if (VCD_STATE_PROCESSING != vcd_config_get_service_state()) {
		if (VCD_RECOGNITION_MODE_RESTART_CONTINUOUSLY != vcd_client_get_recognition_mode()) {
			SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Current state is not 'Processing' and mode is not 'Restart continuously'");
			return;
		}
	}

	vc_info_parser_unset_result(vcd_client_manager_get_exclusive());

	SLOG(LOG_DEBUG, TAG_VCD, "[Server] Event(%d), Text(%s) Nonfixed(%s) Msg(%s) Result count(%d)", 
		event, all_result, non_fixed_result, msg, count);

	if (VCD_RECOGNITION_MODE_RESTART_AFTER_REJECT == vcd_client_get_recognition_mode()) {
		if (VCP_RESULT_EVENT_REJECTED == event) {
			SLOG(LOG_DEBUG, TAG_VCD, "[Server] Restart by no or rejected result");
			/* If no result and restart option is ON */
			/* Send reject message */
			bool temp = vcd_client_manager_get_exclusive();
			vc_info_parser_set_result(all_result, event, msg, NULL, temp);
			if (0 != vcdc_send_result_to_manager(vcd_client_manager_get_pid(), VC_RESULT_TYPE_NOTIFICATION)) {
				SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Fail to send result");
			}

			ecore_timer_add(0, __restart_engine, NULL);
			return;
		}
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Stop recorder due to success");
		vcd_recorder_stop();
	} else if (VCD_RECOGNITION_MODE_RESTART_CONTINUOUSLY == vcd_client_get_recognition_mode()) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Restart continuously");
		/* Restart option is ON */
		ecore_timer_add(0, __restart_engine, NULL);
		if (VCP_RESULT_EVENT_REJECTED == event) {
			bool temp = vcd_client_manager_get_exclusive();
			vc_info_parser_set_result(all_result, event, msg, NULL, temp);
			if (0 != vcdc_send_result_to_manager(vcd_client_manager_get_pid(), VC_RESULT_TYPE_NOTIFICATION)) {
				SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Fail to send result");
			}
			return;
		}
	}

	/* No result */
	if (NULL == result_id) {
		/* No result */
		if (NULL != all_result) {
			SLOG(LOG_DEBUG, TAG_VCD, "[Server] Engine result is no command : %s", all_result);
			bool temp = vcd_client_manager_get_exclusive();
			vc_info_parser_set_result(all_result, event, msg, NULL, temp);
		}

		int pid = vcd_client_widget_get_foreground_pid();
		if (-1 != pid) {
			if (NULL != all_result) {
				/* Send result text to widget */
				vcdc_send_result(pid, VC_COMMAND_TYPE_WIDGET);
			}

			SLOG(LOG_DEBUG, TAG_VCD, "[Server] Request tooltip hide");
			/* Send to hide tooltip */
			vcdc_send_show_tooltip(pid, false);
		}

		if (-1 != vcd_client_manager_get_pid()) {
			/* Manager client is available */
			if (0 != vcdc_send_result_to_manager(vcd_client_manager_get_pid(), VC_RESULT_TYPE_NORMAL)) {
				SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Fail to send result");
			}
		}

		vcd_client_manager_set_exclusive(false);

		return;
	}

	/* Normal result */
	SLOG(LOG_DEBUG, TAG_VCD, "[Server] === Get engine result ===");

	int ret = -1;
	vc_cmd_s* temp_cmd = NULL;
	vc_cmd_list_h vc_cmd_list = NULL;

	if (0 != vc_cmd_list_create(&vc_cmd_list)) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Fail to create command list");
		vcd_client_manager_set_exclusive(false);
		vcd_config_set_service_state(VCD_STATE_READY);
		vcdc_send_service_state(VCD_STATE_READY);
		return;
	}

	int i = 0;
	for (i = 0; i < count; i++) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server]   [%d] Result ID(%d)", i, result_id[i]);

		if (result_id[i] < 0) {
			SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Result ID(%d) is NOT valid", result_id[i]);
			continue;
		}

		ret = vcd_client_get_cmd_from_result_id(result_id[i], &temp_cmd);
		if (0 == ret && NULL != temp_cmd) {
			switch (temp_cmd->format) {
			case VC_CMD_FORMAT_FIXED:
				/* Nonfixed result is NOT valid */
				break;
			case VC_CMD_FORMAT_FIXED_AND_EXTRA:
				if (NULL == temp_cmd->parameter) {
					if (NULL != non_fixed_result) {
						temp_cmd->parameter = strdup(non_fixed_result);
					}
				} else {
					SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Command type is NOT vaild. Parameter (%s)", temp_cmd->parameter);
				}
				break;
			case VC_CMD_FORMAT_EXTRA_AND_FIXED:
				if (NULL == temp_cmd->command) {
					if (NULL != non_fixed_result) {
						temp_cmd->command = strdup(non_fixed_result);
					}
				} else {
					SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Command type is NOT vaild. Command (%s)", temp_cmd->command);
				}

				break;
			default:
				SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Unknown command type : %d", temp_cmd->type);
			}

			temp_cmd->id = i;
			if (0 != vc_cmd_list_add(vc_cmd_list, (vc_cmd_h)temp_cmd)) {
				SLOG(LOG_DEBUG, TAG_VCD, "Fail to add command to list");
				vc_cmd_destroy((vc_cmd_h)temp_cmd);
			}
		} else {
			SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] NOT found matached result(%d)", result_id[i]);
		}
	}

	vc_cmd_print_list(vc_cmd_list);

	SLOG(LOG_DEBUG, TAG_VCD, "[Server] =========================");

	int result_count = 0;
	vc_cmd_list_get_count(vc_cmd_list, &result_count);

	if (false == vcd_client_manager_get_exclusive()) {
		int pid = vcd_client_widget_get_foreground_pid();
		if (-1 != pid) {
			SLOG(LOG_DEBUG, TAG_VCD, "[Server] Request tooltip hide");
			vcdc_send_show_tooltip(pid, false);
		}

		vc_info_parser_set_result(all_result, event, msg, vc_cmd_list, false);

		if (-1 != vcd_client_manager_get_pid()) {
			/* Manager client is available */
			if (0 != vcdc_send_result_to_manager(vcd_client_manager_get_pid(), VC_RESULT_TYPE_NORMAL)) {
				SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Fail to send result");
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCD, "[Server] Manager is NOT available. Send result to client directly");
			/* Send result to client */
			ecore_timer_add(0, __vcd_send_selected_result, NULL);
		}
	} else {
		/* exclusive command */
		vc_info_parser_set_result(all_result, event, msg, vc_cmd_list, true);

		if (-1 != vcd_client_manager_get_pid()) {
			/* Manager client is available */
			if (0 != vcdc_send_result_to_manager(vcd_client_manager_get_pid(), VC_RESULT_TYPE_NORMAL)) {
				SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Fail to send result");
			}
		} else {
			SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Manager is NOT available");
		}

		vcd_client_manager_set_exclusive(false);
	}

	vc_cmd_list_destroy(vc_cmd_list, true);

	return;
}

/*
* vcd server Interfaces
*/
static void __vcd_file_clean_up()
{
	SLOG(LOG_DEBUG, TAG_VCD, "== Old file clean up == ");

	DIR *dp = NULL;
	int ret = -1;
	struct dirent entry;
	struct dirent *dirp = NULL;

	dp = opendir(VC_RUNTIME_INFO_ROOT);
	if (dp == NULL) {
		SLOG(LOG_ERROR, TAG_VCD, "[File message WARN] Fail to open path : %s", VC_RUNTIME_INFO_ROOT);
		return;
	}

	char remove_path[256] = {0, };
	do {
		ret = readdir_r(dp, &entry, &dirp);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_VCD, "[File ERROR] Fail to read directory");
			break;
		}

		if (NULL != dirp) {
			if (!strncmp("vc_", dirp->d_name, strlen("vc_"))) {
				memset(remove_path, 0, 256);
				snprintf(remove_path, 256, "%s/%s", VC_RUNTIME_INFO_ROOT, dirp->d_name);

				/* Clean up code */
				if (0 != remove(remove_path)) {
					SLOG(LOG_WARN, TAG_VCD, "[File message WARN] Fail to remove file : %s", remove_path);
				} else {
					SLOG(LOG_DEBUG, TAG_VCD, "[File message] Remove file : %s", remove_path);
				}
			}
		}
	} while (NULL != dirp);

	closedir(dp);

	return;
}

int vcd_initialize()
{
	int ret = 0;

	/* Remove old file */
	__vcd_file_clean_up();

	/* initialize modules */
	ret = vcd_config_initialize(__config_lang_changed_cb, __config_foreground_changed_cb, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server WARNING] Fail to initialize config.");
	}

	vcd_config_set_service_state(VCD_STATE_NONE);

	ret = vcd_engine_agent_init(__vcd_server_result_cb);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to engine agent initialize : result(%d)", ret);
		return ret;
	}

	if (0 != vcd_recorder_create(__server_recorder_callback, __server_recorder_interrupt_callback)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to create recorder");
		return VCD_ERROR_OPERATION_FAILED;
	}

	/* Find engine */
	ret = vcd_engine_agent_initialize_current_engine();
	if (0 != ret) {
		if (VCD_ERROR_ENGINE_NOT_FOUND == ret)
			SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] There is No Voice control engine");
		else
			SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to init engine");

		g_is_engine = false;
	} else {
		g_is_engine = true;
	}

	/* Load engine */
	if (0 != vcd_engine_agent_load_current_engine()) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to load current engine");
		return VCD_ERROR_OPERATION_FAILED;
	}

	/* Initialize manager info */
	vcd_client_manager_unset();

	vcd_config_set_service_state(VCD_STATE_READY);
	vcdc_send_service_state(VCD_STATE_READY);

	SLOG(LOG_DEBUG, TAG_VCD, "[Server SUCCESS] initialize");

	return 0;
}

void vcd_finalize()
{
	vcd_state_e state = vcd_config_get_service_state();
	if (VCD_STATE_READY != state) {
		if (VCD_STATE_RECORDING == state) {
			vcd_recorder_stop();
		}
		vcd_engine_recognize_cancel();
	}
	if (0 != vcd_recorder_destroy()) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to destroy recorder");
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] destroy recorder");
	}

	if (0 != vcd_engine_agent_release()) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to release engine");
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] release engine");
	}

	vcd_config_set_service_state(VCD_STATE_NONE);
	vcdc_send_service_state(VCD_STATE_NONE);

	SLOG(LOG_DEBUG, TAG_VCD, "[Server] mode finalize");

	return;
}

static Eina_Bool __finalize_quit_ecore_loop(void *data)
{
	SLOG(LOG_DEBUG, TAG_VCD, "[Server] quit ecore main loop");
	ecore_main_loop_quit();
	return EINA_FALSE;
}

Eina_Bool vcd_cleanup_client(void *data)
{
	int* client_list = NULL;
	int client_count = 0;
	int result;
	int i = 0;

	if (0 == vcd_client_get_list(&client_list, &client_count)) {
		SLOG(LOG_DEBUG, TAG_VCD, "===== Clean up client ");
		if (NULL != client_list && client_count > 0) {
			for (i = 0; i < client_count; i++) {
				result = vcdc_send_hello(client_list[i], VCD_CLIENT_TYPE_NORMAL);

				if (0 == result) {
					SLOG(LOG_DEBUG, TAG_VCD, "[Server] pid(%d) should be removed.", client_list[i]);
					vcd_server_finalize(client_list[i]);
				} else if (-1 == result) {
					SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Hello result has error");
				}
			}
		}
		SLOG(LOG_DEBUG, TAG_VCD, "=====");
		SLOG(LOG_DEBUG, TAG_VCD, "  ");
	}
	if (NULL != client_list) {
		free(client_list);
		client_list = NULL;
	}

#if 0
	/* If app is in background state, app cannot response message. */
	if (0 == vcd_client_widget_get_list(&client_list, &client_count)) {
		SLOG(LOG_DEBUG, TAG_VCD, "===== Clean up widget");
		if (NULL != client_list && client_count > 0) {
			for (i = 0; i < client_count; i++) {
				result = vcdc_send_hello(client_list[i], VCD_CLIENT_TYPE_WIDGET);

				if (0 == result) {
					SLOG(LOG_DEBUG, TAG_VCD, "[Server] widget pid(%d) should be removed.", client_list[i]);
					vcd_server_widget_finalize(client_list[i]);
				} else if (-1 == result) {
					SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Hello result has error");
				}
			}
		}
		SLOG(LOG_DEBUG, TAG_VCD, "=====");
		SLOG(LOG_DEBUG, TAG_VCD, "  ");
	}

	if (NULL != client_list) {
		free(client_list);
		client_list = NULL;
	}
#endif

	/* manager */

	return EINA_TRUE;
}

int vcd_server_get_service_state()
{
	return vcd_config_get_service_state();
}

/*
* API for manager
*/
int vcd_server_mgr_initialize(int pid)
{
	if (false == g_is_engine) {
		if (0 != vcd_engine_agent_initialize_current_engine()) {
			SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] No Engine");
			g_is_engine = false;
			return VCD_ERROR_ENGINE_NOT_FOUND;
		} else {
			g_is_engine = true;
		}
	}

	/* check if pid is valid */
	if (false == vcd_client_manager_is_valid(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] The pid(%d) is already exist", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}

	/* Add client information to client manager */
	if (0 != vcd_client_manager_set(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to add manager");
		return VCD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Server Success] Manager initialize : pid(%d)", pid);

	return VCD_ERROR_NONE;
}

int vcd_server_mgr_finalize(int pid)
{
	/* check if pid is valid */
	if (false == vcd_client_manager_is_valid(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] The manager pid(%d) is NOT valid", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}

	/* Remove manager information */
	if (0 != vcd_client_manager_unset()) {
		SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Fail to delete client");
	}

	if (0 == vcd_client_get_ref_count()) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Connected client list is empty");
		ecore_timer_add(0, __finalize_quit_ecore_loop, NULL);
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Server Success] Manager Finalize : pid(%d)", pid);

	return VCD_ERROR_NONE;
}

int vcd_server_mgr_set_command(int pid)
{
	if (0 != vcd_client_manager_set_command(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] The manager pid(%d) is NOT valid", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}
	return VCD_ERROR_NONE;
}

int vcd_server_mgr_unset_command(int pid)
{
	if (0 != vcd_client_manager_unset_command(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] The manager pid(%d) is NOT valid", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}
	return VCD_ERROR_NONE;
}

int vcd_server_mgr_set_demandable_client(int pid)
{
	/* check if pid is valid */
	if (false == vcd_client_manager_is_valid(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] The manager pid(%d) is NOT valid", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}

	GSList* client_list = NULL;
	if (0 != vc_info_parser_get_demandable_clients(&client_list)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to get demandable client");
		return VCD_ERROR_OPERATION_FAILED;
	}

	/* Save client list */
	if (0 != vcd_client_manager_set_demandable_client(pid, client_list)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to set demandable client list");
		return VCD_ERROR_OPERATION_FAILED;
	}

	return VCD_ERROR_NONE;
}

int vcd_server_mgr_set_audio_type(int pid, const char* audio_type)
{
	/* check if pid is valid */
	if (false == vcd_client_manager_is_valid(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] The manager pid(%d) is NOT valid", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	vcp_audio_type_e type = VCP_AUDIO_TYPE_PCM_S16_LE;
	int rate = 16000;
	int channel = 1;

	ret = vcd_engine_get_audio_format(audio_type, &type, &rate, &channel);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to get audio format : %d", ret);
		return VCD_ERROR_OPERATION_FAILED;
	}

	ret = vcd_recorder_set(audio_type, type, rate, channel);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to set audio in type : %d", ret);
		return VCD_ERROR_OPERATION_FAILED;
	}

	return VCD_ERROR_NONE;
}

int vcd_server_mgr_get_audio_type(int pid, char** audio_type)
{
	/* check if pid is valid */
	if (false == vcd_client_manager_is_valid(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] The manager pid(%d) is NOT valid", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}

	int ret = vcd_recorder_get(audio_type);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to get audio in type : %d", ret);
		return VCD_ERROR_OPERATION_FAILED;
	}

	return VCD_ERROR_NONE;
}

int vcd_server_mgr_set_client_info(int pid)
{
	/* check if pid is valid */
	if (false == vcd_client_manager_is_valid(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] The manager pid(%d) is NOT valid", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}

	int ret = vcd_client_save_client_info();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to save client info : %d", ret);
		return VCD_ERROR_OPERATION_FAILED;
	}

	return VCD_ERROR_NONE;
}

static int __start_internal_recognition()
{
	int ret;

	/* 2. Get commands */
	ret = vcd_client_command_collect_command();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to collect command : %d", ret);
		return VCD_ERROR_OPERATION_FAILED;
	}

	ret = vcd_client_get_length();
	if (0 == ret) {
		SLOG(LOG_WARN, TAG_VCD, "[Server WARNIING] No current command : %d", ret);
		return VCD_ERROR_OPERATION_FAILED;
	}

	/* 3. Set command to engine */
	ret = vcd_engine_set_commands();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to collect command : %d", ret);
		return VCD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Server Success] Set command");

	/* 4. start recognition */
	ret = vcd_engine_recognize_start(true);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to start recognition : result(%d)", ret);
		return VCD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Server Success] Start engine");

	/* 5. recorder start */
	ret = vcd_recorder_start();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to start recorder : result(%d)", ret);
		vcd_engine_recognize_cancel();
		return ret;
	}

	vcd_config_set_service_state(VCD_STATE_RECORDING);
	vcdc_send_service_state(VCD_STATE_RECORDING);

	SLOG(LOG_DEBUG, TAG_VCD, "[Server Success] Start recognition");

	return 0;
}

static Eina_Bool __vcd_request_show_tooltip(void *data)
{
	int pid = vcd_client_widget_get_foreground_pid();
	if (-1 != pid) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Request tooltip show and widget command");
		vcdc_send_show_tooltip(pid, (bool)data);
	}

	return EINA_FALSE;
}

int vcd_server_mgr_start(vcd_recognition_mode_e recognition_mode, bool exclusive_cmd, bool start_by_client)
{
	/* 1. check current state */
	vcd_state_e state = vcd_config_get_service_state();

	if (VCD_STATE_READY != state) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Current state is not ready");
		return VCD_ERROR_INVALID_STATE;
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Server] set recognition mode = %d", recognition_mode);
	vcd_client_set_recognition_mode(recognition_mode);

	if (false == exclusive_cmd) {
		/* Notify show tooltip */
		int pid = vcd_client_widget_get_foreground_pid();
		if (-1 != pid) {
			SLOG(LOG_DEBUG, TAG_VCD, "[Server] Request tooltip show and widget command");
			ecore_timer_add(0, __vcd_request_show_tooltip, (void*)true);
			return 0;
		}
	} else {
		vcd_client_manager_set_exclusive(exclusive_cmd);
	}

	int fg_pid = -1;
	if (true == start_by_client) {
		/* Get foreground pid */
		if (0 != vcd_config_get_foreground(&fg_pid)) {
			SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to get foreground");
		}

		/* Set client exclusive option */
		if (0 != vcd_client_set_exclusive_command(fg_pid)) {
			SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to set exclusive command");
		}
	}

	int ret = __start_internal_recognition();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to start recongition : %d", ret);
		return ret;
	}

	if (true == start_by_client) {
		vcd_client_unset_exclusive_command(fg_pid);
	}

	return VCD_ERROR_NONE;
}

int vcd_server_mgr_stop()
{
	/* 1. Check current state is recording */
	if (VCD_STATE_RECORDING != vcd_config_get_service_state()) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Current state is not recording");
		return VCD_ERROR_INVALID_STATE;
	}

	/* 2. Stop recorder */
	vcd_recorder_stop();

	/* 3. Stop engine recognition */
	int ret = vcd_engine_recognize_stop();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to stop recognition : %d", ret);
	}

	/* 4. Set original mode */
	vcd_config_set_service_state(VCD_STATE_PROCESSING);
	vcdc_send_service_state(VCD_STATE_PROCESSING);

	return VCD_ERROR_NONE;
}

int vcd_server_mgr_cancel()
{
	/* 1. Check current state */
	vcd_state_e state = vcd_config_get_service_state();
	if (VCD_STATE_RECORDING != state && VCD_STATE_PROCESSING != state) {
		SLOG(LOG_WARN, TAG_VCD, "[Server ERROR] Current state is not recording or processing");
		return VCD_ERROR_INVALID_STATE;
	}

	/* 2. Stop recorder */
	vcd_recorder_stop();
	/* 3. Cancel engine */
	int ret = vcd_engine_recognize_cancel();
	if (0 != ret) {
		SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Fail to cancel : %d", ret);
	}

	if (false == vcd_client_manager_get_exclusive()) {
		int pid = vcd_client_widget_get_foreground_pid();
		if (-1 != pid) {
			SLOG(LOG_DEBUG, TAG_VCD, "[Server] Request tooltip hide");
			ecore_timer_add(0, __vcd_request_show_tooltip, (void*)false);
		}
	} else {
		vcd_client_manager_set_exclusive(false);
	}

	/* 4. Set state */
	vcd_config_set_service_state(VCD_STATE_READY);
	vcdc_send_service_state(VCD_STATE_READY);

	return VCD_ERROR_NONE;
}


int vcd_server_mgr_result_select()
{
	__vcd_send_selected_result(NULL);

	return VCD_ERROR_NONE;
}

/*
* VC Server Functions for Client
*/
int vcd_server_initialize(int pid)
{
	if (false == g_is_engine) {
		if (0 != vcd_engine_agent_initialize_current_engine()) {
			SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] No Engine");
			g_is_engine = false;
			return VCD_ERROR_ENGINE_NOT_FOUND;
		} else {
			g_is_engine = true;
		}
	}

	if (false == vcd_engine_is_available_engine()) {
		if (0 != vcd_engine_agent_initialize_current_engine()) {
			SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] No Engine");
			return VCD_ERROR_ENGINE_NOT_FOUND;
		}
	}

	/* check if pid is valid */
	if (true == vcd_client_is_available(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] The pid is already exist");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	/* Add client information to client manager */
	if (0 != vcd_client_add(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to add client info");
		return VCD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Server Success] Client Initialize : pid(%d)", pid);

	return VCD_ERROR_NONE;
}

int vcd_server_finalize(int pid)
{
	/* check if pid is valid */
	if (false == vcd_client_is_available(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] pid is NOT valid ");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	/* Remove client information */
	if (0 != vcd_client_delete(pid)) {
		SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Fail to delete client");
	}

	if (0 == vcd_client_get_ref_count()) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Connected client list is empty");
		ecore_timer_add(0, __finalize_quit_ecore_loop, NULL);
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Server Success] Client Finalize : pid(%d)", pid);

	return VCD_ERROR_NONE;
}

int vcd_server_set_command(int pid, vc_cmd_type_e cmd_type)
{
	/* check if pid is valid */
	if (false == vcd_client_is_available(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] pid is NOT valid ");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	if (0 != vcd_client_set_command_type(pid, cmd_type)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to set command type : pid(%d), cmd_type(%d)", pid, cmd_type);
		return VCD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int vcd_server_unset_command(int pid, vc_cmd_type_e cmd_type)
{
	/* check if pid is valid */
	if (false == vcd_client_is_available(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] pid is NOT valid ");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	if (0 != vcd_client_unset_command_type(pid, cmd_type)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to unset command type : pid(%d), cmd_type(%d)", pid, cmd_type);
		return VCD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

#if 0
int vcd_server_set_exclusive_command(int pid, bool value)
{
	/* check if pid is valid */
	if (false == vcd_client_is_available(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] pid is NOT valid ");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	if (true == value) {
		if (0 != vcd_client_set_exclusive_command(pid)) {
			SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to set exclusive command : pid(%d)", pid);
			return VCD_ERROR_OPERATION_FAILED;
		}
	} else {
		if (0 != vcd_client_unset_exclusive_command(pid)) {
			SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to unset exclusive command : pid(%d)", pid);
			return VCD_ERROR_OPERATION_FAILED;
		}
	}

	return 0;
}

int vcd_server_request_start(int pid, bool stop_by_silence)
{
	/* check if pid is valid */
	if (false == vcd_client_is_available(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] pid(%d) is NOT forground client", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}

	int ret;
	/* Check current state */
	vcd_state_e state = vcd_config_get_service_state();

	/* Service state should be ready */
	if (VCD_STATE_READY != state) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Current state is not Ready : pid(%d)", pid);
		return VCD_ERROR_INVALID_STATE;
	}

	if (-1 != vcd_client_manager_get_pid()) {
		/* Check current pid is valid */
		if (false == vcd_client_manager_check_demandable_client(pid)) {
			SLOG(LOG_DEBUG, TAG_VCD, "[Server] Current client is NOT available : pid(%d)", pid);
			return VCD_ERROR_INVALID_PARAMETER;
		}
	}

	ret = vcd_server_mgr_start(stop_by_silence, false);
	if (0 != ret) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Fail to start recognition");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	return 0;
}

int vcd_server_request_stop(int pid)
{
	int ret;
	/* Check current state */
	vcd_state_e state = vcd_config_get_service_state();

	/* Service state should be ready */
	if (VCD_STATE_RECORDING != state) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Current state is not Recording : pid(%d)", pid);
		return VCD_ERROR_INVALID_STATE;
	}

	if (-1 != vcd_client_manager_get_pid()) {
		/* Check current pid is valid */
		if (false == vcd_client_manager_check_demandable_client(pid)) {
			SLOG(LOG_DEBUG, TAG_VCD, "[Server] Current client is NOT available : pid(%d)", pid);
			return VCD_ERROR_INVALID_PARAMETER;
		}
	}

	ret = vcd_server_mgr_stop();
	if (0 != ret) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Fail to start recognition");
		return VCD_ERROR_OPERATION_FAILED;
	}

	return VCD_ERROR_NONE;
}

int vcd_server_request_cancel(int pid)
{
	int ret;
	/* Check current state */
	vcd_state_e state = vcd_config_get_service_state();

	/* Service state should be recording or processing */
	if (VCD_STATE_RECORDING != state && VCD_STATE_PROCESSING != state) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Current state is not Recording or Processing : pid(%d)", pid);
		return VCD_ERROR_INVALID_STATE;
	}

	if (-1 != vcd_client_manager_get_pid()) {
		/* Check current pid is valid */
		if (false == vcd_client_manager_check_demandable_client(pid)) {
			SLOG(LOG_DEBUG, TAG_VCD, "[Server] Current client is NOT available : pid(%d)", pid);
			return VCD_ERROR_INVALID_PARAMETER;
		}
	}

	ret = vcd_server_mgr_cancel();
	if (0 != ret) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Fail to start recognition");
		return VCD_ERROR_OPERATION_FAILED;
	}

	return VCD_ERROR_NONE;
}
#endif

/*
* VC Server Functions for Widget lib
*/
int vcd_server_widget_initialize(int pid)
{
	if (false == g_is_engine) {
		if (0 != vcd_engine_agent_initialize_current_engine()) {
			SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] No Engine");
			g_is_engine = false;
			return VCD_ERROR_ENGINE_NOT_FOUND;
		} else {
			g_is_engine = true;
		}
	}

	if (false == vcd_engine_is_available_engine()) {
		if (0 != vcd_engine_agent_initialize_current_engine()) {
			SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] No Engine");
			return VCD_ERROR_ENGINE_NOT_FOUND;
		}
	}

	/* check if pid is valid */
	if (true == vcd_client_widget_is_available(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] The pid is already exist");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	/* Add client information to client manager */
	if (0 != vcd_client_widget_add(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to add client info");
		return VCD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Server Success] Initialize widget : pid(%d)", pid);

	return VCD_ERROR_NONE;
}

int vcd_server_widget_finalize(int pid)
{
	/* check if pid is valid */
	if (false == vcd_client_widget_is_available(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] pid is NOT valid ");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	/* Remove client information */
	if (0 != vcd_client_widget_delete(pid)) {
		SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Fail to delete client");
	}

	if (0 == vcd_client_get_ref_count()) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] connected client list is empty");
		ecore_timer_add(0, __finalize_quit_ecore_loop, NULL);
	}

	return VCD_ERROR_NONE;
}

int vcd_server_widget_start_recording(int pid, bool widget_command)
{
	/* check if pid is valid */
	if (false == vcd_client_widget_is_available(pid)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] pid is NOT valid ");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	if (true == widget_command) {
		vcd_client_widget_set_command(pid);
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] widget command is available");
	} else {
		vcd_client_widget_unset_command(pid);
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] widget command is NOT available");
	}

	int ret = __start_internal_recognition();
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server ERROR] Fail to start recongition : %d", ret);
		ecore_timer_add(0, __vcd_request_show_tooltip, (void*)false);
	}

	return 0;
}

int vcd_server_widget_start(int pid, bool stop_by_silence)
{
	/* check if pid is valid */
	int fore_pid = vcd_client_widget_get_foreground_pid();
	if (pid != fore_pid) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server] pid is NOT foreground");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	/* Check current state */
	vcd_state_e state = vcd_config_get_service_state();

	/* Service state should be ready */
	if (VCD_STATE_READY != state) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Current state is not Ready : pid(%d)", pid);
		return VCD_ERROR_INVALID_STATE;
	}

	vcd_client_set_slience_detection(stop_by_silence);

	/* Notify show tooltip */
	ecore_timer_add(0, __vcd_request_show_tooltip, (void*)true);

	return 0;
}

int vcd_server_widget_stop(int pid)
{
	/* check if pid is valid */
	int fore_pid = vcd_client_widget_get_foreground_pid();
	if (pid != fore_pid) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server] pid is NOT foreground");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	int ret;
	/* Check current state */
	vcd_state_e state = vcd_config_get_service_state();

	/* Service state should be recording */
	if (VCD_STATE_RECORDING != state) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Current service state is not Recording : pid(%d)", pid);
		return VCD_ERROR_INVALID_STATE;
	}

	ret = vcd_server_mgr_stop();
	if (0 != ret) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Fail to start recognition");
		return VCD_ERROR_OPERATION_FAILED;
	}

	return VCD_ERROR_NONE;
}

int vcd_server_widget_cancel(int pid)
{
	/* check if pid is valid */
	int fore_pid = vcd_client_widget_get_foreground_pid();
	if (pid != fore_pid) {
		SLOG(LOG_ERROR, TAG_VCD, "[Server] pid is NOT foreground");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	int ret;
	/* Check current state */
	vcd_state_e state = vcd_config_get_service_state();

	/* Service state should be recording or processing */
	if (VCD_STATE_RECORDING != state && VCD_STATE_PROCESSING != state) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Current state is not Recording or Processing : pid(%d)", pid);
		return VCD_ERROR_INVALID_STATE;
	}

	ret = vcd_server_mgr_cancel();
	if (0 != ret) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Server] Fail to cancel recognition : %d", ret);
		return ret;
	}

	return VCD_ERROR_NONE;
}

