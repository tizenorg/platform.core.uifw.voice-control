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

#include <dbus/dbus.h>
#include "vcd_client_data.h"
#include "vcd_dbus.h"
#include "vcd_dbus_server.h"
#include "vcd_main.h"


static DBusConnection* g_conn_sender = NULL;
static DBusConnection* g_conn_listener = NULL;

static Ecore_Fd_Handler* g_dbus_fd_handler = NULL;

static int g_waiting_time = 3000;


static DBusMessage* __get_message(int pid, const char* method, vcd_client_type_e type)
{
	char service_name[64] = {0,};
	char object_path[64] = {0,};
	char target_if_name[128] = {0,};

	if (VCD_CLIENT_TYPE_NORMAL == type) {
		snprintf(service_name, 64, "%s", VC_CLIENT_SERVICE_NAME);
		snprintf(object_path, 64, "%s", VC_CLIENT_SERVICE_OBJECT_PATH);
		snprintf(target_if_name, 128, "%s", VC_CLIENT_SERVICE_NAME);

	} else if (VCD_CLIENT_TYPE_WIDGET == type) {
		snprintf(service_name, 64, "%s", VC_WIDGET_SERVICE_NAME);
		snprintf(object_path, 64, "%s", VC_WIDGET_SERVICE_OBJECT_PATH);
		snprintf(target_if_name, 128, "%s", VC_WIDGET_SERVICE_INTERFACE);

	} else if (VCD_CLIENT_TYPE_MANAGER == type) {
		snprintf(service_name, 64, "%s", VC_MANAGER_SERVICE_NAME);
		snprintf(object_path, 64, "%s", VC_MANAGER_SERVICE_OBJECT_PATH);
		snprintf(target_if_name, 128, "%s", VC_MANAGER_SERVICE_INTERFACE);
	} else {
		return NULL;
	}

	return dbus_message_new_method_call(service_name, object_path, target_if_name, method);
}

int vcdc_send_hello(int pid, vcd_client_type_e type)
{
	DBusMessage* msg = NULL;

	if (VCD_CLIENT_TYPE_NORMAL == type) {
		msg = __get_message(pid, VCD_METHOD_HELLO, VCD_CLIENT_TYPE_NORMAL);
	} else if (VCD_CLIENT_TYPE_WIDGET == type) {
		msg = __get_message(pid, VCD_WIDGET_METHOD_HELLO, VCD_CLIENT_TYPE_WIDGET);
	} else if (VCD_CLIENT_TYPE_MANAGER == type) {
		msg = __get_message(pid, VCD_WIDGET_METHOD_HELLO, VCD_CLIENT_TYPE_MANAGER);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Client type is NOT valid");
		return -1;
	}

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Fail to create message");
		return VCD_ERROR_OUT_OF_MEMORY;
	}

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = -1;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] %s", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_VCD, "[Dbus] Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VCD_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Dbus] Result message is NULL. Client is not available");
		result = 0;
	}

	return result;
}

int vcdc_send_show_tooltip(int pid, bool show)
{
	if (0 > pid) {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] widget pid is NOT valid" );
		return -1;
	}

	char service_name[64];
	memset(service_name, 0, 64);
	snprintf(service_name, 64, "%s", VC_WIDGET_SERVICE_NAME);

	char target_if_name[128];
	snprintf(target_if_name, sizeof(target_if_name), "%s", VC_WIDGET_SERVICE_INTERFACE);

	DBusMessage* msg;

	SLOG(LOG_DEBUG, TAG_VCD, "[Dbus] send widget show tooltip signal : pid(%d) show(%d)", pid, show);

	msg = dbus_message_new_method_call(
		service_name, 
		VC_WIDGET_SERVICE_OBJECT_PATH, 
		target_if_name, 
		VCD_WIDGET_METHOD_SHOW_TOOLTIP);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Fail to create message");
		return VCD_ERROR_OUT_OF_MEMORY;
	}

	int temp = (int)show;

	DBusMessageIter args;
	dbus_message_iter_init_append(msg, &args);

	/* Append pid & type */
	dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &pid);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(temp));

	dbus_message_set_no_reply(msg, TRUE);

	if (1 != dbus_connection_send(g_conn_sender, msg, NULL)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Fail to Send");
		return VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Dbus] SUCCESS Send");
		dbus_connection_flush(g_conn_sender);
	}

	return 0;
}



