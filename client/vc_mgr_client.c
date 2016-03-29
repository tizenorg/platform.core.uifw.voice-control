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

typedef struct {
	/* base info */
	vc_h	vc;
	int	pid;
	int	uid;		/*<< unique id = pid + handle */

	vc_mgr_all_result_cb		all_result_cb;
	void*				all_result_user_data;
	vc_result_cb			result_cb;
	void*				result_user_data;

	vc_error_cb			error_cb;
	void*				error_user_data;
	vc_service_state_changed_cb	service_state_changed_cb;
	void*				service_state_changed_user_data;
	vc_state_changed_cb		state_changed_cb;
	void*				state_changed_user_data;
	vc_mgr_begin_speech_detected_cb	speech_detected_cb;
	void*				speech_detected_user_data;
	vc_current_language_changed_cb	current_lang_changed_cb;
	void*				current_lang_changed_user_data;

	/* All result */
	vc_result_event_e	all_result_event;
	char*			all_result_text;

	/* exclusive command flag */
	bool			exclusive_cmd_option;

	/* system result */
	int			result_event;
	char*			result_text;

	/* service state */
	vc_service_state_e	service_state;

	/* state */
	vc_state_e		before_state;
	vc_state_e		current_state;

	/* language */
	char*			before_language;
	char*			current_language;

	/* audio type */
	char*			audio_id;

	/* recognition mode */
	vc_recognition_mode_e	recognition_mode;

	/* mutex */
	int			cb_ref_count;

	/* error data */
	int			reason;

	/* Authorized */
	GSList*			authorized_client_list;
	int			valid_authorized_pid;
	bool			start_by_client;

	/* foreground pid */
	int			foreground_pid;
} vc_mgr_client_s;

typedef struct {
	int pid;
} vc_authorized_client_s;

static GSList *g_mgr_client_list = NULL;

static vc_mgr_client_s* __mgr_client_get(vc_h vc)
{
	if (vc == NULL) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Input parameter is NULL");
		return NULL;
	}

	vc_mgr_client_s *data = NULL;

	int count = g_slist_length(g_mgr_client_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(g_mgr_client_list, i);

		if (NULL != data) {
			if (vc->handle == data->vc->handle) {
				return data;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_VCM, "[DEBUG] Fail to get client by vc");

	return NULL;
}

int vc_mgr_client_create(vc_h* vc)
{
	vc_mgr_client_s *client = NULL;

	client = (vc_mgr_client_s*)calloc(1, sizeof(vc_mgr_client_s));
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to allocate memory");
		return VC_ERROR_OUT_OF_MEMORY;
	}

	vc_h temp = (vc_h)calloc(1, sizeof(struct vc_s));
	if (NULL == temp) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to allocate memory");
		free(client);
		return VC_ERROR_OUT_OF_MEMORY;
	}

	temp->handle = getpid();

	/* initialize client data */
	client->vc = temp;
	client->pid = getpid();
	client->uid = temp->handle;

	client->all_result_cb = NULL;
	client->all_result_user_data = NULL;
	client->result_cb = NULL;
	client->result_user_data = NULL;

	client->error_cb = NULL;
	client->error_user_data = NULL;
	client->service_state_changed_cb = NULL;
	client->service_state_changed_user_data = NULL;
	client->state_changed_cb = NULL;
	client->state_changed_user_data = NULL;
	client->speech_detected_cb = NULL;
	client->speech_detected_user_data = NULL;
	client->current_lang_changed_cb = NULL;
	client->current_lang_changed_user_data = NULL;

	client->exclusive_cmd_option = false;

	client->all_result_event = 0;
	client->all_result_text = NULL;

	client->result_event = -1;
	client->result_text = NULL;

	client->service_state = 0;

	client->before_state = VC_STATE_INITIALIZED;
	client->current_state = VC_STATE_INITIALIZED;

	client->before_language = NULL;
	client->current_language = NULL;

	client->audio_id = NULL;
	client->recognition_mode = VC_RECOGNITION_MODE_STOP_BY_SILENCE;

	client->cb_ref_count = 0;

	/* Authoriry */
	client->authorized_client_list = NULL;
	client->valid_authorized_pid = -1;
	client->start_by_client = false;

	client->foreground_pid = VC_RUNTIME_INFO_NO_FOREGROUND;

	g_mgr_client_list = g_slist_append(g_mgr_client_list, client);

	*vc = temp;

	return 0;
}

