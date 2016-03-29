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


#include "vc_client.h"
#include "vc_dbus.h"
#include "vc_main.h"


static int g_waiting_time = 3000;

static Ecore_Fd_Handler* g_fd_handler = NULL;

static DBusConnection* g_conn_sender = NULL;
static DBusConnection* g_conn_listener = NULL;

extern int __vc_cb_error(int pid, int reason);

extern void __vc_cb_result();

extern int __vc_cb_service_state(int state);


static Eina_Bool listener_event_callback(void* data, Ecore_Fd_Handler *fd_handler)
{
	if (NULL == g_conn_listener)	return ECORE_CALLBACK_RENEW;

	dbus_connection_read_write_dispatch(g_conn_listener, 50);

	while (1) {
		DBusMessage* msg = NULL;
		msg = dbus_connection_pop_message(g_conn_listener);

		/* loop again if we haven't read a message */
		if (NULL == msg) {
			break;
		}

		DBusError err;
		dbus_error_init(&err);

		char if_name[64] = {0, };
		snprintf(if_name, 64, "%s", VC_CLIENT_SERVICE_INTERFACE);

		if (dbus_message_is_method_call(msg, if_name, VCD_METHOD_HELLO)) {
			SLOG(LOG_DEBUG, TAG_VCC, "===== Get Hello");
			int pid = 0;
			int response = -1;

			dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Dbus Error (%s)", err.message);
				dbus_error_free(&err);
			}

			if (pid > 0) {
				SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc get hello : pid(%d) ", pid);
				response = 1;
			} else {
				SLOG(LOG_ERROR, TAG_VCC, "<<<< vc get hello : invalid pid ");
			}

			DBusMessage* reply = NULL;
			reply = dbus_message_new_method_return(msg);

			if (NULL != reply) {
				dbus_message_append_args(reply, DBUS_TYPE_INT32, &response, DBUS_TYPE_INVALID);

				if (!dbus_connection_send(g_conn_listener, reply, NULL))
					SLOG(LOG_ERROR, TAG_VCC, ">>>> vc get hello : fail to send reply");
				else
					SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc get hello : result(%d)", response);

				dbus_connection_flush(g_conn_listener);
				dbus_message_unref(reply);
			} else {
				SLOG(LOG_ERROR, TAG_VCC, ">>>> vc get hello : fail to create reply message");
			}

			SLOG(LOG_DEBUG, TAG_VCC, "=====");
			SLOG(LOG_DEBUG, TAG_VCC, " ");
		} /* VCD_METHOD_HELLO */

		else if (dbus_message_is_signal(msg, if_name, VCD_METHOD_SET_SERVICE_STATE)) {
			int state = 0;

			dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &state, DBUS_TYPE_INVALID);
			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			}

			SLOG(LOG_DEBUG, TAG_VCC, "<<<< state changed : %d", state);

			__vc_cb_service_state(state);

		} /* VCD_METHOD_SET_SERVICE_STATE */

		else if (dbus_message_is_method_call(msg, if_name, VCD_METHOD_RESULT)) {
			SLOG(LOG_DEBUG, TAG_VCC, "===== Get Client Result");

			__vc_cb_result();

			SLOG(LOG_DEBUG, TAG_VCC, "=====");
			SLOG(LOG_DEBUG, TAG_VCC, " ");

		} /* VCD_METHOD_RESULT */

		else if (dbus_message_is_method_call(msg, if_name, VCD_METHOD_ERROR)) {
			SLOG(LOG_DEBUG, TAG_VCC, "===== Get Error");
			int pid;
			int reason;
			char* err_msg;

			dbus_message_get_args(msg, &err,
				DBUS_TYPE_INT32, &pid,
				DBUS_TYPE_INT32, &reason,
				DBUS_TYPE_STRING, &err_msg,
				DBUS_TYPE_INVALID);

			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_VCC, "<<<< vc Get Error message : Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			} else {
				SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc Get Error message : pid(%d), reason(%d), msg(%s)", pid, reason, err_msg);
				__vc_cb_error(pid, reason);
			}

			SLOG(LOG_DEBUG, TAG_VCC, "=====");
			SLOG(LOG_DEBUG, TAG_VCC, " ");
		} /* VCD_METHOD_ERROR */

		else {
			SLOG(LOG_DEBUG, TAG_VCC, "Message is NOT valid");
			dbus_message_unref(msg);
			break;
		}

		/* free the message */
		dbus_message_unref(msg);
	} /* while(1) */

	return ECORE_CALLBACK_PASS_ON;
}

