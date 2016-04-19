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


#include <dirent.h>
#include <dlfcn.h>
#include <dlog.h>
#include <Ecore.h>
#include <fcntl.h>
#include <glib.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <vconf.h>
#include <vconf-internal-keys.h>

#include "vc_config_mgr.h"
#include "vc_defs.h"
#include "vc_config_parser.h"
#include "vc_main.h"
#include "voice_control_command.h"

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (EVENT_SIZE + 16)

typedef struct {
	int	uid;
	vc_config_lang_changed_cb	lang_cb;
	vc_config_enabled_cb		enabled_cb;
} vc_config_client_s;


const char* vc_config_tag()
{
	return TAG_VCCONFIG;
}

static GSList* g_engine_list = NULL;

static GSList* g_config_client_list = NULL;

static vc_config_s* g_config_info;

static int g_foreground_pid;

static int g_lang_ref_count;
static Ecore_Fd_Handler* g_fd_handler_lang = NULL;
static int g_fd_lang;
static int g_wd_lang;


int __vc_config_mgr_print_engine_info();

int __vc_config_mgr_print_client_info();

int vc_config_convert_error_code(vc_config_error_e code)
{
	if (code == VC_CONFIG_ERROR_NONE)			return VC_ERROR_NONE;
	if (code == VC_CONFIG_ERROR_OPERATION_FAILED)		return VC_ERROR_OPERATION_FAILED;
	if (code == VC_CONFIG_ERROR_INVALID_PARAMETER)		return VC_ERROR_INVALID_PARAMETER;
	if (code == VC_CONFIG_ERROR_ENGINE_NOT_FOUND)		return VC_ERROR_ENGINE_NOT_FOUND;
	if (code == VC_CONFIG_ERROR_INVALID_STATE)		return VC_ERROR_INVALID_STATE;
	if (code == VC_CONFIG_ERROR_INVALID_LANGUAGE)		return VC_ERROR_INVALID_LANGUAGE;
	if (code == VC_CONFIG_ERROR_IO_ERROR)			return VC_ERROR_IO_ERROR;
	if (code == VC_CONFIG_ERROR_OUT_OF_MEMORY)		return VC_ERROR_OUT_OF_MEMORY;

	return VC_CONFIG_ERROR_NONE;
}

int __vc_config_mgr_check_engine_is_valid(const char* engine_id)
{
	if (NULL == engine_id) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	GSList *iter = NULL;
	vc_engine_info_s *engine_info = NULL;

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] There is no engine!!");
		return -1;
	}

	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Engine info is NULL");
			return -1;
		}

		if (NULL == engine_info->uuid) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Engine id is NULL");
			return -1;
		}

		if (!strcmp(engine_id, engine_info->uuid)) {
			SLOG(LOG_DEBUG, vc_config_tag(), "Default engine is valid : %s", engine_id);
			return 0;
		}

		iter = g_slist_next(iter);
	}

	iter = g_slist_nth(g_engine_list, 0);
	engine_info = iter->data;

	if (NULL == engine_info) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Engine info is NULL");
		return -1;
	}

	if (NULL == engine_info->uuid) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Engine id is NULL");
		return -1;
	}

	if (NULL == g_config_info) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Config info is NULL");
		return -1;
	}

	if (NULL != g_config_info->engine_id)
		free(g_config_info->engine_id);

	g_config_info->engine_id = strdup(engine_info->uuid);

	SLOG(LOG_DEBUG, vc_config_tag(), "Default engine is changed : %s", g_config_info->engine_id);
	if (0 != vc_parser_set_engine(g_config_info->engine_id)) {
		SLOG(LOG_ERROR, vc_config_tag(), " Fail to save config");
		return -1;
	}

	return 0;
}

