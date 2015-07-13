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


#include <dlog.h>
#include <libxml/parser.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vconf.h>

#include "vc_defs.h"
#include "vc_main.h"
#include "vc_config_parser.h"


#define VC_TAG_ENGINE_BASE_TAG		"voice-control-engine"
#define VC_TAG_ENGINE_NAME		"name"
#define VC_TAG_ENGINE_ID		"id"
#define VC_TAG_ENGINE_LANGUAGE_SET	"languages"
#define VC_TAG_ENGINE_LANGUAGE		"lang"
#define VC_TAG_ENGINE_NON_FIXED_SUPPORT	"non-fixed-support"

#define VC_TAG_CONFIG_BASE_TAG		"voice-control-config"
#define VC_TAG_CONFIG_ENGINE_ID		"engine"
#define VC_TAG_CONFIG_AUTO_LANGUAGE	"auto"
#define VC_TAG_CONFIG_LANGUAGE		"language"
#define VC_TAG_CONFIG_ENABLED		"enabled"

#define VC_TAG_INFO_BASE_TAG		"vc_info_option"
#define VC_TAG_INFO_SERVICE_STATE	"service_state"
#define VC_TAG_INFO_FOREGROUND		"foreground_pid"


extern char* vc_config_tag();

static xmlDocPtr g_config_doc = NULL;

static int __vc_config_parser_set_file_mode(const char* filename)
{
	if (NULL == filename) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Invalid parameter");
		return -1;
	}

	if (0 > chmod(filename, 0666)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to change file mode");
		return -1;
	}

	if (0 > chown(filename, 5000, 5000)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to change file owner");
		return -1;
	}

	return 0;
}

int vc_parser_get_engine_info(const char* path, vc_engine_info_s** engine_info)
{
	if (NULL == path || NULL == engine_info) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *key;

	doc = xmlParseFile(path);
	if (doc == NULL) {
		SLOG(LOG_WARN, vc_config_tag(), "[WARNING] Fail to parse file error : %s", path);
		return -1;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_ENGINE_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] The wrong type, root node is NOT 'voice-control-engine'");
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	/* alloc engine info */
	vc_engine_info_s* temp;
	temp = (vc_engine_info_s*)calloc(1, sizeof(vc_engine_info_s));
	if (NULL == temp) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to allocate memory");
		xmlFreeDoc(doc);
		return VC_ERROR_OUT_OF_MEMORY;
	}

	temp->name = NULL;
	temp->uuid = NULL;
	temp->languages = NULL;
	temp->non_fixed_support = false;

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_ENGINE_NAME)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				// SLOG(LOG_DEBUG, vc_config_tag(), "Engine name : %s", (char *)key);
				if (NULL != temp->name)	free(temp->name);
				temp->name = strdup((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] <%s> has no content", VC_TAG_ENGINE_ID);
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_ENGINE_ID)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				// SLOG(LOG_DEBUG, vc_config_tag(), "Engine uuid : %s", (char *)key);
				if (NULL != temp->uuid)	free(temp->uuid);
				temp->uuid = strdup((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] <%s> has no content", VC_TAG_ENGINE_ID);
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_ENGINE_LANGUAGE_SET)) {
			xmlNodePtr lang_node = NULL;
			char* temp_lang = NULL;

			lang_node = cur->xmlChildrenNode;

			while (lang_node != NULL) {
				if (0 == xmlStrcmp(lang_node->name, (const xmlChar *)VC_TAG_ENGINE_LANGUAGE)){
					key = xmlNodeGetContent(lang_node);
					if (NULL != key) {
						// SLOG(LOG_DEBUG, vc_config_tag(), "language : %s", (char *)key);
						temp_lang = strdup((char*)key);
						temp->languages = g_slist_append(temp->languages, temp_lang);
						xmlFree(key);
					} else {
						SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] <%s> has no content", VC_TAG_ENGINE_LANGUAGE);
					}
				}

				lang_node = lang_node->next;
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_ENGINE_NON_FIXED_SUPPORT)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				// SLOG(LOG_DEBUG, vc_config_tag(), "Engine uuid : %s", (char *)key);

				if (0 == xmlStrcmp(key, (const xmlChar *)"true")) {
					temp->non_fixed_support = true;
				} else if (0 == xmlStrcmp(key, (const xmlChar *)"false")) {
					temp->non_fixed_support = false;
				} else {
					SLOG(LOG_ERROR, vc_config_tag(), "Auto voice is wrong");
					temp->non_fixed_support = false;
				}
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] <%s> has no content", VC_TAG_ENGINE_NON_FIXED_SUPPORT);
			}
		} else {

		}

		cur = cur->next;
	}

	xmlFreeDoc(doc);

	if (NULL == temp->uuid || NULL == temp->languages) {
		/* Invalid engine */
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Invalid engine : %s", path);
		vc_parser_free_engine_info(temp);
		return -1;
	}

	*engine_info = temp;

	return 0;
}

