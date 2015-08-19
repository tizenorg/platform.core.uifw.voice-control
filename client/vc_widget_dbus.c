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


#include "vc_main.h"
#include "vc_widget_client.h"
#include "vc_widget_dbus.h"


static int g_w_waiting_time = 3000;

static Ecore_Fd_Handler* g_w_fd_handler = NULL;

static DBusConnection* g_w_conn_sender = NULL;
static DBusConnection* g_w_conn_listener = NULL;

extern int __vc_widget_cb_error(int pid, int reason);

extern void __vc_widget_cb_show_tooltip(int pid, bool show);

extern void __vc_widget_cb_result();


static Eina_Bool widget_listener_event_callback(void* data, Ecore_Fd_Handler *fd_handler)
{
	if (NULL == g_w_conn_listener)	return ECORE_CALLBACK_RENEW;

	dbus_connection_read_write_dispatch(g_w_conn_listener, 50);

	DBusMessage* msg = NULL;
	msg = dbus_connection_pop_message(g_w_conn_listener);

	/* loop again if we haven't read a message */
	if (NULL == msg) { 
		return ECORE_CALLBACK_RENEW;
	}

	DBusError err;
	dbus_error_init(&err);

	char if_name[64];
	snprintf(if_name, 64, "%s", VC_WIDGET_SERVICE_INTERFACE);

	if (dbus_message_is_method_call(msg, if_name, VCD_WIDGET_METHOD_HELLO)) {
		SLOG(LOG_DEBUG, TAG_VCW, "===== Get widget hello");
		int pid = 0;
		int response = -1;

		dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);
		
		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Dbus Error (%s)", err.message);
			dbus_error_free(&err);
		}

		if (pid > 0) {
			SLOG(LOG_DEBUG, TAG_VCW, "<<<< vc widget get hello : pid(%d) ", pid);
			response = 1;
		} else {
			SLOG(LOG_ERROR, TAG_VCW, "<<<< vc widget get hello : invalid pid ");
		}

		DBusMessage* reply = NULL;
		reply = dbus_message_new_method_return(msg);
		
		if (NULL != reply) {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &response, DBUS_TYPE_INVALID);

			if (!dbus_connection_send(g_w_conn_listener, reply, NULL))
				SLOG(LOG_ERROR, TAG_VCW, ">>>> vc widget get hello : fail to send reply");
			else 
				SLOG(LOG_DEBUG, TAG_VCW, ">>>> vc widget get hello : result(%d)", response);

			dbus_connection_flush(g_w_conn_listener);
			dbus_message_unref(reply);
		} else {
			SLOG(LOG_ERROR, TAG_VCW, ">>>> vc widget get hello : fail to create reply message");
		}
		
		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
	} /* VCD_WIDGET_METHOD_HELLO */

	else if (dbus_message_is_method_call(msg, if_name, VCD_WIDGET_METHOD_SHOW_TOOLTIP)) {
		SLOG(LOG_DEBUG, TAG_VCW, "===== Show / Hide tooltip");
		int pid = 0;
		int show = 0;

		dbus_message_get_args(msg, &err, 
			DBUS_TYPE_INT32, &pid, 
			DBUS_TYPE_INT32, &show,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Dbus Error (%s)", err.message);
			dbus_error_free(&err);
		}

		if (pid > 0) {
			SLOG(LOG_DEBUG, TAG_VCW, "<<<< vc widget show tooltip : pid(%d), show(%d)", pid, show);

			__vc_widget_cb_show_tooltip(pid, (bool)show);
		} else {
			SLOG(LOG_ERROR, TAG_VCW, "<<<< vc widget show tooltip : invalid pid");
		}

		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
	} /* VCD_WIDGET_METHOD_SHOW_TOOLTIP */

	else if (dbus_message_is_method_call(msg, if_name, VCD_WIDGET_METHOD_RESULT)) {
		SLOG(LOG_DEBUG, TAG_VCW, "===== Get widget result");

		__vc_widget_cb_result();

		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");

	} /* VCD_WIDGET_METHOD_RESULT */

	else if (dbus_message_is_method_call(msg, if_name, VCD_WIDGET_METHOD_ERROR)) {
		SLOG(LOG_DEBUG, TAG_VCW, "===== Get widget error");
		int pid;
		int reason;
		char* err_msg;

		dbus_message_get_args(msg, &err,
			DBUS_TYPE_INT32, &pid,
			DBUS_TYPE_INT32, &reason,
			DBUS_TYPE_STRING, &err_msg,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) { 
			SLOG(LOG_ERROR, TAG_VCW, "<<<< vc widget get error message : Get arguments error (%s)", err.message);
			dbus_error_free(&err); 
		} else {
			SLOG(LOG_DEBUG, TAG_VCW, "<<<< vc widget get error message : pid(%d), reason(%d), msg(%s)", pid, reason, err_msg);
			__vc_widget_cb_error(pid, reason);
		}

		SLOG(LOG_DEBUG, TAG_VCW, "=====");
		SLOG(LOG_DEBUG, TAG_VCW, " ");
	} /* VCD_WIDGET_METHOD_ERROR */

	/* free the message */
	dbus_message_unref(msg);

	return ECORE_CALLBACK_PASS_ON;
}

