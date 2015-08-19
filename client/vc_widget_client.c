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
#include "voice_control_command.h"
#include "voice_control_common.h"


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
	vc_widget_show_tooltip_cb	show_tooltip_cb;
	void*				show_tooltip_user_data;
	vc_current_language_changed_cb	current_lang_changed_cb;
	void*				current_lang_changed_user_data;

	vc_widget_send_current_command_list_cb	send_command_list_cb;
	void*					send_command_list_user_data;

	/* tooltip */
	bool			show_tooltip;

	/* service state */
	vc_service_state_e	service_state;

	/* state */
	vc_state_e	before_state;
	vc_state_e	current_state;

	/* mutex */
	int	cb_ref_count;

	/* error data */
	int	reason;
} vc_widget_s;


/* widget list */
static GSList *g_widget_list = NULL;

static vc_widget_s* __widget_get(vc_h vc)
{
	if (vc == NULL) {
		SLOG(LOG_WARN, TAG_VCW, "[WARNING] Input parameter is NULL");
		return NULL;
	}

	vc_widget_s *data = NULL;

	int count = g_slist_length(g_widget_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(g_widget_list, i);

		if (NULL != data) {
			if (vc->handle == data->vc->handle) {
				return data;
			}
		}
	}

	SLOG(LOG_DEBUG, TAG_VCW, "[DEBUG] Fail to get widget by vc");

	return NULL;
}

int vc_widget_client_create(vc_h* vc)
{
	vc_widget_s *widget = NULL;

	widget = (vc_widget_s*)calloc(1, sizeof(vc_widget_s));
	if (NULL == widget) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to allocate memory");
		return VC_ERROR_OUT_OF_MEMORY;
	}

	vc_h temp = (vc_h)calloc(1, sizeof(struct vc_s));
	if (NULL == temp) {
		SLOG(LOG_ERROR, TAG_VCW, "[ERROR] Fail to allocate memory");
		free(widget);
		return VC_ERROR_OUT_OF_MEMORY;
	}

	temp->handle = getpid();

	/* initialize widget data */
	widget->vc = temp;
	widget->pid = getpid();
	widget->uid = temp->handle;
	widget->xid = -1;

	widget->result_cb = NULL;
	widget->result_user_data = NULL;
	widget->service_state_changed_cb = NULL;
	widget->service_state_changed_user_data = NULL;
	widget->state_changed_cb = NULL;
	widget->state_changed_user_data = NULL;
	widget->show_tooltip_cb = NULL;
	widget->show_tooltip_user_data = NULL;
	widget->error_cb = NULL;
	widget->error_user_data = NULL;

	widget->before_state = VC_STATE_INITIALIZED;
	widget->current_state = VC_STATE_INITIALIZED;

	widget->cb_ref_count = 0;

	g_widget_list = g_slist_append(g_widget_list, widget);

	*vc = temp;

	return 0;
}

int vc_widget_client_destroy(vc_h vc)
{
	if (vc == NULL) {
		SLOG(LOG_ERROR, TAG_VCW, "Input parameter is NULL");
		return 0;
	}

	vc_widget_s *data = NULL;

	int count = g_slist_length(g_widget_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(g_widget_list, i);

		if (NULL != data) {
			if (vc->handle == data->vc->handle) {
				g_widget_list =  g_slist_remove(g_widget_list, data);

				while (0 != data->cb_ref_count) {
					/* wait for release callback function */
				}
				free(data);
				free(vc);

				return 0;
			}
		}
	}

	SLOG(LOG_ERROR, TAG_VCW, "[ERROR] widget Not found");

	return -1;
}

bool vc_widget_client_is_valid(vc_h vc)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget) {
		SLOG(LOG_DEBUG, TAG_VCW, "[DEBUG] vc is not valid");
		return false;
	}

	return true;
}

bool vc_widget_client_is_valid_by_uid(int uid)
{
	vc_widget_s *data = NULL;

	int count = g_slist_length(g_widget_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(g_widget_list, i);

		if (NULL != data) {
			if (uid == data->vc->handle)
				return true;
		}
	}

	SLOG(LOG_DEBUG, TAG_VCW, "[DEBUG] Fail to get widget by vc");

	return false;
}

int vc_widget_client_get_handle(int uid, vc_h* vc)
{
	vc_widget_s *data = NULL;

	int count = g_slist_length(g_widget_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(g_widget_list, i);

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
int vc_widget_client_set_result_cb(vc_h vc, vc_result_cb callback, void* user_data)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	widget->result_cb = callback;
	widget->result_user_data = user_data;

	return 0;
}

int vc_widget_client_get_result_cb(vc_h vc, vc_result_cb* callback, void** user_data)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = widget->result_cb;
	*user_data = widget->result_user_data;

	return 0;
}

int vc_widget_client_set_service_state_changed_cb(vc_h vc, vc_service_state_changed_cb callback, void* user_data)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	widget->service_state_changed_cb = callback;
	widget->service_state_changed_user_data = user_data;

	return 0;
}

