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


#ifndef __VCD_DBUS_SERVER_h__
#define __VCD_DBUS_SERVER_h__

#include <dbus/dbus.h>


#ifdef __cplusplus
extern "C" {
#endif

int vcd_dbus_server_hello(DBusConnection* conn, DBusMessage* msg);

/*
* Dbus Server functions for manager
*/

int vcd_dbus_server_mgr_initialize(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_mgr_finalize(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_mgr_set_command(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_mgr_unset_command(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_mgr_set_demandable_client(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_mgr_set_audio_type(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_mgr_get_audio_type(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_mgr_set_client_info(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_mgr_start(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_mgr_stop(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_mgr_cancel(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_mgr_result_selection(DBusConnection* conn, DBusMessage* msg);

/*
* Dbus Server functions for client
*/

int vcd_dbus_server_initialize(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_finalize(DBusConnection* conn, DBusMessage* msg);

#if 0
int vcd_dbus_server_set_exclusive_command(DBusConnection* conn, DBusMessage* msg);
#endif

int vcd_dbus_server_set_command(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_unset_command(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_set_foreground(DBusConnection* conn, DBusMessage* msg);

#if 0
int vcd_dbus_server_start_request(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_stop_request(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_cancel_request(DBusConnection* conn, DBusMessage* msg);
#endif

/*
* Dbus Server functions for widget
*/
int vcd_dbus_server_widget_initialize(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_widget_finalize(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_widget_start_recording(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_widget_start(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_widget_stop(DBusConnection* conn, DBusMessage* msg);

int vcd_dbus_server_widget_cancel(DBusConnection* conn, DBusMessage* msg);


#ifdef __cplusplus
}
#endif


#endif	/* __VCD_DBUS_SERVER_h__ */
