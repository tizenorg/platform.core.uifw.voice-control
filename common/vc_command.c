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

#include <cynara-client.h>
#include <cynara-error.h>
#include <cynara-session.h>
#include <libintl.h>
#include <stdlib.h>
#include <system_info.h>

#include "vc_command.h"
#include "vc_main.h"
#include "voice_control_command.h"
#include "voice_control_command_expand.h"
#include "voice_control_common.h"
#include "voice_control_key_defines.h"

static int g_feature_enabled = -1;

static int g_privilege_allowed = -1;
static cynara *p_cynara = NULL;

static int __vc_cmd_get_feature_enabled()
{
	if (0 == g_feature_enabled) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Voice control feature NOT supported");
		return VC_ERROR_NOT_SUPPORTED;
	} else if (-1 == g_feature_enabled) {
		bool vc_supported = false;
		bool mic_supported = false;
		if (0 == system_info_get_platform_bool(VC_FEATURE_PATH, &vc_supported)) {
			if (0 == system_info_get_platform_bool(VC_MIC_FEATURE_PATH, &mic_supported)) {
				if (false == vc_supported || false == mic_supported) {
					SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Voice control feature NOT supported");
					g_feature_enabled = 0;
					return VC_ERROR_NOT_SUPPORTED;
				}

				g_feature_enabled = 1;
			} else {
				SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Fail to get feature value");
				return VC_ERROR_NOT_SUPPORTED;
			}
		} else {
			SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Fail to get feature value");
			return VC_ERROR_NOT_SUPPORTED;
		}
	}

	return 0;
}

static int __check_privilege_initialize()
{
	int ret = cynara_initialize(&p_cynara, NULL);
	if (CYNARA_API_SUCCESS != ret)
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] fail to initialize");
	
	return ret == CYNARA_API_SUCCESS;
}

static int __check_privilege(const char* uid, const char * privilege)
{
	FILE *fp = NULL;
	char smack_label[1024] = "/proc/self/attr/current";

	if (!p_cynara) {
	    return false;
	}

	fp = fopen(smack_label, "r");
	if (fp != NULL) {
	    if (fread(smack_label, 1, sizeof(smack_label), fp) <= 0)
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] fail to fread");

	    fclose(fp);
	}

	pid_t pid = getpid();
	char *session = cynara_session_from_pid(pid);
	int ret = cynara_check(p_cynara, smack_label, session, uid, privilege);
	SLOG(LOG_DEBUG, TAG_VCCMD, "[Client]cynara_check returned %d(%s)", ret, (CYNARA_API_ACCESS_ALLOWED == ret) ? "Allowed" : "Denied");
	if (session)
	    free(session);

	if (ret != CYNARA_API_ACCESS_ALLOWED)
	    return false;
	return true;
}

static void __check_privilege_deinitialize()
{
	if (p_cynara)
		cynara_finish(p_cynara);
	p_cynara = NULL;
}

static int __vc_cmd_check_privilege()
{
#if 0
        char uid[16];
	int ret = -1;

	if (0 == g_privilege_allowed) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Permission is denied");
		return VC_ERROR_PERMISSION_DENIED;
	} else if (-1 == g_privilege_allowed) {
		if (false == __check_privilege_initialize()){
			SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] privilege initialize is failed");
			return VC_ERROR_PERMISSION_DENIED;
		}
		snprintf(uid, 16, "%d", getuid());
		if (false == __check_privilege(uid, VC_PRIVILEGE)) {
			SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Permission is denied");
			g_privilege_allowed = 0;
			__check_privilege_deinitialize();
			return VC_ERROR_PERMISSION_DENIED;
		}
		__check_privilege_deinitialize();
	}

	g_privilege_allowed = 1;
#endif
	return VC_ERROR_NONE;	
}

