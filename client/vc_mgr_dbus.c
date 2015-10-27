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
#include "vc_mgr_client.h"
#include "vc_mgr_dbus.h"
#include "vc_command.h"


static int g_m_waiting_time = 3000;

static Ecore_Fd_Handler* g_m_fd_handler = NULL;

static DBusConnection* g_m_conn_sender = NULL;
static DBusConnection* g_m_conn_listener = NULL;


extern void __vc_mgr_cb_all_result(vc_result_type_e type);

extern void __vc_mgr_cb_system_result();

extern void __vc_mgr_cb_speech_detected();

extern int __vc_mgr_cb_error(int pid, int reason);

extern int __vc_mgr_cb_set_volume(float volume);

extern int __vc_mgr_cb_service_state(int state);

/* Authority */
extern int __vc_mgr_request_auth_enable(int pid);

extern int __vc_mgr_request_auth_disable(int pid);

extern int __vc_mgr_request_auth_start(int pid);

extern int __vc_mgr_request_auth_stop(int pid);

extern int __vc_mgr_request_auth_cancel(int pid);

static Eina_Bool vc_mgr_listener_event_callback(void* data, Ecore_Fd_Handler *fd_handler)
{
	if (NULL == g_m_conn_listener)	return ECORE_CALLBACK_RENEW;

	dbus_connection_read_write_dispatch(g_m_conn_listener, 50);

	while (1) {
		DBusMessage* msg = NULL;
		msg = dbus_connection_pop_message(g_m_conn_listener);

		/* loop again if we haven't read a message */
		if (NULL == msg) {
			break;
		}

		SLOG(LOG_DEBUG, TAG_VCM, "[DEBUG] Message is arrived");

		DBusError err;
		dbus_error_init(&err);

		char if_name[64];
		snprintf(if_name, 64, "%s", VC_MANAGER_SERVICE_INTERFACE);

		if (dbus_message_is_method_call(msg, if_name, VCD_MANAGER_METHOD_HELLO)) {
			SLOG(LOG_DEBUG, TAG_VCM, "===== Get Hello");
			int pid = 0;
			int response = -1;

			dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);
			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Dbus Error (%s)", err.message);
				dbus_error_free(&err);
			}

			if (pid > 0) {
				SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr get hello : pid(%d) ", pid);
				response = 1;
			}
			else {
				SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr get hello : invalid pid ");
			}

			DBusMessage *reply = NULL;
			reply = dbus_message_new_method_return(msg);

			if (NULL != reply) {
				dbus_message_append_args(reply, DBUS_TYPE_INT32, &response, DBUS_TYPE_INVALID);

				if (!dbus_connection_send(g_m_conn_listener, reply, NULL))
					SLOG(LOG_ERROR, TAG_VCM, ">>>> vc get hello : fail to send reply");
				else
					SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc get hello : result(%d)", response);

				dbus_connection_flush(g_m_conn_listener);
				dbus_message_unref(reply);
			}
			else {
				SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr get hello : fail to create reply message");
			}

			SLOG(LOG_DEBUG, TAG_VCM, "=====");
			SLOG(LOG_DEBUG, TAG_VCM, " ");
		} /* VCD_METHOD_HELLO */

		else if (dbus_message_is_signal(msg, if_name, VCD_MANAGER_METHOD_SET_VOLUME)) {
			/* SLOG(LOG_DEBUG, TAG_VCM, "===== Set volume"); */
			float volume = 0;

			dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &volume, DBUS_TYPE_INVALID);

			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			}

			SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr set volume : volume(%f)", volume);
			__vc_mgr_cb_set_volume(volume);

			/* SLOG(LOG_DEBUG, TAG_VCM, "====="); */
			/* SLOG(LOG_DEBUG, TAG_VCM, " "); */
		} /* VCD_MANAGER_METHOD_SET_VOLUME */

		else if (dbus_message_is_signal(msg, if_name, VCD_MANAGER_METHOD_SET_SERVICE_STATE)) {
			int state = 0;

			dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &state, DBUS_TYPE_INVALID);
			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			}

			SLOG(LOG_DEBUG, TAG_VCM, "<<<< state changed : %d", state);

			__vc_mgr_cb_service_state(state);

		} /* VCD_MANAGER_METHOD_SET_SERVICE_STATE */

		else if (dbus_message_is_signal(msg, if_name, VCD_MANAGER_METHOD_SPEECH_DETECTED)) {
			SLOG(LOG_DEBUG, TAG_VCM, "===== Get Speech detected");

			__vc_mgr_cb_speech_detected();

			SLOG(LOG_DEBUG, TAG_VCM, "=====");
			SLOG(LOG_DEBUG, TAG_VCM, " ");

		} /* VCD_MANAGER_METHOD_SPEECH_DETECTED */

		else if (dbus_message_is_signal(msg, if_name, VCD_MANAGER_METHOD_ALL_RESULT)) {
			SLOG(LOG_DEBUG, TAG_VCM, "===== Get All Result");
			int result_type = 0;

			dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &result_type, DBUS_TYPE_INVALID);

			__vc_mgr_cb_all_result((vc_result_type_e)result_type);

			SLOG(LOG_DEBUG, TAG_VCM, "=====");
			SLOG(LOG_DEBUG, TAG_VCM, " ");

		} /* VCD_MANAGER_METHOD_ALL_RESULT */

		else if (dbus_message_is_signal(msg, if_name, VCD_MANAGER_METHOD_RESULT)) {
			SLOG(LOG_DEBUG, TAG_VCM, "===== Get System Result");

			__vc_mgr_cb_system_result();

			SLOG(LOG_DEBUG, TAG_VCM, "=====");
			SLOG(LOG_DEBUG, TAG_VCM, " ");

		} /* VCD_MANAGER_METHOD_RESULT */

		else if (dbus_message_is_signal(msg, if_name, VCD_MANAGER_METHOD_ERROR)) {
			SLOG(LOG_DEBUG, TAG_VCM, "===== Get Error");
			int pid;
			int reason;
			char* err_msg;

			dbus_message_get_args(msg, &err,
				DBUS_TYPE_INT32, &pid,
				DBUS_TYPE_INT32, &reason,
				DBUS_TYPE_STRING, &err_msg,
				DBUS_TYPE_INVALID);

			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr Get Error message : Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			} else {
				SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr Get Error message : pid(%d), reason(%d), msg(%s)", pid, reason, err_msg);
				__vc_mgr_cb_error(pid, reason);
			}

			SLOG(LOG_DEBUG, TAG_VCM, "=====");
			SLOG(LOG_DEBUG, TAG_VCM, " ");
		} /* VCD_MANAGER_METHOD_ERROR */

		/* Authority */
		else if (dbus_message_is_method_call(msg, if_name, VC_METHOD_AUTH_ENABLE)) {
			SLOG(LOG_DEBUG, TAG_VCM, "===== Get request auth enable");
			int pid;
			int ret = 0;

			dbus_message_get_args(msg, &err,
				DBUS_TYPE_INT32, &pid,
				DBUS_TYPE_INVALID);

			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr request auth enable : Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			}
			else {
				SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr request auth enable : pid(%d)", pid);
				ret = __vc_mgr_request_auth_enable(pid);
			}

			DBusMessage *reply = NULL;
			reply = dbus_message_new_method_return(msg);

			if (NULL != reply) {
				dbus_message_append_args(reply,
					DBUS_TYPE_INT32, &ret,
					DBUS_TYPE_INVALID);
				if (!dbus_connection_send(g_m_conn_listener, reply, NULL))
					SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr request auth enable : fail to send reply");
				else
					SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc mgr request auth enable : ret(%d)", ret);
				dbus_connection_flush(g_m_conn_listener);
				dbus_message_unref(reply);
			}
			else {
				SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr request auth enable : fail to create reply message");
			}

			SLOG(LOG_DEBUG, TAG_VCM, "=====");
			SLOG(LOG_DEBUG, TAG_VCM, " ");
		} /* VC_METHOD_AUTH_ENABLE */

		else if (dbus_message_is_method_call(msg, if_name, VC_METHOD_AUTH_DISABLE)) {
			SLOG(LOG_DEBUG, TAG_VCM, "===== Get request auth disable");
			int pid;
			int ret = 0;

			dbus_message_get_args(msg, &err,
				DBUS_TYPE_INT32, &pid,
				DBUS_TYPE_INVALID);

			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr request auth disable : Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			}
			else {
				SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr request auth disable : pid(%d)", pid);
				ret = __vc_mgr_request_auth_disable(pid);
			}

			DBusMessage *reply = NULL;
			reply = dbus_message_new_method_return(msg);

			if (NULL != reply) {
				dbus_message_append_args(reply,
					DBUS_TYPE_INT32, &ret,
					DBUS_TYPE_INVALID);
				if (!dbus_connection_send(g_m_conn_listener, reply, NULL))
					SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr request auth disable : fail to send reply");
				else
					SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc mgr request auth disable : ret(%d)", ret);
				dbus_connection_flush(g_m_conn_listener);
				dbus_message_unref(reply);
			}
			else {
				SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr request auth disable : fail to create reply message");
			}

			SLOG(LOG_DEBUG, TAG_VCM, "=====");
			SLOG(LOG_DEBUG, TAG_VCM, " ");
		} /* VC_METHOD_AUTH_DISABLE */

		else if (dbus_message_is_method_call(msg, if_name, VC_METHOD_AUTH_START)) {
			SLOG(LOG_DEBUG, TAG_VCM, "===== Get request auth start");
			int pid;
			int ret = 0;

			dbus_message_get_args(msg, &err,
				DBUS_TYPE_INT32, &pid,
				DBUS_TYPE_INVALID);

			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr request auth start : Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			} else {
				SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr request auth start : pid(%d)", pid);
				ret = __vc_mgr_request_auth_start(pid);
			}

			DBusMessage *reply = NULL;
			reply = dbus_message_new_method_return(msg);

			if (NULL != reply) {
				dbus_message_append_args(reply,
					DBUS_TYPE_INT32, &ret,
					DBUS_TYPE_INVALID);
				if (!dbus_connection_send(g_m_conn_listener, reply, NULL))
					SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr request auth start : fail to send reply");
				else
					SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc mgr request auth start : ret(%d)", ret);
				dbus_connection_flush(g_m_conn_listener);
				dbus_message_unref(reply);
			} else {
				SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr request auth start : fail to create reply message");
			}

			SLOG(LOG_DEBUG, TAG_VCM, "=====");
			SLOG(LOG_DEBUG, TAG_VCM, " ");
		} /* VC_METHOD_AUTH_START */

		else if (dbus_message_is_method_call(msg, if_name, VC_METHOD_AUTH_STOP)) {
			SLOG(LOG_DEBUG, TAG_VCM, "===== Get request auth stop");
			int pid;
			int ret = 0;

			dbus_message_get_args(msg, &err,
				DBUS_TYPE_INT32, &pid,
				DBUS_TYPE_INVALID);

			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr request auth stop : Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			} else {
				SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr request auth stop : pid(%d)", pid);
				ret = __vc_mgr_request_auth_stop(pid);
			}

			DBusMessage *reply = NULL;
			reply = dbus_message_new_method_return(msg);

			if (NULL != reply) {
				dbus_message_append_args(reply,
					DBUS_TYPE_INT32, &ret,
					DBUS_TYPE_INVALID);
				if (!dbus_connection_send(g_m_conn_listener, reply, NULL))
					SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr request auth stop : fail to send reply");
				else
					SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc mgr request auth stop : ret(%d)", ret);
				dbus_connection_flush(g_m_conn_listener);
				dbus_message_unref(reply);
			} else {
				SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr request auth stop : fail to create reply message");
			}

			SLOG(LOG_DEBUG, TAG_VCM, "=====");
			SLOG(LOG_DEBUG, TAG_VCM, " ");
		} /* VC_METHOD_AUTH_STOP */

		else if (dbus_message_is_method_call(msg, if_name, VC_METHOD_AUTH_CANCEL)) {
			SLOG(LOG_DEBUG, TAG_VCM, "===== Get request auth cancel");
			int pid;
			int ret = 0;

			dbus_message_get_args(msg, &err,
				DBUS_TYPE_INT32, &pid,
				DBUS_TYPE_INVALID);

			if (dbus_error_is_set(&err)) {
				SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr request auth cancel : Get arguments error (%s)", err.message);
				dbus_error_free(&err);
			} else {
				SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr request auth cancel : pid(%d)", pid);
				ret = __vc_mgr_request_auth_cancel(pid);
			}

			DBusMessage *reply = NULL;
			reply = dbus_message_new_method_return(msg);

			if (NULL != reply) {
				dbus_message_append_args(reply,
					DBUS_TYPE_INT32, &ret,
					DBUS_TYPE_INVALID);
				if (!dbus_connection_send(g_m_conn_listener, reply, NULL))
					SLOG(LOG_ERROR, TAG_VCM, ">>>> vc request auth cancel : fail to send reply");
				else
					SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc request auth cancel : ret(%d)", ret);
				dbus_connection_flush(g_m_conn_listener);
				dbus_message_unref(reply);
			} else {
				SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr request auth cancel : fail to create reply message");
			}

			SLOG(LOG_DEBUG, TAG_VCM, "=====");
			SLOG(LOG_DEBUG, TAG_VCM, " ");
		} /* VC_METHOD_AUTH_CANCEL */

		else {
			SLOG(LOG_DEBUG, TAG_VCM, "Message is NOT valid");
			dbus_message_unref(msg);
			break;
		}

		/* free the message */
		dbus_message_unref(msg);
	} /* while(1) */

	return ECORE_CALLBACK_PASS_ON;
}