bool __vc_config_mgr_check_lang_is_valid(const char* engine_id, const char* language)
{
	if (NULL == engine_id || NULL == language) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Input parameter is NULL");
		return false;
	}

	GSList *iter = NULL;
	vc_engine_info_s *engine_info = NULL;

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] There is no engine!!");
		return false;
	}

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Engine info is NULL");
			return false;
		}

		if (0 != strcmp(engine_id, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		GSList *iter_lang = NULL;
		char* engine_lang;
		if (g_slist_length(engine_info->languages) > 0) {
			/* Get a first item */
			iter_lang = g_slist_nth(engine_info->languages, 0);

			int i = 1;
			while (NULL != iter_lang) {
				/*Get handle data from list*/
				engine_lang = iter_lang->data;

				SLOG(LOG_DEBUG, vc_config_tag(), "  [%dth] %s", i, engine_lang);

				if (0 == strcmp(language, engine_lang)) {
					return true;
				}

				/*Get next item*/
				iter_lang = g_slist_next(iter_lang);
				i++;
			}
		}
		break;
	}

	return false;
}

int __vc_config_mgr_select_lang(const char* engine_id, char** language)
{
	if (NULL == engine_id || NULL == language) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Input parameter is NULL");
		return false;
	}

	GSList *iter = NULL;
	vc_engine_info_s *engine_info = NULL;

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] There is no engine!!");
		return false;
	}

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, vc_config_tag(), "engine info is NULL");
			return false;
		}

		if (0 != strcmp(engine_id, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		GSList *iter_lang = NULL;
		char* engine_lang = NULL;
		if (g_slist_length(engine_info->languages) > 0) {
			/* Get a first item */
			iter_lang = g_slist_nth(engine_info->languages, 0);

			while (NULL != iter_lang) {
				engine_lang = iter_lang->data;
				if (NULL != engine_lang) {
					/* Check base language */
					if (0 == strcmp(VC_BASE_LANGUAGE, engine_lang)) {
						*language = strdup(engine_lang);
						SLOG(LOG_DEBUG, vc_config_tag(), "Selected language : %s", *language);
						return 0;
					}
				}

				iter_lang = g_slist_next(iter_lang);
			}

			/* Not support base language */
			if (NULL != engine_lang) {
				*language = strdup(engine_lang);
				SLOG(LOG_DEBUG, vc_config_tag(), "Selected language : %s", *language);
				return 0;
			}
		}
		break;
	}

	return -1;
}

Eina_Bool vc_config_mgr_inotify_event_cb(void* data, Ecore_Fd_Handler *fd_handler)
{
	SLOG(LOG_DEBUG, vc_config_tag(), "===== Config changed callback event");

	int length;
	struct inotify_event event;
	memset(&event, '\0', sizeof(struct inotify_event));

	length = read(g_fd_lang, &event, sizeof(struct inotify_event));

	if (0 > length) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty Inotify event");
		SLOG(LOG_DEBUG, vc_config_tag(), "=====");
		SLOG(LOG_DEBUG, vc_config_tag(), " ");
		return ECORE_CALLBACK_PASS_ON;
	}

	if (IN_CLOSE_WRITE == event.mask) {
		int auto_lang = -1;
		char* lang = NULL;
		int enabled = -1;

		GSList *iter = NULL;
		vc_config_client_s* temp_client = NULL;

		if (0 != vc_parser_find_config_changed(&auto_lang, &lang, &enabled))
			return ECORE_CALLBACK_PASS_ON;

		if (-1 != auto_lang) {
			g_config_info->auto_lang = auto_lang;
		}

		/* Only language changed */
		if (NULL != lang) {
			char* before_lang = NULL;

			before_lang = strdup(g_config_info->language);

			if (NULL != g_config_info->language)	free(g_config_info->language);
			g_config_info->language = strdup(lang);

			/* Call all callbacks of client*/
			iter = g_slist_nth(g_config_client_list, 0);

			while (NULL != iter) {
				temp_client = iter->data;

				if (NULL != temp_client) {
					if (NULL != temp_client->lang_cb) {
						temp_client->lang_cb(before_lang, lang);
					}
				}

				iter = g_slist_next(iter);
			}

			if (NULL != before_lang)	free(before_lang);
		}

		if (NULL != lang)	free(lang);

		if (-1 != enabled) {
			g_config_info->enabled = enabled;

			/* Call all callbacks of client*/
			iter = g_slist_nth(g_config_client_list, 0);

			while (NULL != iter) {
				temp_client = iter->data;

				if (NULL != temp_client) {
					if (NULL != temp_client->enabled_cb) {
						temp_client->enabled_cb(enabled);
					}
				}

				iter = g_slist_next(iter);
			}
		}
	} else {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Undefined event");
	}

	SLOG(LOG_DEBUG, vc_config_tag(), "=====");
	SLOG(LOG_DEBUG, vc_config_tag(), " ");

	return ECORE_CALLBACK_PASS_ON;
}

