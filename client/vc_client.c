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

typedef struct {
	/* base info */
	vc_h	vc;
	int	pid;
	int	uid;		/*<< unique id = pid + handle */
	int	xid;		/*<< main X window id */

	vc_result_cb			result_cb;
	void*				result_user_data;
	vc_error_cb			error_cb;
	void*				error_user_data;
	vc_service_state_changed_cb	service_state_changed_cb;
	void*				service_state_changed_user_data;
	vc_state_changed_cb		state_changed_cb;
	void*				state_changed_user_data;
	vc_current_language_changed_cb	current_lang_changed_cb;
	void*				current_lang_changed_user_data;

#if 0
	/* exclusive option */
	bool			exclusive_cmd;
#endif

	/* service state */
	vc_service_state_e	service_state;

	/* state */
	vc_state_e	before_state;
	vc_state_e	current_state;

	/* mutex */
	int	cb_ref_count;

	/* error data */
	int	reason;

	/* Authority */
	vc_auth_state_e		auth_before_state;
	vc_auth_state_e		auth_current_state;
	vc_auth_state_changed_cb	auth_state_changed_cb;
	void*				auth_state_changed_user_data;

	int	mgr_pid;

	/* is foreground */
	bool	is_foreground;
} vc_client_s;

/* client list */
static GSList *g_client_list = NULL;


static vc_client_s* __client_get(vc_h vc)
{
	if (vc == NULL) {
		return NULL;
	}

	vc_client_s *data = NULL;

	int count = g_slist_length(g_client_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(g_client_list, i);

		if (NULL != data) {
			if (vc->handle == data->vc->handle) {
				return data;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_VCC, "[DEBUG] Fail to get client by vc");

	return NULL;
}

int vc_client_create(vc_h* vc)
{
	vc_client_s *client = NULL;

	client = (vc_client_s*)calloc(1, sizeof(vc_client_s));
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to allocate memory");
		return VC_ERROR_OUT_OF_MEMORY;
	}

	vc_h temp = (vc_h)calloc(1, sizeof(struct vc_s));
	if (NULL == temp) {
		SLOG(LOG_ERROR, TAG_VCC, "[ERROR] Fail to allocate memory");
		free(client);
		return VC_ERROR_OUT_OF_MEMORY;
	}

	temp->handle = getpid();

	/* initialize client data */
	client->vc = temp;
	client->pid = getpid();
	client->uid = temp->handle;
	client->xid = -1;

	client->result_cb = NULL;
	client->result_user_data = NULL;
	client->service_state_changed_cb = NULL;
	client->service_state_changed_user_data = NULL;
	client->state_changed_cb = NULL;
	client->state_changed_user_data = NULL;
	client->current_lang_changed_cb = NULL;
	client->current_lang_changed_user_data = NULL;
	client->error_cb = NULL;
	client->error_user_data = NULL;

#if 0
	client->exclusive_cmd = false;
#endif

	client->service_state = VC_RUNTIME_INFO_NO_FOREGROUND;

	client->before_state = VC_STATE_INITIALIZED;
	client->current_state = VC_STATE_INITIALIZED;

	client->cb_ref_count = 0;

	/* Authority */
	client->auth_before_state = VC_AUTH_STATE_NONE;
	client->auth_current_state = VC_AUTH_STATE_NONE;
	client->auth_state_changed_cb = NULL;
	client->auth_state_changed_user_data = NULL;

	client->is_foreground = false;

	g_client_list = g_slist_append(g_client_list, client);

	*vc = temp;

	return 0;
}

int vc_client_destroy(vc_h vc)
{
	if (vc == NULL) {
		SLOG(LOG_ERROR, TAG_VCC, "Input parameter is NULL");
		return 0;
	}

	vc_client_s *data = NULL;

	int count = g_slist_length(g_client_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(g_client_list, i);

		if (NULL != data) {
			if (vc->handle == data->vc->handle) {
				g_client_list =  g_slist_remove(g_client_list, data);

				while (0 != data->cb_ref_count) {
					/* wait for release callback function */
				}
				free(data);
				free(vc);

				return 0;
			}
		}
	}

	SLOG(LOG_ERROR, TAG_VCC, "[ERROR] client Not found");

	return -1;
}

bool vc_client_is_valid(vc_h vc)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_DEBUG, TAG_VCC, "[DEBUG] vc is not valid");
		return false;
	}

	return true;
}

bool vc_client_is_valid_by_uid(int uid)
{
	vc_client_s *data = NULL;

	int count = g_slist_length(g_client_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(g_client_list, i);

		if (NULL != data) {
			if (uid == data->vc->handle)
				return true;
		}
	}

	SLOG(LOG_DEBUG, TAG_VCC, "[DEBUG] Fail to get client by vc");

	return false;
}

int vc_client_get_handle(int uid, vc_h* vc)
{
	vc_client_s *data = NULL;

	int count = g_slist_length(g_client_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(g_client_list, i);

		if (NULL != data) {
			if (uid == data->vc->handle) {
				*vc = data->vc;
				return 0;
			}
		}
	}

	return -1;
}

/* set/get callback function */
int vc_client_set_result_cb(vc_h vc, vc_result_cb callback, void* user_data)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->result_cb = callback;
	client->result_user_data = user_data;

	return 0;
}

int vc_client_get_result_cb(vc_h vc, vc_result_cb* callback, void** user_data)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = client->result_cb;
	*user_data = client->result_user_data;

	return 0;
}

