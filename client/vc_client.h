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


#ifndef __VC_CLIENT_H_
#define __VC_CLIENT_H_

#include "vc_info_parser.h"
#include "vc_main.h"
#include "voice_control.h"
#include "voice_control_authority.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
* Common function
*/

#ifndef EXPORT_API
#define EXPORT_API
#endif // EXPORT_API

EXPORT_API int vc_client_create(vc_h* vc);

EXPORT_API int vc_client_destroy(vc_h vc);

EXPORT_API bool vc_client_is_valid(vc_h vc);

EXPORT_API bool vc_client_is_valid_by_uid(int uid);

EXPORT_API int vc_client_get_handle(int uid, vc_h* vc);

/*
* set/get callback function
*/
EXPORT_API int vc_client_set_result_cb(vc_h vc, vc_result_cb callback, void* user_data);

EXPORT_API int vc_client_get_result_cb(vc_h vc, vc_result_cb* callback, void** user_data);

EXPORT_API int vc_client_set_service_state_changed_cb(vc_h vc, vc_service_state_changed_cb callback, void* user_data);

EXPORT_API int vc_client_get_service_state_changed_cb(vc_h vc, vc_service_state_changed_cb* callback, void** user_data);

EXPORT_API int vc_client_set_state_changed_cb(vc_h vc, vc_state_changed_cb callback, void* user_data);

EXPORT_API int vc_client_get_state_changed_cb(vc_h vc, vc_state_changed_cb* callback, void** user_data);

EXPORT_API int vc_client_set_current_lang_changed_cb(vc_h vc, vc_current_language_changed_cb callback, void* user_data);

EXPORT_API int vc_client_get_current_lang_changed_cb(vc_h vc, vc_current_language_changed_cb* callback, void** user_data);

EXPORT_API int vc_client_set_error_cb(vc_h vc, vc_error_cb callback, void* user_data);

EXPORT_API int vc_client_get_error_cb(vc_h vc, vc_error_cb* callback, void** user_data);


/*
* set/get option
*/
EXPORT_API int vc_client_set_service_state(vc_h vc, vc_service_state_e state);

EXPORT_API int vc_client_get_service_state(vc_h vc, vc_service_state_e* state);

EXPORT_API int vc_client_set_client_state(vc_h vc, vc_state_e state);

EXPORT_API int vc_client_get_client_state(vc_h vc, vc_state_e* state);

EXPORT_API int vc_client_get_client_state_by_uid(int uid, vc_state_e* state);

EXPORT_API int vc_client_get_before_state(vc_h vc, vc_state_e* state, vc_state_e* before_state);

EXPORT_API int vc_client_set_xid(vc_h vc, int xid);

EXPORT_API int vc_client_get_xid(vc_h vc, int* xid);

EXPORT_API int vc_client_set_is_foreground(vc_h vc, bool value);

EXPORT_API int vc_client_get_is_foreground(vc_h vc, bool* value);

#if 0
int vc_client_set_exclusive_cmd(vc_h vc, bool value);

int vc_client_get_exclusive_cmd(vc_h vc, bool* value);
#endif

EXPORT_API int vc_client_set_error(vc_h vc, int reason);

EXPORT_API int vc_client_get_error(vc_h vc, int* reason);


/* utils */
EXPORT_API int vc_client_get_count();

EXPORT_API int vc_client_use_callback(vc_h vc);

EXPORT_API int vc_client_not_use_callback(vc_h vc);

/* Authority */
EXPORT_API int vc_client_set_auth_state_changed_cb(vc_h vc, vc_auth_state_changed_cb callback, void* user_data);

EXPORT_API int vc_client_get_auth_state_changed_cb(vc_h vc, vc_auth_state_changed_cb* callback, void** user_data);

EXPORT_API int vc_client_unset_auth_state_changed_cb(vc_h vc);

EXPORT_API int vc_client_set_auth_state(vc_h vc, vc_auth_state_e state);

EXPORT_API int vc_client_get_auth_state(vc_h vc, vc_auth_state_e* state);

EXPORT_API int vc_client_get_before_auth_state(vc_h vc, vc_auth_state_e* before, vc_auth_state_e* current);

EXPORT_API int vc_client_set_mgr_pid(vc_h vc, int mgr_pid);

EXPORT_API int vc_client_get_mgr_pid(vc_h vc, int* mgr_pid);


#ifdef __cplusplus
}
#endif

#endif /* __VC_CLIENT_H_ */