int __vc_config_set_auto_language()
{
	char candidate_lang[6] = {'\0', };
	char* value = NULL;

	value = vconf_get_str(VCONFKEY_LANGSET);
	if (NULL == value) {
		SLOG(LOG_ERROR, vc_config_tag(), "[Config ERROR] Fail to get display language");
		return -1;
	}

	strncpy(candidate_lang, value, 5);
	free(value);

	SLOG(LOG_DEBUG, vc_config_tag(), "[Config] Display language : %s", candidate_lang);

	/* Check current language */
	if (0 == strncmp(g_config_info->language, candidate_lang, 5)) {
		SLOG(LOG_DEBUG, vc_config_tag(), "[Config] VC language(%s) is same with display language", g_config_info->language);
		return 0;
	}

	if (true == __vc_config_mgr_check_lang_is_valid(g_config_info->engine_id, candidate_lang)) {
		/* stt default language change */
		if (NULL == g_config_info->language) {
			SLOG(LOG_ERROR, vc_config_tag(), "Current config language is NULL");
			return -1;
		}

		char* before_lang = NULL;
		if (0 != vc_parser_set_language(candidate_lang)) {
			SLOG(LOG_ERROR, vc_config_tag(), "Fail to save default language");
			return -1;
		}

		before_lang = strdup(g_config_info->language);

		free(g_config_info->language);
		g_config_info->language = strdup(candidate_lang);

		SLOG(LOG_DEBUG, vc_config_tag(), "[Config] Default language change : before(%s) current(%s)",
			 before_lang, g_config_info->language);

		/* Call all callbacks of client*/
		GSList *iter = NULL;
		vc_config_client_s* temp_client = NULL;

		iter = g_slist_nth(g_config_client_list, 0);

		while (NULL != iter) {
			temp_client = iter->data;

			if (NULL != temp_client) {
				if (NULL != temp_client->lang_cb) {
					temp_client->lang_cb(before_lang, g_config_info->language);
				}
			}

			iter = g_slist_next(iter);
		}

		if (NULL != before_lang)	free(before_lang);
	} else {
		/* Candidate language is not valid */
		char* tmp_language = NULL;
		if (0 != __vc_config_mgr_select_lang(g_config_info->engine_id, &tmp_language)) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to select language");
			return -1;
		}

		if (NULL == tmp_language) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Selected language is NULL");
			return -1;
		}

		char* before_lang = NULL;
		if (0 != vc_parser_set_language(tmp_language)) {
			free(tmp_language);
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to save config");
			return -1;
		}

		before_lang = strdup(g_config_info->language);

		if (NULL != g_config_info->language) {
			free(g_config_info->language);
			g_config_info->language = strdup(tmp_language);
		}
		free(tmp_language);

		SLOG(LOG_DEBUG, vc_config_tag(), "[Config] Default language change : before(%s) current(%s)",
			 before_lang, g_config_info->language);

		/* Call all callbacks of client*/
		GSList *iter = NULL;
		vc_config_client_s* temp_client = NULL;

		iter = g_slist_nth(g_config_client_list, 0);

		while (NULL != iter) {
			temp_client = iter->data;

			if (NULL != temp_client) {
				if (NULL != temp_client->lang_cb) {
					temp_client->lang_cb(before_lang, g_config_info->language);
				}
			}

			iter = g_slist_next(iter);
		}

		if (NULL != before_lang)	free(before_lang);
	}

	return 0;
}

void __vc_config_language_changed_cb(keynode_t *key, void *data)
{
	if (true == g_config_info->auto_lang) {
		/* Get voice input vconf key */
		__vc_config_set_auto_language();
	}

	return;
}

