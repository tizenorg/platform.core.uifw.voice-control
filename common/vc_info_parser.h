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


#ifndef __VC_CMD_PARSER_H_
#define __VC_CMD_PARSER_H_

#include <glib.h>

#include "vc_command.h"
#include "voice_control_command.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EXPORT_API
#define EXPORT_API
#endif // EXPORT_API



typedef struct _demandable_client_s {
	char*	appid;
} vc_demandable_client_s;

typedef struct _client_s {
	int	pid;
	bool	fg_cmd;
	bool	bg_cmd;
	bool	exclusive_cmd;
} vc_client_info_s;


EXPORT_API int vc_cmd_parser_save_file(int pid, vc_cmd_type_e type, GSList* cmd_list);

EXPORT_API int vc_cmd_parser_delete_file(int pid, vc_cmd_type_e type);

EXPORT_API int vc_cmd_parser_get_commands(int pid, vc_cmd_type_e type, GSList** cmd_list);

EXPORT_API int vc_cmd_parser_get_command_info(int pid, vc_cmd_type_e type, int index, vc_cmd_s** info);

EXPORT_API int vc_cmd_parser_append_commands(int pid, vc_cmd_type_e type, vc_cmd_list_h vc_cmd_list);


/* client request rule */
EXPORT_API int vc_info_parser_set_demandable_client(const char* filepath);

EXPORT_API int vc_info_parser_get_demandable_clients(GSList** client_list);


/* Result info */
EXPORT_API int vc_info_parser_set_result(const char* result_text, int event, const char* msg, vc_cmd_list_h vc_cmd_list, bool exclusive);

EXPORT_API int vc_info_parser_get_result(char** result_text, int* event, char** result_message, int pid, vc_cmd_list_h vc_cmd_list, bool exclusive);

EXPORT_API int vc_info_parser_unset_result(bool exclusive);

EXPORT_API int vc_info_parser_get_result_pid_list(GSList** pid_list);


/* Client info */
EXPORT_API int vc_info_parser_set_client_info(GSList* client_info_list);

EXPORT_API int vc_info_parser_get_client_info(GSList** client_info_list);


/* for debug */
EXPORT_API int __vc_cmd_parser_print_commands(GSList* cmd_list);

EXPORT_API int __vc_info_parser_print_results(GSList* result_list);


#ifdef __cplusplus
}
#endif

#endif /* __VC_CMD_PARSER_H_ */
