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


#ifndef __VCD_DBUS_h__
#define __VCD_DBUS_h__

#include "vcd_main.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VCD_CLIENT_TYPE_NORMAL,
	VCD_CLIENT_TYPE_WIDGET,
	VCD_CLIENT_TYPE_MANAGER
} vcd_client_type_e;

int vcd_dbus_open_connection();

int vcd_dbus_close_connection();


int vcdc_send_hello(int pid, vcd_client_type_e type);

int vcdc_send_show_tooltip(int pid, bool show);

int vcdc_send_result(int pid, int cmd_type);

int vcdc_send_result_to_manager(int manger_pid);

int vcdc_send_speech_detected(int manger_pid);

int vcdc_send_error_signal(int pid, int reason, char *err_msg);


#ifdef __cplusplus
}
#endif

#endif	/* __VCD_DBUS_h__ */
