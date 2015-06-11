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


#include <dlfcn.h>
#include <dirent.h>

#include "vcd_client_data.h"
#include "vcd_config.h"
#include "vcd_engine_agent.h"
#include "vcd_main.h"
#include "vcd_recorder.h"

/*
* Internal data structure
*/
typedef struct {
	/* engine info */
	char*	engine_uuid;
	char*	engine_name;
	char*	engine_path;

	/* engine load info */
	bool	is_set;
	bool	is_loaded;	
	bool	is_command_ready;
	void	*handle;

	vcpe_funcs_s*	pefuncs;
	vcpd_funcs_s*	pdfuncs;

	int (*vcp_load_engine)(vcpd_funcs_s* pdfuncs, vcpe_funcs_s* pefuncs);
	int (*vcp_unload_engine)();
} vcengine_s;

typedef struct _vcengine_info {
	char*	engine_uuid;
	char*	engine_path;
	char*	engine_name;
} vcengine_info_s;


/*
* static data
*/

/** vc engine agent init */
static bool g_agent_init;

/** vc engine list */
static GList *g_engine_list;		

/** current engine information */
static vcengine_s g_dynamic_engine;

static char* g_default_lang;

/** callback functions */
static result_callback g_result_cb;

bool __supported_language_cb(const char* language, void* user_data);

void __engine_info_cb(const char* engine_uuid, const char* engine_name, const char* engine_setting, bool use_network, void* user_data);

bool __engine_setting_cb(const char* key, const char* value, void* user_data);

/** Free voice list */
void __free_language_list(GList* lang_list);


/*
* Internal Interfaces 
*/

/** check engine id */
int __internal_check_engine_id(const char* engine_uuid);

/** update engine list */
int __internal_update_engine_list();

/** get engine info */
int __internal_get_engine_info(const char* filepath, vcengine_info_s** info);

int __log_enginelist();

/*
* VCS Engine Agent Interfaces
*/
int vcd_engine_agent_init(result_callback result_cb)
{
	if (NULL == result_cb) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Input parameter is NULL");
		return VCD_ERROR_OPERATION_FAILED;
	}

	/* init dynamic engine */
	g_dynamic_engine.engine_uuid = NULL;
	g_dynamic_engine.engine_name = NULL;
	g_dynamic_engine.engine_path = NULL;

	g_dynamic_engine.is_set = false;
	g_dynamic_engine.is_loaded = false;
	g_dynamic_engine.handle = NULL;
	g_dynamic_engine.is_command_ready = false;
	g_dynamic_engine.pefuncs = (vcpe_funcs_s*)calloc(1, sizeof(vcpe_funcs_s));
	g_dynamic_engine.pdfuncs = (vcpd_funcs_s*)calloc(1, sizeof(vcpd_funcs_s));

	g_agent_init = true;

	g_result_cb = result_cb;

	if (0 != vcd_config_get_default_language(&g_default_lang)) {
		SLOG(LOG_WARN, TAG_VCD, "[Server WARNING] There is No default voice in config");
		/* Set default voice */
		g_default_lang = strdup(VC_BASE_LANGUAGE);
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent SUCCESS] Engine Agent Initialize");

	return 0;
}

int vcd_engine_agent_release()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Not Initialized");
		return VCD_ERROR_OPERATION_FAILED;
	}

	/* unload current engine */
	vcd_engine_agent_unload_current_engine();

	/* release engine list */
	GList *iter = NULL;
	vcengine_s *data = NULL;

	if (g_list_length(g_engine_list) > 0) {
		/* Get a first item */
		iter = g_list_first(g_engine_list);

		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;
			iter = g_list_remove(iter, data);
		}
	}

	g_list_free(iter);
	
	/* release current engine data */
	if (NULL != g_dynamic_engine.pefuncs)	free(g_dynamic_engine.pefuncs);
	if (NULL != g_dynamic_engine.pdfuncs)	free(g_dynamic_engine.pdfuncs);

	g_agent_init = false;

	SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent SUCCESS] Engine Agent release");

	return 0;
}

bool vcd_engine_is_available_engine()
{
	if (true == g_dynamic_engine.is_loaded) 
		return true;
	
	return false;
}

