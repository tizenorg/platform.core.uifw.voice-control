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


#ifndef __VOICE_CONTROL_PLUGIN_ENGINE_H__
#define __VOICE_CONTROL_PLUGIN_ENGINE_H__

#include <tizen.h>

/**
* @addtogroup VOICE_CONTROL_PLUGIN_ENGINE
* @{
*/

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Enumerations of error codes.
*/
typedef enum {
	VCP_ERROR_NONE			= TIZEN_ERROR_NONE,		/**< Successful */
	VCP_ERROR_OUT_OF_MEMORY		= TIZEN_ERROR_OUT_OF_MEMORY,	/**< Out of Memory */
	VCP_ERROR_IO_ERROR		= TIZEN_ERROR_IO_ERROR,		/**< I/O error */
	VCP_ERROR_INVALID_PARAMETER	= TIZEN_ERROR_INVALID_PARAMETER,/**< Invalid parameter */
	VCP_ERROR_OUT_OF_NETWORK	= TIZEN_ERROR_NETWORK_DOWN,	/**< Out of network */
	VCP_ERROR_INVALID_STATE		= -0x0100031,			/**< Invalid state */
	VCP_ERROR_INVALID_LANGUAGE	= -0x0100032,			/**< Invalid language */
	VCP_ERROR_OPERATION_FAILED	= -0x0100034,			/**< Operation failed */
	VCP_ERROR_NOT_SUPPORTED_FEATURE	= -0x0100035			/**< Not supported feature */
} vcp_error_e;

/**
* @brief Enumerations of audio type.
*/
typedef enum {
	VCP_AUDIO_TYPE_PCM_S16_LE = 0,	/**< Signed 16bit audio type, Little endian */
	VCP_AUDIO_TYPE_PCM_U8		/**< Unsigned 8bit audio type */
} vcp_audio_type_e;

/**
* @brief Enumerations of callback event.
*/
typedef enum {
	VCP_RESULT_EVENT_SUCCESS = 0,		/**< Event when the recognition full result is ready  */
	VCP_RESULT_EVENT_REJECTED,		/**< Event when the recognition result is rejected */
	VCP_RESULT_EVENT_ERROR			/**< Event when the recognition has failed */
} vcp_result_event_e;

/**
* @brief Enumerations of command type.
*/
typedef enum {
	VCP_COMMAND_TYPE_FIXED = 0,		/**< Fixed command */
	VCP_COMMAND_TYPE_FIXED_AND_NON_FIXED,	/**< Fixed command + Non-fixed command */
	VCP_COMMAND_TYPE_NON_FIXED_AND_FIXED	/**< Non-fixed command + Fixed command */
} vcp_command_type_e;

/**
* @brief Enumerations of speech detect.
*/
typedef enum {
	VCP_SPEECH_DETECT_NONE = 0,	/**< No event */
	VCP_SPEECH_DETECT_BEGIN,	/**< Begin of speech detected */
	VCP_SPEECH_DETECT_END,		/**< End of speech detected */
} vcp_speech_detect_e;

/**
* @brief A structure of handle for VC command
*/
typedef int vcp_cmd_h;

/**
 * @brief Defines of bluetooth audio id.
*/
#define VCP_AUDIO_ID_BLUETOOTH		"VC_AUDIO_ID_BLUETOOTH"		/**< Bluetooth audio id */

/**
* @brief Definition for none message.
*/
#define VC_RESULT_MESSAGE_NONE			"vc.result.message.none"

/**
* @brief Definition for failed recognition because the speech is too loud to listen.
*/
#define VC_RESULT_MESSAGE_ERROR_TOO_LOUD	"vc.result.message.error.too.loud"


/**
* @brief Called when the daemon gets synthesized result.
*
* @param[in] event A result event
* @param[in] result_id Result ids
* @param[in] count Result count
* @param[in] all_result All result text
* @param[in] non_fixed_result Non-fixed command result text
* @param[in] msg Engine message (e.g. #VC_RESULT_MESSAGE_NONE, #VC_RESULT_MESSAGE_ERROR_TOO_LOUD)
* @param[in] user_data The user data passed from the start synthesis function
*
* @pre vcpe_stop() will invoke this callback.
*
* @see vcpe_stop()
*/
typedef void (*vcpe_result_cb)(vcp_result_event_e event, int* result_id, int count, const char* all_result,
							   const char* non_fixed_result, const char* msg, void *user_data);

