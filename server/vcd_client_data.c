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
#include "vcd_client_data.h"
#include "vcd_config.h"
#include "vcd_main.h"

/* Client list */
static GSList* g_client_list = NULL;

static GSList* g_widget_list = NULL;

static manager_info_s g_manager;

/* Command list */
static current_commands_list_s g_cur_cmd_list;

/* Demandable client list */
static GSList* g_demandable_client = NULL;

/* Runtime info */
static bool g_silence_detection;


/* Function definitions */
widget_info_s* __widget_get_element(int pid);

vc_client_info_s* __client_get_element(int pid);


int vcd_client_manager_set(int pid)
{
	if (-1 != g_manager.pid) {
		SLOG(LOG_DEBUG, TAG_VCD, "Manager has already registered");
		return -1;
	}
	g_manager.pid = pid;
	g_manager.manager_cmd = false;
	g_manager.exclusive_cmd_option = false;

	return 0;
}

int vcd_client_manager_unset()
{
	g_manager.pid = -1;
	g_manager.manager_cmd = false;
	g_manager.exclusive_cmd_option = false;

	return 0;
}

bool vcd_client_manager_is_valid(int pid)
{
	if (-1 == g_manager.pid || pid == g_manager.pid) {
		return true;
	}
	return false;
}

int vcd_client_manager_set_command(int pid)
{
	if (pid != g_manager.pid) {
		SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] pid(%d) is NOT valid", pid);
		return -1;
	}
	g_manager.manager_cmd = true;
	return 0;
}

int vcd_client_manager_unset_command(int pid)
{
	if (pid != g_manager.pid) {
		SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] pid(%d) is NOT valid", pid);
		return -1;
	}
	g_manager.manager_cmd = false;
	return 0;
}

int vcd_client_manager_set_demandable_client(int pid, GSList* client_list)
{
	if (0 != g_slist_length(g_demandable_client)) {
		/* releaes data */
		GSList *iter = NULL;
		vc_demandable_client_s* temp_client;
		iter = g_slist_nth(g_demandable_client, 0);
		
		while (NULL != iter) {
			temp_client = iter->data;

			if (NULL != temp_client) {
				if (NULL != temp_client->appid)		free(temp_client->appid);
				free(temp_client);
			}

			iter = g_slist_next(iter);
		}
		g_demandable_client = NULL;
	}

	g_demandable_client = client_list;

	return 0;
}

bool vcd_client_manager_check_demandable_client(int pid)
{
	if (0 == g_slist_length(g_demandable_client)) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] All client is available to request start");
		return true;
	}

	/* Check demandable appid */
	char appid[128] = {0, };
	aul_app_get_appid_bypid(pid, appid, sizeof(appid));
	
	if (0 < strlen(appid)) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] %s(%d) requests start", appid, pid);
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] daemon(%d) requests start", pid);
	}

	/* Compare appid */
	GSList *iter = NULL;
	vc_demandable_client_s* temp_client;
	iter = g_slist_nth(g_demandable_client, 0);
	
	while (NULL != iter) {
		temp_client = iter->data;

		if (NULL != temp_client) {

			SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] demandable appid(%s)", temp_client->appid);

			if (NULL != temp_client->appid) {
				if (0 == strcmp(temp_client->appid, appid)) {
					SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] pid(%d) is available", pid);
					return true;
				}
			} else {
				if (0 == strlen(appid)) {
					SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] pid(%d) is available", pid);
					return true;
				}
			}
		}

		iter = g_slist_next(iter);
	}

	return false;
}

bool vcd_client_manager_get_exclusive()
{
	return g_manager.exclusive_cmd_option;
}

int vcd_client_manager_set_exclusive(bool value)
{
	g_manager.exclusive_cmd_option = value;
	return 0;
}

int vcd_client_manager_get_pid()
{
	return g_manager.pid;
}