int vcd_engine_agent_initialize_current_engine()
{
	/* check agent init */
	if (false == g_agent_init ) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Not Initialized");
		return VCD_ERROR_OPERATION_FAILED;
	}

	/* update engine list */
	if (0 != __internal_update_engine_list()) {
		SLOG(LOG_ERROR, TAG_VCD, "[engine agent] vcd_engine_agent_init : __internal_update_engine_list : no engine error");
		return VCD_ERROR_ENGINE_NOT_FOUND;
	}

	/* check whether engine id is valid or not.*/
	GList *iter = NULL;
	vcengine_info_s *dynamic_engine = NULL;

	if (g_list_length(g_engine_list) > 0) {
		/*Get a first item*/
		iter = g_list_first(g_engine_list);

		while (NULL != iter) {
			/*Get handle data from list*/
			dynamic_engine = iter->data;
			if (NULL != dynamic_engine) {
				break;
			}

			/*Get next item*/
			iter = g_list_next(iter);
		}
	} else {
		return VCD_ERROR_ENGINE_NOT_FOUND;
	}

	if (NULL == dynamic_engine) {
		return VCD_ERROR_ENGINE_NOT_FOUND;
	} else {
		if (NULL != g_dynamic_engine.engine_uuid) {
			/* set data from g_engine_list */
			if (g_dynamic_engine.engine_uuid != NULL)	free(g_dynamic_engine.engine_uuid);
			if (g_dynamic_engine.engine_name != NULL)	free(g_dynamic_engine.engine_name);
			if (g_dynamic_engine.engine_path != NULL)	free(g_dynamic_engine.engine_path);
		}

		g_dynamic_engine.engine_uuid = g_strdup(dynamic_engine->engine_uuid);
		g_dynamic_engine.engine_name = g_strdup(dynamic_engine->engine_name);
		g_dynamic_engine.engine_path = g_strdup(dynamic_engine->engine_path);

		g_dynamic_engine.handle = NULL;
		g_dynamic_engine.is_loaded = false;
		g_dynamic_engine.is_set = true;

		SLOG(LOG_DEBUG, TAG_VCD, "-----");
		SLOG(LOG_DEBUG, TAG_VCD, " Dynamic engine uuid : %s", g_dynamic_engine.engine_uuid);
		SLOG(LOG_DEBUG, TAG_VCD, " Dynamic engine name : %s", g_dynamic_engine.engine_name);
		SLOG(LOG_DEBUG, TAG_VCD, " Dynamic engine path : %s", g_dynamic_engine.engine_path);
		SLOG(LOG_DEBUG, TAG_VCD, "-----");
		
	}

	return 0;
}

int __internal_check_engine_id(const char* engine_uuid)
{
	if (NULL == engine_uuid) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Invalid Parameter");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	GList *iter = NULL;
	vcengine_s *data = NULL;

	if (0 < g_list_length(g_engine_list)) {
		/*Get a first item*/
		iter = g_list_first(g_engine_list);

		while (NULL != iter) {
			data = iter->data;
			
			if (0 == strncmp(engine_uuid, data->engine_uuid, strlen(data->engine_uuid))) {
				return 0;
			}

			iter = g_list_next(iter);
		}
	}

	return -1;
}

void __engine_info_cb(const char* engine_uuid, const char* engine_name, const char* engine_setting, bool use_network, void* user_data)
{
	vcengine_info_s* temp = (vcengine_info_s*)user_data;

	temp->engine_uuid = g_strdup(engine_uuid);
	temp->engine_name = g_strdup(engine_name);
}