int vc_config_mgr_initialize(int uid)
{
	GSList *iter = NULL;
	int* get_uid;
	vc_config_client_s* temp_client = NULL;

	if (0 < g_slist_length(g_config_client_list)) {
		/* Check uid */
		iter = g_slist_nth(g_config_client_list, 0);

		while (NULL != iter) {
			get_uid = iter->data;

			if (uid == *get_uid) {
				SLOG(LOG_WARN, vc_config_tag(), "[CONFIG] uid(%d) has already registered", uid);
				return 0;
			}

			iter = g_slist_next(iter);
		}

		temp_client = (vc_config_client_s*)calloc(1, sizeof(vc_config_client_s));
		if (NULL == temp_client) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to allocate memory");
			return VC_ERROR_OUT_OF_MEMORY;
		}

		temp_client->uid = uid;

		/* Add uid */
		g_config_client_list = g_slist_append(g_config_client_list, temp_client);

		SLOG(LOG_WARN, vc_config_tag(), "[CONFIG] Add uid(%d) but config has already initialized", uid);

		__vc_config_mgr_print_client_info();
		return 0;
	}

	/* Get file name from default engine directory */
	DIR *dp = NULL;
	int ret = -1;
	struct dirent entry;
	struct dirent *dirp = NULL;

	g_engine_list = NULL;

	if (0 != access(VC_CONFIG_BASE, F_OK)) {
		if (0 != mkdir(VC_CONFIG_BASE, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to make directory : %s", VC_CONFIG_BASE);
			return -1;
		} else {
			SLOG(LOG_DEBUG, vc_config_tag(), "Success to make directory : %s", VC_CONFIG_BASE);
		}
	}
	if (0 != access(VC_RUNTIME_INFO_ROOT, F_OK)) {
		if (0 != mkdir(VC_RUNTIME_INFO_ROOT, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to make directory : %s", VC_RUNTIME_INFO_ROOT);
			return -1;
		} else {
			SLOG(LOG_DEBUG, vc_config_tag(), "Success to make directory : %s", VC_RUNTIME_INFO_ROOT);
		}
	}

	dp  = opendir(VC_DEFAULT_ENGINE_INFO);
	if (NULL != dp) {
		do {
			ret = readdir_r(dp, &entry, &dirp);
			if (0 != ret) {
				SLOG(LOG_ERROR, vc_config_tag(), "[File ERROR] Fail to read directory");
				break;
			}

			if (NULL != dirp) {
				if (!strcmp(".", dirp->d_name) || !strcmp("..", dirp->d_name))
					continue;

				vc_engine_info_s* info = NULL;
				char* filepath = NULL;
				int filesize = 0;

				filesize = strlen(VC_DEFAULT_ENGINE_INFO) + strlen(dirp->d_name) + 5;
				filepath = (char*)calloc(filesize, sizeof(char));

				if (NULL != filepath) {
					snprintf(filepath, filesize, "%s/%s", VC_DEFAULT_ENGINE_INFO, dirp->d_name);
				} else {
					SLOG(LOG_ERROR, vc_config_tag(), "[Engine Agent ERROR] Memory not enough!!");
					continue;
				}

				if (0 == vc_parser_get_engine_info(filepath, &info)) {
					g_engine_list = g_slist_append(g_engine_list, info);
				}

				if (NULL != filepath) {
					free(filepath);
					filepath = NULL;
				}
			}
		} while (NULL != dirp);

		closedir(dp);
	} else {
		SLOG(LOG_WARN, vc_config_tag(), "[Engine Agent WARNING] Fail to open default directory");
	}

	__vc_config_mgr_print_engine_info();

	if (0 != vc_parser_load_config(&g_config_info)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to parse configure information");
		return -1;
	}

	if (0 != __vc_config_mgr_check_engine_is_valid(g_config_info->engine_id)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to get default engine");
		return VC_CONFIG_ERROR_ENGINE_NOT_FOUND;
	}

	if (true == g_config_info->auto_lang) {
		/* Check language with display language */
		__vc_config_set_auto_language();
	} else {
		if (false == __vc_config_mgr_check_lang_is_valid(g_config_info->engine_id, g_config_info->language)) {
			/* Default language is not valid */
			char* tmp_language;
			if (0 != __vc_config_mgr_select_lang(g_config_info->engine_id, &tmp_language)) {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to select language");
				return -1;
			}

			if (NULL != tmp_language) {
				if (NULL != g_config_info->language) {
					free(g_config_info->language);
					g_config_info->language = strdup(tmp_language);
				}

				if (0 != vc_parser_set_language(tmp_language)) {
					free(tmp_language);
					SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to save config");
					return -1;
				}

				free(tmp_language);
			}
		}
	}

	/* print daemon config */
	SLOG(LOG_DEBUG, vc_config_tag(), "== Daemon config ==");
	SLOG(LOG_DEBUG, vc_config_tag(), " engine : %s", g_config_info->engine_id);
	SLOG(LOG_DEBUG, vc_config_tag(), " auto language : %s", g_config_info->auto_lang ? "on" : "off");
	SLOG(LOG_DEBUG, vc_config_tag(), " language : %s", g_config_info->language);
	SLOG(LOG_DEBUG, vc_config_tag(), "===================");

	if (0 != vc_parser_get_foreground(&g_foreground_pid)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to get foreground pid");
		return VC_CONFIG_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, vc_config_tag(), "Current foreground pid : %d", g_foreground_pid);

	g_lang_ref_count = 0;

	/* Register to detect display language change */
	vconf_notify_key_changed(VCONFKEY_LANGSET, __vc_config_language_changed_cb, NULL);

	temp_client = (vc_config_client_s*)calloc(1, sizeof(vc_config_client_s));
	if (NULL == temp_client) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to allocate memory");
		return VC_ERROR_OUT_OF_MEMORY;
	}

	temp_client->uid = uid;

	SLOG(LOG_DEBUG, vc_config_tag(), "uid(%d) temp_uid(%d)", uid, temp_client->uid);

	/* Add uid */
	g_config_client_list = g_slist_append(g_config_client_list, temp_client);

	__vc_config_mgr_print_client_info();

	return 0;
}

