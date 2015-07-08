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


#ifndef __VOICE_CONTROL_COMMAND_H__
#define __VOICE_CONTROL_COMMAND_H__

#include <tizen.h>


/**
* @defgroup CAPI_UIX_VOICE_CONTROL_COMMAND_MODULE Voice control command
* @ingroup CAPI_UIX_VOICE_CONTROL_MODULE
*
* @brief The @ref CAPI_UIX_VOICE_CONTROL_COMMAND_MODULE API provides functions for creating/destroying command list and add/remove/retrieve commands of list.
* @{
*/

#ifdef __cplusplus
extern "C" 
{
#endif


/**
* @brief The voice command handle.
* @since_tizen 2.4
*/
typedef struct vc_cmd_s* vc_cmd_h;

/**
* @brief The voice command list handle.
* @since_tizen 2.4
*/
typedef struct vc_cmd_list_s* vc_cmd_list_h;

/**
* @brief Called to retrieve The commands in list.
* @since_tizen 2.4
*
* @param[in] vc_command The command handle
* @param[in] user_data The user data passed from the foreach function
*
* @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop.
* @pre vc_cmd_list_foreach_commands() will invoke this callback.
*
* @see vc_cmd_list_foreach_commands()
*/
typedef bool (*vc_cmd_list_cb)(vc_cmd_h vc_command, void* user_data);


/**
* @brief Creates a handle for command list.
* @since_tizen 2.4
*
* @remarks If the function succeeds, @a The list handle must be released with vc_cmd_list_destroy().
*
* @param[out] vc_cmd_list The command list handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_OUT_OF_MEMORY Out of memory
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @see vc_cmd_list_destroy()
*/
int vc_cmd_list_create(vc_cmd_list_h* vc_cmd_list);

/**
* @brief Destroys the handle for command list.
* @since_tizen 2.4
*
* @param[in] vc_cmd_list The command list handle
* @param[in] free_command The command free option @c true = release each commands in list, 
*			@c false = remove command from list
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @see vc_cmd_list_create()
*/
int vc_cmd_list_destroy(vc_cmd_list_h vc_cmd_list, bool free_command);

/**
* @brief Gets command count of list.
* @since_tizen 2.4
*
* @param[in] vc_cmd_list The command list handle
* @param[out] count The count
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*/
int vc_cmd_list_get_count(vc_cmd_list_h vc_cmd_list, int* count);

/**
* @brief Adds command to command list.
* @since_tizen 2.4
*
* @param[in] vc_cmd_list The command list handle
* @param[in] vc_command The command handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @see vc_cmd_list_remove()
*/
int vc_cmd_list_add(vc_cmd_list_h vc_cmd_list, vc_cmd_h vc_command);

/**
* @brief Removes command from command list.
* @since_tizen 2.4
*
* @param[in] vc_cmd_list The command list handle
* @param[in] vc_command The command handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @see vc_cmd_list_add()
*/
int vc_cmd_list_remove(vc_cmd_list_h vc_cmd_list, vc_cmd_h vc_command);

/**
* @brief Retrieves all commands of command list using callback function.
* @since_tizen 2.4
*
* @param[in] vc_cmd_list The command list handle
* @param[in] callback Callback function to invoke
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @post	This function invokes vc_cmd_list_cb() repeatedly for getting commands.
*
* @see vc_cmd_list_cb()
*/
int vc_cmd_list_foreach_commands(vc_cmd_list_h vc_cmd_list, vc_cmd_list_cb callback, void* user_data);

/**
* @brief Moves index to first command.
* @since_tizen 2.4
*
* @param[in] vc_cmd_list The command list handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_EMPTY List empty
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @see vc_cmd_list_last()
*/
int vc_cmd_list_first(vc_cmd_list_h vc_cmd_list);

/**
* @brief Moves index to last command.
* @since_tizen 2.4
*
* @param[in] vc_cmd_list The command list handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_EMPTY List empty
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @see vc_cmd_list_first()
*/
int vc_cmd_list_last(vc_cmd_list_h vc_cmd_list);

/**
* @brief Moves index to next command.
* @since_tizen 2.4
*
* @param[in] vc_cmd_list The command list handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_EMPTY List empty
* @retval #VC_ERROR_ITERATION_END List reached end
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @see vc_cmd_list_prev()
*/
int vc_cmd_list_next(vc_cmd_list_h vc_cmd_list);

/**
* @brief Moves index to previous command.
* @since_tizen 2.4
*
* @param[in] vc_cmd_list The command list handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_EMPTY List empty
* @retval #VC_ERROR_ITERATION_END List reached end
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @see vc_cmd_list_next()
*/
int vc_cmd_list_prev(vc_cmd_list_h vc_cmd_list);

/**
* @brief Get current command from command list by index.
* @since_tizen 2.4
*
* @param[in] vc_cmd_list The command list handle
* @param[out] vc_command The command handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_EMPTY List empty
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @see vc_cmd_list_first()
* @see vc_cmd_list_last()
* @see vc_cmd_list_prev()
* @see vc_cmd_list_next()
*/
int vc_cmd_list_get_current(vc_cmd_list_h vc_cmd_list, vc_cmd_h* vc_command);


/**
* @brief Creates a handle for command.
* @since_tizen 2.4
*
* @remarks If the function succeeds, @a The command handle must be released 
*	with vc_cmd_destroy() or vc_cmd_list_destroy().
*	You should set command and type if command is valid
*
* @param[out] vc_command The command handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_OUT_OF_MEMORY Out of memory
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @see vc_cmd_destroy()
*/
int vc_cmd_create(vc_cmd_h* vc_command);

/**
* @brief Destroys the handle.
* @since_tizen 2.4
*
* @param[in] vc_command The command handle
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @see vc_cmd_create()
*/
int vc_cmd_destroy(vc_cmd_h vc_command);

/**
* @brief Sets command.
* @since_tizen 2.4
*
* @param[in] vc_command The command handle
* @param[in] command The command text
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @see vc_cmd_get_command()
*/
int vc_cmd_set_command(vc_cmd_h vc_command, const char* command);

/**
* @brief Gets command.
* @since_tizen 2.4
*
* @remark If the function succeeds, @a command must be released with free() by you if they are not NULL.
*
* @param[in] vc_command The command handle
* @param[out] command The command text
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @see vc_cmd_set_command()
*/
int vc_cmd_get_command(vc_cmd_h vc_command, char** command);

/**
* @brief Sets command type.
* @since_tizen 2.4
*
* @remark If you do not set the command type, the default value is -1.
*	You should set type if command is valid
*
* @param[in] vc_command The command handle
* @param[in] type The command type
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @see vc_cmd_get_type()
*/
int vc_cmd_set_type(vc_cmd_h vc_command, int type);

/**
* @brief Gets command type.
* @since_tizen 2.4
*
* @param[in] vc_command The command handle
* @param[out] type The command type
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported
*
* @see vc_cmd_set_type()
*/
int vc_cmd_get_type(vc_cmd_h vc_command, int* type);


#ifdef __cplusplus
}
#endif

/**
 * @}@}
 */

#endif /* __VOICE_CONTROL_COMMAND_H__ */