int vcdc_send_result(int pid, int cmd_type)
{
	DBusMessage* msg = NULL;

	SLOG(LOG_DEBUG, TAG_VCD, "[Dbus] Result command type(%d)", cmd_type);

	switch (cmd_type) {
	case VC_COMMAND_TYPE_FOREGROUND:
		msg = __get_message(pid, VCD_METHOD_RESULT, VCD_CLIENT_TYPE_NORMAL);
		break;
	case VC_COMMAND_TYPE_BACKGROUND:
		msg = __get_message(pid, VCD_METHOD_RESULT, VCD_CLIENT_TYPE_NORMAL);
		break;
	case VC_COMMAND_TYPE_WIDGET:
		msg = __get_message(pid, VCD_WIDGET_METHOD_RESULT, VCD_CLIENT_TYPE_WIDGET);
		break;
	case VC_COMMAND_TYPE_SYSTEM:
	case VC_COMMAND_TYPE_EXCLUSIVE:
		msg = __get_message(pid, VCD_MANAGER_METHOD_RESULT, VCD_CLIENT_TYPE_MANAGER);
		break;

	default:
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Command type is NOT valid(%d)", cmd_type);
		return -1;
	}

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Message is NULL");
		return VCD_ERROR_OUT_OF_MEMORY;
	}

	dbus_message_set_no_reply(msg, TRUE);

	if (1 != dbus_connection_send(g_conn_sender, msg, NULL)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Fail to Send");
		return VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Dbus] SUCCESS Send");
		dbus_connection_flush(g_conn_sender);
	}

	return 0;
}

int vcdc_send_result_to_manager(int manger_pid)
{
	DBusError err;
	dbus_error_init(&err);

	DBusMessage* msg = NULL;

	msg = __get_message(manger_pid, VCD_MANAGER_METHOD_ALL_RESULT, VCD_CLIENT_TYPE_MANAGER);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Message is NULL");
		return VCD_ERROR_OUT_OF_MEMORY;
	}

	dbus_message_set_no_reply(msg, TRUE);

	if (1 != dbus_connection_send(g_conn_sender, msg, NULL)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Fail to Send");
		return VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Dbus] SUCCESS Send");
		dbus_connection_flush(g_conn_sender);
	}

	return 0;
}

int vcdc_send_speech_detected(int manger_pid)
{
	DBusError err;
	dbus_error_init(&err);

	DBusMessage* msg = NULL;

	msg = __get_message(manger_pid, VCD_MANAGER_METHOD_SPEECH_DETECTED, VCD_CLIENT_TYPE_MANAGER);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Message is NULL");
		return VCD_ERROR_OUT_OF_MEMORY;
	}

	dbus_message_set_no_reply(msg, TRUE);

	if (1 != dbus_connection_send(g_conn_sender, msg, NULL)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Fail to Send");
		return VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Dbus] SUCCESS Send");
		dbus_connection_flush(g_conn_sender);
	}

	return 0;
}

int vcdc_send_error_signal(int pid, int reason, char *err_msg)
{
	if (NULL == err_msg) {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Input parameter is NULL");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	char service_name[64];
	memset(service_name, 0, 64);
	snprintf(service_name, 64, "%s", VC_CLIENT_SERVICE_NAME);

	char target_if_name[128];
	snprintf(target_if_name, sizeof(target_if_name), "%s", VC_CLIENT_SERVICE_INTERFACE);

	DBusMessage* msg;
	SLOG(LOG_DEBUG, TAG_VCD, "[Dbus] send error signal : reason(%d), Error Msg(%s)", reason, err_msg);

	msg = dbus_message_new_method_call(
		service_name, 
		VC_CLIENT_SERVICE_OBJECT_PATH, 
		target_if_name, 
		VCD_METHOD_ERROR);

	if (NULL == msg) { 
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Fail to create message");
		return VCD_ERROR_OUT_OF_MEMORY;
	}

	dbus_message_append_args(msg, 
		DBUS_TYPE_INT32, &pid, 
		DBUS_TYPE_INT32, &reason, 
		DBUS_TYPE_STRING, &err_msg,
		DBUS_TYPE_INVALID);

	dbus_message_set_no_reply(msg, TRUE);

	if (1 != dbus_connection_send(g_conn_sender, msg, NULL)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Fail to Send");
		return VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Dbus] SUCCESS Send");
		dbus_connection_flush(g_conn_sender);
	}

	return 0;
}

static Eina_Bool listener_event_callback(void* data, Ecore_Fd_Handler *fd_handler)
{
	if (NULL == g_conn_listener)	return ECORE_CALLBACK_RENEW;

	dbus_connection_read_write_dispatch(g_conn_listener, 50);

	DBusMessage* msg = NULL;
	msg = dbus_connection_pop_message(g_conn_listener);

	/* loop again if we haven't read a message */
	if (NULL == msg) { 
		return ECORE_CALLBACK_RENEW;
	}

	/* Common event */
	if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_METHOD_HELLO))
		vcd_dbus_server_hello(g_conn_listener, msg);

	/* manager event */
	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_MANAGER_METHOD_INITIALIZE))
		vcd_dbus_server_mgr_initialize(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_MANAGER_METHOD_FINALIZE))
		vcd_dbus_server_mgr_finalize(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_MANAGER_METHOD_SET_COMMAND))
		vcd_dbus_server_mgr_set_command(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_MANAGER_METHOD_UNSET_COMMAND))
		vcd_dbus_server_mgr_unset_command(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_MANAGER_METHOD_SET_DEMANDABLE))
		vcd_dbus_server_mgr_set_demandable_client(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_MANAGER_METHOD_SET_AUDIO_TYPE))
		vcd_dbus_server_mgr_set_audio_type(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_MANAGER_METHOD_GET_AUDIO_TYPE))
		vcd_dbus_server_mgr_get_audio_type(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_MANAGER_METHOD_SET_CLIENT_INFO))
		vcd_dbus_server_mgr_set_client_info(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_MANAGER_METHOD_START))
		vcd_dbus_server_mgr_start(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_MANAGER_METHOD_STOP))
		vcd_dbus_server_mgr_stop(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_MANAGER_METHOD_CANCEL))
		vcd_dbus_server_mgr_cancel(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_MANAGER_METHOD_RESULT_SELECTION))
		vcd_dbus_server_mgr_result_selection(g_conn_listener, msg);


	/* client event */
	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_METHOD_INITIALIZE))
		vcd_dbus_server_initialize(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_METHOD_FINALIZE))
		vcd_dbus_server_finalize(g_conn_listener, msg);