int vc_mgr_client_destroy(vc_h vc)
{
	if (vc == NULL) {
		SLOG(LOG_ERROR, TAG_VCM, "Input parameter is NULL");
		return 0;
	}

	vc_mgr_client_s *data = NULL;

	int count = g_slist_length(g_mgr_client_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(g_mgr_client_list, i);

		if (NULL != data) {
			if (vc->handle == data->vc->handle) {
				g_mgr_client_list =  g_slist_remove(g_mgr_client_list, data);

				while (0 != data->cb_ref_count) {
					/* wait for release callback function */
				}

				if (NULL != data->audio_id) {
					free(data->audio_id);
				}

				if (NULL != data->all_result_text) {
					free(data->all_result_text);
				}

				free(data);	
				free(vc);	

				data = NULL;
				vc = NULL;

				return 0;
			}
		}
	}

	SLOG(LOG_ERROR, TAG_VCM, "[ERROR] client Not found");

	return -1;
}

bool vc_mgr_client_is_valid(vc_h vc)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_DEBUG, TAG_VCM, "[DEBUG] vc is not valid");
		return false;
	}

	return true;
}

bool vc_mgr_client_is_valid_by_uid(int uid)
{
	vc_mgr_client_s *data = NULL;

	int count = g_slist_length(g_mgr_client_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(g_mgr_client_list, i);

		if (NULL != data) {
			if (uid == data->vc->handle)
				return true;
		}
	}

	SLOG(LOG_DEBUG, TAG_VCM, "[DEBUG] Fail to get client by vc");

	return false;
}

int vc_mgr_client_get_handle(int uid, vc_h* vc)
{
	vc_mgr_client_s *data = NULL;

	int count = g_slist_length(g_mgr_client_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(g_mgr_client_list, i);

		if (NULL != data) {
			if (uid == data->vc->handle) {
				*vc = data->vc;
				return 0;
			}
		}
	}

	return -1;
}

int vc_mgr_client_get_pid(vc_h vc, int* pid)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_DEBUG, TAG_VCM, "[DEBUG] vc is not valid");
		return -1;
	}

	*pid = client->pid;
	return 0;
}

/* set/get callback function */
int vc_mgr_client_set_all_result_cb(vc_h vc, vc_mgr_all_result_cb callback, void* user_data)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->all_result_cb = callback;
	client->all_result_user_data = user_data;

	return 0;
}

int vc_mgr_client_get_all_result_cb(vc_h vc, vc_mgr_all_result_cb* callback, void** user_data)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = client->all_result_cb;
	*user_data = client->all_result_user_data;

	return 0;
}

int vc_mgr_client_set_result_cb(vc_h vc, vc_result_cb callback, void* user_data)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->result_cb = callback;
	client->result_user_data = user_data;

	return 0;
}

int vc_mgr_client_get_result_cb(vc_h vc, vc_result_cb* callback, void** user_data)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = client->result_cb;
	*user_data = client->result_user_data;

	return 0;
}

int vc_mgr_client_set_service_state_changed_cb(vc_h vc, vc_service_state_changed_cb callback, void* user_data)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->service_state_changed_cb = callback;
	client->service_state_changed_user_data = user_data;

	return 0;
}

int vc_mgr_client_get_service_state_changed_cb(vc_h vc, vc_service_state_changed_cb* callback, void** user_data)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = client->service_state_changed_cb;
	*user_data = client->service_state_changed_user_data;

	return 0;
}

int vc_mgr_client_set_state_changed_cb(vc_h vc, vc_state_changed_cb callback, void* user_data)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->state_changed_cb = callback;
	client->state_changed_user_data = user_data;

	return 0;
}

int vc_mgr_client_get_state_changed_cb(vc_h vc, vc_state_changed_cb* callback, void** user_data)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = client->state_changed_cb;
	*user_data = client->state_changed_user_data;

	return 0;
}