int vc_config_mgr_finalize(int uid)
{
	GSList *iter = NULL;
	vc_config_client_s* temp_client = NULL;

	if (0 < g_slist_length(g_config_client_list)) {
		/* Check uid */
		iter = g_slist_nth(g_config_client_list, 0);

		while (NULL != iter) {
			temp_client = iter->data;

			if (NULL != temp_client) {
				if (uid == temp_client->uid) {
					g_config_client_list = g_slist_remove(g_config_client_list, temp_client);
					free(temp_client);
					break;
				}
			}

			iter = g_slist_next(iter);
		}
	}

	if (0 < g_slist_length(g_config_client_list)) {
		SLOG(LOG_DEBUG, vc_config_tag(), "Client count (%d)", g_slist_length(g_config_client_list));
		return 0;
	}

	vc_engine_info_s *engine_info = NULL;

	if (0 < g_slist_length(g_engine_list)) {

		/* Get a first item */
		iter = g_slist_nth(g_engine_list, 0);

		while (NULL != iter) {
			engine_info = iter->data;

			if (NULL != engine_info) {
				g_engine_list = g_slist_remove(g_engine_list, engine_info);

				vc_parser_free_engine_info(engine_info);
			}

			iter = g_slist_nth(g_engine_list, 0);
		}
	}

	vconf_ignore_key_changed(VCONFKEY_LANGSET, __vc_config_language_changed_cb);

	vc_parser_unload_config(g_config_info);

	SLOG(LOG_DEBUG, vc_config_tag(), "[Success] Finalize config");

	return 0;
}


int __vc_config_mgr_register_lang_event()
{
	if (0 == g_lang_ref_count) {
		/* get file notification handler */
		int fd;
		int wd;

		fd = inotify_init();
		if (fd < 0) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail get inotify fd");
			return -1;
		}
		g_fd_lang = fd;

		wd = inotify_add_watch(fd, VC_CONFIG, IN_CLOSE_WRITE);
		g_wd_lang = wd;

		g_fd_handler_lang = ecore_main_fd_handler_add(fd, ECORE_FD_READ, (Ecore_Fd_Cb)vc_config_mgr_inotify_event_cb, NULL, NULL, NULL);
		if (NULL == g_fd_handler_lang) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to get handler_noti");
			return -1;
		}

		/* Set non-blocking mode of file */
		int value;
		value = fcntl(fd, F_GETFL, 0);
		value |= O_NONBLOCK;

		if (0 > fcntl(fd, F_SETFL, value)) {
			SLOG(LOG_WARN, vc_config_tag(), "[WARNING] Fail to set non-block mode");
		}
	}
	g_lang_ref_count++;

	return 0;
}