int __vcd_client_release_commands()
{
	g_cur_cmd_list.total_cmd_count = 0;
	g_cur_cmd_list.foreground = VC_NO_FOREGROUND_PID;

	GSList *iter = NULL;
	vc_cmd_s* temp_cmd;

	if (0 < g_slist_length(g_cur_cmd_list.widget_cmds)) {
		iter = g_slist_nth(g_cur_cmd_list.widget_cmds, 0);
		while (NULL != iter) {
			temp_cmd = iter->data;

			if (NULL != temp_cmd) {
				if (NULL != temp_cmd->command)		free(temp_cmd->command);
				if (NULL != temp_cmd->parameter)	free(temp_cmd->parameter);
				free(temp_cmd);
			}

			iter = g_slist_next(iter);
		}
		g_cur_cmd_list.widget_cmds = NULL;
	}

	if (0 < g_slist_length(g_cur_cmd_list.foreground_cmds)) {
		iter = g_slist_nth(g_cur_cmd_list.foreground_cmds, 0);
		while (NULL != iter) {
			temp_cmd = iter->data;

			if (NULL != temp_cmd) {
				if (NULL != temp_cmd->command)		free(temp_cmd->command);
				if (NULL != temp_cmd->parameter)	free(temp_cmd->parameter);
				free(temp_cmd);
			}

			iter = g_slist_next(iter);
		}
		g_cur_cmd_list.foreground_cmds = NULL;
	}

	if (0 < g_slist_length(g_cur_cmd_list.system_cmds)) {
		iter = g_slist_nth(g_cur_cmd_list.system_cmds, 0);
		while (NULL != iter) {
			temp_cmd = iter->data;

			if (NULL != temp_cmd) {
				if (NULL != temp_cmd->command)		free(temp_cmd->command);
				if (NULL != temp_cmd->parameter)	free(temp_cmd->parameter);
				free(temp_cmd);
			}

			iter = g_slist_next(iter);
		}
		g_cur_cmd_list.system_cmds = NULL;
	}

	if (0 < g_slist_length(g_cur_cmd_list.exclusive_system_cmds)) {
		iter = g_slist_nth(g_cur_cmd_list.exclusive_system_cmds, 0);
		while (NULL != iter) {
			temp_cmd = iter->data;

			if (NULL != temp_cmd) {
				if (NULL != temp_cmd->command)		free(temp_cmd->command);
				if (NULL != temp_cmd->parameter)	free(temp_cmd->parameter);
				free(temp_cmd);
			}

			iter = g_slist_next(iter);
		}
		g_cur_cmd_list.exclusive_system_cmds = NULL;
	}

	if (0 < g_slist_length(g_cur_cmd_list.background_cmds)) {
		background_command_s* back_cmd_info;
		iter = g_slist_nth(g_cur_cmd_list.background_cmds, 0);

		GSList* back_iter = NULL;

		while (NULL != iter) {
			back_cmd_info = iter->data;

			if (NULL != back_cmd_info) {
				back_iter = g_slist_nth(back_cmd_info->cmds, 0);

				while (NULL != back_iter) {
					temp_cmd = back_iter->data;

					if (NULL != temp_cmd) {
						if (NULL != temp_cmd->command)		free(temp_cmd->command);
						if (NULL != temp_cmd->parameter)	free(temp_cmd->parameter);
						free(temp_cmd);
					}

					back_iter = g_slist_next(back_iter);
				}

				back_cmd_info->cmds = NULL;
			}

			iter = g_slist_next(iter);
		}

		g_cur_cmd_list.background_cmds = NULL;
	}

	g_cur_cmd_list.bg_cmd_count = 0;

	return 0;
}