int __internal_get_engine_info(const char* filepath, vcengine_info_s** info)
{
	if (NULL == filepath || NULL == info) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Invalid Parameter");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	/* load engine */
	char *error;
	void* handle;

	handle = dlopen(filepath, RTLD_LAZY);

	if (!handle) {
		SLOG(LOG_WARN, TAG_VCD, "[Engine Agent] Invalid engine : %s", filepath);
		return -1;
	}

	/* link engine to daemon */
	dlsym(handle, "vcp_load_engine");
	if ((error = dlerror()) != NULL) {
		SLOG(LOG_WARN, TAG_VCD, "[Engine Agent] Invalid engine. Fail to open vcp_load_engine : %s", filepath);
		dlclose(handle);
		return -1;
	}

	dlsym(handle, "vcp_unload_engine");
	if ((error = dlerror()) != NULL) {
		SLOG(LOG_WARN, TAG_VCD, "[Engine Agent] Invalid engine. Fail to open vcp_unload_engine : %s", filepath);
		dlclose(handle);
		return -1;
	}

	int (*get_engine_info)(vcpe_engine_info_cb callback, void* user_data);

	get_engine_info = (int (*)(vcpe_engine_info_cb, void*))dlsym(handle, "vcp_get_engine_info");
	if ((error = dlerror()) != NULL) {
		SLOG(LOG_WARN, TAG_VCD, "[Engine Agent WARNING] Invalid engine. Fail to open vcp_get_engine_info : %s", filepath);
		dlclose(handle);
		return -1;
	}

	vcengine_info_s* temp;
	temp = (vcengine_info_s*)calloc(1, sizeof(vcengine_info_s));
	if (NULL == temp) {
		SLOG(LOG_ERROR, TAG_VCD, "[ERROR] Fail to allocate memory");
		dlclose(handle);
		return VCD_ERROR_OUT_OF_MEMORY;
	}

	/* get engine info */
	if (0 != get_engine_info(__engine_info_cb, (void*)temp)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Fail to get engine info from engine");
		dlclose(handle);
		free(temp);
		return -1;
	}

	/* close engine */
	dlclose(handle);

	temp->engine_path = g_strdup(filepath);

	SLOG(LOG_DEBUG, TAG_VCD, "----- Valid Engine");
	SLOG(LOG_DEBUG, TAG_VCD, "Engine uuid : %s", temp->engine_uuid);
	SLOG(LOG_DEBUG, TAG_VCD, "Engine name : %s", temp->engine_name);
	SLOG(LOG_DEBUG, TAG_VCD, "Engine path : %s", temp->engine_path);
	SLOG(LOG_DEBUG, TAG_VCD, "-----");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	*info = temp;

	return 0;
}

int __internal_update_engine_list()
{
	/* relsease engine list */
	GList *iter = NULL;
	vcengine_info_s *data = NULL;

	if (0 < g_list_length(g_engine_list)) {
		/* Get a first item */
		iter = g_list_first(g_engine_list);

		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			if (NULL != data) {
				if (NULL != data->engine_uuid)		free(data->engine_uuid);
				if (NULL != data->engine_path)		free(data->engine_path);
				if (NULL != data->engine_name)		free(data->engine_name);
				
				free(data);
			}

			g_engine_list = g_list_remove_link(g_engine_list, iter);
			iter = g_list_first(g_engine_list);
		}
	}

	/* Get file name from default engine directory */
	DIR *dp = NULL;
	int ret = -1;
	struct dirent entry;
	struct dirent *dirp = NULL;

	dp  = opendir(ENGINE_DIRECTORY_DEFAULT);
	if (NULL != dp) {
		do {
			ret = readdir_r(dp, &entry, &dirp);
			if (0 != ret) {
				SLOG(LOG_ERROR, TAG_VCD, "[File ERROR] Fail to read directory");
				break;
			}

			if (NULL != dirp) {
				vcengine_info_s* info = NULL;
				char* filepath = NULL;
				int filesize = 0;

				filesize = strlen(ENGINE_DIRECTORY_DEFAULT) + strlen(dirp->d_name) + 5;
				filepath = (char*)calloc(filesize, sizeof(char));

				if (NULL != filepath) {
					snprintf(filepath, filesize, "%s/%s", ENGINE_DIRECTORY_DEFAULT, dirp->d_name);
				} else {
					SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Memory not enough!!");
					continue;
				}

				/* get its info and update engine list */
				if (0 == __internal_get_engine_info(filepath, &info)) {
					/* add engine info to g_engine_list */
					g_engine_list = g_list_append(g_engine_list, info);
				}

				if (NULL != filepath) {
					free(filepath);
					filepath = NULL;
				}
			}
		} while (NULL != dirp);

		closedir(dp);
	} else {
		SLOG(LOG_WARN, TAG_VCD, "[Engine Agent WARNING] Fail to open default directory");
	}
	
	if (0 >= g_list_length(g_engine_list)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] No Engine");
		return VCD_ERROR_ENGINE_NOT_FOUND;	
	}

	__log_enginelist();
	
	return 0;
}


int __foreach_command(vcp_cmd_h vc_command, vcpd_foreach_command_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent] Request foreach command from engine");
	return vcd_client_foreach_command((client_foreach_command_cb)callback, user_data);
}