int vc_dbus_open_connection()
{
	if (NULL != g_conn_sender && NULL != g_conn_listener) {
		SLOG(LOG_WARN, TAG_VCC, "already existed connection ");
		return 0;
	}

	DBusError err;
	int ret;

	/* initialise the error value */
	dbus_error_init(&err);

	/* connect to the DBUS system bus, and check for errors */
	g_conn_sender = dbus_bus_get_private(DBUS_BUS_SESSION, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "Dbus Connection Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL == g_conn_sender) {
		SLOG(LOG_ERROR, TAG_VCC, "Fail to get dbus connection ");
		return VC_ERROR_OPERATION_FAILED;
	}

	g_conn_listener = dbus_bus_get_private(DBUS_BUS_SESSION, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "Dbus Connection Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL == g_conn_listener) {
		SLOG(LOG_ERROR, TAG_VCC, "Fail to get dbus connection ");
		return VC_ERROR_OPERATION_FAILED;
	}

	int pid = getpid();

	char service_name[64];
	memset(service_name, '\0', 64);
	snprintf(service_name, 64, "%s%d", VC_CLIENT_SERVICE_NAME, pid);

	SLOG(LOG_DEBUG, TAG_VCC, "service name is %s", service_name);

	/* register our name on the bus, and check for errors */
	ret = dbus_bus_request_name(g_conn_listener, service_name, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "Name Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
		SLOG(LOG_ERROR, TAG_VCC, "fail dbus_bus_request_name()");
		return -2;
	}

	if (NULL != g_fd_handler) {
		SLOG(LOG_WARN, TAG_VCC, "The handler already exists.");
		return 0;
	}

	char rule[128] = {0, };
	snprintf(rule, 128, "type='signal',interface='%s'", VC_CLIENT_SERVICE_INTERFACE);

	/* add a rule for which messages we want to see */
	dbus_bus_add_match(g_conn_listener, rule, &err);
	dbus_connection_flush(g_conn_listener);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "Match Error (%s)", err.message);
		dbus_error_free(&err);
		return VC_ERROR_OPERATION_FAILED;
	}

	int fd = 0;
	if (1 != dbus_connection_get_unix_fd(g_conn_listener, &fd)) {
		SLOG(LOG_ERROR, TAG_VCC, "fail to get fd from dbus ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, "Get fd from dbus : %d", fd);
	}

	g_fd_handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ, (Ecore_Fd_Cb)listener_event_callback, g_conn_listener, NULL, NULL);
	if (NULL == g_fd_handler) {
		SLOG(LOG_ERROR, TAG_VCC, "fail to get fd handler from ecore ");
		return VC_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int vc_dbus_close_connection()
{
	DBusError err;
	dbus_error_init(&err);

	if (NULL != g_fd_handler) {
		ecore_main_fd_handler_del(g_fd_handler);
		g_fd_handler = NULL;
	}

	int pid = getpid();

	char service_name[64];
	memset(service_name, '\0', 64);
	snprintf(service_name, 64, "%s%d", VC_CLIENT_SERVICE_NAME, pid);

	dbus_bus_release_name(g_conn_listener, service_name, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	dbus_connection_close(g_conn_sender);
	dbus_connection_close(g_conn_listener);

	g_conn_sender = NULL;
	g_conn_listener = NULL;

	return 0;
}

int vc_dbus_reconnect()
{
	bool sender_connected = dbus_connection_get_is_connected(g_conn_sender);
	bool listener_connected = dbus_connection_get_is_connected(g_conn_listener);
	SLOG(LOG_DEBUG, TAG_VCC, "[DBUS] Sender(%s) Listener(%s)",
		 sender_connected ? "Connected" : "Not connected", listener_connected ? "Connected" : "Not connected");

	if (false == sender_connected || false == listener_connected) {
		vc_dbus_close_connection();

		if (0 != vc_dbus_open_connection()) {
			SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to reconnect");
			return -1;
		}

		SLOG(LOG_DEBUG, TAG_VCC, "[DBUS] Reconnect");
	}

	return 0;
}

int vc_dbus_request_hello()
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_METHOD_HELLO);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> Request vc hello : Fail to make message");
		return VC_ERROR_OPERATION_FAILED;
	}

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg = NULL;
	int result = 0;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, 500, &err);

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


