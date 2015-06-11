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

 
#ifndef __VC_CONFIG_MANAGER_H_
#define __VC_CONFIG_MANAGER_H_

#include <stdbool.h>
#include <voice_control_common.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VC_CONFIG_ERROR_NONE			= TIZEN_ERROR_NONE,			/**< Success, No error */
	VC_CONFIG_ERROR_OUT_OF_MEMORY		= TIZEN_ERROR_OUT_OF_MEMORY,		/**< Out of Memory */
	VC_CONFIG_ERROR_IO_ERROR		= TIZEN_ERROR_IO_ERROR,			/**< I/O error */
	VC_CONFIG_ERROR_INVALID_PARAMETER	= TIZEN_ERROR_INVALID_PARAMETER,	/**< Invalid parameter */
	VC_CONFIG_ERROR_INVALID_STATE		= TIZEN_ERROR_VOICE_CONTROL | 0x011,	/**< Invalid state */
	VC_CONFIG_ERROR_INVALID_LANGUAGE	= TIZEN_ERROR_VOICE_CONTROL | 0x012,	/**< Invalid voice */
	VC_CONFIG_ERROR_ENGINE_NOT_FOUND	= TIZEN_ERROR_VOICE_CONTROL | 0x013,	/**< No available VC-engine  */
	VC_CONFIG_ERROR_OPERATION_FAILED	= TIZEN_ERROR_VOICE_CONTROL | 0x014	/**< Operation failed  */
}vc_config_error_e;


typedef void (*vc_config_lang_changed_cb)(const char* before_lang, const char* current_lang);

typedef void (*vc_config_foreground_changed_cb)(int previous, int current);

typedef void (*vc_config_state_changed_cb)(int previous, int current);

typedef void (*vc_config_enabled_cb)(bool enable);


int vc_config_mgr_initialize(int uid);

int vc_config_mgr_finalize(int uid);


/* Set / Unset callback */
int vc_config_mgr_set_lang_cb(int uid, vc_config_lang_changed_cb lang_cb);

int vc_config_mgr_unset_lang_cb(int uid);

int vc_config_mgr_set_service_state_cb(int uid, vc_config_state_changed_cb state_cb);

int vc_config_mgr_unset_service_state_cb(int uid);

int vc_config_mgr_set_foreground_cb(int uid, vc_config_foreground_changed_cb foreground_cb);

int vc_config_mgr_unset_foreground_cb(int uid);

int vc_config_mgr_set_enabled_cb(int uid, vc_config_enabled_cb enabled_cb);

int vc_config_mgr_unset_enabled_cb(int uid);


int vc_config_mgr_get_auto_language(bool* value);

int vc_config_mgr_set_auto_language(bool value);

int vc_config_mgr_get_language_list(vc_supported_language_cb callback, void* user_data);

int vc_config_mgr_get_default_language(char** language);

int vc_config_mgr_set_default_language(const char* language);

int vc_config_mgr_get_enabled(bool* value);

int vc_config_mgr_set_enabled(bool value);


int vc_config_mgr_get_nonfixed_support(bool* value);

bool vc_config_check_default_language_is_valid(const char* language);

int vc_config_convert_error_code(vc_config_error_e code);


int vc_config_mgr_set_service_state(int state);

int vc_config_mgr_get_service_state(int* state);


int vc_config_mgr_set_foreground(int pid, bool value);

int vc_config_mgr_get_foreground(int* pid);


#ifdef __cplusplus
}
#endif

#endif /* __VC_CONFIG_MANAGER_H_ */