int __command_get_length(vcp_cmd_h vc_command)
{
	SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent] Request command length from engine");
	return vcd_client_get_length();
}

int __get_audio_type(char** audio_type)
{
	SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent] Request audio type");

	return vcd_recorder_get(audio_type);
}

void __result_cb(vcp_result_event_e event, int* result_id, int count, const char* all_result, const char* non_fixed, const char* msg, void *user_data)
{
	SLOG(LOG_DEBUG, TAG_VCD, "[Engine agent] Event(%d), Count(%d) Text(%s) Nonfixed(%s) Msg(%s)", event, count, all_result, non_fixed, msg);

	if (NULL != g_result_cb) {
		g_result_cb(event, result_id, count, all_result, non_fixed, msg, user_data);
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent ERROR] Result callback function is NOT valid");
	}

	return;
}

int __load_engine(vcengine_s* engine)
{
	/* check whether current engine is loaded or not */
	if (true == engine->is_loaded) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent] Engine has already been loaded ");
		return 0;
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent] Current engine path : %s", engine->engine_path);

	/* open engine */
	char *error;
	engine->handle = dlopen(engine->engine_path, RTLD_LAZY);

	if ((error = dlerror()) != NULL || !engine->handle) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Fail to get engine handle");
		return VCD_ERROR_OPERATION_FAILED;
	}

	engine->vcp_unload_engine = (int (*)())dlsym(engine->handle, "vcp_unload_engine");
	if ((error = dlerror()) != NULL) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Fail to link daemon to vcp_unload_engine()");
		dlclose(engine->handle);
		return VCD_ERROR_OPERATION_FAILED;
	}

	engine->vcp_load_engine = (int (*)(vcpd_funcs_s*, vcpe_funcs_s*) )dlsym(engine->handle, "vcp_load_engine");
	if ((error = dlerror()) != NULL) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Fail to link daemon to vcp_load_engine()");
		dlclose(engine->handle);
		return VCD_ERROR_OPERATION_FAILED;
	}

	/* load engine */
	engine->pdfuncs->version = 1;
	engine->pdfuncs->size = sizeof(vcpd_funcs_s);

	engine->pdfuncs->foreach_command = __foreach_command;
	engine->pdfuncs->get_command_count = __command_get_length;
	engine->pdfuncs->get_audio_type = __get_audio_type;

	if (0 != engine->vcp_load_engine(engine->pdfuncs, engine->pefuncs)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Fail vcp_load_engine()");
		dlclose(engine->handle);
		return VCD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent] engine info : version(%d), size(%d)",engine->pefuncs->version, engine->pefuncs->size);

	/* engine error check */
	if (engine->pefuncs->size != sizeof(vcpe_funcs_s)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Engine is not valid");
		return VCD_ERROR_OPERATION_FAILED;
	}

	/* Check all engine functions */
	if (NULL == engine->pefuncs->initialize ||
		NULL == engine->pefuncs->deinitialize ||
		NULL == engine->pefuncs->foreach_langs ||
		NULL == engine->pefuncs->is_lang_supported ||
		NULL == engine->pefuncs->set_result_cb ||
		NULL == engine->pefuncs->set_language ||
		NULL == engine->pefuncs->set_recording ||
		NULL == engine->pefuncs->stop ||
		NULL == engine->pefuncs->cancel) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] The current engine is NOT valid");
		return VCD_ERROR_OPERATION_FAILED;
	}

	/* initalize engine */
	if (0 != engine->pefuncs->initialize()) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Fail to initialize vc-engine");
		return VCD_ERROR_OPERATION_FAILED;
	}
	
	if (0 != engine->pefuncs->set_result_cb(__result_cb, NULL)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Fail to set result callback of vc-engine");
		return VCD_ERROR_OPERATION_FAILED;
	}

	/* load engine */
	if (true == engine->pefuncs->is_lang_supported(g_default_lang)) {
		if (0 != engine->pefuncs->set_language(g_default_lang)) {
			SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Fail to load current engine");
			return VCD_ERROR_OPERATION_FAILED;
		}
		SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent SUCCESS] The %s has been loaded !!!", engine->engine_name);
		engine->is_loaded = true;
	} else {
		SLOG(LOG_WARN, TAG_VCD, "[Engine Agent WARNING] This engine do not support default language : lang(%s)", g_default_lang);
		engine->is_loaded = false;
	}

	return 0;
}