int vc_client_set_service_state_changed_cb(vc_h vc, vc_service_state_changed_cb callback, void* user_data)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->service_state_changed_cb = callback;
	client->service_state_changed_user_data = user_data;

	return 0;
}

int vc_client_get_service_state_changed_cb(vc_h vc, vc_service_state_changed_cb* callback, void** user_data)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = client->service_state_changed_cb;
	*user_data = client->service_state_changed_user_data;

	return 0;
}

int vc_client_set_state_changed_cb(vc_h vc, vc_state_changed_cb callback, void* user_data)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->state_changed_cb = callback;
	client->state_changed_user_data = user_data;

	return 0;
}

int vc_client_get_state_changed_cb(vc_h vc, vc_state_changed_cb* callback, void** user_data)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = client->state_changed_cb;
	*user_data = client->state_changed_user_data;

	return 0;
}

int vc_client_set_current_lang_changed_cb(vc_h vc, vc_current_language_changed_cb callback, void* user_data)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->current_lang_changed_cb = callback;
	client->current_lang_changed_user_data = user_data;

	return 0;
}

int vc_client_get_current_lang_changed_cb(vc_h vc, vc_current_language_changed_cb* callback, void** user_data)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = client->current_lang_changed_cb;
	*user_data = client->current_lang_changed_user_data;

	return 0;
}

int vc_client_set_error_cb(vc_h vc, vc_error_cb callback, void* user_data)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->error_cb = callback;
	client->error_user_data = user_data;

	return 0;
}

int vc_client_get_error_cb(vc_h vc, vc_error_cb* callback, void** user_data)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = client->error_cb;
	*user_data = client->error_user_data;

	return 0;
}

/* set/get option */
int vc_client_set_service_state(vc_h vc, vc_service_state_e state)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->service_state = state;

	return 0;
}

int vc_client_get_service_state(vc_h vc, vc_service_state_e* state)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*state = client->service_state;

	return 0;
}

int vc_client_set_client_state(vc_h vc, vc_state_e state)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->before_state = client->current_state;
	client->current_state = state;

	return 0;
}

int vc_client_get_client_state(vc_h vc, vc_state_e* state)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*state = client->current_state;

	return 0;
}

int vc_client_get_client_state_by_uid(int uid, vc_state_e* state)
{
	vc_client_s *data = NULL;

	int count = g_slist_length(g_client_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(g_client_list, i);

		if (NULL != data) {
			if (uid == data->vc->handle) {
				*state = data->current_state;
				return 0;
			}
		}
	}

	return -1;
}

int vc_client_get_before_state(vc_h vc, vc_state_e* state, vc_state_e* before_state)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*before_state = client->before_state;
	*state = client->current_state;

	return 0;
}

int vc_client_set_xid(vc_h vc, int xid)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->xid = xid;

	return 0;
}

int vc_client_get_xid(vc_h vc, int* xid)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*xid = client->xid;

	return 0;
}

int vc_client_set_is_foreground(vc_h vc, bool value)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->is_foreground = value;
	return 0;
}

int vc_client_get_is_foreground(vc_h vc, bool* value)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*value = client->is_foreground;

	return 0;
}

#if 0
int vc_client_set_exclusive_cmd(vc_h vc, bool value)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->exclusive_cmd = value;

	return 0;
}

int vc_client_get_exclusive_cmd(vc_h vc, bool* value)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*value = client->exclusive_cmd;

	return 0;
}
#endif

int vc_client_set_error(vc_h vc, int reason)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->reason = reason;

	return 0;
}

int vc_client_get_error(vc_h vc, int* reason)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*reason = client->reason;

	return 0;
}


/* utils */
int vc_client_get_count()
{
	return g_slist_length(g_client_list);
}

int vc_client_use_callback(vc_h vc)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->cb_ref_count++;
	return 0;
}

int vc_client_not_use_callback(vc_h vc)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->cb_ref_count--;
	return 0;
}

/* Authority */
int vc_client_set_auth_state_changed_cb(vc_h vc, vc_auth_state_changed_cb callback, void* user_data)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->auth_state_changed_cb = callback;
	client->auth_state_changed_user_data = user_data;

	return 0;
}

int vc_client_get_auth_state_changed_cb(vc_h vc, vc_auth_state_changed_cb* callback, void** user_data)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = client->auth_state_changed_cb;
	*user_data = client->auth_state_changed_user_data;

	return 0;
}

int vc_client_unset_auth_state_changed_cb(vc_h vc)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->auth_state_changed_cb = NULL;
	client->auth_state_changed_user_data = NULL;

	return 0;
}

int vc_client_set_auth_state(vc_h vc, vc_auth_state_e state)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->auth_before_state = client->auth_current_state;
	client->auth_current_state = state;

	return 0;
}

int vc_client_get_auth_state(vc_h vc, vc_auth_state_e* state)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*state = client->auth_current_state;

	return 0;
}

int vc_client_get_before_auth_state(vc_h vc, vc_auth_state_e* before, vc_auth_state_e* current)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*before = client->auth_before_state;
	*current = client->auth_current_state;

	return 0;
}

int vc_client_set_mgr_pid(vc_h vc, int mgr_pid)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->mgr_pid = mgr_pid;

	return 0;
}

int vc_client_get_mgr_pid(vc_h vc, int* mgr_pid)
{
	vc_client_s* client = __client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*mgr_pid = client->mgr_pid;

	return 0;
}