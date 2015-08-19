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


#ifndef __VC_MANAGER_CLIENT_H_
#define __VC_MANAGER_CLIENT_H_


#include "vc_info_parser.h"
#include "voice_control_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
* Common function
*/
int vc_mgr_client_create(vc_h* vc);

int vc_mgr_client_destroy(vc_h vc);

bool vc_mgr_client_is_valid(vc_h vc);

bool vc_mgr_client_is_valid_by_uid(int uid);

int vc_mgr_client_get_handle(int uid, vc_h* vc);

int vc_mgr_client_get_pid(vc_h vc, int* pid);

/*
* set/get callback function
*/
int vc_mgr_client_set_all_result_cb(vc_h vc, vc_mgr_all_result_cb callback, void* user_data);

int vc_mgr_client_get_all_result_cb(vc_h vc, vc_mgr_all_result_cb* callback, void** user_data);

int vc_mgr_client_set_result_cb(vc_h vc, vc_result_cb callback, void* user_data);

int vc_mgr_client_get_result_cb(vc_h vc, vc_result_cb* callback, void** user_data);

int vc_mgr_client_set_service_state_changed_cb(vc_h vc, vc_service_state_changed_cb callback, void* user_data);

int vc_mgr_client_get_service_state_changed_cb(vc_h vc, vc_service_state_changed_cb* callback, void** user_data);

int vc_mgr_client_set_state_changed_cb(vc_h vc, vc_state_changed_cb callback, void* user_data);

int vc_mgr_client_get_state_changed_cb(vc_h vc, vc_state_changed_cb* callback, void** user_data);

int vc_mgr_client_set_speech_detected_cb(vc_h vc, vc_mgr_begin_speech_detected_cb callback, void* user_data);

int vc_mgr_client_get_speech_detected_cb(vc_h vc, vc_mgr_begin_speech_detected_cb* callback, void** user_data);

int vc_mgr_client_set_current_lang_changed_cb(vc_h vc, vc_current_language_changed_cb callback, void* user_data);

int vc_mgr_client_get_current_lang_changed_cb(vc_h vc, vc_current_language_changed_cb* callback, void** user_data);

int vc_mgr_client_set_error_cb(vc_h vc, vc_error_cb callback, void* user_data);

int vc_mgr_client_get_error_cb(vc_h vc, vc_error_cb* callback, void** user_data);


/*
* set/get option
*/
int vc_mgr_client_set_service_state(vc_h vc, vc_service_state_e state);

int vc_mgr_client_get_service_state(vc_h vc, vc_service_state_e* state);

int vc_mgr_client_set_client_state(vc_h vc, vc_state_e state);

int vc_mgr_client_get_client_state(vc_h vc, vc_state_e* state);

int vc_mgr_client_get_client_state_by_uid(int uid, vc_state_e* state);

int vc_mgr_client_get_before_state(vc_h vc, vc_state_e* state, vc_state_e* before_state);

int vc_mgr_client_set_error(vc_h vc, int reason);

int vc_mgr_client_get_error(vc_h vc, int* reason);

int vc_mgr_client_set_exclusive_command(vc_h vc, bool value);

bool vc_mgr_client_get_exclusive_command(vc_h vc);

int vc_mgr_client_set_all_result(vc_h vc, int event, const char* result_text);

int vc_mgr_client_get_all_result(vc_h vc, int* event, char** result_text);

int vc_mgr_client_unset_all_result(vc_h vc);

int vc_mgr_client_set_audio_type(vc_h vc, const char* audio_id);

int vc_mgr_client_get_audio_type(vc_h vc, char** audio_id);

int vc_mgr_client_set_recognition_mode(vc_h vc, vc_recognition_mode_e mode);

int vc_mgr_client_get_recognition_mode(vc_h vc, vc_recognition_mode_e* mode);


/* utils */
int vc_mgr_client_get_count();

int vc_mgr_client_use_callback(vc_h vc);

int vc_mgr_client_not_use_callback(vc_h vc);

/* Authority */
int vc_mgr_client_add_authorized_client(vc_h vc, int pid);

int vc_mgr_client_remove_authorized_client(vc_h vc, int pid);

bool vc_mgr_client_is_authorized_client(vc_h vc, int pid);

int vc_mgr_client_set_valid_authorized_client(vc_h vc, int pid);

int vc_mgr_client_get_valid_authorized_client(vc_h vc, int* pid);

bool vc_mgr_client_is_valid_authorized_client(vc_h vc, int pid);

int vc_mgr_client_set_start_by_client(vc_h vc, bool option);

int vc_mgr_client_get_start_by_client(vc_h vc, bool* option);

#ifdef __cplusplus
}
#endif

#endif /* __VC_MANAGER_CLIENT_H_ */
