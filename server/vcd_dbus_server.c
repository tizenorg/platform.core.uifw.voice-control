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


#include "vcd_client_data.h"
#include "vcd_dbus.h"
#include "vcd_dbus_server.h"
#include "vcd_main.h"
#include "vcd_server.h"


int __dbus_error_return(DBusConnection* conn, DBusMessage* msg, int ret)
{
	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret, 
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)");
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_hello(DBusConnection* conn, DBusMessage* msg)
{
	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Hello");

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

/*
* Dbus Server functions for manager
*/

int vcd_dbus_server_mgr_initialize(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = VCD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);
	
	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Manager Initialize");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd mgr initialize : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd mgr initialize : pid(%d)", pid);
		ret =  vcd_server_mgr_initialize(pid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret, 
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)");
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_mgr_finalize(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = VCD_ERROR_OPERATION_FAILED;
	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Manager Finalize");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd mgr finalize : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd mgr finalize : pid(%d)", pid);
		ret =  vcd_server_mgr_finalize(pid);
	}

	DBusMessage* reply;
	
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_mgr_set_command(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = VCD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Manager Set command");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd mgr set command : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd mgr set command : pid(%d)", pid);
		ret = vcd_server_mgr_set_command(pid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			/* Append result and language */
			dbus_message_append_args( reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_mgr_unset_command(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = VCD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD manager unset command");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd mgr unset command : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd mgr unset command : pid(%d)", pid);
		ret = vcd_server_mgr_unset_command(pid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			/* Append result and language */
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_mgr_set_demandable_client(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = VCD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Manager Set demandable client");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd mgr set demandable client : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd mgr set demandable client : pid(%d)", pid);
		ret = vcd_server_mgr_set_demandable_client(pid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			/* Append result and language */
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_mgr_set_audio_type(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid = 0;
	char* audio_type = NULL;
	
	int ret = VCD_ERROR_OPERATION_FAILED;

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Manager set audio type");

	dbus_message_get_args(msg, &err, 
		DBUS_TYPE_INT32, &pid, 
		DBUS_TYPE_STRING, &audio_type,
		DBUS_TYPE_INVALID);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd mgr set audio type : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd mgr set audio type : pid(%d), audio type(%s)", pid, audio_type);
		ret = vcd_server_mgr_set_audio_type(pid, audio_type);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_mgr_get_audio_type(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid = 0;
	char* audio_type = NULL;
	
	int ret = VCD_ERROR_OPERATION_FAILED;

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Manager get audio type");

	dbus_message_get_args(msg, &err, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd mgr set audio type : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd mgr set audio type : pid(%d)", pid);
		ret = vcd_server_mgr_get_audio_type(pid, &audio_type);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret, 
			DBUS_TYPE_STRING, &audio_type, 
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d), audio type(%s)", ret, audio_type);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	if (NULL != audio_type)	free(audio_type);

	return 0;
}

int vcd_dbus_server_mgr_set_client_info(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid = 0;
	
	int ret = VCD_ERROR_OPERATION_FAILED;

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Manager set client info");

	dbus_message_get_args(msg, &err, 
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd mgr set client info : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd mgr set client info : pid(%d)", pid);
		ret = vcd_server_mgr_set_client_info(pid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret, 
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_mgr_start(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid = 0;
	int silence = 0;
	int exclusive = 0;
	int start_by_client = 0;
	
	int ret = VCD_ERROR_OPERATION_FAILED;

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Manager start");

	dbus_message_get_args(msg, &err, 
		DBUS_TYPE_INT32, &pid, 
		DBUS_TYPE_INT32, &silence,
		DBUS_TYPE_INT32, &exclusive,
		DBUS_TYPE_INT32, &start_by_client,
		DBUS_TYPE_INVALID);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd mgr start : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd mgr start : pid(%d), silence(%d), exclusive(%d), start by client(%d)", pid, silence, exclusive, start_by_client);
		ret = vcd_server_mgr_start((bool)silence, (bool)exclusive, (bool)start_by_client);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_mgr_stop(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = VCD_ERROR_OPERATION_FAILED;
	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Manager stop");

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd mgr stop : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd mgr stop : pid(%d)", pid);
		ret = vcd_server_mgr_stop();
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_mgr_cancel(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = VCD_ERROR_OPERATION_FAILED;
	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Manager cancel");

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd mgr cancel : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd mgr cancel : pid(%d)", pid);
		ret = vcd_server_mgr_cancel();
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_mgr_result_selection(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Manager result selection");

	if (dbus_error_is_set(&err)) {
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd mgr result selection : get arguments error (%s)", err.message);
		dbus_error_free(&err);
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd mgr result selection : pid(%d)", pid);
		vcd_server_mgr_result_select();
	}
	return 0;

	/*
	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
	*/
}

/*
* Dbus Server functions for client
*/
int vcd_dbus_server_initialize(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = VCD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);
	
	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Initialize");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd initialize : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd initialize : pid(%d)", pid);
		ret =  vcd_server_initialize(pid);
	}

	int mgr_pid = vcd_client_manager_get_pid();

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret,
			DBUS_TYPE_INT32, &mgr_pid,
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)");
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_finalize(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = VCD_ERROR_OPERATION_FAILED;
	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Finalize");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd finalize : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd finalize : pid(%d)", pid);
		ret =  vcd_server_finalize(pid);
	}

	DBusMessage* reply;
	
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

#if 0
int vcd_dbus_server_set_exclusive_command(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int value;
	int ret = VCD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INT32, &value, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD set exclusive command");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd unset command : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd unset command : pid(%d) value(%d)", pid, value);
		ret = vcd_server_set_exclusive_command(pid, (bool)value);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			/* Append result and language */
			dbus_message_append_args( reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}
#endif

int vcd_dbus_server_set_command(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int cmd_type;
	int ret = VCD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INT32, &cmd_type, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD set command");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd set command : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd set command : pid(%d), cmd_type(%d)", pid, cmd_type);
		ret = vcd_server_set_command(pid, (vc_cmd_type_e)cmd_type);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			/* Append result and language */
			dbus_message_append_args( reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_unset_command(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int cmd_type;
	int ret = VCD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INT32, &cmd_type, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD unset command");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd unset command : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd unset command : pid(%d), cmd_type(%d)", pid, cmd_type);
		ret = vcd_server_unset_command(pid, (vc_cmd_type_e)cmd_type);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			/* Append result and language */
			dbus_message_append_args( reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

#if 0
int vcd_dbus_server_start_request(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int silence;
	
	int ret = VCD_ERROR_OPERATION_FAILED;

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Request Start");

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INT32, &silence, DBUS_TYPE_INVALID);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd start : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd request start : pid(%d), silence(%d)", pid, silence);
		ret = vcd_server_request_start(pid, (bool)silence);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_stop_request(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = VCD_ERROR_OPERATION_FAILED;
	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Request Stop");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd stop : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd request stop : pid(%d)", pid);
		ret = vcd_server_request_stop(pid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_cancel_request(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = VCD_ERROR_OPERATION_FAILED;
	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Request Cancel");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd cancel : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd request cancel : pid(%d)", pid);
		ret = vcd_server_request_cancel(pid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}
#endif

/*
* Dbus Widget-Daemon Server
*/ 
int vcd_dbus_server_widget_initialize(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;

	int ret = VCD_ERROR_OPERATION_FAILED;

	dbus_message_get_args(msg, &err,
		DBUS_TYPE_INT32, &pid,
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Widget Initialize");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd widget initialize : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd widget initialize : pid(%d)", pid);
		ret =  vcd_server_widget_initialize(pid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, 
			DBUS_TYPE_INT32, &ret, 
			DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)");
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_widget_finalize(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = VCD_ERROR_OPERATION_FAILED;
	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD Widget Finalize");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd widget finalize : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd widget finalize : pid(%d)", pid);
		ret =  vcd_server_widget_finalize(pid);
	}

	DBusMessage* reply;

	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_widget_start_recording(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = VCD_ERROR_OPERATION_FAILED;
	int widget_command;

	dbus_message_get_args(msg, &err, 
		DBUS_TYPE_INT32, &pid, 
		DBUS_TYPE_INT32, &widget_command, 
		DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD widget start recording");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd widget start recording : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd widget start recording : pid(%d)", pid);
		ret = vcd_server_widget_start_recording(pid, widget_command);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		if (0 == ret) {
			/* Append result and language */
			dbus_message_append_args( reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_widget_start(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int silence;
	
	int ret = VCD_ERROR_OPERATION_FAILED;

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD widget start");

	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INT32, &silence, DBUS_TYPE_INVALID);

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd start : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd widget start : pid(%d), silence(%d)", pid, silence);
		ret = vcd_server_widget_start(pid, (bool)silence);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_widget_stop(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = VCD_ERROR_OPERATION_FAILED;
	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD widget stop");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd widget stop : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd widget stop : pid(%d)", pid);
		ret = vcd_server_widget_stop(pid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

int vcd_dbus_server_widget_cancel(DBusConnection* conn, DBusMessage* msg)
{
	DBusError err;
	dbus_error_init(&err);

	int pid;
	int ret = VCD_ERROR_OPERATION_FAILED;
	dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &pid, DBUS_TYPE_INVALID);

	SLOG(LOG_DEBUG, TAG_VCD, ">>>>> VCD widget cancel");

	if (dbus_error_is_set(&err)) { 
		SLOG(LOG_ERROR, TAG_VCD, "[IN ERROR] vcd widget cancel : get arguments error (%s)", err.message);
		dbus_error_free(&err);
		ret = VCD_ERROR_OPERATION_FAILED;
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[IN] vcd widget cancel : pid(%d)", pid);
		ret = vcd_server_widget_cancel(pid);
	}

	DBusMessage* reply;
	reply = dbus_message_new_method_return(msg);

	if (NULL != reply) {
		dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);

		if (0 == ret) {
			SLOG(LOG_DEBUG, TAG_VCD, "[OUT SUCCESS] Result(%d)", ret);
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Result(%d)", ret);
		}

		if (!dbus_connection_send(conn, reply, NULL)) {
			SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Out Of Memory!");
		}

		dbus_connection_flush(conn);
		dbus_message_unref(reply);
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[OUT ERROR] Fail to create reply message!!");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "<<<<<");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}