int vc_mgr_client_set_speech_detected_cb(vc_h vc, vc_mgr_begin_speech_detected_cb callback, void* user_data)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->speech_detected_cb = callback;
	client->speech_detected_user_data = user_data;

	return 0;
}

int vc_mgr_client_get_speech_detected_cb(vc_h vc, vc_mgr_begin_speech_detected_cb* callback, void** user_data)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = client->speech_detected_cb;
	*user_data = client->speech_detected_user_data;

	return 0;
}

int vc_mgr_client_set_current_lang_changed_cb(vc_h vc, vc_current_language_changed_cb callback, void* user_data)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->current_lang_changed_cb = callback;
	client->current_lang_changed_user_data = user_data;

	return 0;
}

int vc_mgr_client_get_current_lang_changed_cb(vc_h vc, vc_current_language_changed_cb* callback, void** user_data)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = client->current_lang_changed_cb;
	*user_data = client->current_lang_changed_user_data;

	return 0;
}

int vc_mgr_client_set_error_cb(vc_h vc, vc_error_cb callback, void* user_data)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->error_cb = callback;
	client->error_user_data = user_data;

	return 0;
}

int vc_mgr_client_get_error_cb(vc_h vc, vc_error_cb* callback, void** user_data)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = client->error_cb;
	*user_data = client->error_user_data;

	return 0;
}


/* set/get option */
int vc_mgr_client_set_service_state(vc_h vc, vc_service_state_e state)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->service_state = state;

	return 0;
}

int vc_mgr_client_get_service_state(vc_h vc, vc_service_state_e* state)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*state = client->service_state;

	return 0;
}

int vc_mgr_client_set_client_state(vc_h vc, vc_state_e state)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->before_state = client->current_state;
	client->current_state = state;

	return 0;
}

int vc_mgr_client_get_client_state(vc_h vc, vc_state_e* state)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*state = client->current_state;

	return 0;
}

int vc_mgr_client_get_client_state_by_uid(int uid, vc_state_e* state)
{
	vc_mgr_client_s *data = NULL;

	int count = g_slist_length(g_mgr_client_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(g_mgr_client_list, i);

		if (NULL != data) {
			if (uid == data->vc->handle) {
				*state = data->current_state;
				return 0;
			}
		}
	}

	return -1;
}

int vc_mgr_client_get_before_state(vc_h vc, vc_state_e* state, vc_state_e* before_state)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*before_state = client->before_state;
	*state = client->current_state;

	return 0;
}

int vc_mgr_client_set_error(vc_h vc, int reason)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->reason = reason;

	return 0;
}

int vc_mgr_client_get_error(vc_h vc, int* reason)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*reason = client->reason;

	return 0;
}

int vc_mgr_client_set_exclusive_command(vc_h vc, bool value)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->exclusive_cmd_option = value;

	return 0;
}

bool vc_mgr_client_get_exclusive_command(vc_h vc)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	return client->exclusive_cmd_option;
}

int vc_mgr_client_set_all_result(vc_h vc, int event, const char* result_text)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->all_result_event = event;

	if (NULL != client->all_result_text) {
		free(client->all_result_text);
	}
	client->all_result_text = strdup(result_text);

	return 0;
}

int vc_mgr_client_get_all_result(vc_h vc, int* event, char** result_text)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*event = client->all_result_event;
	if (NULL != result_text) {
		if (NULL != client->all_result_text) {
			*result_text = strdup(client->all_result_text);
		}
	}

	return 0;
}

int vc_mgr_client_unset_all_result(vc_h vc)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->all_result_event = -1;

	if (NULL != client->all_result_text) {
		free(client->all_result_text);
		client->all_result_text = NULL;
	}

	return 0;
}

int vc_mgr_client_set_audio_type(vc_h vc, const char* audio_id)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	if (NULL != audio_id) {
		if (NULL != client->audio_id) {
			free(client->audio_id);
			client->audio_id = NULL;
		}
		client->audio_id = strdup(audio_id);
	}

	return 0;
}