int vcd_client_command_collect_command()
{
	/* 1. Get foreground pid */
	int fg_pid = 0;
	int ret = -1;
	if (0 != vcd_config_get_foreground(&fg_pid)) {
		SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Fail to get foreground pid");
		/* There is no foreground app for voice control */
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] Foreground pid(%d)", fg_pid);

	/* 2. Clean up command list */
	__vcd_client_release_commands();

	/* Check exclusive system command */
	if (true == g_manager.exclusive_cmd_option) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] Exclusive option of manager is ON");

		GSList* ex_sys_cmd_list = NULL;
		if (true == g_manager.manager_cmd) {
			ret = vc_cmd_parser_get_commands(g_manager.pid, VC_COMMAND_TYPE_EXCLUSIVE, &ex_sys_cmd_list);
			if (0 != ret) {
				SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] Fail to get the system command list");
			} else {
				g_cur_cmd_list.exclusive_system_cmds = ex_sys_cmd_list;
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No exclusive system commands");
		}

		return 0;
	}

	/* 3. Set system command */
	GSList* sys_cmd_list = NULL;
	if (true == g_manager.manager_cmd) {
		ret = vc_cmd_parser_get_commands(g_manager.pid, VC_COMMAND_TYPE_SYSTEM, &sys_cmd_list);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] Fail to get the system command list");
		} else {
			g_cur_cmd_list.system_cmds = sys_cmd_list;
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No system commands");
	}

	vc_client_info_s* client_info = NULL;
	GSList *iter = NULL;

	/* 4. Set foreground commands and widget */
	if (VC_NO_FOREGROUND_PID != fg_pid) {
		GSList* fg_cmd_list = NULL;
		GSList* widget_cmd_list = NULL;

		g_cur_cmd_list.foreground = fg_pid;

		/* 4-1. Set widget command */
		widget_info_s* widget_info = NULL;
		widget_info = __widget_get_element(fg_pid);
		if (NULL != widget_info) {
			if (true == widget_info->widget_cmd) {
				ret = vc_cmd_parser_get_commands(fg_pid, VC_COMMAND_TYPE_WIDGET, &widget_cmd_list);
				if (0 != ret) {
					SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] Fail to get the WIDGET command list");
				} else {
					g_cur_cmd_list.widget_cmds = widget_cmd_list;
				}
			}
		} else {
			SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No widget commands");
		}

		/* Get handle */
		client_info = __client_get_element(fg_pid);
		if (NULL != client_info) {
			SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] fore cmd(%d)", client_info->fg_cmd);

			/* 4-2. Set foreground command */
			if (true == client_info->fg_cmd) {
				ret = vc_cmd_parser_get_commands(fg_pid, VC_COMMAND_TYPE_FOREGROUND, &fg_cmd_list);
				if (0 != ret) {
					SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] Fail to get the fg command list");
				} else {
					g_cur_cmd_list.foreground_cmds = fg_cmd_list;
				}
			} else {
				SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No foreground commands");
			}

			/* 4-3. Check exclusive option */
			if (true == client_info->exclusive_cmd) {
				SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] Exclusive command is ON");

				/* 4-4. Set background command for exclusive option */
				if (true == client_info->bg_cmd) {
					SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] Set background command");
					GSList*	bg_cmd_list = NULL;
					ret = vc_cmd_parser_get_commands(client_info->pid, VC_COMMAND_TYPE_BACKGROUND, &bg_cmd_list);
					if (0 != ret) {
						SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] Fail to get the bg command list : pid(%d)", client_info->pid);
					} else {
						background_command_s* bg_cmd = (background_command_s*)calloc(1, sizeof(background_command_s));
						if (NULL == bg_cmd) {
							SLOG(LOG_ERROR, TAG_VCD, "[ERROR] Fail to allocate memory");
							return VCD_ERROR_OUT_OF_MEMORY;
						}

						bg_cmd->pid = client_info->pid;
						bg_cmd->cmds = bg_cmd_list;
						bg_cmd->cmd_count = g_slist_length(bg_cmd_list);

						/* Add item to global command list */
						g_cur_cmd_list.background_cmds = g_slist_append(g_cur_cmd_list.background_cmds, bg_cmd);
					}
				}

				return 0;
			}
		} else {
			SLOG(LOG_WARN, TAG_VCD, "[Client Data] No foreground client : pid(%d)", fg_pid);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No foreground app");
	}

	/* 5. Set background commands */
	if (0 < g_slist_length(g_client_list)) {
		iter = g_slist_nth(g_client_list, 0);

		while (NULL != iter) {
			client_info = iter->data;
			GSList*	bg_cmd_list = NULL;

			SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] Pid(%d) Back cmd(%d)", client_info->pid, client_info->bg_cmd);

			if (true == client_info->bg_cmd) {
				ret = vc_cmd_parser_get_commands(client_info->pid, VC_COMMAND_TYPE_BACKGROUND, &bg_cmd_list);
				if (0 != ret) {
					SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] Fail to get the bg command list : pid(%d)", client_info->pid);
				} else {
					background_command_s* bg_cmd = (background_command_s*)calloc(1, sizeof(background_command_s));
					if (NULL == bg_cmd) {
						SLOG(LOG_ERROR, TAG_VCD, "[ERROR] Fail to allocate memory");
						return VCD_ERROR_OUT_OF_MEMORY;
					}

					bg_cmd->pid = client_info->pid;
					bg_cmd->cmds = bg_cmd_list;
					bg_cmd->cmd_count = g_slist_length(bg_cmd_list);

					/* Add item to global command list */
					g_cur_cmd_list.background_cmds = g_slist_append(g_cur_cmd_list.background_cmds, bg_cmd);
				}
			}

			iter = g_slist_next(iter);
		}
	} else {
		/* NO client */
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No background commands");
	}

	return 0;
}

int vcd_client_get_length()
{
	int command_count = 0;
	command_count += g_slist_length(g_cur_cmd_list.widget_cmds);
	command_count += g_slist_length(g_cur_cmd_list.foreground_cmds);
	command_count += g_slist_length(g_cur_cmd_list.system_cmds);
	command_count += g_slist_length(g_cur_cmd_list.exclusive_system_cmds);

	GSList *iter = NULL;
	background_command_s* back_cmd_info;

	if (0 < g_slist_length(g_cur_cmd_list.background_cmds)) {
		iter = g_slist_nth(g_cur_cmd_list.background_cmds, 0);

		while (NULL != iter) {
			back_cmd_info = iter->data;

			command_count += g_slist_length(back_cmd_info->cmds);

			SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] background commands count : %d", g_slist_length(back_cmd_info->cmds));

			iter = g_slist_next(iter);
		}
	} else {
		/* NO client */
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No background commands");
	}

	g_cur_cmd_list.total_cmd_count = command_count;

	SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] Command count : %d ", g_cur_cmd_list.total_cmd_count);

	return command_count;
}