int __vc_config_mgr_unregister_config_event()
{
	if (0 == g_lang_ref_count) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Lang ref count is 0");
		return VC_CONFIG_ERROR_INVALID_STATE;
	}

	g_lang_ref_count--;
	if (0 == g_lang_ref_count) {
		/* delete inotify variable */
		ecore_main_fd_handler_del(g_fd_handler_lang);
		inotify_rm_watch(g_fd_lang, g_wd_lang);
		close(g_fd_lang);

		vconf_ignore_key_changed(VCONFKEY_LANGSET, __vc_config_language_changed_cb);
	}

	return 0;
}

int vc_config_mgr_set_lang_cb(int uid, vc_config_lang_changed_cb lang_cb)
{
	GSList *iter = NULL;
	vc_config_client_s* temp_client = NULL;

	/* Call all callbacks of client*/
	iter = g_slist_nth(g_config_client_list, 0);

	while (NULL != iter) {
		temp_client = iter->data;

		if (NULL != temp_client) {
			if (uid == temp_client->uid) {
				temp_client->lang_cb = lang_cb;
				if (0 != __vc_config_mgr_register_lang_event()) {
					SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to register config event");
					return VC_CONFIG_ERROR_OPERATION_FAILED;
				}
			}
		}
		iter = g_slist_next(iter);
	}

	return 0;
}

int vc_config_mgr_unset_lang_cb(int uid)
{
	GSList *iter = NULL;
	vc_config_client_s* temp_client = NULL;

	/* Call all callbacks of client*/
	iter = g_slist_nth(g_config_client_list, 0);

	while (NULL != iter) {
		temp_client = iter->data;

		if (NULL != temp_client) {
			if (uid == temp_client->uid) {
				temp_client->lang_cb = NULL;
				__vc_config_mgr_unregister_config_event();
			}
		}
		iter = g_slist_next(iter);
	}

	return 0;
}

int vc_config_mgr_set_enabled_cb(int uid, vc_config_enabled_cb enabled_cb)
{
	if (NULL == enabled_cb) {
		SLOG(LOG_ERROR, vc_config_tag(), "enabled cb is NULL : uid(%d) ", uid);
		return VC_CONFIG_ERROR_INVALID_PARAMETER;
	}

	GSList *iter = NULL;
	vc_config_client_s* temp_client = NULL;

	/* Call all callbacks of client*/
	iter = g_slist_nth(g_config_client_list, 0);

	while (NULL != iter) {
		temp_client = iter->data;

		if (NULL != temp_client) {
			if (uid == temp_client->uid) {
				temp_client->enabled_cb = enabled_cb;
				return VC_CONFIG_ERROR_NONE;
			}
		}
		iter = g_slist_next(iter);
	}

	return VC_CONFIG_ERROR_INVALID_PARAMETER;
}

int vc_config_mgr_unset_enabled_cb(int uid)
{
	GSList *iter = NULL;
	vc_config_client_s* temp_client = NULL;

	/* Call all callbacks of client*/
	iter = g_slist_nth(g_config_client_list, 0);

	while (NULL != iter) {
		temp_client = iter->data;

		if (NULL != temp_client) {
			if (uid == temp_client->uid) {
				temp_client->enabled_cb = NULL;
				return 0;
			}
		}
		iter = g_slist_next(iter);
	}

	return VC_CONFIG_ERROR_INVALID_PARAMETER;
}

int vc_config_mgr_get_auto_language(bool* value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "Not initialized");
		return -1;
	}

	if (NULL == value) {
		return -1;
	}

	*value = g_config_info->auto_lang;

	return 0;
}

