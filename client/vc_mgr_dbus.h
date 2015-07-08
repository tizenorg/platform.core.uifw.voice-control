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

 
#ifndef __VC_DBUS_H_
#define __VC_DBUS_H_


#ifdef __cplusplus
extern "C" {
#endif

int vc_mgr_dbus_open_connection();

int vc_mgr_dbus_close_connection();


int vc_mgr_dbus_request_hello();

int vc_mgr_dbus_request_initialize(int pid);

int vc_mgr_dbus_request_finalize(int pid);

int vc_mgr_dbus_request_set_command(int pid);

int vc_mgr_dbus_request_unset_command(int pid);

int vc_mgr_dbus_request_demandable_client(int pid);

int vc_mgr_dbus_request_set_audio_type(int pid, const char* audio_type);

int vc_mgr_dbus_request_get_audio_type(int pid, char** audio_type);

int vc_mgr_dbus_request_set_client_info(int pid);

int vc_mgr_dbus_request_start(int pid, int silence, bool exclusive_command_option, bool start_by_client);

int vc_mgr_dbus_request_stop(int pid);

int vc_mgr_dbus_request_cancel(int pid);

int vc_mgr_dbus_send_result(int pid, int cmd_type, int result_id);

int vc_mgr_dbus_send_result_selection(int pid);


#ifdef __cplusplus
}
#endif

#endif /* __VC_DBUS_H_ */