int vc_cmd_list_create(vc_cmd_list_h* vc_cmd_list)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_cmd_list) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_list_s* list = (vc_cmd_list_s*)calloc(1, sizeof(vc_cmd_list_s));

	if (NULL == list) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Not enough memory");
		return VC_ERROR_OUT_OF_MEMORY;
	}

	list->index = -1;
	list->list = NULL;

	*vc_cmd_list = (vc_cmd_list_h)list;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[List] list(%p)", *vc_cmd_list);

	return VC_ERROR_NONE;
}

int vc_cmd_list_destroy(vc_cmd_list_h vc_cmd_list, bool release_command)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_cmd_list) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_list_remove_all(vc_cmd_list, release_command);

	vc_cmd_list_s* list = NULL;
	list = (vc_cmd_list_s*)vc_cmd_list;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[List] list(%p)", list);

	if (NULL != list) {
		free(list);
		list = NULL;
	}

	return VC_ERROR_NONE;
}

int vc_cmd_list_get_count(vc_cmd_list_h vc_cmd_list, int* count)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_cmd_list || NULL == count) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Get command count : Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_list_s* list = NULL;
	list = (vc_cmd_list_s*)vc_cmd_list;

	*count = g_slist_length(list->list);

	SLOG(LOG_DEBUG, TAG_VCCMD, "[List] list(%p), count(%d)", list, *count);

	return VC_ERROR_NONE;
}

int vc_cmd_list_add(vc_cmd_list_h vc_cmd_list, vc_cmd_h vc_command)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_cmd_list || NULL == vc_command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_list_s* list = NULL;
	list = (vc_cmd_list_s*)vc_cmd_list;

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	list->list = g_slist_append(list->list, cmd);

	if (1 == g_slist_length(list->list)) {
		list->index = 0;
	}

	SLOG(LOG_DEBUG, TAG_VCCMD, "[List] list(%p), command(%p)", list, cmd);

	return VC_ERROR_NONE;
}

int vc_cmd_list_remove(vc_cmd_list_h vc_cmd_list, vc_cmd_h vc_command)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_cmd_list || NULL == vc_command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_list_s* list = NULL;
	list = (vc_cmd_list_s*)vc_cmd_list;

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[List] list(%p), command(%p)", list, cmd);

	vc_cmd_s* temp_cmd = NULL;
	GSList *iter = NULL;

	iter = g_slist_nth(list->list, 0);

	while (NULL != iter) {
		temp_cmd = iter->data;

		if (NULL != temp_cmd && cmd == temp_cmd) {
			list->list = g_slist_remove(list->list, temp_cmd);
			/*
			if (true == release_command) {
				SLOG(LOG_DEBUG, TAG_VCCMD, "Release command data");
				if (NULL != temp_cmd->command)		free(temp_cmd->command);
				if (NULL != temp_cmd->parameter)	free(temp_cmd->parameter);
				free(temp_cmd);
				temp_cmd = NULL;
			}
			*/
		}

		iter = g_slist_next(iter);
	}

	int count = g_slist_length(list->list);

	if (0 == count) {
		list->index = -1;
	} else if (list->index == count) {
		list->index = count - 1;
	}

	return VC_ERROR_NONE;
}

int vc_cmd_list_remove_all(vc_cmd_list_h vc_cmd_list, bool release_command)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	SLOG(LOG_DEBUG, TAG_VCCMD, "===== Destroy all command");

	if (NULL == vc_cmd_list) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_list_s* list = NULL;
	list = (vc_cmd_list_s*)vc_cmd_list;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[List] list (%p), release command (%s)"
		 , list, release_command ? "true" : "false");

	int count = g_slist_length(list->list);

	int i ;
	vc_cmd_s *temp_cmd;

	for (i = 0; i < count ; i++) {
		temp_cmd = g_slist_nth_data(list->list, 0);

		if (NULL != temp_cmd) {
			list->list = g_slist_remove(list->list, temp_cmd);

			if (true == release_command) {
				SLOG(LOG_DEBUG, TAG_VCCMD, "Free command(%p)", temp_cmd);
				if (NULL != temp_cmd->command)		free(temp_cmd->command);
				if (NULL != temp_cmd->parameter)	free(temp_cmd->parameter);
				free(temp_cmd);
				temp_cmd = NULL;
			}
		}
	}

	list->index = -1;

	SLOG(LOG_DEBUG, TAG_VCCMD, "=====");
	SLOG(LOG_DEBUG, TAG_VCCMD, " ");

	return VC_ERROR_NONE;
}