int vc_parser_free_engine_info(vc_engine_info_s* engine_info)
{
	if (NULL == engine_info) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	if (NULL != engine_info->name)		free(engine_info->name);
	if (NULL != engine_info->uuid)		free(engine_info->uuid);

	int count = g_slist_length(engine_info->languages);

	int i ;
	char *temp_lang;

	for (i = 0;i < count ;i++) {
		temp_lang = g_slist_nth_data(engine_info->languages, 0);

		if (NULL != temp_lang) {
			engine_info->languages = g_slist_remove(engine_info->languages, temp_lang);

			if (NULL != temp_lang)
				free(temp_lang);
		} 
	}

	if (NULL != engine_info)	free(engine_info);

	return 0;	
}

int vc_parser_print_engine_info(vc_engine_info_s* engine_info)
{
	if (NULL == engine_info)
		return -1;

	SLOG(LOG_DEBUG, vc_config_tag(), "== engine info ==");
	SLOG(LOG_DEBUG, vc_config_tag(), " id   : %s", engine_info->uuid);
	
	SLOG(LOG_DEBUG, vc_config_tag(), " languages");
	GSList *iter = NULL;
	char* lang;
	if (g_slist_length(engine_info->languages) > 0) {
		/* Get a first item */
		iter = g_slist_nth(engine_info->languages, 0);

		int i = 1;	
		while (NULL != iter) {
			/*Get handle data from list*/
			lang = iter->data;

			SLOG(LOG_DEBUG, vc_config_tag(), "  [%dth] %s", i, lang);

			/*Get next item*/
			iter = g_slist_next(iter);
			i++;
		}
	} else {
		SLOG(LOG_ERROR, vc_config_tag(), "  language is NONE");
	}
	SLOG(LOG_DEBUG, vc_config_tag(), "=====================");

	return 0;
}