int vc_mgr_dbus_open_connection()
{
	if (NULL != g_m_conn_sender && NULL != g_m_conn_listener) {
		SLOG(LOG_WARN, TAG_VCM, "already existed connection ");
		return 0;
	}

	DBusError err;
	int ret;

	/* initialise the error value */
	dbus_error_init(&err);

	/* connect to the DBUS system bus, and check for errors */
	g_m_conn_sender = dbus_bus_get_private(DBUS_BUS_SESSION, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "Dbus Connection Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL == g_m_conn_sender) {
		SLOG(LOG_ERROR, TAG_VCM, "Fail to get dbus connection ");
		return VC_ERROR_OPERATION_FAILED;
	}

	/* connect to the DBUS system bus, and check for errors */
	g_m_conn_listener = dbus_bus_get_private(DBUS_BUS_SESSION, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "Dbus Connection Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL == g_m_conn_listener) {
		SLOG(LOG_ERROR, TAG_VCM, "Fail to get dbus connection ");
		return VC_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_VCM, "service name is %s", VC_MANAGER_SERVICE_NAME);

	/* register our name on the bus, and check for errors */
	ret = dbus_bus_request_name(g_m_conn_listener, VC_MANAGER_SERVICE_NAME, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "Name Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
		SLOG(LOG_ERROR, TAG_VCM, "fail dbus_bus_request_name()");
		return -2;
	}

	if (NULL != g_m_fd_handler) {
		SLOG(LOG_WARN, TAG_VCM, "The handler already exists.");
		return 0;
	}

	char rule[128] = {0, };
	snprintf(rule, 128, "type='signal',interface='%s'", VC_MANAGER_SERVICE_INTERFACE);

	/* add a rule for which messages we want to see */
	dbus_bus_add_match(g_m_conn_listener, rule, &err);
	dbus_connection_flush(g_m_conn_listener);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "Match Error (%s)", err.message);
		dbus_error_free(&err);
		return VC_ERROR_OPERATION_FAILED;
	}

	int fd = 0;
	if (1 != dbus_connection_get_unix_fd(g_m_conn_listener, &fd)) {
		SLOG(LOG_ERROR, TAG_VCM, "fail to get fd from dbus ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, "Get fd from dbus : %d", fd);
	}

	g_m_fd_handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ, (Ecore_Fd_Cb)vc_mgr_listener_event_callback, g_m_conn_listener, NULL, NULL);

	if (NULL == g_m_fd_handler) {
		SLOG(LOG_ERROR, TAG_VCM, "fail to get fd handler from ecore ");
		return VC_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int vc_mgr_dbus_close_connection()
{
	DBusError err;
	dbus_error_init(&err);

	if (NULL != g_m_fd_handler) {
		ecore_main_fd_handler_del(g_m_fd_handler);
		g_m_fd_handler = NULL;
	}

	dbus_bus_release_name(g_m_conn_listener, VC_MANAGER_SERVICE_NAME, &err);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	g_m_conn_sender = NULL;
	g_m_conn_listener = NULL;

	return 0;
}

int vc_mgr_dbus_reconnect()
{
	bool sender_connected = dbus_connection_get_is_connected(g_m_conn_sender);
	bool listener_connected = dbus_connection_get_is_connected(g_m_conn_listener);

	SLOG(LOG_DEBUG, TAG_VCM, "[DBUS] Sender(%s) Listener(%s)",
		 sender_connected ? "Connected" : "Not connected", listener_connected ? "Connected" : "Not connected");

	if (false == sender_connected || false == listener_connected) {
		vc_mgr_dbus_close_connection();

		if (0 != vc_mgr_dbus_open_connection()) {
			SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to reconnect");
			return -1;
		}

		SLOG(LOG_DEBUG, TAG_VCM, "[DBUS] Reconnect");
	}

	return 0;
}

int vc_mgr_dbus_request_hello()
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_METHOD_HELLO);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCM, ">>>> Request vc hello : Fail to make message");
		return VC_ERROR_OPERATION_FAILED;
	}

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg = NULL;
	int result = 0;

	result_msg = dbus_connection_send_with_reply_and_block(g_m_conn_sender, msg, 500, &err);

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


