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


#ifndef __VCD_CLIENT_DATA_H_
#define __VCD_CLIENT_DATA_H_

#include <glib.h>
#include "vc_command.h"
#include "vc_info_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int	pid;
	int	cmd_count;
	
	GSList*	cmds;
}background_command_s;

typedef struct {
	int total_cmd_count;

	/* Foreground application */
	int foreground;
	GSList*	widget_cmds;
	GSList*	foreground_cmds;

	/* Manager application */
	GSList*	system_cmds;
	GSList*	exclusive_system_cmds;

	/* Other applications */
	int	bg_cmd_count;
	GSList*	background_cmds;
}current_commands_list_s;


typedef struct {
	int	pid;
	bool	manager_cmd;
	bool	exclusive_cmd_option;
}manager_info_s;

typedef struct {
	int	pid;
	bool	widget_cmd;
}widget_info_s;


/*
* Command API
*/
typedef bool (* client_foreach_command_cb)(int id, int type, const char* command, const char* param, int domain, void* user_data);

int vcd_client_command_collect_command();

int vcd_client_get_length();

int vcd_client_foreach_command(client_foreach_command_cb callback, void* user_data);

int vcd_client_get_cmd_from_result_id(int result_id, vc_cmd_s** result);

int vcd_client_get_cmd_info_from_result_id(int result_id, int* pid, int* cmd_type, vc_cmd_s** result);

int vcd_client_set_slience_detection(bool value);

bool vcd_client_get_slience_detection();

/*
* Manager API
*/
int vcd_client_manager_set(int pid);

int vcd_client_manager_unset();

bool vcd_client_manager_is_valid(int pid);

int vcd_client_manager_set_command(int pid);

int vcd_client_manager_unset_command(int pid);

int vcd_client_manager_set_demandable_client(int pid, GSList* client_list);

bool vcd_client_manager_check_demandable_client(int pid);

bool vcd_client_manager_get_exclusive();

int vcd_client_manager_set_exclusive(bool value);

int vcd_client_manager_get_pid();


/*
* client API
*/
int vcd_client_add(int pid);

int vcd_client_delete(int pid);

bool vcd_client_is_available(int pid);

int vcd_client_get_ref_count();

int vcd_client_get_list(int** pids, int* pid_count);

int vcd_client_set_command_type(int pid, int type);

int vcd_client_unset_command_type(int pid, int type);

int vcd_client_set_exclusive_command(int pid);

int vcd_client_unset_exclusive_command(int pid);

int vcd_client_save_client_info();

/*
* widget API
*/
int vcd_client_widget_get_list(int** pids, int* pid_count);

int vcd_client_widget_add(int pid);

int vcd_client_widget_delete(int pid);

bool vcd_client_widget_is_available(int pid);

int vcd_client_widget_get_foreground_pid();

int vcd_client_widget_set_command(int pid);

int vcd_client_widget_unset_command(int pid);


#ifdef __cplusplus
}
#endif

#endif	/* __VCD_CLIENT_DATA_H_ */