int vc_cmd_list_foreach_commands(vc_cmd_list_h vc_cmd_list, vc_cmd_list_cb callback, void* user_data)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_cmd_list) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_list_s* list = NULL;
	list = (vc_cmd_list_s*)vc_cmd_list;

	int count = g_slist_length(list->list);
	int i ;

	GSList *iter = NULL;
	vc_cmd_s *temp_cmd;

	iter = g_slist_nth(list->list, 0);

	for (i = 0; i < count; i++) {
		if (NULL == iter) {
			SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] No command in list");
			return VC_ERROR_OPERATION_FAILED;
		}

		temp_cmd = iter->data;

		if (NULL != temp_cmd) {
			if (false == callback((vc_cmd_h)temp_cmd, user_data)) {
				break;
			}

		}
		iter = g_slist_next(iter);
	}

	SLOG(LOG_DEBUG, TAG_VCCMD, "===== Foreach commands Done");

	return VC_ERROR_NONE;
}

int vc_cmd_list_filter_by_type(vc_cmd_list_h original, int type, vc_cmd_list_h* filtered)
{
	SLOG(LOG_DEBUG, TAG_VCCMD, "===== Filter by type");

	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == original) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	if (VC_COMMAND_TYPE_NONE >= type || VC_COMMAND_TYPE_EXCLUSIVE < type) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid type");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_list_s* list = NULL;
	list = (vc_cmd_list_s*)original;

	vc_cmd_list_h temp_list;
	if (0 != vc_cmd_list_create(&temp_list)) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Fail to list create");
		return VC_ERROR_OPERATION_FAILED;
	}

	int count = g_slist_length(list->list);
	int i;

	GSList *iter = NULL;
	vc_cmd_s *iter_cmd;

	iter = g_slist_nth(list->list, 0);

	for (i = 0; i < count; i++) {
		if (NULL == iter) {
			SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] No command in list");
			return VC_ERROR_OPERATION_FAILED;
		}
		if (NULL != iter->data) {
			iter_cmd = iter->data;

			if (NULL != iter_cmd) {
				int iter_type;
				if (0 != vc_cmd_get_type((vc_cmd_h)iter_cmd, &iter_type)) {
					SLOG(LOG_ERROR,TAG_VCCMD, "[ERROR] Fail to get command type");
					continue;
				}

				if (iter_type == type) {
					vc_cmd_h temp_cmd;
					if (0 != vc_cmd_create(&temp_cmd)) {
						SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Fail to create cmd");
						continue;
					}

					memcpy(temp_cmd, iter_cmd, sizeof(vc_cmd_s));
					if (NULL != iter_cmd->command) {
						((vc_cmd_s*)temp_cmd)->command = strdup(iter_cmd->command);
					}
					if (NULL != iter_cmd->parameter) {
						((vc_cmd_s*)temp_cmd)->parameter = strdup(iter_cmd->parameter);
					}

					if (0 != vc_cmd_list_add(temp_list, temp_cmd)) {
						SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Fail to cmd list add");
						vc_cmd_destroy(temp_cmd);
						continue;
					}
				}
			}
		}
		iter = g_slist_next(iter);
	}

	count = 0;
	if (0 != vc_cmd_list_get_count(temp_list, &count)) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Fail to get count");
	} else {
		SLOG(LOG_DEBUG, TAG_VCCMD, "Filtering result : (%d) command", count);
	}

	*filtered = temp_list;

	SLOG(LOG_DEBUG, TAG_VCCMD, "=====");

	return VC_ERROR_NONE;
}