int vcd_client_foreach_command(client_foreach_command_cb callback, void* user_data)
{
	if (NULL == callback) {
		SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] input parameter is NULL");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	int id_count = 1;
	GSList *iter = NULL;
	vc_cmd_s* temp_cmd;

	if (0 < g_slist_length(g_cur_cmd_list.widget_cmds)) {
		iter = g_slist_nth(g_cur_cmd_list.widget_cmds, 0);
		while (NULL != iter) {
			temp_cmd = iter->data;

			temp_cmd->id = id_count;
			id_count++;

			SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] Widget : id(%d) index(%d) format(%d) command(%s) param(%s) domain(%d)"
				, temp_cmd->id, temp_cmd->index, temp_cmd->format, temp_cmd->command, temp_cmd->parameter, temp_cmd->domain);

			callback(temp_cmd->id, temp_cmd->format, temp_cmd->command, temp_cmd->parameter, temp_cmd->domain, user_data);

			iter = g_slist_next(iter);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No widget commands");
	}

	if (0 < g_slist_length(g_cur_cmd_list.foreground_cmds)) {
		iter = g_slist_nth(g_cur_cmd_list.foreground_cmds, 0);
		while (NULL != iter) {
			temp_cmd = iter->data;

			temp_cmd->id = id_count;
			id_count++;

			SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] Foreground : id(%d) index(%d) format(%d) command(%s) param(%s) domain(%d)"
				, temp_cmd->id, temp_cmd->index, temp_cmd->format, temp_cmd->command, temp_cmd->parameter, temp_cmd->domain);

			callback(temp_cmd->id, temp_cmd->format, temp_cmd->command, temp_cmd->parameter, temp_cmd->domain, user_data);

			iter = g_slist_next(iter);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No foreground commands");
	}

	if (0 < g_slist_length(g_cur_cmd_list.system_cmds)) {
		iter = g_slist_nth(g_cur_cmd_list.system_cmds, 0);
		while (NULL != iter) {
			temp_cmd = iter->data;

			temp_cmd->id = id_count;
			id_count++;

			SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] System : id(%d) index(%d) format(%d) domain(%d) command(%s) param(%s)"
				, temp_cmd->id, temp_cmd->index, temp_cmd->format, temp_cmd->domain, temp_cmd->command, temp_cmd->parameter);

			callback(temp_cmd->id, temp_cmd->format, temp_cmd->command, temp_cmd->parameter, temp_cmd->domain, user_data);

			iter = g_slist_next(iter);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No system commands");
	}

	if (0 < g_slist_length(g_cur_cmd_list.exclusive_system_cmds)) {
		iter = g_slist_nth(g_cur_cmd_list.exclusive_system_cmds, 0);
		while (NULL != iter) {
			temp_cmd = iter->data;

			temp_cmd->id = id_count;
			id_count++;

			SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] Exclusive system : id(%d) index(%d) format(%d) command(%s) param(%s) domain(%d)"
				, temp_cmd->id, temp_cmd->index, temp_cmd->format, temp_cmd->command, temp_cmd->parameter, temp_cmd->domain);

			callback(temp_cmd->id, temp_cmd->type, temp_cmd->command, temp_cmd->parameter, temp_cmd->domain, user_data);

			iter = g_slist_next(iter);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No exclusive system commands");
	}

	if (0 < g_slist_length(g_cur_cmd_list.background_cmds)) {
		background_command_s* back_cmd_info;
		iter = g_slist_nth(g_cur_cmd_list.background_cmds, 0);

		while (NULL != iter) {
			back_cmd_info = iter->data;

			GSList* back_iter = NULL;
			back_iter = g_slist_nth(back_cmd_info->cmds, 0);

			SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] pid(%d) command count(%d)", back_cmd_info->pid, back_cmd_info->cmd_count);

			while (NULL != back_iter) {
				temp_cmd = back_iter->data;

				temp_cmd->id = id_count;
				id_count++;

				SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] Background : id(%d) index(%d) format(%d) command(%s) param(%s) domain(%d)"
					, temp_cmd->id, temp_cmd->index, temp_cmd->format, temp_cmd->command, temp_cmd->parameter, temp_cmd->domain);

				callback(temp_cmd->id, temp_cmd->format, temp_cmd->command, temp_cmd->parameter, temp_cmd->domain, user_data);

				back_iter = g_slist_next(back_iter);
			}

			iter = g_slist_next(iter);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No background commands");
	}

	return 0;
}