int vc_parser_load_config(vc_config_s** config_info)
{
	if (NULL == config_info) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *key;
	bool is_default_open = false;

	if (0 != access(VC_RUNTIME_INFO_ROOT, F_OK)) {
		SLOG(LOG_DEBUG, vc_config_tag(), "No info root directory");
		if (0 != access(VC_CONFIG_ROOT, F_OK)) {
			SLOG(LOG_DEBUG, vc_config_tag(), "No root directory");
			if (0 != mkdir(VC_CONFIG_ROOT, 0755)) {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to make directory");
			}
		}
		if (0 != mkdir(VC_RUNTIME_INFO_ROOT, 0755)) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to make directory");
		}
	}

	if (0 != access(VC_CONFIG, F_OK)) {
		doc = xmlParseFile(VC_CONFIG_DEFAULT);
		if (doc == NULL) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to parse file error : %s", VC_CONFIG_DEFAULT);
			return -1;
		}
		SLOG(LOG_DEBUG, vc_config_tag(), "Use default config : %s", VC_CONFIG_DEFAULT);
		is_default_open = true;
	} else {
		int retry_count = 0;

		while (NULL == doc) {
			doc = xmlParseFile(VC_CONFIG);
			if (NULL != doc) {
				break;
			}
			retry_count++;
			usleep(10000);

			if (VC_RETRY_COUNT == retry_count) {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to parse file error : %s", VC_CONFIG);
				return -1;
			}
		}
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) VC_TAG_CONFIG_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] The wrong type, root node is NOT %s", VC_TAG_CONFIG_BASE_TAG);
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	/* alloc engine info */
	vc_config_s* temp;
	temp = (vc_config_s*)calloc(1, sizeof(vc_config_s));
	if (NULL == temp) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to allocate memory");
		xmlFreeDoc(doc);
		return VC_ERROR_OUT_OF_MEMORY;
	}

	temp->engine_id = NULL;
	temp->language = NULL;

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_CONFIG_ENGINE_ID)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				//SLOG(LOG_DEBUG, vc_config_tag(), "Engine id : %s", (char *)key);
				if (NULL != temp->engine_id)	free(temp->engine_id);
				temp->engine_id = strdup((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] enable is NULL");
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_CONFIG_AUTO_LANGUAGE)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_config_tag(), "Auto language : %s", (char *)key);

				if (0 == xmlStrcmp(key, (const xmlChar *)"on")) {
					temp->auto_lang = true;
				} else if (0 == xmlStrcmp(key, (const xmlChar *)"off")) {
					temp->auto_lang = false;
				} else {
					SLOG(LOG_ERROR, vc_config_tag(), "Auto voice is wrong");
					temp->auto_lang = false;
				}

				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] voice type is NULL");
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_CONFIG_LANGUAGE)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				//SLOG(LOG_DEBUG, vc_config_tag(), "language : %s", (char *)key);
				if (NULL != temp->language)	free(temp->language);
				temp->language = strdup((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] language is NULL");
			}
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar*)VC_TAG_CONFIG_ENABLED)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				//SLOG(LOG_DEBUG, vc_config_tag(), "Enabled service : %s", (char *)key);

				if (0 == xmlStrcmp(key, (const xmlChar *)"on")) {
					temp->enabled = true;
				}
				else if (0 == xmlStrcmp(key, (const xmlChar *)"off")) {
					temp->enabled = false;
				}
				else {
					SLOG(LOG_ERROR, vc_config_tag(), "Enabled service is wrong");
					temp->enabled = false;
				}
			} else {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Enabled service is NULL");
			}
		} else {

		}

		cur = cur->next;
	}

	*config_info = temp;
	g_config_doc = doc;

	if (is_default_open) {
		xmlSaveFile(VC_CONFIG, g_config_doc);
		if (0 != __vc_config_parser_set_file_mode(VC_CONFIG))
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to set file mode - %s", VC_CONFIG);
	}

	return 0;
}

int vc_parser_unload_config(vc_config_s* config_info)
{
	if (NULL != g_config_doc)	xmlFreeDoc(g_config_doc);

	if (NULL != config_info) {
		if (NULL != config_info->engine_id)	free(config_info->engine_id);
		if (NULL != config_info->language)	free(config_info->language);
		free(config_info);
	}

	return 0;
}

int vc_parser_set_engine(const char* engine_id)
{
	if (NULL == engine_id) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	xmlNodePtr cur = NULL;
	cur = xmlDocGetRootElement(g_config_doc);
	if (NULL == cur) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) VC_TAG_CONFIG_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] The wrong type, root node is NOT %s", VC_TAG_CONFIG_BASE_TAG);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (NULL == cur) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		return -1;
	}

	while (NULL != cur) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_CONFIG_ENGINE_ID)) {
			xmlNodeSetContent(cur, (const xmlChar *)engine_id);
		}

		cur = cur->next;
	}

	int ret = xmlSaveFile(VC_CONFIG, g_config_doc);
	if (0 > ret) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Save result : %d", ret);
	}

	return 0;
}