int vc_cmd_list_first(vc_cmd_list_h vc_cmd_list)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_cmd_list) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_list_s* list = NULL;
	list = (vc_cmd_list_s*)vc_cmd_list;

	if (0 == g_slist_length(list->list)) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] List is empty");
		return VC_ERROR_EMPTY;
	}

	list->index = 0;

	return VC_ERROR_NONE;
}

int vc_cmd_list_last(vc_cmd_list_h vc_cmd_list)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_cmd_list) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_list_s* list = NULL;
	list = (vc_cmd_list_s*)vc_cmd_list;

	int count = g_slist_length(list->list);

	if (0 == count) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] List is empty");
		return VC_ERROR_EMPTY;
	} else {
		list->index = count - 1;
		SLOG(LOG_DEBUG, TAG_VCCMD, "[DEBUG] List index : %d", list->index);
	}

	return VC_ERROR_NONE;
}

int vc_cmd_list_next(vc_cmd_list_h vc_cmd_list)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_cmd_list) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_list_s* list = NULL;
	list = (vc_cmd_list_s*)vc_cmd_list;

	int count = g_slist_length(list->list);

	if (list->index < count - 1) {
		list->index = list->index + 1;
		SLOG(LOG_DEBUG, TAG_VCCMD, "[DEBUG] List index : %d", list->index);
	} else {
		SLOG(LOG_DEBUG, TAG_VCCMD, "[DEBUG] List index : %d", list->index);
		return VC_ERROR_ITERATION_END;
	}

	return VC_ERROR_NONE;
}

int vc_cmd_list_prev(vc_cmd_list_h vc_cmd_list)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_cmd_list) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_list_s* list = NULL;
	list = (vc_cmd_list_s*)vc_cmd_list;

	if (list->index > 0) {
		list->index = list->index - 1;
		SLOG(LOG_DEBUG, TAG_VCCMD, "[DEBUG] List index : %d", list->index);
	} else {
		SLOG(LOG_DEBUG, TAG_VCCMD, "[DEBUG] List index : %d", list->index);
		return VC_ERROR_ITERATION_END;
	}

	return VC_ERROR_NONE;
}

int vc_cmd_list_get_current(vc_cmd_list_h vc_cmd_list, vc_cmd_h* vc_command)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_cmd_list || NULL == vc_command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_list_s* list = NULL;
	list = (vc_cmd_list_s*)vc_cmd_list;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[List] list (%p), index (%d)", list, list->index);

	if (0 == g_slist_length(list->list)) {
		SLOG(LOG_DEBUG, TAG_VCCMD, "[List] list is empty");
		*vc_command = NULL;
		return VC_ERROR_EMPTY;
	}

	vc_cmd_s *temp_cmd = NULL;
	temp_cmd = g_slist_nth_data(list->list, list->index);

	*vc_command = (vc_cmd_h)temp_cmd;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[List] Get current command (%p)", *vc_command);

	return VC_ERROR_NONE;
}


int vc_cmd_create(vc_cmd_h* vc_command)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* command = (vc_cmd_s*)calloc(1, sizeof(vc_cmd_s));

	if (NULL == command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Not enough memory");
		return VC_ERROR_OUT_OF_MEMORY;
	}

	command->pid = 0;
	command->id = 0;
	command->index = 0;
	command->type = VC_COMMAND_TYPE_NONE;
	command->format = VC_CMD_FORMAT_FIXED;
	command->command = NULL;
	command->parameter = NULL;
	command->domain = 0;
	command->key = VC_KEY_NONE;
	command->modifier = VC_MODIFIER_NONE;

	*vc_command = (vc_cmd_h)command;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[Create command][%p]", *vc_command);

	return VC_ERROR_NONE;
}