int vc_widget_dbus_open_connection()
{
	if (NULL != g_w_conn_sender && NULL != g_w_conn_listener) {
		SLOG(LOG_WARN, TAG_VCW, "Already existed connection ");
		return 0;
	}

	DBusError err;
	int ret;

	/* initialise the error value */
	dbus_error_init(&err);

	/* connect to the DBUS system bus, and check for errors */
	g_w_conn_sender = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCW, "Dbus Connection Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL == g_w_conn_sender) {
		SLOG(LOG_ERROR, TAG_VCW, "Fail to get dbus connection ");
		return VC_ERROR_OPERATION_FAILED;
	}

	g_w_conn_listener = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCW, "Dbus Connection Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL == g_w_conn_listener) {
		SLOG(LOG_ERROR, TAG_VCW, "Fail to get dbus connection ");
		return VC_ERROR_OPERATION_FAILED;
	}

	int pid = getpid();

	char service_name[64];
	memset(service_name, '\0', 64);
	snprintf(service_name, 64, "%s", VC_WIDGET_SERVICE_NAME);

	SLOG(LOG_DEBUG, TAG_VCW, "service name is %s", service_name);

	/* register our name on the bus, and check for errors */
	ret = dbus_bus_request_name(g_w_conn_listener, service_name, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCW, "Name Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
		SLOG(LOG_ERROR, TAG_VCW, "fail dbus_bus_request_name()");
		return -2;
	}

	if (NULL != g_w_fd_handler) {
		SLOG(LOG_WARN, TAG_VCW, "The handler already exists.");
		return 0;
	}

	char rule[128] = {0, };
	snprintf(rule, 128, "type='signal',interface='%s'", VC_WIDGET_SERVICE_INTERFACE);

	/* add a rule for which messages we want to see */
	dbus_bus_add_match(g_w_conn_listener, rule, &err);
	dbus_connection_flush(g_w_conn_listener);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCW, "Match Error (%s)", err.message);
		dbus_error_free(&err);
		return VC_ERROR_OPERATION_FAILED;
	}

	int fd = 0;
	if (1 != dbus_connection_get_unix_fd(g_w_conn_listener, &fd)) {
		SLOG(LOG_ERROR, TAG_VCW, "fail to get fd from dbus ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCW, "Get fd from dbus : %d", fd);
	}

	g_w_fd_handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ, (Ecore_Fd_Cb)widget_listener_event_callback, g_w_conn_listener, NULL, NULL);

	if (NULL == g_w_fd_handler) {
		SLOG(LOG_ERROR, TAG_VCW, "fail to get fd handler from ecore ");
		return VC_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int vc_widget_dbus_close_connection()
{
	DBusError err;
	dbus_error_init(&err);

	if (NULL != g_w_fd_handler) {
		ecore_main_fd_handler_del(g_w_fd_handler);
		g_w_fd_handler = NULL;
	}

	int pid = getpid();

	char service_name[64];
	memset(service_name, '\0', 64);
	snprintf(service_name, 64, "%s", VC_WIDGET_SERVICE_NAME);

	dbus_bus_release_name(g_w_conn_listener, service_name, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	g_w_conn_sender = NULL;
	g_w_conn_listener = NULL;

	return 0;
}

int vc_widget_dbus_reconnect()
{
	bool sender_connected = dbus_connection_get_is_connected(g_w_conn_sender);
	bool listener_connected = dbus_connection_get_is_connected(g_w_conn_listener);
	SLOG(LOG_DEBUG, TAG_VCW, "[DBUS] Sender(%s) Listener(%s)",
		 sender_connected ? "Connected" : "Not connected", listener_connected ? "Connected" : "Not connected");

	if (false == sender_connected || false == listener_connected) {
		vc_widget_dbus_close_connection();

		if (0 != vc_widget_dbus_open_connection()) {
			SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to reconnect");
			return -1;
		}

		SLOG(LOG_DEBUG, TAG_VCW, "[DBUS] Reconnect");
	}

	return 0;
}

int vc_widget_dbus_request_hello()
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_METHOD_HELLO);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCW, ">>>> Request vc hello : Fail to make message");
		return VC_ERROR_OPERATION_FAILED;
	}

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg = NULL;
	int result = 0;

	result_msg = dbus_connection_send_with_reply_and_block(g_w_conn_sender, msg, 500, &err);

	if (dbus_error_is_set(&err)) {
		dbus_error_free(&err);
	}

	dbus_message_unref(msg);

	if (NULL != result_msg) {
		dbus_message_unref(result_msg);
		result = 0;
	} else {
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}