int vc_dbus_request_initialize(int pid, int* mgr_pid, int* service_state)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_METHOD_INITIALIZE);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> vc initialize : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc initialize : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
			DBUS_TYPE_INT32, &pid,
			DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		int tmp = -1;
		int tmp_service_state = 0;
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INT32, &tmp,
			DBUS_TYPE_INT32, &tmp_service_state,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);

		if (0 == result) {
			*mgr_pid = tmp;
			*service_state = tmp_service_state;
			SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc initialize : result = %d mgr = %d service = %d", result, *mgr_pid, *service_state);
		} else {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< vc initialize : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_VCC, "<<<< Result message is NULL ");
		vc_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_dbus_request_finalize(int pid)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_METHOD_FINALIZE);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> vc finalize : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc finalize : pid(%d)", pid);
	}

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc finalize : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< vc finalize : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_VCC, "<<<< Result message is NULL ");
		vc_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_dbus_request_set_exclusive_command(int pid, bool value)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_METHOD_SET_EXCLUSIVE_CMD);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> vc set exclusive command : Fail to make message");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc set exclusive command : pid(%d)", pid);
	}

	int temp = value;

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INT32, &temp,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc set exclusive command : result = %d", result);
		} else {
			SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc set exclusive command : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_VCC, "<<<< Result message is NULL");
		vc_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_dbus_request_set_command(int pid, vc_cmd_type_e cmd_type)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_METHOD_SET_COMMAND);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> vc set command : Fail to make message");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc set command : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INT32, &cmd_type,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc set command : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< vc set command : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_VCC, "<<<< Result message is NULL");
		vc_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_dbus_request_unset_command(int pid, vc_cmd_type_e cmd_type)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_METHOD_UNSET_COMMAND);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> vc unset command : Fail to make message");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc unset command : pid(%d), type(%d)", pid, cmd_type);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INT32, &cmd_type,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc unset command : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< vc unset command : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_VCC, "<<<< Result message is NULL");
		vc_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_dbus_set_foreground(int pid, bool value)
{
	DBusMessage* msg = NULL;
	int tmp_value = 0;

	tmp_value = (int)value;

	msg = dbus_message_new_signal(
		VC_MANAGER_SERVICE_OBJECT_PATH,
		VC_MANAGER_SERVICE_INTERFACE,
		VCC_MANAGER_METHOD_SET_FOREGROUND);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> vc set foreground to manager : Fail to make message");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc set foreground to manager : client pid(%d), value(%s)", pid, tmp_value ? "true" : "false");
	}

	dbus_message_append_args(msg,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INT32, &tmp_value,
		DBUS_TYPE_INVALID);

	if (1 != dbus_connection_send(g_conn_sender, msg, NULL)) {
		SLOG(LOG_ERROR, TAG_VCC, "[Dbus ERROR] Fail to Send");
		return VC_ERROR_OPERATION_FAILED;
	}

	dbus_message_unref(msg);

	msg = NULL;
	msg = dbus_message_new_method_call(
		VC_SERVER_SERVICE_NAME,
		VC_SERVER_SERVICE_OBJECT_PATH,
		VC_SERVER_SERVICE_INTERFACE,
		VC_METHOD_SET_FOREGROUND);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> vc set foreground to daemon : Fail to make message");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc set foreground to daemon : client pid(%d), value(%s)", pid, tmp_value ? "true" : "false");
	}

	dbus_message_append_args(msg,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INT32, &tmp_value,
		DBUS_TYPE_INVALID);

	dbus_message_set_no_reply(msg, TRUE);

	if (1 != dbus_connection_send(g_conn_sender, msg, NULL)) {
		SLOG(LOG_ERROR, TAG_VCC, "[Dbus ERROR] Fail to Send");
		return VC_ERROR_OPERATION_FAILED;
	}

	dbus_connection_flush(g_conn_sender);

	dbus_message_unref(msg);

	return 0;
}

#if 0
int vc_dbus_request_start(int pid, int silence)
{
	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_METHOD_REQUEST_START);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> vc start : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc start : pid(%d), silence(%d)", pid, silence);
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

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc start : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< vc start : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, "<<<< Result Message is NULL");
		vc_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_dbus_request_stop(int pid)
{
	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_METHOD_REQUEST_STOP);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> vc stop : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc stop : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc stop : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< vc stop : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, "<<<< Result Message is NULL");
		vc_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_dbus_request_cancel(int pid)
{
	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,	/* object name of the signal */
			  VC_SERVER_SERVICE_INTERFACE,	/* interface name of the signal */
			  VC_METHOD_REQUEST_CANCEL);	/* name of the signal */

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> vc cancel : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc cancel : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc cancel : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< vc cancel : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, "<<<< Result Message is NULL");
		vc_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}
#endif