int vc_config_mgr_set_auto_language(bool value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "Not initialized");
		return -1;
	}

	if (g_config_info->auto_lang != value) {
		/* Check language is valid */
		if (0 != vc_parser_set_auto_lang(value)) {
			SLOG(LOG_ERROR, vc_config_tag(), "Fail to save engine id");
			return -1;
		}
		g_config_info->auto_lang = value;

		if (true == g_config_info->auto_lang) {
			__vc_config_set_auto_language();
		}
	}

	return 0;
}

int vc_config_mgr_get_language_list(vc_supported_language_cb callback, void* user_data)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "Not initialized");
		return -1;
	}

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "There is no engine");
		return -1;
	}

	GSList *iter = NULL;
	vc_engine_info_s *engine_info = NULL;

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] engine info is NULL");
			return -1;
		}

		if (0 != strcmp(g_config_info->engine_id, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		GSList *iter_lang = NULL;
		char* lang;

		/* Get a first item */
		iter_lang = g_slist_nth(engine_info->languages, 0);

		while (NULL != iter_lang) {
			/*Get handle data from list*/
			lang = iter_lang->data;

			SLOG(LOG_DEBUG, vc_config_tag(), " %s", lang);
			if (NULL != lang) {
				if (false == callback(lang, user_data))
					break;
			}

			/*Get next item*/
			iter_lang = g_slist_next(iter_lang);
		}
		break;
	}

	return 0;
}

int vc_config_mgr_get_default_language(char** language)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "Not initialized");
		return -1;
	}

	if (NULL == language) {
		return -1;
	}

	if (NULL != g_config_info->language) {
		*language = strdup(g_config_info->language);
	} else {
		SLOG(LOG_ERROR, vc_config_tag(), " language is NULL");
		return -1;
	}

	return 0;
}

int vc_config_mgr_set_default_language(const char* language)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "Not initialized");
		return -1;
	}

	if (NULL == language) {
		return -1;
	}

	/* Check language is valid */
	if (NULL != g_config_info->language) {
		if (0 != vc_parser_set_language(language)) {
			SLOG(LOG_ERROR, vc_config_tag(), "Fail to save engine id");
			return -1;
		}
		free(g_config_info->language);
		g_config_info->language = strdup(language);
	} else {
		SLOG(LOG_ERROR, vc_config_tag(), " language is NULL");
		return -1;
	}

	return 0;
}

int vc_config_mgr_get_enabled(bool* value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "Not initialized");
		return -1;
	}

	if (NULL == value) {
		return -1;
	}

	*value = g_config_info->enabled;

	return 0;
}

int vc_config_mgr_set_enabled(bool value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "Not initialized");
		return -1;
	}

	if (0 != vc_parser_set_enabled(value)) {
		SLOG(LOG_ERROR, vc_config_tag(), "Fail to set enabled");
		return -1;
	}

	g_config_info->enabled = value;

	return 0;
}

int vc_config_mgr_get_nonfixed_support(bool* value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "Not initialized");
		return -1;
	}

	if (NULL == value) {
		SLOG(LOG_ERROR, vc_config_tag(), "Input parameter is NULL");
		return -1;
	}

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "There is no engine");
		return -1;
	}

	GSList *iter = NULL;
	vc_engine_info_s *engine_info = NULL;

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] engine info is NULL");
			return -1;
		}

		if (0 != strcmp(g_config_info->engine_id, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		*value = engine_info->non_fixed_support;

		break;
	}

	return 0;
}

bool vc_config_check_default_engine_is_valid(const char* engine)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "Not initialized");
		return -1;
	}

	if (NULL == engine) {
		return false;
	}

	if (0 >= g_slist_length(g_engine_list))
		return false;

	GSList *iter = NULL;
	vc_engine_info_s *engine_info = NULL;

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL != engine_info) {
			if (0 == strcmp(engine, engine_info->uuid)) {
				return true;
			}
		}
		iter = g_slist_next(iter);
	}

	return false;
}

