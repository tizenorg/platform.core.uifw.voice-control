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


#ifndef __VOICE_CONTROL_INTERNAL_COMMAND_h_
#define __VOICE_CONTROL_INTERNAL_COMMAND_h_

#include <glib.h>

#include "voice_control_command.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _command_s {
	int	pid;
	int	id;
	int	index;
	int	type;
	int	format;
	int	domain;
	char*	command;
	char*	parameter;

	/* not used */
	int	key;
	int	modifier;
} vc_cmd_s;

typedef struct {
	int index;
	GSList*	list;
} vc_cmd_list_s;

/**
* @brief Enumerations of command type.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*/
typedef enum {
	VC_COMMAND_TYPE_NONE = 0,	/**< No command position */
	VC_COMMAND_TYPE_FOREGROUND = 1,	/**< Foreground command by client*/
	VC_COMMAND_TYPE_BACKGROUND = 2,	/**< Background command by client */
	VC_COMMAND_TYPE_WIDGET = 3,	/**< Widget command by widget client */
	VC_COMMAND_TYPE_SYSTEM = 4,	/**< System command by manager client */
	VC_COMMAND_TYPE_EXCLUSIVE = 5	/**< exclusive command by manager client */
} vc_cmd_type_e;


int vc_cmd_set_id(vc_cmd_h vc_command, int id);

int vc_cmd_get_id(vc_cmd_h vc_command, int* id);

int vc_cmd_print_list(vc_cmd_list_h vc_cmd_list);

/**
* @brief Remove all commands from command list.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*
* @param[in] vc_cmd_list The command list handle
* @param[in] free_command The command free option @c true = release each commands in list,
*			@c false = remove command from list
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_CMD_ERROR_NONE Successful
* @retval #VC_CMD_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_CMD_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_CMD_ERROR_NOT_SUPPORTED Not supported feature
*
* @see vc_cmd_list_add()
*/
int vc_cmd_list_remove_all(vc_cmd_list_h vc_cmd_list, bool free_command);

/**
* @brief Sets extra unfixed command.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*
* @param[in] vc_command The command handle
* @param[in] command The unfixed command
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_CMD_ERROR_NONE Successful
* @retval #VC_CMD_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_CMD_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_CMD_ERROR_NOT_SUPPORTED Not supported feature
*
* @see vc_cmd_get_non_fixed_command()
*/
int vc_cmd_set_unfixed_command(vc_cmd_h vc_command, const char* command);

/**
* @brief Sets pid.
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*
* @param[in] vc_command The command handle
* @param[in] pid process id
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_CMD_ERROR_NONE Successful
* @retval #VC_CMD_ERROR_INVALID_PARAMETER Invalid parameter
*
* @see vc_cmd_set_pid()
*/
int vc_cmd_set_pid(vc_cmd_h vc_command, int pid);

/**
* @brief Sets command domain
* @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
*
* @param[in] vc_command The command handle
* @param[in] domain domain
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_CMD_ERROR_NONE Successful
* @retval #VC_CMD_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_CMD_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_CMD_ERROR_NOT_SUPPORTED Not supported feature
*
* @see vc_cmd_get_domain()
*/
int vc_cmd_get_pid(vc_cmd_h vc_command, int* pid);


#ifdef __cplusplus
}
#endif

#endif /* __VOICE_CONTROL_INTERNAL_COMMAND_h_ */