int vcd_engine_agent_load_current_engine()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Not Initialized");
		return VCD_ERROR_OPERATION_FAILED;
	}

	if (true == g_dynamic_engine.is_set) {
		if (0 != __load_engine(&g_dynamic_engine)) {
			SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Fail to load dynamic engine");

			/* need to initialize dynamic engine data */
			g_dynamic_engine.is_loaded = false;
		} else {
			SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent SUCCESS] Load dynamic engine");
		}
	}
	
	return 0;
}

int vcd_engine_agent_unload_current_engine()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Not Initialized ");
		return VCD_ERROR_OPERATION_FAILED;
	}

	if (true == g_dynamic_engine.is_set) {
		/* unload dynamic engine */
		if (true == g_dynamic_engine.is_loaded) {
			/* shutdown engine */
			g_dynamic_engine.pefuncs->deinitialize();
			g_dynamic_engine.vcp_unload_engine();
			dlclose(g_dynamic_engine.handle);
			g_dynamic_engine.handle = NULL;
			g_dynamic_engine.is_loaded = false;
		}
	}
	return 0;
}


/*
* VCS Engine Interfaces for client
*/

int vcd_engine_set_commands()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Not Initialized");
		return VCD_ERROR_OPERATION_FAILED;
	}

	int ret = -1;

	if (true == g_dynamic_engine.is_loaded) {
		/* Set dynamic command */
		ret = g_dynamic_engine.pefuncs->set_commands((vcp_cmd_h)0);
		if (0 != ret) {
			SLOG(LOG_WARN, TAG_VCD, "[Engine Agent ERROR] Fail to set command of dynamic engine : error(%d)", ret);
			g_dynamic_engine.is_command_ready = false;
		} else {
			g_dynamic_engine.is_command_ready = true;
		}

		SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent SUCCESS] set command");
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent] Dynamic engine is not available");
	}

	return 0;
}

int vcd_engine_recognize_start(bool silence)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Not Initialized");
		return VCD_ERROR_OPERATION_FAILED;
	}

	int ret = -1;
	SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent] silence is %s", silence ? "true" : "false");
	
	if (true == g_dynamic_engine.is_loaded && true == g_dynamic_engine.is_command_ready) {
		ret = g_dynamic_engine.pefuncs->start(silence);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent] Fail to start engine error(%d)", ret);
			return VCD_ERROR_OPERATION_FAILED;
		}
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent] Engine is not available (Cannot start)");
		return VCD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent SUCCESS] Engine start");
	return 0;
}

int vcd_engine_recognize_audio(const void* data, unsigned int length, vcp_speech_detect_e* speech_detected)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Not Initialized");
		return VCD_ERROR_OPERATION_FAILED;
	}

	if (NULL == data) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Invalid Parameter");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	int ret = -1;

	if (true == g_dynamic_engine.is_loaded) {
		ret = g_dynamic_engine.pefuncs->set_recording(data, length, speech_detected);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Fail to set recording dynamic engine error(%d)", ret);
			return VCD_ERROR_OPERATION_FAILED;
		}
	}

	return 0;
}

int vcd_engine_recognize_stop()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Not Initialized");
		return VCD_ERROR_OPERATION_FAILED;
	}

	if (true == g_dynamic_engine.is_loaded) {
		int ret = -1;
		ret = g_dynamic_engine.pefuncs->stop();
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Fail to stop dynamic engine error(%d)", ret);
			return VCD_ERROR_OPERATION_FAILED;
		}
	} else {
		SLOG(LOG_WARN, TAG_VCD, "[Engine Agent] Dynamic engine is not recording state");
		return VCD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

int vcd_engine_recognize_cancel()
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Not Initialized");
		return VCD_ERROR_OPERATION_FAILED;
	}

	int ret = -1;
	if (true == g_dynamic_engine.is_loaded) {
		ret = g_dynamic_engine.pefuncs->cancel();
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Fail to cancel dynamic engine error(%d)", ret);
		}
	} 

	return 0;
}


/*
* VCS Engine Interfaces for client and setting
*/