int vc_parser_set_auto_lang(bool value)
{
	if (NULL == g_config_doc)
		return -1;

	xmlNodePtr cur = NULL;
	cur = xmlDocGetRootElement(g_config_doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) VC_TAG_CONFIG_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] The wrong type, root node is NOT %s", VC_TAG_CONFIG_BASE_TAG);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_CONFIG_AUTO_LANGUAGE)) {
			if (true == value) {
				xmlNodeSetContent(cur, (const xmlChar *)"on");
			} else if (false == value) {
				xmlNodeSetContent(cur, (const xmlChar *)"off");
			} else {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] The wrong value of auto voice");
				return -1;
			}
			break;
		}
		cur = cur->next;
	}

	int ret = xmlSaveFile(VC_CONFIG, g_config_doc);
	SLOG(LOG_DEBUG, vc_config_tag(), "Save result : %d", ret);

	return 0;
}

int vc_parser_set_language(const char* language)
{
	if (NULL == g_config_doc || NULL == language)
		return -1;

	xmlNodePtr cur = NULL;
	cur = xmlDocGetRootElement(g_config_doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) VC_TAG_CONFIG_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] The wrong type, root node is NOT %s", VC_TAG_CONFIG_BASE_TAG);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_CONFIG_LANGUAGE)) {
			xmlNodeSetContent(cur, (const xmlChar *)language);
		} 

		cur = cur->next;
	}

	int ret = xmlSaveFile(VC_CONFIG, g_config_doc);
	SLOG(LOG_DEBUG, vc_config_tag(), "Save result : %d", ret);

	return 0;
}

int vc_parser_set_enabled(bool value)
{
	if (NULL == g_config_doc)
		return -1;

	xmlNodePtr cur = NULL;
	cur = xmlDocGetRootElement(g_config_doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_CONFIG_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] The wrong type, root node is NOT %s", VC_TAG_CONFIG_BASE_TAG);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_CONFIG_ENABLED)) {
			xmlNodeSetContent(cur, (const xmlChar *)(value ? "on" : "off"));
		}

		cur = cur->next;
	}

	int ret = xmlSaveFile(VC_CONFIG, g_config_doc);
	SLOG(LOG_DEBUG, vc_config_tag(), "Save result : %d", ret);

	return 0;
}