int vc_widget_client_get_service_state_changed_cb(vc_h vc, vc_service_state_changed_cb* callback, void** user_data)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = widget->service_state_changed_cb;
	*user_data = widget->service_state_changed_user_data;

	return 0;
}

int vc_widget_client_set_state_changed_cb(vc_h vc, vc_state_changed_cb callback, void* user_data)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	widget->state_changed_cb = callback;
	widget->state_changed_user_data = user_data;

	return 0;
}

int vc_widget_client_get_state_changed_cb(vc_h vc, vc_state_changed_cb* callback, void** user_data)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = widget->state_changed_cb;
	*user_data = widget->state_changed_user_data;

	return 0;
}

int vc_widget_client_set_show_tooltip_cb(vc_h vc, vc_widget_show_tooltip_cb callback, void* user_data)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	widget->show_tooltip_cb = callback;
	widget->show_tooltip_user_data = user_data;

	return 0;
}

int vc_widget_client_get_show_tooltip_cb(vc_h vc, vc_widget_show_tooltip_cb* callback, void** user_data)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = widget->show_tooltip_cb;
	*user_data = widget->show_tooltip_user_data;

	return 0;
}

int vc_widget_client_set_current_lang_changed_cb(vc_h vc, vc_current_language_changed_cb callback, void* user_data)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	widget->current_lang_changed_cb = callback;
	widget->current_lang_changed_user_data = user_data;

	return 0;
}

int vc_widget_client_get_current_lang_changed_cb(vc_h vc, vc_current_language_changed_cb* callback, void** user_data)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = widget->current_lang_changed_cb;
	*user_data = widget->current_lang_changed_user_data;

	return 0;
}

int vc_widget_client_set_error_cb(vc_h vc, vc_error_cb callback, void* user_data)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	widget->error_cb = callback;
	widget->error_user_data = user_data;

	return 0;
}

int vc_widget_client_get_error_cb(vc_h vc, vc_error_cb* callback, void** user_data)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = widget->error_cb;
	*user_data = widget->error_user_data;

	return 0;
}

int vc_widget_client_set_send_command_list_cb(vc_h vc, vc_widget_send_current_command_list_cb callback, void* user_data)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	widget->send_command_list_cb = callback;
	widget->send_command_list_user_data = user_data;

	return 0;
}

int vc_widget_client_get_send_command_list_cb(vc_h vc, vc_widget_send_current_command_list_cb* callback, void** user_data)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	*callback = widget->send_command_list_cb;
	*user_data = widget->send_command_list_user_data;

	return 0;
}


/* set/get option */
int vc_widget_client_set_service_state(vc_h vc, vc_service_state_e state)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	widget->service_state = state;

	return 0;
}

int vc_widget_client_get_service_state(vc_h vc, vc_service_state_e* state)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	*state = widget->service_state;

	return 0;
}


int vc_widget_client_set_state(vc_h vc, vc_state_e state)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	widget->before_state = widget->current_state;
	widget->current_state = state;

	return 0;
}

int vc_widget_client_get_state(vc_h vc, vc_state_e* state)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	*state = widget->current_state;

	return 0;
}

int vc_widget_client_get_state_by_uid(int uid, vc_state_e* state)
{
	vc_widget_s *data = NULL;

	int count = g_slist_length(g_widget_list);
	int i;

	for (i = 0; i < count; i++) {
		data = g_slist_nth_data(g_widget_list, i);

		if (NULL != data) {
			if (uid == data->vc->handle) {
				*state = data->current_state;
				return 0;
			}
		}
	}

	return -1;
}

int vc_widget_client_get_before_state(vc_h vc, vc_state_e* state, vc_state_e* before_state)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	*before_state = widget->before_state;
	*state = widget->current_state;

	return 0;
}

int vc_widget_client_set_xid(vc_h vc, int xid)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	widget->xid = xid;

	return 0;
}

int vc_widget_cilent_get_xid(vc_h vc, int* xid)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	*xid = widget->xid;

	return 0;
}

int vc_widget_client_set_error(vc_h vc, int reason)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	widget->reason = reason;

	return 0;
}

int vc_widget_client_get_error(vc_h vc, int* reason)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	*reason = widget->reason;

	return 0;
}

int vc_widget_client_set_show_tooltip(vc_h vc, bool show)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	widget->show_tooltip = show;

	return 0;
}

int vc_widget_client_get_show_tooltip(vc_h vc, bool* show)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	*show = widget->show_tooltip;

	return 0;
}

int vc_widget_client_get_count()
{
	return g_slist_length(g_widget_list);
}

int vc_widget_client_use_callback(vc_h vc)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	widget->cb_ref_count++;
	return 0;
}

int vc_widget_client_not_use_callback(vc_h vc)
{
	vc_widget_s* widget = __widget_get(vc);

	/* check handle */
	if (NULL == widget)
		return VC_ERROR_INVALID_PARAMETER;

	widget->cb_ref_count--;
	return 0;
}