static vc_cmd_s* __command_copy(vc_cmd_s* src_cmd)
{
	if (NULL == src_cmd) {
		SLOG(LOG_WARN, TAG_VCD, "[Client Data] Input command is NULL");
		return NULL;
	}

	vc_cmd_s* temp_cmd = NULL;
	temp_cmd = (vc_cmd_s*)calloc(sizeof(vc_cmd_s), 1);
	if (NULL == temp_cmd) {
		SLOG(LOG_ERROR, TAG_VCD, "[ERROR] Fail to allocate memory");
		return NULL;
	}

	temp_cmd->id = src_cmd->id;
	temp_cmd->pid = src_cmd->pid;
	temp_cmd->index = src_cmd->index;
	temp_cmd->type = src_cmd->type;
	temp_cmd->format = src_cmd->format;
	temp_cmd->domain = src_cmd->domain;

	if (NULL != src_cmd->command) {
		temp_cmd->command = strdup(src_cmd->command);
	}

	if (NULL != src_cmd->parameter) {
		temp_cmd->parameter = strdup(src_cmd->parameter);
	}
	
	temp_cmd->key = src_cmd->key;
	temp_cmd->modifier = src_cmd->modifier;

	return temp_cmd;
}

//int vcd_client_get_cmd_info_from_result_id(int result_id, int* pid, int* cmd_type, vc_cmd_s** result)
int vcd_client_get_cmd_from_result_id(int result_id, vc_cmd_s** result)
{
	GSList *iter = NULL;
	vc_cmd_s* temp_cmd;

	SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] Result id(%d)", result_id);

	if (0 < g_slist_length(g_cur_cmd_list.widget_cmds)) {
		iter = g_slist_nth(g_cur_cmd_list.widget_cmds, 0);
		while (NULL != iter) {
			temp_cmd = iter->data;

			if (result_id == temp_cmd->id) {
				//*pid = g_cur_cmd_list.foreground;
				//*cmd_type = VCD_CLIENT_COMMAND_GROUP_TYPE_UI_CONTROL;
				//SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] Find result pid(%d) type(%d)", *pid, *cmd_type);

				*result = __command_copy(temp_cmd);

				return 0;
			}

			iter = g_slist_next(iter);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No widget commands");
	}

	if (0 < g_slist_length(g_cur_cmd_list.foreground_cmds)) {
		iter = g_slist_nth(g_cur_cmd_list.foreground_cmds, 0);

		while (NULL != iter) {
			temp_cmd = iter->data;

			SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] command id (%d)", temp_cmd->id);

			if (result_id == temp_cmd->id) {
				//*pid = g_cur_cmd_list.foreground;
				//*cmd_type = VCD_CLIENT_COMMAND_GROUP_TYPE_FOREGROUND;
				//SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] Find result pid(%d) type(%d)", *pid, *cmd_type);

				*result = __command_copy(temp_cmd);
				return 0;
			}

			iter = g_slist_next(iter);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No foreground commands");
	}

	if (0 < g_slist_length(g_cur_cmd_list.system_cmds)) {
		iter = g_slist_nth(g_cur_cmd_list.system_cmds, 0);
		while (NULL != iter) {
			temp_cmd = iter->data;

			if (result_id == temp_cmd->id) {
				//*pid = g_manager.pid;
				//*cmd_type = VCD_CLIENT_COMMAND_GROUP_TYPE_SYSTEM;
				//SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] Find result pid(%d) type(%d)", *pid, *cmd_type);

				*result = __command_copy(temp_cmd);
				return 0;
			}

			iter = g_slist_next(iter);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No system commands");
	}

	if (0 < g_slist_length(g_cur_cmd_list.exclusive_system_cmds)) {
		iter = g_slist_nth(g_cur_cmd_list.exclusive_system_cmds, 0);
		while (NULL != iter) {
			temp_cmd = iter->data;

			if (result_id == temp_cmd->id) {
				//*pid = g_manager.pid;
				//*cmd_type = VCD_CLIENT_COMMAND_GROUP_TYPE_SYSTEM_EXCLUSIVE;
				//SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] Find result pid(%d) type(%d)", *pid, *cmd_type);

				*result = __command_copy(temp_cmd);
				return 0;
			}

			iter = g_slist_next(iter);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No exclusive system commands");
	}

	if (0 < g_slist_length(g_cur_cmd_list.background_cmds)) {
		background_command_s* back_cmd_info;
		iter = g_slist_nth(g_cur_cmd_list.background_cmds, 0);

		while (NULL != iter) {
			back_cmd_info = iter->data;

			GSList* back_iter = NULL;
			back_iter = g_slist_nth(back_cmd_info->cmds, 0);

			SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] pid(%d) command count(%d)", back_cmd_info->pid, back_cmd_info->cmd_count);

			while (NULL != back_iter) {
				temp_cmd = back_iter->data;

				if (result_id == temp_cmd->id) {
					//*pid = back_cmd_info->pid;
					//*cmd_type = VCD_CLIENT_COMMAND_GROUP_TYPE_BACKGROUND;

					*result = __command_copy(temp_cmd);
					return 0;
				}
				back_iter = g_slist_next(back_iter);
			}

			iter = g_slist_next(iter);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] No background commands");
	}

	SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] Not find matched result");

	return -1;
}