int vc_parser_find_config_changed(int* auto_lang, char** language, int* enabled)
{
	if (NULL == auto_lang || NULL == language) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	xmlDocPtr doc = NULL;
	xmlNodePtr cur_new = NULL;
	xmlNodePtr cur_old = NULL;

	xmlChar *key_new;
	xmlChar *key_old;

	int retry_count = 0;

	while (NULL == doc) {
		doc = xmlParseFile(VC_CONFIG);
		if (NULL != doc) {
			break;
		}
		retry_count++;
		usleep(10000);

		if (VC_RETRY_COUNT == retry_count) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to parse file error : %s", VC_CONFIG);
			return -1;
		}
	}

	cur_new = xmlDocGetRootElement(doc);
	cur_old = xmlDocGetRootElement(g_config_doc);
	if (cur_new == NULL || cur_old == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur_new->name, (const xmlChar*)VC_TAG_CONFIG_BASE_TAG) || xmlStrcmp(cur_old->name, (const xmlChar*)VC_TAG_CONFIG_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] The wrong type, root node is NOT %s", VC_TAG_CONFIG_BASE_TAG);
		xmlFreeDoc(doc);
		return -1;
	}

	cur_new = cur_new->xmlChildrenNode;
	cur_old = cur_old->xmlChildrenNode;
	if (cur_new == NULL || cur_old == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	*language = NULL;
	while (cur_new != NULL && cur_old != NULL) {

		// SLOG(LOG_DEBUG, vc_config_tag(), "cur_new->name(%s), cur_old->name(%s)", (char*)cur_new->name, (char*)cur_old->name);

		if (0 == xmlStrcmp(cur_new->name, (const xmlChar*)VC_TAG_CONFIG_AUTO_LANGUAGE)) {
			if (0 == xmlStrcmp(cur_old->name, (const xmlChar*)VC_TAG_CONFIG_AUTO_LANGUAGE)) {
				key_old = xmlNodeGetContent(cur_old);
				if (NULL != key_old) {
					key_new = xmlNodeGetContent(cur_new);
					if (NULL != key_new) {
						if (0 != xmlStrcmp(key_old, key_new)) {
							SLOG(LOG_DEBUG, vc_config_tag(), "Old auto lang(%s), New auto lang(%s)", (char*)key_old, (char*)key_new);
							if (0 == xmlStrcmp(key_new, (const xmlChar*)"on")) {
								*auto_lang = 1;
							} else {
								*auto_lang = 0;
							}
						}
						xmlFree(key_new);
					}
					xmlFree(key_old);
				}
			} else {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Old config and new config are different");
			}
		} else if (0 == xmlStrcmp(cur_new->name, (const xmlChar*)VC_TAG_CONFIG_LANGUAGE)) {
			if (0 == xmlStrcmp(cur_old->name, (const xmlChar*)VC_TAG_CONFIG_LANGUAGE)) {
				key_old = xmlNodeGetContent(cur_old);
				if (NULL != key_old) {
					key_new = xmlNodeGetContent(cur_new);
					if (NULL != key_new) {
						if (0 != xmlStrcmp(key_old, key_new)) {
							SLOG(LOG_DEBUG, vc_config_tag(), "Old language(%s), New language(%s)", (char*)key_old, (char*)key_new);
							if (NULL != *language)	free(*language);
							*language = strdup((char*)key_new);
						}
						xmlFree(key_new);
					}
					xmlFree(key_old);
				}
			} else {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] old config and new config are different");
			}
		} else if (0 == xmlStrcmp(cur_new->name, (const xmlChar*)VC_TAG_CONFIG_ENABLED)) {
			if (0 == xmlStrcmp(cur_old->name, (const xmlChar*)VC_TAG_CONFIG_ENABLED)) {
				key_old = xmlNodeGetContent(cur_old);
				if (NULL != key_old) {
					key_new = xmlNodeGetContent(cur_new);
					if (NULL != key_new) {
						if (0 != xmlStrcmp(key_old, key_new)) {
							SLOG(LOG_DEBUG, vc_config_tag(), "Old enabled(%s), New enabled(%s)", (char*)key_old, (char*)key_new);
							if (0 == xmlStrcmp(key_new, (const xmlChar*)"on")) {
								*enabled = 1;
							} else {
								*enabled = 0;
							}
						}
						xmlFree(key_new);
					}
					xmlFree(key_old);
				}
			} else {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] old config and new config are different");
			}
		} else {

		}

		cur_new = cur_new->next;
		cur_old = cur_old->next;
	}
	
	xmlFreeDoc(g_config_doc);
	g_config_doc = doc;

	return 0;
}

int vc_parser_init_service_state()
{
	if (0 != access(VC_RUNTIME_INFO_SERVICE_STATE, R_OK | W_OK)) {
		/* make file */
		xmlDocPtr doc;
		xmlNodePtr root_node;
		xmlNodePtr info_node;

		doc = xmlNewDoc((const xmlChar*)"1.0");
		doc->encoding = (const xmlChar*)"utf-8";
		doc->charset = 1;

		root_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_INFO_BASE_TAG);
		xmlDocSetRootElement(doc,root_node);

		/* Make new command node */
		info_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_INFO_SERVICE_STATE);
		xmlNodeSetContent(info_node, (const xmlChar *)"0");
		xmlAddChild(root_node, info_node);

		int ret = xmlSaveFormatFile(VC_RUNTIME_INFO_SERVICE_STATE, doc, 1);
		SLOG(LOG_DEBUG, vc_config_tag(), "Save service state info file : %d", ret);

		if (0 != __vc_config_parser_set_file_mode(VC_RUNTIME_INFO_SERVICE_STATE))
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to set file mode - %s", VC_RUNTIME_INFO_SERVICE_STATE);
	}

	return 0;
}

