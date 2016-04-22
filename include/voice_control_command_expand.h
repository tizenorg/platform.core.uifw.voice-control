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


#ifndef __VOICE_CONTROL_COMMAND_EXPAND_H__
#define __VOICE_CONTROL_COMMAND_EXPAND_H__

#include <tizen.h>
#include <voice_control_command.h>


#ifdef __cplusplus
extern "C"
{
#endif


/**
* @brief Enumerations of command format.
* @since_tizen 2.4
*/
typedef enum {
	VC_CMD_FORMAT_FIXED = 0,	/**< fixed command only */
	VC_CMD_FORMAT_FIXED_AND_EXTRA,	/**< Fixed + extra unfixed command */
	VC_CMD_FORMAT_EXTRA_AND_FIXED,	/**< Extra unfixed + fixed command */
	VC_CMD_FORMAT_UNFIXED_ONLY	/**< Unfixed command */
} vc_cmd_format_e;


/**
* @brief Gets extra unfixed command.
* @since_tizen 2.4
*
* @remark If the function succeeds, @a The command must be released with free() by you if they are not NULL.
*	If you get the result command list in result callback and the command type of commands has non-fixed format,
*	you should check non-fixed result using this function.
*
* @param[in] vc_command The command handle
* @param[out] command The unfixed command text
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported feature
*/
int vc_cmd_get_unfixed_command(vc_cmd_h vc_command, char** command);

/**
* @brief Sets command format.
* @since_tizen 2.4
*
* @param[in] vc_command The command handle
* @param[in] format The command format
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported feature
*
* @see vc_cmd_get_format()
*/
int vc_cmd_set_format(vc_cmd_h vc_command, vc_cmd_format_e format);

/**
* @brief Gets command format.
* @since_tizen 2.4
*
* @remark If you do not set the format, the default format is #VC_CMD_FORMAT_FIXED.
*
* @param[in] vc_command The command handle
* @param[out] format The command format
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported feature
*
* @see vc_cmd_set_format()
*/
int vc_cmd_get_format(vc_cmd_h vc_command, vc_cmd_format_e* format);

/**
* @brief Sets command domain
* @since_tizen 2.4
*
* @param[in] vc_command The command handle
* @param[in] domain The domain
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported feature
*
* @see vc_cmd_get_domain()
*/
int vc_cmd_set_domain(vc_cmd_h vc_command, int domain);

/**
* @brief Gets command domain.
* @since_tizen 2.4
*
* @param[in] vc_command The command handle
* @param[out] domain The domain
*
* @return 0 on success, otherwise a negative error value
* @retval #VC_ERROR_NONE Successful
* @retval #VC_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VC_ERROR_PERMISSION_DENIED Permission denied
* @retval #VC_ERROR_NOT_SUPPORTED Not supported feature
*
* @see vc_cmd_set_domain()
*/
int vc_cmd_get_domain(vc_cmd_h vc_command, int* domain);

/**
* @brief Retrieves all commands of command list using callback function.
* @since_tizen 3.0
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
int vc_cmd_list_filter_by_type(vc_cmd_list_h original, int type, vc_cmd_list_h* filtered);

#ifdef __cplusplus
}
#endif

/**
 * @}@}
 */

#endif /* __VOICE_CONTROL_COMMAND_EXPAND_H__ */