/**
* @brief Called to retrieve the supported languages.
*
* @param[in] language A language is specified as an ISO 3166 alpha-2 two letter country-code
*		followed by ISO 639-1 for the two-letter language code \n
*		For example, "ko_KR" for Korean, "en_US" for American English
* @param[in] user_data The user data passed from the foreach function
*
* @return @c true to continue with the next iteration of the loop \n @c false to break out of the loop
*
* @pre vcpe_foreach_supported_languages() will invoke this callback.
*
* @see vcpe_foreach_supported_languages()
*/
typedef bool (*vcpe_supported_language_cb)(const char* language, void* user_data);

/**
* @brief Initializes the engine.
*
* @param[in] result_cb A callback function for recognition result
* @param[in] silence_cb A callback function for silence detection
*
* @return 0 on success, otherwise a negative error value
* @retval #VCP_ERROR_NONE Successful
* @retval #VCP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VCP_ERROR_INVALID_STATE Already initialized
* @retval #VCP_ERROR_OPERATION_FAILED Operation failed
*
* @see vcpe_deinitialize()
*/
typedef int (*vcpe_initialize)(void);

/**
* @brief Deinitializes the engine
*
* @return 0 on success, otherwise a negative error value
* @retval #VCP_ERROR_NONE Successful
* @retval #VCP_ERROR_INVALID_STATE Not initialized
*
* @see vcpe_initialize()
*/
typedef void (*vcpe_deinitialize)(void);

/**
* @brief Registers a callback function for getting recognition result.
*
* @param[in] callback Callback function to register
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #VCP_ERROR_NONE Successful
* @retval #VCP_ERROR_INVALID_PARAMETER Invalid parameter
*
* @see vcpe_result_cb()
*/
typedef int (*vcpe_set_result_cb)(vcpe_result_cb callback, void* user_data);

/**
* @brief Gets recording format of the engine.
*
* @param[in] audio_id The audio device id.
* @param[out] types The format used by the recorder.
* @param[out] rate The sample rate used by the recorder.
* @param[out] channels The number of channels used by the recorder.
*
* @return 0 on success, otherwise a negative error value
* @retval #VCP_ERROR_NONE Successful
* @retval #VCP_ERROR_INVALID_PARAMETER Not initialized
*/
typedef int (*vcpe_get_recording_format)(const char* audio_id, vcp_audio_type_e* types, int* rate, int* channels);

/**
* @brief Retrieves all supported languages of the engine.
*
* @param[in] callback a callback function
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #VCP_ERROR_NONE Successful
* @retval #VCP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VCP_ERROR_INVALID_STATE Not initialized
*
* @post	This function invokes vcpe_supported_language_cb() repeatedly for getting supported languages.
*
* @see vcpe_supported_language_cb()
*/
typedef int (*vcpe_foreach_supported_languages)(vcpe_supported_language_cb callback, void* user_data);

/**
* @brief Checks whether a language is supported or not.
*
* @param[in] language A language
*
* @return @c true = supported, \n @c false = not supported.
*/
typedef bool (*vcpe_is_language_supported)(const char* language);

/**
* @brief Sets language.
*
* @param[in] language language.
*
* @return 0 on success, otherwise a negative error value
* @retval #VCP_ERROR_NONE Successful
* @retval #VCP_ERROR_INVALID_LANGUAGE Invalid language
* @retval #VCP_ERROR_INVALID_STATE Not initialized
*/
typedef int (*vcpe_set_language)(const char* language);

/**
* @brief Sets command list before recognition.
*
* @remark This function should set commands via vcpd_foreach_command().
*
* @param[in] vcp_command command handle.
*
* @return 0 on success, otherwise a negative error value
* @retval #VCP_ERROR_NONE Successful
* @retval #VCP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VCP_ERROR_INVALID_STATE Invalid state
* @retval #VCP_ERROR_OPERATION_FAILED Operation failed
* @retval #VCP_ERROR_NOT_SUPPORTED_FEATURE Not supported command type
*
* @post vcpe_start() is called after this function is successful.
*
* @see vcpe_start()
* @see vcpd_foreach_command()
* @see vcpe_unset_commands()
*/
typedef int (*vcpe_set_commands)(vcp_cmd_h vcp_command);

