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


#include "vc_config_mgr.h"
#include "vcd_config.h"
#include "vcd_main.h"


static vcd_config_lang_changed_cb g_lang_cb;

static vcd_config_foreground_changed_cb g_fore_cb;

static void* g_user_data;

static vcd_state_e g_state;


void __vcd_config_lang_changed_cb(const char* before_lang, const char* current_lang)
{
	if (NULL != g_lang_cb)
		g_lang_cb(current_lang, g_user_data);
	else
		SLOG(LOG_ERROR, TAG_VCD, "Language changed callback is NULL");
}

void __vcd_config_foreground_changed_cb(int previous, int current)
{
	if (NULL != g_fore_cb)
		g_fore_cb(previous, current, g_user_data);
	else
		SLOG(LOG_ERROR, TAG_VCD, "Foreground changed callback is NULL");
}

int vcd_config_initialize(vcd_config_lang_changed_cb lang_cb, vcd_config_foreground_changed_cb fore_cb, void* user_data)
{
	if (NULL == lang_cb) {
		SLOG(LOG_ERROR, TAG_VCD, "[Config] Invalid parameter");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	int ret = -1;
	ret = vc_config_mgr_initialize(getpid());
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Config] Fail to initialize config manager : %d", ret);
		return VCD_ERROR_OPERATION_FAILED;
	}

	ret = vc_config_mgr_set_lang_cb(getpid(), __vcd_config_lang_changed_cb);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[ERROR] Fail to set config changed callback : %d", ret);
		return VCD_ERROR_OPERATION_FAILED;
	}

	ret = vc_config_mgr_set_foreground_cb(getpid(), __vcd_config_foreground_changed_cb);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[ERROR] Fail to set foreground changed callback : %d", ret);
		return VCD_ERROR_OPERATION_FAILED;
	}

	g_lang_cb = lang_cb;
	g_fore_cb = fore_cb;
	g_user_data = user_data;

	return 0;
}

int vcd_config_finalize()
{
	vc_config_mgr_unset_foreground_cb(getpid());
	vc_config_mgr_unset_lang_cb(getpid());
	vc_config_mgr_finalize(getpid());
	return 0;
}

int vcd_config_get_default_language(char** language)
{
	if (NULL == language)
		return VCD_ERROR_INVALID_PARAMETER;

	if (0 != vc_config_mgr_get_default_language(language)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Config ERROR] Fail to get language");
		return VCD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int vcd_config_set_service_state(vcd_state_e state)
{
	g_state = state;

	if (0 != vc_config_mgr_set_service_state((int)state)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Config ERROR] Fail to set service state");
		return VCD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Config] Config is changed : %d", g_state);

	return 0;
}

vcd_state_e vcd_config_get_service_state()
{
	return g_state;
}

int vcd_config_get_foreground(int* pid)
{
	return vc_config_mgr_get_foreground(pid);
}