/* Authority */
int vc_dbus_request_auth_enable(int pid, int mgr_pid)
{
	DBusMessage* msg;

	char service_name[64] = {0,};
	char object_path[64] = {0,};
	char target_if_name[128] = {0,};

	snprintf(service_name, 64, "%s%d", VC_MANAGER_SERVICE_NAME, mgr_pid);
	snprintf(object_path, 64, "%s", VC_MANAGER_SERVICE_OBJECT_PATH);
	snprintf(target_if_name, 128, "%s%d", VC_MANAGER_SERVICE_INTERFACE, mgr_pid);

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  service_name,
			  object_path,	/* object name of the signal */
			  target_if_name,	/* interface name of the signal */
			  VC_METHOD_AUTH_ENABLE);	/* name of the signal */

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> vc auth enable : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc auth enable : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc auth enable : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< vc auth enable : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, "<<<< Result Message is NULL");
		vc_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_dbus_request_auth_disable(int pid, int mgr_pid)
{
	DBusMessage* msg;

	char service_name[64] = {0,};
	char object_path[64] = {0,};
	char target_if_name[128] = {0,};

	snprintf(service_name, 64, "%s%d", VC_MANAGER_SERVICE_NAME, mgr_pid);
	snprintf(object_path, 64, "%s", VC_MANAGER_SERVICE_OBJECT_PATH);
	snprintf(target_if_name, 128, "%s%d", VC_MANAGER_SERVICE_INTERFACE, mgr_pid);

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  service_name,
			  object_path,	/* object name of the signal */
			  target_if_name,	/* interface name of the signal */
			  VC_METHOD_AUTH_DISABLE);	/* name of the signal */

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> vc auth disable : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc auth disable : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc auth disable : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< vc auth disable : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, "<<<< Result Message is NULL");
		vc_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_dbus_request_auth_start(int pid, int mgr_pid)
{
	DBusMessage* msg;

	char service_name[64] = {0,};
	char object_path[64] = {0,};
	char target_if_name[128] = {0,};

	snprintf(service_name, 64, "%s%d", VC_MANAGER_SERVICE_NAME, mgr_pid);
	snprintf(object_path, 64, "%s", VC_MANAGER_SERVICE_OBJECT_PATH);
	snprintf(target_if_name, 128, "%s%d", VC_MANAGER_SERVICE_INTERFACE, mgr_pid);

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  service_name,
			  object_path,
			  target_if_name,
			  VC_METHOD_AUTH_START);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> vc auth start : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc auth start : pid(%d)", pid);
	}

	DBusMessageIter args;
	dbus_message_iter_init_append(msg, &args);

	/* Append result*/
	dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &(pid));

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc auth start : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< vc auth start : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, "<<<< Result Message is NULL");
		vc_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_dbus_request_auth_stop(int pid, int mgr_pid)
{
	DBusMessage* msg;

	char service_name[64] = {0,};
	char object_path[64] = {0,};
	char target_if_name[128] = {0,};

	snprintf(service_name, 64, "%s%d", VC_MANAGER_SERVICE_NAME, mgr_pid);
	snprintf(object_path, 64, "%s", VC_MANAGER_SERVICE_OBJECT_PATH);
	snprintf(target_if_name, 128, "%s%d", VC_MANAGER_SERVICE_INTERFACE, mgr_pid);

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  service_name,
			  object_path,
			  target_if_name,
			  VC_METHOD_AUTH_STOP);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> vc auth stop : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc auth stop : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc auth stop : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< vc auth stop : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, "<<<< Result Message is NULL");
		vc_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_dbus_request_auth_cancel(int pid, int mgr_pid)
{
	DBusMessage* msg;

	char service_name[64] = {0,};
	char object_path[64] = {0,};
	char target_if_name[128] = {0,};

	snprintf(service_name, 64, "%s%d", VC_MANAGER_SERVICE_NAME, mgr_pid);
	snprintf(object_path, 64, "%s", VC_MANAGER_SERVICE_OBJECT_PATH);
	snprintf(target_if_name, 128, "%s%d", VC_MANAGER_SERVICE_INTERFACE, mgr_pid);

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  service_name,
			  object_path,	/* object name of the signal */
			  target_if_name,	/* interface name of the signal */
			  VC_METHOD_AUTH_CANCEL);	/* name of the signal */

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCC, ">>>> vc auth cancel : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, ">>>> vc auth cancel : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_conn_sender, msg, g_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCC, "<<<< vc auth cancel : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCC, "<<<< vc auth cancel : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCC, "<<<< Result Message is NULL");
		vc_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}