int vc_mgr_dbus_request_initialize(int pid, int* service_state)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_MANAGER_METHOD_INITIALIZE);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr initialize : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc mgr initialize : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_m_conn_sender, msg, g_m_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
			DBUS_TYPE_INT32, &result,
			DBUS_TYPE_INT32, service_state,
			DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr initialize : result = %d, service state = %d", result, *service_state);
		} else {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr initialize : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_VCM, "<<<< Result message is NULL ");
		vc_mgr_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_mgr_dbus_request_finalize(int pid)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_MANAGER_METHOD_FINALIZE);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr finalize : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc mgr finalize : pid(%d)", pid);
	}

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_m_conn_sender, msg, g_m_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}

		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr finalize : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr finalize : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_VCM, "<<<< Result message is NULL ");
		vc_mgr_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_mgr_dbus_request_set_command(int pid)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_MANAGER_METHOD_SET_COMMAND);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr set command : Fail to make message");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc mgr set command : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_m_conn_sender, msg, g_m_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr set command : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr set command : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_VCM, "<<<< Result message is NULL");
		vc_mgr_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_mgr_dbus_request_unset_command(int pid)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_MANAGER_METHOD_UNSET_COMMAND);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr unset command : Fail to make message");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc mgr unset command : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_m_conn_sender, msg, g_m_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr unset command : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr unset command : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_VCM, "<<<< Result message is NULL");
		vc_mgr_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_mgr_dbus_request_demandable_client(int pid)
{
	DBusMessage* msg;

	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_MANAGER_METHOD_SET_DEMANDABLE);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr set demandable client : Fail to make message");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc mgr set demandable client : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_m_conn_sender, msg, g_m_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr set demandable client : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr set demandable client : result = %d", result);
		}
	} else {
		SLOG(LOG_ERROR, TAG_VCM, "<<<< Result message is NULL");
		vc_mgr_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_mgr_dbus_request_set_audio_type(int pid, const char* audio_type)
{
	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_MANAGER_METHOD_SET_AUDIO_TYPE);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr set audio type : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc mgr set audio type : pid(%d), audio type(%s)", pid, audio_type);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_STRING, &(audio_type),
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_m_conn_sender, msg, g_m_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr set audio type : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr set audio type : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, "<<<< Result Message is NULL");
		vc_mgr_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_mgr_dbus_request_get_audio_type(int pid, char** audio_type)
{
	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_MANAGER_METHOD_GET_AUDIO_TYPE);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr get audio type : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc mgr get audio type : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;
	char* temp = NULL;

	result_msg = dbus_connection_send_with_reply_and_block(g_m_conn_sender, msg, g_m_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_STRING, &temp,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			if (NULL != audio_type && NULL != temp) {
				*audio_type = strdup(temp);
			}
			SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr get audio type : result = %d audio type = %s", result, temp);
		} else {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr get audio type : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, "<<<< Result Message is NULL");
		vc_mgr_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_mgr_dbus_request_set_client_info(int pid)
{
	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_MANAGER_METHOD_SET_CLIENT_INFO);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr set client info : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc mgr set client info : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_m_conn_sender, msg, g_m_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr set client info : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr set client info : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, "<<<< Result Message is NULL");
		vc_mgr_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_mgr_dbus_request_start(int pid, int recognition_mode, bool exclusive_command_option, bool start_by_client)
{
	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_MANAGER_METHOD_START);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr start : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc mgr start : pid(%d), recognition_mode(%d) exclusive(%d) start by client(%d)",
			 pid, recognition_mode, exclusive_command_option, start_by_client);
	}

	int exclusive = exclusive_command_option;
	int by = start_by_client;

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INT32, &(recognition_mode),
							 DBUS_TYPE_INT32, &(exclusive),
							 DBUS_TYPE_INT32, &(by),
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_m_conn_sender, msg, g_m_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr start : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr start : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, "<<<< Result Message is NULL");
		vc_mgr_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_mgr_dbus_request_stop(int pid)
{
	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,
			  VC_SERVER_SERVICE_INTERFACE,
			  VC_MANAGER_METHOD_STOP);

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCM, ">>>> vc mgr stop : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc mgr stop : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_m_conn_sender, msg, g_m_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc mgr stop : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< vc mgr stop : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, "<<<< Result Message is NULL");
		vc_mgr_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

