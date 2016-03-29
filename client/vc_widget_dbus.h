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


#ifndef __VC_WIDGET_DBUS_H_
#define __VC_WIDGET_DBUS_H_


#ifdef __cplusplus
extern "C" {
#endif

int vc_widget_dbus_open_connection();

int vc_widget_dbus_close_connection();


int vc_widget_dbus_request_hello();

int vc_widget_dbus_request_initialize(int pid, int* service_state);

int vc_widget_dbus_request_finalize(int pid);

int vc_widget_dbus_request_start_recording(int pid, bool command);

int vc_widget_dbus_set_foreground(int pid, bool value);


int vc_widget_dbus_request_start(int pid, int silence);

int vc_widget_dbus_request_stop(int pid);

int vc_widget_dbus_request_cancel(int pid);


#ifdef __cplusplus
}
#endif

#endif /* __VC_WIDGET_DBUS_H_ */