int vc_parser_set_service_state(int state)
{
	/* Set service state */
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	int retry_count = 0;

	while (NULL == doc) {
		doc = xmlParseFile(VC_RUNTIME_INFO_SERVICE_STATE);
		if (NULL != doc) {
			break;
		}
		retry_count++;
		usleep(10000);

		if (VC_RETRY_COUNT == retry_count) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to parse file error : %s", VC_RUNTIME_INFO_SERVICE_STATE);
			return -1;
		}
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) VC_TAG_INFO_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] The wrong type, root node is NOT %s", VC_TAG_INFO_BASE_TAG);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_INFO_SERVICE_STATE)) {			
			char temp[16];
			memset(temp, 0, 16);
			snprintf(temp, 16, "%d", state);

			xmlNodeSetContent(cur, (const xmlChar *)temp);
			break;
		} 

		cur = cur->next;
	}

	int ret = xmlSaveFile(VC_RUNTIME_INFO_SERVICE_STATE, doc);
	if (0 >= ret) {
		SLOG(LOG_DEBUG, vc_config_tag(), "[ERROR] Fail to save service state info file : %d", ret);
		return -1;
	}
	SLOG(LOG_DEBUG, vc_config_tag(), "[Success] Save service state info file : state(%d)", state);

	return 0;
}

int vc_parser_get_service_state(int* state)
{
	if (NULL == state) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	if (0 != access(VC_RUNTIME_INFO_SERVICE_STATE, F_OK)) {
		/* make file */
		xmlDocPtr doc;
		xmlNodePtr root_node;
		xmlNodePtr info_node;

		doc = xmlNewDoc((const xmlChar*)"1.0");
		doc->encoding = (const xmlChar*)"utf-8";
		doc->charset = 1;

		root_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_INFO_BASE_TAG);
		xmlDocSetRootElement(doc, root_node);

		/* Make new command node */
		info_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_INFO_SERVICE_STATE);
		xmlNodeSetContent(info_node, (const xmlChar *)"0");
		xmlAddChild(root_node, info_node);

		int ret = xmlSaveFormatFile(VC_RUNTIME_INFO_SERVICE_STATE, doc, 1);
		SLOG(LOG_DEBUG, vc_config_tag(), "Save runtime info file : %d", ret);

		if (0 != __vc_config_parser_set_file_mode(VC_RUNTIME_INFO_SERVICE_STATE))
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to set file mode - %s", VC_RUNTIME_INFO_SERVICE_STATE);

		*state = 0;
		return 0;
	}

	/* Check file */
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *key;
	int retry_count = 0;

	while (NULL == doc) {
		doc = xmlParseFile(VC_RUNTIME_INFO_SERVICE_STATE);
		if (NULL != doc) {
			break;
		}
		retry_count++;
		usleep(10000);

		if (VC_RETRY_COUNT == retry_count) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to parse file error : %s", VC_RUNTIME_INFO_SERVICE_STATE);
			return -1;
		}
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_INFO_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] The wrong type, root node is NOT '%s'", VC_TAG_INFO_BASE_TAG);
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_INFO_SERVICE_STATE)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_config_tag(), "Service state : %s", (char *)key);
				*state = atoi((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] enable is NULL");
			}
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);

	return 0;
}