int vc_cmd_destroy(vc_cmd_h vc_command)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Input parameter is NULL");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* command = NULL;
	command = (vc_cmd_s*)vc_command;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[Destroy command][%p]", command);

	if (NULL != command) {
		if (NULL != command->command)	free(command->command);
		if (NULL != command->parameter)	free(command->parameter);
		free(command);
		command = NULL;
	}

	return VC_ERROR_NONE;
}

int vc_cmd_set_id(vc_cmd_h vc_command, int id)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid parameter ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	if (NULL != cmd) {
		cmd->id = id;
		SLOG(LOG_DEBUG, TAG_VCCMD, "[Set id][%p] id(%d)", vc_command, cmd->id);
	}

	return 0;
}

int vc_cmd_get_id(vc_cmd_h vc_command, int* id)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command || NULL == id) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid handle ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	if (NULL != cmd) {
		*id = cmd->id;
		SLOG(LOG_DEBUG, TAG_VCCMD, "[Get id][%p] id(%d)", vc_command, *id);
	}

	return 0;
}

int vc_cmd_set_command(vc_cmd_h vc_command, const char* command)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid parameter ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	if (NULL != cmd->command) {
		free(cmd->command);
	}

	cmd->command = NULL;

	if (NULL != command) {
		cmd->command = strdup(command);
	}

	SLOG(LOG_DEBUG, TAG_VCCMD, "[Set command][%p] Command(%s)", vc_command, cmd->command);

	return 0;
}

int vc_cmd_get_command(vc_cmd_h vc_command, char** command)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command || NULL == command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid handle ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	if (NULL != cmd->command) {
		*command = strdup(gettext(cmd->command));
	}

	SLOG(LOG_DEBUG, TAG_VCCMD, "[Get command][%p] Command(%s)", vc_command, *command);

	return 0;
}

int vc_cmd_set_unfixed_command(vc_cmd_h vc_command, const char* command)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid parameter ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	if (NULL != cmd->parameter) {
		free(cmd->parameter);
	}

	cmd->parameter = NULL;

	if (NULL != command) {
		cmd->parameter = strdup(command);
		SLOG(LOG_DEBUG, TAG_VCCMD, "[Set parameter][%p] parameter(%s)", vc_command, cmd->parameter);
	}

	return 0;
}

int vc_cmd_get_unfixed_command(vc_cmd_h vc_command, char** command)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command || NULL == command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid handle ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	if (NULL != cmd->parameter) {
		*command = strdup(gettext(cmd->parameter));
		SLOG(LOG_DEBUG, TAG_VCCMD, "[Get nonfixed command][%p] nonfixed command(%s)", vc_command, *command);
	}

	return 0;
}

int vc_cmd_set_type(vc_cmd_h vc_command, int type)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid parameter ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	cmd->type = type;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[Set type][%p] type(%d)", vc_command, cmd->type);

	return 0;
}

int vc_cmd_get_type(vc_cmd_h vc_command, int* type)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command || NULL == type) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid parameter ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	*type = cmd->type;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[Get type][%p] type(%d)", vc_command, *type);

	return 0;
}

int vc_cmd_set_format(vc_cmd_h vc_command, vc_cmd_format_e format)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid parameter ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	cmd->format = format;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[Set format][%p] format(%d)", vc_command, format);

	return 0;
}

int vc_cmd_get_format(vc_cmd_h vc_command, vc_cmd_format_e* format)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command || NULL == format) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid parameter ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	*format = cmd->format;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[Get format][%p] format(%d)", vc_command, *format);

	return 0;
}

int vc_cmd_set_pid(vc_cmd_h vc_command, int pid)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid parameter ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	cmd->pid = pid;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[Set pid][%p] pid(%d)", vc_command, cmd->pid);

	return 0;
}

int vc_cmd_get_pid(vc_cmd_h vc_command, int* pid)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command || NULL == pid) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid parameter ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	*pid = cmd->pid;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[Get pid][%p] pid(%d)", vc_command, *pid);

	return 0;
}