int vcd_client_set_slience_detection(bool value)
{
	g_silence_detection = value;
	return 0;
}

bool vcd_client_get_slience_detection()
{
	return g_silence_detection;
}

int __show_client_list()
{
	GSList *iter = NULL;
	vc_client_info_s *data = NULL;

	SLOG(LOG_DEBUG, TAG_VCD, "----- client list");

	int count = g_slist_length(g_client_list);
	int i;

	if (0 == count) {
		SLOG(LOG_DEBUG, TAG_VCD, "No Client");
	} else {
		iter = g_slist_nth(g_client_list, 0);
		for (i = 0;i < count;i++) {
			data = iter->data;

			SLOG(LOG_DEBUG, TAG_VCD, "[%dth] pid(%d)", i, data->pid);
			iter = g_slist_next(iter);
		}
	}

	SLOG(LOG_DEBUG, TAG_VCD, "-----");

	SLOG(LOG_DEBUG, TAG_VCD, "----- widget list");

	widget_info_s *widget_data = NULL;

	count = g_slist_length(g_widget_list);

	if (0 == count) {
		SLOG(LOG_DEBUG, TAG_VCD, "No widget");
	} else {
		iter = g_slist_nth(g_widget_list, 0);
		for (i = 0;i < count;i++) {
			widget_data = iter->data;

			SLOG(LOG_DEBUG, TAG_VCD, "[%dth] pid(%d)", i, widget_data->pid);
			iter = g_slist_next(iter);
		}
	}

	SLOG(LOG_DEBUG, TAG_VCD, "-----");

	return 0;
}

int __show_command_list(GSList* cmd_group)
{
	GSList *iter = NULL;
	vc_cmd_s *data = NULL;

	SLOG(LOG_DEBUG, TAG_VCD, "----- command group");

	int count = g_slist_length(cmd_group);
	int i;

	if (0 == count) {
		SLOG(LOG_DEBUG, TAG_VCD, "No command");
	} else {
		iter = g_slist_nth(cmd_group, 0);
		for (i = 0;i < count;i++) {
			data = iter->data;

			if (NULL != data->parameter) {
				SLOG(LOG_DEBUG, TAG_VCD, "[%dth] command(%s) parameter(%s) key(%d)", i, data->command, data->parameter, data->key);
			} else {
				SLOG(LOG_DEBUG, TAG_VCD, "[%dth] command(%s) key(%d)", i, data->command, data->key);
			}
			iter = g_slist_next(iter);
		}
	}

	SLOG(LOG_DEBUG, TAG_VCD, "-----");

	return 0;
}

GSList* __client_get_item(const int pid)
{
	GSList *iter = NULL;
	vc_client_info_s *data = NULL;

	int count = g_slist_length(g_client_list);
	int i;

	if (0 < count) {
		iter = g_slist_nth(g_client_list, 0);
		for (i = 0;i < count;i++) {
			data = iter->data;

			if (pid == data->pid) 
				return iter;

			iter = g_slist_next(iter);
		}
	}

	return NULL;
}

vc_client_info_s* __client_get_element(int pid)
{
	GSList *iter = NULL;
	vc_client_info_s *data = NULL;
	
	int count = g_slist_length(g_client_list);
	int i;

	if (0 < count) {
		iter = g_slist_nth(g_client_list, 0);
		for (i = 0;i < count;i++) {
			data = iter->data;

			if (pid == data->pid) 
				return data;

			iter = g_slist_next(iter);
		}
	}

	return NULL;
}

int vcd_client_add(int pid)
{
	/*Check pid is duplicated*/
	GSList *tmp = NULL;
	tmp = __client_get_item(pid);
	
	if (NULL != tmp) {
		SLOG(LOG_WARN, TAG_VCD, "[Client Data] Client pid is already registered");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	vc_client_info_s *info = (vc_client_info_s*)calloc(1, sizeof(vc_client_info_s));
	if (NULL == info) {
		SLOG(LOG_ERROR, TAG_VCD, "[ERROR] Fail to allocate memory");
		return VCD_ERROR_OUT_OF_MEMORY;
	}

	info->pid = pid;

	info->fg_cmd = false;
	info->bg_cmd = false;
	info->exclusive_cmd = false;

	/* Add item to global list */
	g_client_list = g_slist_append(g_client_list, info);
	
	if (NULL == g_client_list) {
		SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] Fail to add new client");
		return -1;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data SUCCESS] Add new client");
	}