int vc_mgr_dbus_request_cancel(int pid)
{
	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,	/* object name of the signal */
			  VC_SERVER_SERVICE_INTERFACE,	/* interface name of the signal */
			  VC_MANAGER_METHOD_CANCEL);	/* name of the signal */

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCM, ">>>> vc cancel : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc cancel : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* result_msg;
	int result = VC_ERROR_OPERATION_FAILED;

	result_msg = dbus_connection_send_with_reply_and_block(g_m_conn_sender, msg, g_m_waiting_time, &err);
	dbus_message_unref(msg);

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Dbus Error (%s)", err.message);
		dbus_error_free(&err);
	}

	if (NULL != result_msg) {
		dbus_message_get_args(result_msg, &err,
							  DBUS_TYPE_INT32, &result,
							  DBUS_TYPE_INVALID);

		if (dbus_error_is_set(&err)) {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< Get arguments error (%s)", err.message);
			dbus_error_free(&err);
			result = VC_ERROR_OPERATION_FAILED;
		}
		dbus_message_unref(result_msg);

		if (0 == result) {
			SLOG(LOG_DEBUG, TAG_VCM, "<<<< vc cancel : result = %d", result);
		} else {
			SLOG(LOG_ERROR, TAG_VCM, "<<<< vc cancel : result = %d", result);
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, "<<<< Result Message is NULL");
		vc_mgr_dbus_reconnect();
		result = VC_ERROR_TIMED_OUT;
	}

	return result;
}