int vc_parser_set_foreground(int pid, bool value)
{
	int cur_pid = 0;
	/* Get foreground pid */
	if (0 != vc_parser_get_foreground(&cur_pid)) {
		SLOG(LOG_DEBUG, vc_config_tag(), "Fail to get pid from info file");
		return -1;
	}

	if (true == value) {
		/* Focus in */
		if (cur_pid == pid) {
			return 0;
		}
		SLOG(LOG_DEBUG, vc_config_tag(), "Current foreground pid (%d)", cur_pid);
	} else {
		/* Focus out */
		if (VC_NO_FOREGROUND_PID != cur_pid) {
			if (cur_pid != pid) {
				SLOG(LOG_DEBUG, vc_config_tag(), "Input pid(%d) is NOT different from the saved pid(%d)", pid, cur_pid);
				return 0;
			}
		}
	}

	/* Set foreground pid */
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	int retry_count = 0;

	while (NULL == doc) {
		doc = xmlParseFile(VC_RUNTIME_INFO_FOREGROUND);
		if (NULL != doc) {
			break;
		}
		retry_count++;
		usleep(10000);

		if (VC_RETRY_COUNT == retry_count) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to parse file error : %s", VC_RUNTIME_INFO_FOREGROUND);
			return -1;
		}
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) VC_TAG_INFO_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] The wrong type, root node is NOT %s", VC_TAG_INFO_BASE_TAG);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_INFO_FOREGROUND)) {			
			char temp[16];
			memset(temp, 0, 16);
			if (true == value) {
				snprintf(temp, 16, "%d", pid);
			} else {
				snprintf(temp, 16, "%d", VC_NO_FOREGROUND_PID);
			}

			xmlNodeSetContent(cur, (const xmlChar *)temp);
			break;
		} 

		cur = cur->next;
	}

	int ret = xmlSaveFile(VC_RUNTIME_INFO_FOREGROUND, doc);
	if (0 >= ret) {
		SLOG(LOG_DEBUG, vc_config_tag(), "[ERROR] Fail to save foreground info : %d", ret);
		return -1;
	}
	SLOG(LOG_DEBUG, vc_config_tag(), "[Success] Save foreground info pid(%d)", pid);

	return 0;
}

int vc_parser_get_foreground(int* pid)
{
	if (NULL == pid) {
		SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	int ret;

	if (0 != access(VC_RUNTIME_INFO_FOREGROUND, F_OK)) {
		/* make file */
		xmlDocPtr doc;
		xmlNodePtr root_node;
		xmlNodePtr info_node;

		doc = xmlNewDoc((const xmlChar*)"1.0");
		doc->encoding = (const xmlChar*)"utf-8";
		doc->charset = 1;

		root_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_INFO_BASE_TAG);
		xmlDocSetRootElement(doc,root_node);

		/* Make new command node */
		info_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_INFO_FOREGROUND);
		xmlNodeSetContent(info_node, (const xmlChar *)"0");
		xmlAddChild(root_node, info_node);

		ret = xmlSaveFormatFile(VC_RUNTIME_INFO_FOREGROUND, doc, 1);
		SLOG(LOG_DEBUG, vc_config_tag(), "Save runtime info file : %d", ret);

		*pid = 0;

		if (0 != __vc_config_parser_set_file_mode(VC_RUNTIME_INFO_FOREGROUND))
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to set file mode - %s", VC_RUNTIME_INFO_FOREGROUND);
	} else {
		/* Check file */
		xmlDocPtr doc = NULL;
		xmlNodePtr cur = NULL;
		xmlChar *key;

		int retry_count = 0;

		while (NULL == doc) {
			doc = xmlParseFile(VC_RUNTIME_INFO_FOREGROUND);
			if (NULL != doc) {
				break;
			}
			retry_count++;
			usleep(10000);

			if (VC_RETRY_COUNT == retry_count) {
				SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Fail to parse file error : %s", VC_RUNTIME_INFO_FOREGROUND);
				return -1;
			}
		}

		cur = xmlDocGetRootElement(doc);
		if (cur == NULL) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
			xmlFreeDoc(doc);
			return -1;
		}

		if (xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_INFO_BASE_TAG)) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] The wrong type, root node is NOT '%s'", VC_TAG_INFO_BASE_TAG);
			xmlFreeDoc(doc);
			return -1;
		}

		cur = cur->xmlChildrenNode;
		if (cur == NULL) {
			SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] Empty document");
			xmlFreeDoc(doc);
			return -1;
		}

		while (cur != NULL) {
			if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_INFO_FOREGROUND)) {
				key = xmlNodeGetContent(cur);
				if (NULL != key) {
					SLOG(LOG_DEBUG, vc_config_tag(), "Foreground pid : %s", (char *)key);
					*pid = atoi((char*)key);
					xmlFree(key);
				} else {
					SLOG(LOG_ERROR, vc_config_tag(), "[ERROR] enable is NULL");
				}
			}
			cur = cur->next;
		}

		xmlFreeDoc(doc);
	}

	return 0;
}