#ifdef CLIENT_DATA_DEBUG
	__show_client_list();
#endif 
	return 0;
}

int vcd_client_delete(int pid)
{
	GSList *tmp = NULL;
	vc_client_info_s* client_info = NULL;

	/*Get handle*/
	tmp = __client_get_item(pid);
	if (NULL == tmp) {
		SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] pid(%d) is NOT valid", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}

	/*Free client structure*/
	client_info = tmp->data;
	if (NULL != client_info) {
		free(client_info);
	}

	/*Remove handle from list*/
	g_client_list = g_slist_remove_link(g_client_list, tmp);


#ifdef CLIENT_DATA_DEBUG
	__show_client_list();
#endif 

	return 0;
}

bool vcd_client_is_available(int pid)
{
	vc_client_info_s* client_info = NULL;

	client_info = __client_get_element(pid);
	if (NULL == client_info) {
		SLOG(LOG_WARN, TAG_VCD, "[Client Data] pid(%d) is NOT valid", pid);
		return false;
	}

	return true;
}

int vcd_client_get_ref_count()
{
	int count = 0;
	
	count = g_slist_length(g_client_list) + g_slist_length(g_widget_list);
	if (0 < g_manager.pid) {
		count++;
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Client Data] client count : %d", count);

	return count;
}

int vcd_client_get_list(int** pids, int* pid_count)
{
	if (NULL == pids || NULL == pid_count)
		return -1;
	
	int count = g_slist_length(g_client_list);

	if (0 == count)
		return -1;

	int *tmp;
	tmp = (int*)calloc(count, sizeof(int));
	if (NULL == tmp) {
		SLOG(LOG_ERROR, TAG_VCD, "[ERROR] Fail to allocate memory");
		return VCD_ERROR_OUT_OF_MEMORY;
	}
	
	GSList *iter = NULL;
	vc_client_info_s *data = NULL;
	int i = 0;

	iter = g_slist_nth(g_client_list, 0);

	while (NULL != iter) {
		data = iter->data;

		if (NULL != data) {
			tmp[i] = data->pid;
		}

		iter = g_slist_next(iter);
		i++;
	}

	*pids = tmp;
	*pid_count = count;

	return 0;
}

int vcd_client_set_command_type(int pid, int type)
{
	vc_client_info_s* client_info = NULL;

	client_info = __client_get_element(pid);
	if (NULL == client_info) {
		SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] vcd_client_get_pid : pid(%d) is not found", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}

	switch (type) {
	case VC_COMMAND_TYPE_FOREGROUND:
		client_info->fg_cmd = true;
		break;
	case VC_COMMAND_TYPE_BACKGROUND:
		client_info->bg_cmd = true;
		break;
	default:
		SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] not supported command type(%d)", type);
		return -1;
	}

	return 0;
}

int vcd_client_unset_command_type(int pid, int type)
{
	vc_client_info_s* client_info = NULL;

	client_info = __client_get_element(pid);
	if (NULL == client_info) {
		SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] vcd_client_get_pid : pid(%d) is not found", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}

	switch (type) {
	case VC_COMMAND_TYPE_FOREGROUND:
		client_info->fg_cmd = false;
		break;
	case VC_COMMAND_TYPE_BACKGROUND:
		client_info->bg_cmd = false;
		break;
	default:
		SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] not supported command type(%d)", type);
		return -1;
	}

	return 0;
}

int vcd_client_set_exclusive_command(int pid)
{
	vc_client_info_s* client_info = NULL;

	client_info = __client_get_element(pid);
	if (NULL == client_info) {
		SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] pid(%d) is not found", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}

	client_info->exclusive_cmd = true;

	return 0;
}

int vcd_client_unset_exclusive_command(int pid)
{
	vc_client_info_s* client_info = NULL;

	client_info = __client_get_element(pid);
	if (NULL == client_info) {
		SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] pid(%d) is not found", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}

	client_info->exclusive_cmd = false;

	return 0;
}

int vcd_client_save_client_info()
{
	if (0 != vc_info_parser_set_client_info(g_client_list)) {
		SLOG(LOG_ERROR, TAG_VCD, "[ERROR] Fail to save client info");
		return -1;
	}

	return 0;
}

/*
* Functions for widget
*/
GSList* __widget_get_item(int pid)
{
	GSList *iter = NULL;
	widget_info_s *data = NULL;

	int count = g_slist_length(g_widget_list);
	int i;

	if (0 < count) {
		iter = g_slist_nth(g_widget_list, 0);
		for (i = 0;i < count;i++) {
			data = iter->data;

			if (pid == data->pid) 
				return iter;

			iter = g_slist_next(iter);
		}
	}

	return NULL;
}