static DBusMessage* __get_message(int pid, const char* method, int type)
{
	char service_name[64];
	char object_path[64];
	char target_if_name[128];

	memset(service_name, '\0', 64);
	memset(object_path, '\0', 64);
	memset(target_if_name, '\0', 128);

	if (VC_COMMAND_TYPE_FOREGROUND == type || VC_COMMAND_TYPE_BACKGROUND == type) {
		snprintf(service_name, 64, "%s", VC_CLIENT_SERVICE_NAME);
		snprintf(object_path, 64, "%s", VC_CLIENT_SERVICE_OBJECT_PATH);
		snprintf(target_if_name, 128, "%s", VC_CLIENT_SERVICE_NAME);

	} else if (VC_COMMAND_TYPE_WIDGET == type) {
		snprintf(service_name, 64, "%s", VC_WIDGET_SERVICE_NAME);
		snprintf(object_path, 64, "%s", VC_WIDGET_SERVICE_OBJECT_PATH);
		snprintf(target_if_name, 128, "%s", VC_WIDGET_SERVICE_INTERFACE);
	} else {
		return NULL;
	}

	SLOG(LOG_DEBUG, TAG_VCM, "[Dbus] Service(%s) object(%s) if(%s)", service_name, object_path, target_if_name);

	return dbus_message_new_method_call(service_name, object_path, target_if_name, method);
}

