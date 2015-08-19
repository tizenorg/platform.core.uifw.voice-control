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


#ifndef __VC_WIDGET_CLIENT_H_
#define __VC_WIDGET_CLIENT_H_

#include "voice_control_common.h"
#include "voice_control_widget.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
* Common function
*/
int vc_widget_client_create(vc_h* vc);

int vc_widget_client_destroy(vc_h vc);

bool vc_widget_client_is_valid(vc_h vc);

bool vc_widget_client_is_valid_by_uid(int uid);

int vc_widget_client_get_handle(int uid, vc_h* vc);

/*
* set/get callback function
*/
int vc_widget_client_set_result_cb(vc_h vc, vc_result_cb callback, void* user_data);

int vc_widget_client_get_result_cb(vc_h vc, vc_result_cb* callback, void** user_data);

int vc_widget_client_set_service_state_changed_cb(vc_h vc, vc_service_state_changed_cb callback, void* user_data);

int vc_widget_client_get_service_state_changed_cb(vc_h vc, vc_service_state_changed_cb* callback, void** user_data);

int vc_widget_client_set_state_changed_cb(vc_h vc, vc_state_changed_cb callback, void* user_data);

int vc_widget_client_get_state_changed_cb(vc_h vc, vc_state_changed_cb* callback, void** user_data);

int vc_widget_client_set_show_tooltip_cb(vc_h vc, vc_widget_show_tooltip_cb callback, void* user_data);

int vc_widget_client_get_show_tooltip_cb(vc_h vc, vc_widget_show_tooltip_cb* callback, void** user_data);

int vc_widget_client_set_current_lang_changed_cb(vc_h vc, vc_current_language_changed_cb callback, void* user_data);

int vc_widget_client_get_current_lang_changed_cb(vc_h vc, vc_current_language_changed_cb* callback, void** user_data);

int vc_widget_client_set_error_cb(vc_h vc, vc_error_cb callback, void* user_data);

int vc_widget_client_get_error_cb(vc_h vc, vc_error_cb* callback, void** user_data);

int vc_widget_client_set_send_command_list_cb(vc_h vc, vc_widget_send_current_command_list_cb callback, void* user_data);

int vc_widget_client_get_send_command_list_cb(vc_h vc, vc_widget_send_current_command_list_cb* callback, void** user_data);



/*
* set/get option
*/
int vc_widget_client_set_service_state(vc_h vc, vc_service_state_e state);

int vc_widget_client_get_service_state(vc_h vc, vc_service_state_e* state);

int vc_widget_client_set_state(vc_h vc, vc_state_e state);

int vc_widget_client_get_state(vc_h vc, vc_state_e* state);

int vc_widget_client_get_state_by_uid(int uid, vc_state_e* state);

int vc_widget_client_get_before_state(vc_h vc, vc_state_e* state, vc_state_e* before_state);

int vc_widget_client_set_xid(vc_h vc, int xid);

int vc_widget_cilent_get_xid(vc_h vc, int* xid);

int vc_widget_client_set_error(vc_h vc, int reason);

int vc_widget_client_get_error(vc_h vc, int* reason);

int vc_widget_client_set_show_tooltip(vc_h vc, bool show);

int vc_widget_client_get_show_tooltip(vc_h vc, bool* show);


/* utils */
int vc_widget_client_get_count();

int vc_widget_client_use_callback(vc_h vc);

int vc_widget_client_not_use_callback(vc_h vc);


#ifdef __cplusplus
}
#endif

#endif /* __VC_WIDGET_CLIENT_H_ */