/**
* @brief Unset command list for reset.
*
* @return 0 on success, otherwise a negative error value
* @retval #VCP_ERROR_NONE Successful
* @retval #VCP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VCP_ERROR_INVALID_STATE Invalid state
* @retval #VCP_ERROR_OPERATION_FAILED Operation failed
*
* @see vcpe_set_commands()
*/
typedef int (*vcpe_unset_commands)();

/**
* @brief Start recognition.
*
* @return 0 on success, otherwise a negative error value
* @retval #VCP_ERROR_NONE Successful
* @retval #VCP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VCP_ERROR_INVALID_STATE Invalid state
* @retval #VCP_ERROR_INVALID_LANGUAGE Invalid language
* @retval #VCP_ERROR_OUT_OF_NETWORK Out of network
* @retval #VCP_ERROR_OPERATION_FAILED Operation failed
*
* @pre vcpd_foreach_command() is successful.
*
* @see vcpe_set_recording_data()
* @see vcpe_stop()
* @see vcpe_cancel()
*/
typedef int (*vcpe_start)(bool stop_by_silence);

/**
* @brief Sets recording data for speech recognition from recorder.
*
* @remark This function should be returned immediately after recording data copy.
*
* @param[in] data A recording data
* @param[in] length A length of recording data
* @param[out] silence_detected @c true Silence detected \n @c false No silence detected
*
* @return 0 on success, otherwise a negative error value
* @retval #VCP_ERROR_NONE Successful
* @retval #VCP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VCP_ERROR_INVALID_STATE Invalid state
* @retval #VCP_ERROR_OPERATION_FAILED Operation failed
*
* @pre vcpe_start() is successful.
*
* @see vcpe_start()
* @see vcpe_cancel()
* @see vcpe_stop()
*/
typedef int(*vcpe_set_recording_data)(const void* data, unsigned int length, vcp_speech_detect_e* speech_detected);

/**
* @brief Stops to get the result of recognition.
*
* @return 0 on success, otherwise a negative error value
* @retval #VCP_ERROR_NONE Successful
* @retval #VCP_ERROR_INVALID_STATE Invalid state
* @retval #VCP_ERROR_OPERATION_FAILED Operation failed
* @retval #VCP_ERROR_OUT_OF_NETWORK Out of network
*
* @pre vcpe_set_recording_data() is successful.
*
* @see vcpe_start()
* @see vcpe_set_recording_data()
* @see vcpe_result_cb()
* @see vcpe_cancel()
*/
typedef int (*vcpe_stop)(void);

/**
* @brief Cancels the recognition process.
*
* @return 0 on success, otherwise a negative error value.
* @retval #VCP_ERROR_NONE Successful.
* @retval #VCP_ERROR_INVALID_STATE Invalid state.
*
* @pre vcpe_start() is successful.
*
* @see vcpe_start()
* @see vcpe_stop()
*/
typedef int (*vcpe_cancel)(void);


/**
* Daemon API.
*/

/**
* @brief Called to retrieve the commands.
*
* @param[in] id command id
* @param[in] type command type
* @param[in] command command text
* @param[in] param parameter text
* @param[in] domain command domain
* @param[in] user_data The user data passed from the foreach function
*
* @return @c true to continue with the next iteration of the loop, \n @c false to break out of the loop.
* @pre vcpd_foreach_command() will invoke this callback.
*
* @see vcpd_foreach_command()
*/
typedef bool (*vcpd_foreach_command_cb)(int id, int type, const char* command, const char* param, int domain, void* user_data);

/**
* @brief Retrieves all commands using callback function.
*
* @param[in] vcp_command The handle to be passed to the vcpe_set_commands() function
* @param[in] callback The callback function to invoke
* @param[in] user_data The user data to be passed to the callback function
*
* @return 0 on success, otherwise a negative error value
* @retval #VCP_ERROR_NONE Successful
* @retval #VCP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VCP_ERROR_OPERATION_FAILED Operation failure
* @retval #VCP_ERROR_INVALID_STATE Invalid state
*
* @post	This function invokes vcpd_foreach_command_cb() repeatedly for getting commands.
*
* @see vcpd_foreach_command_cb()
* @see vcpe_set_commands()
*/
typedef int (*vcpd_foreach_command)(vcp_cmd_h vcp_command, vcpd_foreach_command_cb callback, void* user_data);

/**
* @brief Gets command length.
*
* @param[in] vcp_command The handle to be passed to the vcpe_set_commands() function
*
* @return the value greater than 0 on success, otherwise a negative error value
*
* @see vcpe_set_commands()
*/
typedef int (*vcpd_get_command_count)(vcp_cmd_h vcp_command);