#if 0
	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_METHOD_SET_EXCLUSIVE_CMD))
		vcd_dbus_server_set_exclusive_command(g_conn_listener, msg);
#endif
	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_METHOD_SET_COMMAND))
		vcd_dbus_server_set_command(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_METHOD_UNSET_COMMAND))
		vcd_dbus_server_unset_command(g_conn_listener, msg);
#if 0
	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_METHOD_REQUEST_START))
		vcd_dbus_server_start_request(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_METHOD_REQUEST_STOP))
		vcd_dbus_server_stop_request(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_METHOD_REQUEST_CANCEL))
		vcd_dbus_server_cancel_request(g_conn_listener, msg);
#endif
	/* widget event */
	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_WIDGET_METHOD_INITIALIZE))
		vcd_dbus_server_widget_initialize(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_WIDGET_METHOD_FINALIZE))
		vcd_dbus_server_widget_finalize(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_WIDGET_METHOD_START_RECORDING))
		vcd_dbus_server_widget_start_recording(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_WIDGET_METHOD_START))
		vcd_dbus_server_widget_start(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_WIDGET_METHOD_STOP))
		vcd_dbus_server_widget_stop(g_conn_listener, msg);

	else if (dbus_message_is_method_call(msg, VC_SERVER_SERVICE_INTERFACE, VC_WIDGET_METHOD_CANCEL))
		vcd_dbus_server_widget_cancel(g_conn_listener, msg);

	else 
		return ECORE_CALLBACK_RENEW;

	/* free the message */
	dbus_message_unref(msg);

	return ECORE_CALLBACK_RENEW;
}

int vcd_dbus_open_connection()
{
	DBusError err;
	dbus_error_init(&err);

	int ret;

	/* Create connection for sender */
	g_conn_sender = dbus_bus_get(DBUS_BUS_SYSTEM, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Fail dbus_bus_get : %s", err.message);
		dbus_error_free(&err);
	}

	if (NULL == g_conn_sender) {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Fail to get dbus connection");
		return VCD_ERROR_OPERATION_FAILED;
	}

	/* connect to the bus and check for errors */
	g_conn_listener = dbus_bus_get(DBUS_BUS_SYSTEM, &err);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Fail dbus_bus_get : %s", err.message);
		dbus_error_free(&err);
	}

	if (NULL == g_conn_listener) {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Fail to get dbus connection");
		return VCD_ERROR_OPERATION_FAILED;
	}

	/* request our name on the bus and check for errors */
	ret = dbus_bus_request_name(g_conn_listener, VC_SERVER_SERVICE_NAME, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);

	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
		printf("Fail to be primary owner in dbus request.");
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Fail to be primary owner");
		return VCD_ERROR_OPERATION_FAILED;
	}

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] dbus_bus_request_name() : %s", err.message);
		dbus_error_free(&err);
		return VCD_ERROR_OPERATION_FAILED;
	}

	/* add a rule for getting signal */
	char rule[128];
	snprintf(rule, 128, "type='signal',interface='%s'", VC_SERVER_SERVICE_INTERFACE);

	/* add a rule for which messages we want to see */
	dbus_bus_add_match(g_conn_listener, rule, &err);/* see signals from the given interface */
	dbus_connection_flush(g_conn_listener);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] dbus_bus_add_match() : %s", err.message);
		dbus_error_free(&err);
		return VCD_ERROR_OPERATION_FAILED;
	}

	int fd = 0;
	dbus_connection_get_unix_fd(g_conn_listener, &fd);

	g_dbus_fd_handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ, (Ecore_Fd_Cb)listener_event_callback, g_conn_listener, NULL, NULL);

	if (NULL == g_dbus_fd_handler) {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] Fail to get fd handler");
		return VCD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int vcd_dbus_close_connection()
{
	DBusError err;
	dbus_error_init(&err);

	if (NULL != g_dbus_fd_handler) {
		ecore_main_fd_handler_del(g_dbus_fd_handler);
		g_dbus_fd_handler = NULL;
	}

	dbus_bus_release_name(g_conn_listener, VC_SERVER_SERVICE_NAME, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Dbus ERROR] dbus_bus_release_name() : %s", err.message);
		dbus_error_free(&err);
	}

	g_conn_listener = NULL;
	g_conn_sender = NULL;

	return 0;
}