int vc_mgr_dbus_send_result(int pid, int cmd_type, int result_id)
{
	DBusMessage* msg = NULL;

	switch (cmd_type) {
	case VC_COMMAND_TYPE_FOREGROUND:
	case VC_COMMAND_TYPE_BACKGROUND:
		msg = __get_message(pid, VCD_METHOD_RESULT, cmd_type);
		break;
	case VC_COMMAND_TYPE_WIDGET:
		msg = __get_message(pid, VCD_WIDGET_METHOD_RESULT, cmd_type);
		break;
	default:
		SLOG(LOG_ERROR, TAG_VCM, "[Dbus ERROR] Command type is NOT valid(%d)", cmd_type);
		return -1;
	}

	if (NULL == msg)
		SLOG(LOG_ERROR, TAG_VCM, "[Dbus ERROR] Message is NULL");

	dbus_message_append_args(msg, DBUS_TYPE_INT32, &result_id, DBUS_TYPE_INVALID);

	dbus_message_set_no_reply(msg, TRUE);

	/* send the message and flush the connection */
	if (!dbus_connection_send(g_m_conn_sender, msg, NULL)) {
		SLOG(LOG_ERROR, TAG_VCM, "[Dbus ERROR] Fail to send result message");
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, "[Dbus] Success to send result");

		dbus_connection_flush(g_m_conn_sender);
	}

	dbus_message_unref(msg);

	return 0;
}

