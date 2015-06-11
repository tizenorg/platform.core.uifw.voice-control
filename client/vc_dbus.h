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

int vc_dbus_open_connection();

int vc_dbus_close_connection();


int vc_dbus_request_hello();

int vc_dbus_request_initialize(int pid, int* mgr_pid);

int vc_dbus_request_finalize(int pid);

int vc_dbus_request_set_exclusive_command(int pid, bool value);

int vc_dbus_request_set_command(int pid, vc_cmd_type_e cmd_type);

int vc_dbus_request_unset_command(int pid, vc_cmd_type_e cmd_type);

#if 0
int vc_dbus_request_start(int pid, int silence);

int vc_dbus_request_stop(int pid);

int vc_dbus_request_cancel(int pid);
#endif

/* Authority */
int vc_dbus_request_auth_enable(int pid, int mgr_pid);

int vc_dbus_request_auth_disable(int pid, int mgr_pid);

int vc_dbus_request_auth_start(int pid, int mgr_pid);

int vc_dbus_request_auth_stop(int pid, int mgr_pid);

int vc_dbus_request_auth_cancel(int pid, int mgr_pid);


#ifdef __cplusplus
}
#endif

#endif /* __VC_DBUS_H_ */