widget_info_s* __widget_get_element(int pid)
{
	GSList *iter = NULL;
	widget_info_s *data = NULL;
	
	int count = g_slist_length(g_widget_list);
	int i;

	if (0 < count) {
		iter = g_slist_nth(g_widget_list, 0);
		for (i = 0;i < count;i++) {
			data = iter->data;

			if (NULL != data) {
				if (pid == data->pid)
					return data;
			}

			iter = g_slist_next(iter);
		}
	}

	return NULL;
}

int vcd_client_widget_get_list(int** pids, int* pid_count)
{
	if (NULL == pids || NULL == pid_count)
		return -1;

	int count = g_slist_length(g_widget_list);

	if (0 == count)
		return -1;

	int *tmp;
	tmp = (int*)calloc(count, sizeof(int));
	if (NULL == tmp) {
		SLOG(LOG_ERROR, TAG_VCD, "[ERROR] Fail to allocate memory");
		return VCD_ERROR_OUT_OF_MEMORY;
	}

	GSList *iter = NULL;
	widget_info_s *data = NULL;
	int i = 0;

	iter = g_slist_nth(g_widget_list, 0);
	while (NULL != iter) {
		data = iter->data;

		if (NULL != data) {
			tmp[i] = data->pid;
		}

		iter = g_slist_next(iter);
		i++;
	}

	*pids = tmp;
	*pid_count = count;

	return 0;
}

int vcd_client_widget_add(int pid)
{
	/*Check pid is duplicated*/
	GSList *tmp = NULL;
	tmp = __widget_get_item(pid);

	if (NULL != tmp) {
		SLOG(LOG_WARN, TAG_VCD, "[Client Data] widget pid is already registered");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	widget_info_s *info = (widget_info_s*)calloc(1, sizeof(widget_info_s));
	if (NULL == info) {
		SLOG(LOG_ERROR, TAG_VCD, "[ERROR] Fail to allocate memory");
		return VCD_ERROR_OUT_OF_MEMORY;
	}

	info->pid = pid;
	info->widget_cmd = false;

	/* Add item to global list */
	g_widget_list = g_slist_append(g_widget_list, info);

	if (NULL == g_widget_list) {
		SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] Fail to add new widget");
		return -1;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Client Data SUCCESS] Add new widget");
	}

#ifdef CLIENT_DATA_DEBUG
	__show_client_list();
#endif 
	return 0;
}

int vcd_client_widget_delete(int pid)
{
	GSList *tmp = NULL;
	widget_info_s* widget_info = NULL;

	/*Get handle*/
	tmp = __widget_get_item(pid);
	if (NULL == tmp) {
		SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] pid(%d) is NOT valid", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}

	/*Free client structure*/
	widget_info = tmp->data;
	if (NULL != widget_info) {
		free(widget_info);
	}

	/*Remove handle from list*/
	g_widget_list = g_slist_remove_link(g_widget_list, tmp);


#ifdef CLIENT_DATA_DEBUG
	__show_client_list();
#endif 

	return 0;
}

int vcd_client_widget_get_foreground_pid()
{
	/* 1. Get foreground pid */
	int fg_pid = 0;
	if (0 != vcd_config_get_foreground(&fg_pid)) {
		SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] Fail to get foreground pid");
		/* There is no foreground app for voice control */
	}

	widget_info_s* widget = NULL;

	widget = __widget_get_element(fg_pid);
	if (NULL == widget) {
		SLOG(LOG_WARN, TAG_VCD, "[Client Data] Not found foreground pid of widget");
		return -1;
	}

	return widget->pid;
}


bool vcd_client_widget_is_available(int pid)
{
	widget_info_s* widget_info = NULL;

	widget_info = __widget_get_element(pid);
	if (NULL == widget_info) {
		SLOG(LOG_WARN, TAG_VCD, "[Client Data] pid(%d) is NOT valid", pid);
		return false;
	}

	return true;
}

int vcd_client_widget_set_command(int pid)
{
	widget_info_s *info = NULL;
	info = __widget_get_element(pid);
	if (NULL == info) {
		SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] pid(%d) is NOT valid", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}

	info->widget_cmd = true;
	
	return 0;
}

int vcd_client_widget_unset_command(int pid)
{
	widget_info_s *info = NULL;
	info = __widget_get_element(pid);
	if (NULL == info) {
		SLOG(LOG_ERROR, TAG_VCD, "[Client Data ERROR] pid(%d) is NOT valid", pid);
		return VCD_ERROR_INVALID_PARAMETER;
	}

	info->widget_cmd = false;
	
	return 0;
}