int vc_cmd_set_domain(vc_cmd_h vc_command, int domain)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid parameter ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	cmd->domain = domain;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[Set domain] domain : %d", domain);

	return 0;
}

int vc_cmd_get_domain(vc_cmd_h vc_command, int* domain)
{
	if (0 != __vc_cmd_get_feature_enabled()) {
		return VC_ERROR_NOT_SUPPORTED;
	}
	if (0 != __vc_cmd_check_privilege()) {
		return VC_ERROR_PERMISSION_DENIED;
	}

	if (NULL == vc_command || NULL == domain) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid parameter ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	*domain = cmd->domain;

	SLOG(LOG_DEBUG, TAG_VCCMD, "[Get domain] domain : %d", *domain);

	return 0;
}

/**
* @brief Sets key value of command.
*
* @param[in] vc_command Command handle
* @param[in] key key value
* @param[in] modifier modifier value
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
*
* @see vc_cmd_get_result_key()
*/
int vc_cmd_set_result_key(vc_cmd_h vc_command, int key, int modifier)
{
	SLOG(LOG_DEBUG, TAG_VCCMD, "===== Set result key");

	if (NULL == vc_command) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid parameter ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	SLOG(LOG_DEBUG, TAG_VCCMD, "key : %d, modifier : %d", key, modifier);

	cmd->key = key;
	cmd->modifier = modifier;

	SLOG(LOG_DEBUG, TAG_VCCMD, "=====");
	SLOG(LOG_DEBUG, TAG_VCCMD, " ");

	return 0;
}

/**
* @brief Gets key value of command.
*
* @param[in] vc_command Command handle
* @param[out] key key value
* @param[out] modifier modifier value
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
*
* @see vc_cmd_add_result_key()
*/
int vc_cmd_get_result_key(vc_cmd_h vc_command, int* key, int* modifier)
{
	SLOG(LOG_DEBUG, TAG_VCCMD, "===== Get result key");

	if (NULL == vc_command || NULL == key || NULL == modifier) {
		SLOG(LOG_ERROR, TAG_VCCMD, "[ERROR] Invalid parameter ");
		return VC_ERROR_INVALID_PARAMETER;
	}

	vc_cmd_s* cmd = NULL;
	cmd = (vc_cmd_s*)vc_command;

	*key = cmd->key;
	*modifier = cmd->modifier;

	SLOG(LOG_DEBUG, TAG_VCCMD, "=====");
	SLOG(LOG_DEBUG, TAG_VCCMD, " ");

	return 0;
}

int vc_cmd_print_list(vc_cmd_list_h vc_cmd_list)
{
	if (NULL == vc_cmd_list) {
		return -1;
	}

	vc_cmd_list_s* list = NULL;
	list = (vc_cmd_list_s*)vc_cmd_list;

	SLOG(LOG_DEBUG, TAG_VCCMD, "=== Command List ===");
	SLOG(LOG_DEBUG, TAG_VCCMD, "[List][%p]", list);

	int count = g_slist_length(list->list);

	int i;
	vc_cmd_s *temp_cmd = NULL;

	for (i = 0; i < count ; i++) {
		temp_cmd = g_slist_nth_data(list->list, i);

		if (NULL != temp_cmd) {
			SLOG(LOG_DEBUG, TAG_VCCMD, "  [%d][%p] PID(%d) ID(%d) Type(%d) Format(%d) Domain(%d) Command(%s) Param(%s)",
				 i, temp_cmd, temp_cmd->pid, temp_cmd->index, temp_cmd->type, temp_cmd->format, temp_cmd->domain, temp_cmd->command, temp_cmd->parameter);
		}
	}

	SLOG(LOG_DEBUG, TAG_VCCMD, "==================");
	SLOG(LOG_DEBUG, TAG_VCCMD, " ");

	return 0;
}