int vc_mgr_client_get_audio_type(vc_h vc, char** audio_id)
{
	if (NULL == audio_id)	{
		return -1;
	}

	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	if (NULL != client->audio_id)
		*audio_id = strdup(client->audio_id);
	else
		*audio_id = NULL;

	return 0;
}

int vc_mgr_client_set_recognition_mode(vc_h vc, vc_recognition_mode_e mode)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->recognition_mode = mode;

	return 0;
}

int vc_mgr_client_get_recognition_mode(vc_h vc, vc_recognition_mode_e* mode)
{
	if (NULL == mode) {
		return -1;
	}

	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*mode = client->recognition_mode;
	return 0;
}

int vc_mgr_client_set_foreground(vc_h vc, int pid, bool value)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	if (true == value) {
		client->foreground_pid = pid;
	} else {
		if (pid == client->foreground_pid) {
			client->foreground_pid = VC_RUNTIME_INFO_NO_FOREGROUND;
		}
	}

	return 0;
}

int vc_mgr_client_get_foreground(vc_h vc, int* pid)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*pid = client->foreground_pid;
	return 0;
}

/* utils */
int vc_mgr_client_get_count()
{
	return g_slist_length(g_mgr_client_list);
}

int vc_mgr_client_use_callback(vc_h vc)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->cb_ref_count++;
	return 0;
}

int vc_mgr_client_not_use_callback(vc_h vc)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->cb_ref_count--;
	return 0;
}

/* Authority */
int vc_mgr_client_add_authorized_client(vc_h vc, int pid)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	vc_authorized_client_s *authorized_client = NULL;

	authorized_client = (vc_authorized_client_s*)calloc(1, sizeof(vc_authorized_client_s));
	if (NULL == authorized_client) {
		SLOG(LOG_ERROR, TAG_VCM, "[ERROR] Fail to make authorized client");
		return VC_ERROR_OPERATION_FAILED;
	}

	authorized_client->pid = pid;

	client->authorized_client_list = g_slist_append(client->authorized_client_list, authorized_client);

	SLOG(LOG_DEBUG, TAG_VCM, "Add authorized client - %d", pid);

	return 0;
}

int vc_mgr_client_remove_authorized_client(vc_h vc, int pid)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	vc_authorized_client_s *data = NULL;

	int count = g_slist_length(client->authorized_client_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(client->authorized_client_list, i);

		if (NULL != data) {
			if (pid == data->pid) {
				client->authorized_client_list = g_slist_remove(client->authorized_client_list, data);

				free(data);
				data = NULL;

				SLOG(LOG_DEBUG, TAG_VCM, "Remove authorized client - %d", pid);
				return 0;
			}
		}
	}

	SLOG(LOG_ERROR, TAG_VCM, "[ERROR] client Not found");

	return VC_ERROR_OPERATION_FAILED;
}

bool vc_mgr_client_is_authorized_client(vc_h vc, int pid)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	vc_authorized_client_s *data = NULL;

	int count = g_slist_length(client->authorized_client_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(client->authorized_client_list, i);

		if (NULL != data) {
			if (pid == data->pid) {
				SLOG(LOG_DEBUG, TAG_VCM, "Authorized client - %d", pid);
				return true;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_VCM, "Un-Authorized client - %d", pid);

	return false;
}

int vc_mgr_client_set_valid_authorized_client(vc_h vc, int pid)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->valid_authorized_pid = pid;

	return 0;
}

int vc_mgr_client_get_valid_authorized_client(vc_h vc, int* pid)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*pid = client->valid_authorized_pid;

	return 0;
}

bool vc_mgr_client_is_valid_authorized_client(vc_h vc, int pid)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	if (pid == client->valid_authorized_pid)
		return true;
	else
		return false;
}

int vc_mgr_client_set_start_by_client(vc_h vc, bool option)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	client->start_by_client = option;

	return 0;
}

int vc_mgr_client_get_start_by_client(vc_h vc, bool* option)
{
	vc_mgr_client_s* client = __mgr_client_get(vc);

	/* check handle */
	if (NULL == client)
		return VC_ERROR_INVALID_PARAMETER;

	*option = client->start_by_client;

	return 0;
}