bool vc_config_check_default_language_is_valid(const char* language)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "Not initialized");
		return -1;
	}

	if (NULL == language) {
		return false;
	}

	if (NULL == g_config_info->engine_id) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Default engine id is NULL");
		return false;
	}

	if (0 >= g_slist_length(g_engine_list))
		return false;

	GSList *iter = NULL;
	vc_engine_info_s *engine_info = NULL;

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	while (NULL != iter) {
		engine_info = iter->data;

		if (NULL == engine_info) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Engine info is NULL");
			iter = g_slist_next(iter);
			continue;
		}

		if (0 == strcmp(g_config_info->engine_id, engine_info->uuid)) {
			iter = g_slist_next(iter);
			continue;
		}

		GSList *iter_lang = NULL;
		char* lang;

		/* Get a first item */
		iter_lang = g_slist_nth(engine_info->languages, 0);

		while (NULL != iter_lang) {
			lang = iter_lang->data;

			if (0 == strcmp(language, lang))
				return true;

			/*Get next item*/
			iter_lang = g_slist_next(iter_lang);
		}
		break;
	}

	return false;
}

int vc_config_mgr_set_foreground(int pid, bool value)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "Not initialized");
		return -1;
	}

	return vc_parser_set_foreground(pid, value);
}

int vc_config_mgr_get_foreground(int* pid)
{
	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_ERROR, vc_config_tag(), "Not initialized");
		return -1;
	}

	return vc_parser_get_foreground(pid);
}



int __vc_config_mgr_print_engine_info()
{
	GSList *iter = NULL;
	vc_engine_info_s *engine_info = NULL;

	if (0 >= g_slist_length(g_engine_list)) {
		SLOG(LOG_DEBUG, vc_config_tag(), "-------------- engine list -----------------");
		SLOG(LOG_DEBUG, vc_config_tag(), "  No Engine in engine directory");
		SLOG(LOG_DEBUG, vc_config_tag(), "--------------------------------------------");
		return 0;
	}

	/* Get a first item */
	iter = g_slist_nth(g_engine_list, 0);

	SLOG(LOG_DEBUG, vc_config_tag(), "--------------- engine list -----------------");

	int i = 1;
	while (NULL != iter) {
		engine_info = iter->data;

		SLOG(LOG_DEBUG, vc_config_tag(), "[%dth]", i);
		SLOG(LOG_DEBUG, vc_config_tag(), " name : %s", engine_info->name);
		SLOG(LOG_DEBUG, vc_config_tag(), " id   : %s", engine_info->uuid);


		SLOG(LOG_DEBUG, vc_config_tag(), " languages");
		GSList *iter_lang = NULL;
		char* lang;
		if (g_slist_length(engine_info->languages) > 0) {
			/* Get a first item */
			iter_lang = g_slist_nth(engine_info->languages, 0);

			int j = 1;
			while (NULL != iter_lang) {
				/*Get handle data from list*/
				lang = iter_lang->data;

				SLOG(LOG_DEBUG, vc_config_tag(), "  [%dth] %s", j, lang);

				/*Get next item*/
				iter_lang = g_slist_next(iter_lang);
				j++;
			}
		} else {
			SLOG(LOG_ERROR, vc_config_tag(), "  language is NONE");
		}
		SLOG(LOG_DEBUG, vc_config_tag(), " ");
		iter = g_slist_next(iter);
		i++;
	}
	SLOG(LOG_DEBUG, vc_config_tag(), "--------------------------------------------");

	return 0;
}

int __vc_config_mgr_print_client_info()
{
	GSList *iter = NULL;
	vc_config_client_s* temp_client = NULL;

	if (0 >= g_slist_length(g_config_client_list)) {
		SLOG(LOG_DEBUG, vc_config_tag(), "-------------- Client list -----------------");
		SLOG(LOG_DEBUG, vc_config_tag(), "  No Client");
		SLOG(LOG_DEBUG, vc_config_tag(), "--------------------------------------------");
		return 0;
	}

	/* Get a first item */
	iter = g_slist_nth(g_config_client_list, 0);

	SLOG(LOG_DEBUG, vc_config_tag(), "--------------- Client list -----------------");

	int i = 1;
	while (NULL != iter) {
		temp_client = iter->data;

		SLOG(LOG_DEBUG, vc_config_tag(), "[%dth] uid(%d)", i, temp_client->uid);

		iter = g_slist_next(iter);
		i++;
	}
	SLOG(LOG_DEBUG, vc_config_tag(), "--------------------------------------------");

	return 0;
}