int vcd_engine_get_audio_format(const char* audio_id, vcp_audio_type_e* types, int* rate, int* channels)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Not Initialized");
		return VCD_ERROR_OPERATION_FAILED;
	}

	if (true != g_dynamic_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Engine is not loaded");
	}

	if (NULL == g_dynamic_engine.pefuncs->get_recording_format) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] The function of engine is NULL!!");
		return VCD_ERROR_OPERATION_FAILED;
	}

	int ret = g_dynamic_engine.pefuncs->get_recording_format(audio_id, types, rate, channels);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] get recording format(%d)", ret);
		return VCD_ERROR_OPERATION_FAILED;
	}

	return 0;
}

bool __supported_language_cb(const char* language, void* user_data)
{
	GList** lang_list = (GList**)user_data;

	if (NULL == language || NULL == lang_list) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Input parameter is NULL in callback!!!!");
		return false;
	}

	SLOG(LOG_DEBUG, TAG_VCD, "-- Language(%s)", language);

	char* temp_lang = g_strdup(language);

	*lang_list = g_list_append(*lang_list, temp_lang);

	return true;
}

int vcd_engine_supported_langs(GList** lang_list)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Not Initialized");
		return VCD_ERROR_OPERATION_FAILED;
	}

	if (true != g_dynamic_engine.is_loaded) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Engine is not loaded");
	}

	if (NULL == g_dynamic_engine.pefuncs->foreach_langs) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] The function of engine is NULL!!");
		return VCD_ERROR_OPERATION_FAILED;
	}

	int ret = g_dynamic_engine.pefuncs->foreach_langs(__supported_language_cb, (void*)lang_list);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] get language list error(%d)", ret);
		return VCD_ERROR_OPERATION_FAILED;
	}
	
	return 0;
}


int vcd_engine_get_current_language(char** lang)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Not Initialized");
		return VCD_ERROR_OPERATION_FAILED;
	}

	if (NULL == lang) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Invalid Parameter");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	/* get default language */
	*lang = g_strdup(g_default_lang);

	return 0;
}

int vcd_engine_set_current_language(const char* language)
{
	if (false == g_agent_init) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Not Initialized");
		return VCD_ERROR_OPERATION_FAILED;
	}

	if (NULL == language) {
		SLOG(LOG_ERROR, TAG_VCD, "[Engine Agent ERROR] Invalid Parameter");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	int ret;

	if (true == g_dynamic_engine.is_loaded) {
		g_dynamic_engine.is_command_ready = false;

		ret = g_dynamic_engine.pefuncs->set_language(language);
		if (0 != ret) {
			SLOG(LOG_WARN, TAG_VCD, "[Engine Agent] Fail to set language of dynamic engine error(%d, %s)", ret, language);
		} 
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "[Engine Agent] Dynamic engine is not available (Cannot start)");
	}

	return 0;
}

void __free_language_list(GList* lang_list)
{
	GList *iter = NULL;
	char* data = NULL;

	/* if list have item */
	if (g_list_length(lang_list) > 0) {
		/* Get a first item */
		iter = g_list_first(lang_list);

		while (NULL != iter) {
			data = iter->data;

			if (NULL != data)
				free(data);
			
			lang_list = g_list_remove_link(lang_list, iter);

			iter = g_list_first(lang_list);
		}
	}
}

int __log_enginelist()
{
	GList *iter = NULL;
	vcengine_info_s *data = NULL;

	if (0 < g_list_length(g_engine_list)) {

		/* Get a first item */
		iter = g_list_first(g_engine_list);

		SLOG(LOG_DEBUG, TAG_VCD, "--------------- engine list -------------------");

		int i = 1;	
		while (NULL != iter) {
			/* Get handle data from list */
			data = iter->data;

			SLOG(LOG_DEBUG, TAG_VCD, "[%dth]", i);
			SLOG(LOG_DEBUG, TAG_VCD, "  engine uuid : %s", data->engine_uuid);
			SLOG(LOG_DEBUG, TAG_VCD, "  engine name : %s", data->engine_name);
			SLOG(LOG_DEBUG, TAG_VCD, "  engine path : %s", data->engine_path);
			iter = g_list_next(iter);
			i++;
		}
		SLOG(LOG_DEBUG, TAG_VCD, "----------------------------------------------");
	} else {
		SLOG(LOG_DEBUG, TAG_VCD, "-------------- engine list -------------------");
		SLOG(LOG_DEBUG, TAG_VCD, "  No Engine in engine directory");
		SLOG(LOG_DEBUG, TAG_VCD, "----------------------------------------------");
	}

	return 0;
}