int vc_mgr_dbus_send_result_selection(int pid)
{
	DBusMessage* msg;

	/* create a signal & check for errors */
	msg = dbus_message_new_method_call(
			  VC_SERVER_SERVICE_NAME,
			  VC_SERVER_SERVICE_OBJECT_PATH,	/* object name of the signal */
			  VC_SERVER_SERVICE_INTERFACE,	/* interface name of the signal */
			  VC_MANAGER_METHOD_RESULT_SELECTION);	/* name of the signal */

	if (NULL == msg) {
		SLOG(LOG_ERROR, TAG_VCM, ">>>> vc result selection : Fail to make message ");
		return VC_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, ">>>> vc result selection : pid(%d)", pid);
	}

	dbus_message_append_args(msg,
							 DBUS_TYPE_INT32, &pid,
							 DBUS_TYPE_INVALID);

	dbus_message_set_no_reply(msg, TRUE);

	if (1 != dbus_connection_send(g_m_conn_sender, msg, NULL)) {
		SLOG(LOG_ERROR, TAG_VCM, "[Dbus ERROR] Fail to Send");
		return -1;
	} else {
		SLOG(LOG_DEBUG, TAG_VCM, "[Dbus] SUCCESS Send");
		dbus_connection_flush(g_m_conn_sender);
	}

	return 0;
}