int vc_widget_dbus_request_initialize(int pid)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_WIDGET_METHOD_INITIALIZE);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCW, ">>>> vc widget initialize : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCW, ">>>> vc widget initialize : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_w_conn_sender, msg, g_w_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCW, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCW, "<<<< vc widget initialize : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCW, "<<<< vc widget initialize : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_VCW, "<<<< Result message is NULL ");
		vc_widget_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_widget_dbus_request_finalize(int pid)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_WIDGET_METHOD_FINALIZE);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCW, ">>>> vc widget finalize : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCW, ">>>> vc widget finalize : pid(%d)", pid);
	}

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_w_conn_sender, msg, g_w_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCW, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCW, "<<<< vc widget finalize : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCW, "<<<< vc widget finalize : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_VCW, "<<<< Result message is NULL ");
		vc_widget_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_widget_dbus_request_start_recording(int pid, bool command)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_WIDGET_METHOD_START_RECORDING);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCW, ">>>> vc widget start recording : Fail to make message");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCW, ">>>> vc widget start recording : pid(%d)", pid);
	}

	int temp = (int)command;

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INT32, &temp,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_w_conn_sender, msg, g_w_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCW, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCW, "<<<< vc widget start recording : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCW, "<<<< vc widget start recording : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_VCW, "<<<< Result message is NULL");
		vc_widget_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_widget_dbus_request_start(int pid, int silence)
{
	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_WIDGET_METHOD_START);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCW, ">>>> vc widget start : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCW, ">>>> vc widget start : pid(%d), silence(%d)", pid, silence);
	}

	DBusMessageIter args;
	dbus_message_iter_init_append(msg, &args);

	/* Append result*/
	dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(pid));
	dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(silence));

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_w_conn_sender, msg, g_w_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCW, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCW, "<<<< vc widget start : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCW, "<<<< vc widget start : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCW, "<<<< Result Message is NULL");
		vc_widget_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_widget_dbus_request_stop(int pid)
{
	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_WIDGET_METHOD_STOP);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCW, ">>>> vc widget stop : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCW, ">>>> vc widget stop : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_w_conn_sender, msg, g_w_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCW, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCW, "<<<< vc widget stop : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCW, "<<<< vc widget stop : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCW, "<<<< Result Message is NULL");
		vc_widget_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_widget_dbus_request_cancel(int pid)
{
	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,	/* object name of the signal */
			  VC_SERVER_SERVICE_INTERFACE,	/* interface name of the signal */
			  VC_WIDGET_METHOD_CANCEL);	/* name of the signal */

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCW, ">>>> vc widget cancel : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCW, ">>>> vc widget cancel : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_w_conn_sender, msg, g_w_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCW, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCW, "<<<< vc widget cancel : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCW, "<<<< vc widget cancel : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCW, "<<<< Result Message is NULL");
		vc_widget_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}