/**
* @brief Gets current audio type.
*
* @remarks audio_type must be released using free() when it is no longer required.
*
* @param[in] audio_type Current audio type  (e.g. #VCP_AUDIO_ID_BLUETOOTH or usb device id)
*
* @return the value greater than 0 on success, otherwise a negative error value
*
*/
typedef int (*vcpd_get_audio_type)(char** audio_type);

/**
* @brief A structure of the engine functions.
*/
typedef struct {
	int size;						/**< Size of structure */
	int version;						/**< Version */

	vcpe_initialize			initialize;		/**< Initialize engine */
	vcpe_deinitialize		deinitialize;		/**< Shutdown engine */

	/* Get engine information */
	vcpe_get_recording_format	get_recording_format;	/**< Get recording format */
	vcpe_foreach_supported_languages foreach_langs;		/**< Foreach language list */
	vcpe_is_language_supported	is_lang_supported;	/**< Check language */

	/* Set info */
	vcpe_set_result_cb		set_result_cb;		/**< Set result callback */
	vcpe_set_language		set_language;		/**< Set language */
	vcpe_set_commands		set_commands;		/**< Request to set current commands */
	vcpe_unset_commands		unset_commands;		/**< Request to unset current commands */

	/* Control recognition */
	vcpe_start			start;			/**< Start recognition */
	vcpe_set_recording_data		set_recording;		/**< Set recording data */
	vcpe_stop			stop;			/**< Stop recording for getting result */
	vcpe_cancel			cancel;			/**< Cancel recording and processing */
} vcpe_funcs_s;

/**
* @brief A structure of the daemon functions.
*/
typedef struct {
	int size;						/**< Size of structure */
	int version;						/**< Version */

	vcpd_foreach_command		foreach_command;	/**< Foreach command */
	vcpd_get_command_count		get_command_count;	/**< Get command count */

	vcpd_get_audio_type		get_audio_type;		/**< Get audio type */
} vcpd_funcs_s;

/**
* @brief Loads the engine.
*
* @param[in] pdfuncs The daemon functions
* @param[out] pefuncs The engine functions
*
* @return This function returns zero on success, or negative with error code on failure
* @retval #VCP_ERROR_NONE Successful
* @retval #VCP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VCP_ERROR_OPERATION_FAILED Operation failed
*
* @pre The vcp_get_engine_info() should be successful.
* @post The daemon calls engine functions of vcpe_funcs_s.
*
* @see vcp_get_engine_info()
* @see vcp_unload_engine()
*/
int vcp_load_engine(vcpd_funcs_s* pdfuncs, vcpe_funcs_s* pefuncs);

/**
* @brief Unloads this engine by the daemon.
*
* @pre The vcp_load_engine() should be successful.
*
* @see vcp_load_engine()
*/
void vcp_unload_engine(void);

/**
* @brief Called to get the engine base information.
*
* @param[in] engine_uuid The engine id
* @param[in] engine_name The engine name
* @param[in] engine_setting The setting ug name
* @param[in] use_network @c true to need network @c false not to need network.
* @param[in] user_data The User data passed from vcp_get_engine_info()
*
* @pre vcp_get_engine_info() will invoke this callback.
*
* @see vcp_get_engine_info()
*/
typedef void (*vcpe_engine_info_cb)(const char* engine_uuid, const char* engine_name, const char* engine_setting,
									bool use_network, void* user_data);

/**
* @brief Gets the engine base information before the engine is loaded by the daemon.
*
* @param[in] callback Callback function
* @param[in] user_data User data to be passed to the callback function
*
* @return This function returns zero on success, or negative with error code on failure
* @retval #VCP_ERROR_NONE Successful
* @retval #VCP_ERROR_INVALID_PARAMETER Invalid parameter
* @retval #VCP_ERROR_OPERATION_FAILED Operation failed
*
* @post	This function invokes vcpe_engine_info_cb() for getting engine information.
*
* @see vcpe_engine_info_cb()
* @see vcp_load_engine()
*/
int vcp_get_engine_info(vcpe_engine_info_cb callback, void* user_data);


#ifdef __cplusplus
}
#endif

/**
* @}@}
*/

#endif /* __VOICE_CONTROL_PLUGIN_ENGINE_H__ */
