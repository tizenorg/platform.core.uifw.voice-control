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

#include "vc_defs.h"
#include "vc_info_parser.h"
#include "vc_main.h"
#include "voice_control_command.h"
#include "voice_control_command_expand.h"
#include "voice_control_common.h"


#define VC_TAG_CMD_BASE_TAG		"vc_commands"

#define VC_TAG_CMD_COMMAND		"command"
#define VC_TAG_CMD_ID			"cmd_id"
#define VC_TAG_CMD_PID			"cmd_pid"
#define VC_TAG_CMD_TYPE			"cmd_type"
#define VC_TAG_CMD_FORMAT		"cmd_format"
#define VC_TAG_CMD_COMMAND_TEXT		"cmd_cmd_text"
#define VC_TAG_CMD_PARAMETER_TEXT	"cmd_param_text"
#define VC_TAG_CMD_DOMAIN		"cmd_domain"
#define VC_TAG_CMD_KEY			"cmd_key"
#define VC_TAG_CMD_MODIFIER		"cmd_modifier"

#define VC_TAG_RESULT_BASE_TAG		"vc_results"
#define VC_TAG_RESULT_TEXT		"result_text"
#define VC_TAG_RESULT_EVENT		"result_event"
#define VC_TAG_RESULT_MESSAGE		"result_message"

#define VC_TAG_INFO_BASE_TAG		"vc_info_option"
#define VC_TAG_INFO_FOREGROUND		"foreground_pid"

#define VC_TAG_DEMANDABLE_CLIENT_BASE_TAG	"vc_demandable_client"
#define VC_TAG_DEMANDABLE_CLIENT_APPID		"appid"

#define VC_TAG_CLIENT_BASE_TAG		"vc_client_info"
#define VC_TAG_CLIENT_CLIENT		"client"
#define VC_TAG_CLIENT_PID		"pid"
#define VC_TAG_CLIENT_FGCMD		"fgcmd"
#define VC_TAG_CLIENT_BGCMD		"bgcmd"
#define VC_TAG_CLIENT_EXCMD		"excmd"


const char* vc_info_tag()
{
	return TAG_VCINFO;
}

int __vc_cmd_parser_print_commands(GSList* cmd_list);


static int __vc_info_parser_set_file_mode(const char* filename)
{
	if (NULL == filename) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Invalid parameter");
		return -1;
	}

	if (0 > chmod(filename, 0666)) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Fail to change file mode");
		return -1;
	}

#if 0 /*Does not need to change owner on Tizen 3.0*/
	if (0 > chown(filename, 5000, 5000)) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Fail to change file owner");
		return -1;
	}
#endif

	return 0;
}

int __vc_cmd_parser_make_filepath(int pid, vc_cmd_type_e type, char** path)
{
	if (NULL == path) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	*path = (char*)calloc(256, sizeof(char));
	if (NULL == *path) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Fail to allocate memory");
		return VC_ERROR_OUT_OF_MEMORY;
	}

	snprintf(*path, 256, "%s/vc_%d_%d.xml", VC_RUNTIME_INFO_ROOT, (int)type, pid);

	return 0;
}

int vc_cmd_parser_save_file(int pid, vc_cmd_type_e type, GSList* cmd_list)
{
	if (0 >= g_slist_length(cmd_list)) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Command list is invalid");
		return -1;
	}

	/* Check file */
	char* filepath = NULL;
	__vc_cmd_parser_make_filepath(pid, type, &filepath);

	if (NULL == filepath) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Fail to create file path");
		return -1;
	}

	remove(filepath);

	xmlDocPtr doc;
	xmlNodePtr root_node;
	xmlNodePtr cmd_node;
	xmlNodePtr tmp_node;

	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->encoding = (const xmlChar*)"utf-8";
	doc->charset = 1;

	root_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_BASE_TAG);
	xmlDocSetRootElement(doc, root_node);

	GSList *iter = NULL;
	vc_cmd_s *temp_cmd;

	int i;
	int count = g_slist_length(cmd_list);
	iter = g_slist_nth(cmd_list, 0);

	SLOG(LOG_DEBUG, vc_info_tag(), "list count : %d", count);
	char temp[16] = {0, };
	int selected_count = 0;

	for (i = 0; i < count; i++) {
		if (NULL == iter)
			break;

		temp_cmd = iter->data;

		if (NULL == temp_cmd) {
			SLOG(LOG_ERROR, vc_info_tag(), "comamnd is NULL");
			break;
		}

		if (type == temp_cmd->type) {
			SLOG(LOG_DEBUG, vc_info_tag(), "[%dth] type(%d) format(%d) domain(%d) cmd(%s) param(%s)",
				 i, temp_cmd->type, temp_cmd->format, temp_cmd->domain, temp_cmd->command, temp_cmd->parameter);

			/* Make new command node */
			cmd_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_COMMAND);

			/* ID */
			memset(temp, 0, 16);
			snprintf(temp, 16, "%d", i);

			tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_ID);
			xmlNodeSetContent(tmp_node, (const xmlChar *)temp);
			xmlAddChild(cmd_node, tmp_node);

			/* PID */
			memset(temp, 0, 16);
			snprintf(temp, 16, "%d", getpid());

			tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_PID);
			xmlNodeSetContent(tmp_node, (const xmlChar *)temp);
			xmlAddChild(cmd_node, tmp_node);

			/* TYPE */
			memset(temp, 0, 16);
			snprintf(temp, 16, "%d", temp_cmd->type);

			tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_TYPE);
			xmlNodeSetContent(tmp_node, (const xmlChar *)temp);
			xmlAddChild(cmd_node, tmp_node);

			/* FORMAT */
			memset(temp, 0, 16);
			snprintf(temp, 16, "%d", temp_cmd->format);

			tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_FORMAT);
			xmlNodeSetContent(tmp_node, (const xmlChar *)temp);
			xmlAddChild(cmd_node, tmp_node);

			/* DOMAIN */
			memset(temp, 0, 16);
			snprintf(temp, 16, "%d", temp_cmd->domain);

			tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_DOMAIN);
			xmlNodeSetContent(tmp_node, (const xmlChar *)temp);
			xmlAddChild(cmd_node, tmp_node);

			/* COMMAND */
			tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_COMMAND_TEXT);
			xmlNodeSetContent(tmp_node, (const xmlChar *)temp_cmd->command);
			xmlAddChild(cmd_node, tmp_node);

			/* PARAMETER */
			tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_PARAMETER_TEXT);
			xmlNodeSetContent(tmp_node, (const xmlChar *)temp_cmd->parameter);
			xmlAddChild(cmd_node, tmp_node);

			xmlAddChild(root_node, cmd_node);

			selected_count++;
		} else {
			SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Command type(%d) is NOT valid : request type(%d)", temp_cmd->type, type);
		}
		iter = g_slist_next(iter);
	}

	if (0 < selected_count) {
		int ret = xmlSaveFormatFile(filepath, doc, 1);
		if (0 >= ret) {
			SLOG(LOG_DEBUG, vc_info_tag(), "[ERROR] Fail to save command file : %d, filepath(%s)", ret, filepath);
			free(filepath);
			return -1;
		}

		if (0 != __vc_info_parser_set_file_mode(filepath)) {
			SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Fail to set file mode - %s", filepath);
		} else {
			SLOG(LOG_DEBUG, vc_info_tag(), "[Success] Save command file");
		}
		free(filepath);
	} else {
		free(filepath);

		SLOG(LOG_DEBUG, vc_info_tag(), "No command");
		return -1;
	}


	return 0;
}

int vc_cmd_parser_delete_file(int pid, vc_cmd_type_e type)
{
	/* Check file */
	char* filepath = NULL;
	__vc_cmd_parser_make_filepath(pid, type, &filepath);

	if (NULL != filepath) {
		remove(filepath);
		free(filepath);
	}
	return 0;
}

int vc_cmd_parser_get_commands(int pid, vc_cmd_type_e type, GSList** cmd_list)
{
	/* Check file */
	char* filepath = NULL;
	__vc_cmd_parser_make_filepath(pid, type, &filepath);

	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *key;

	doc = xmlParseFile(filepath);
	if (doc == NULL) {
		SECURE_SLOG(LOG_WARN, vc_info_tag(), "[WARNING] Fail to parse file error : %s", filepath);
		return -1;
	}
	if (NULL != filepath)	free(filepath);

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_CMD_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] The wrong type, root node is NOT '%s'", VC_TAG_CMD_BASE_TAG);
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	GSList* temp_cmd_list = NULL;

	while (cur != NULL) {
		cur = cur->next;

		if (NULL == cur) {
			break;
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)"text")) {
			continue;
		}

		xmlNodePtr command_node = NULL;
		command_node = cur->xmlChildrenNode;
		command_node = command_node->next;

		vc_cmd_s* temp_cmd;
		temp_cmd = (vc_cmd_s*)calloc(1, sizeof(vc_cmd_s));

		if (NULL == temp_cmd) {
			SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Memory alloc error!!");
			return -1;
		}

		/* ID */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_ID)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "ID : %s", (char *)key);
				temp_cmd->index = atoi((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_ID);
				free(temp_cmd);
				break;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* PID */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_PID)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "PID : %s", (char *)key);
				temp_cmd->pid = atoi((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_PID);
				free(temp_cmd);
				break;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* Type */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_TYPE)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "Type : %s", (char *)key);
				temp_cmd->type = atoi((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_TYPE);
				free(temp_cmd);
				break;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* Format */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_FORMAT)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "Format : %s", (char *)key);
				temp_cmd->format = atoi((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_FORMAT);
				free(temp_cmd);
				break;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* Domain */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_DOMAIN)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "Domain : %s", (char *)key);
				temp_cmd->domain = atoi((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_DOMAIN);
				free(temp_cmd);
				break;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* Command */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_COMMAND_TEXT)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "Command : %s", (char *)key);
				if (0 < xmlStrlen(key)) {
					temp_cmd->command = strdup((char*)key);
				} else {
					temp_cmd->command = NULL;
				}
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_COMMAND_TEXT);
				free(temp_cmd);
				break;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* Parameter */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_PARAMETER_TEXT)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "Parameter : %s", (char *)key);
				if (0 < xmlStrlen(key)) {
					temp_cmd->parameter = strdup((char*)key);
				} else {
					temp_cmd->parameter = NULL;
				}
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_PARAMETER_TEXT);
				if (NULL != temp_cmd->command)	free(temp_cmd->command);
				free(temp_cmd);
				break;
			}
		}

		if (type == temp_cmd->type) {
			temp_cmd_list = g_slist_append(temp_cmd_list, temp_cmd);
		} else {
			SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Command type(%d) is NOT valid : request type(%d)", temp_cmd->type, type);
			vc_cmd_destroy((vc_cmd_h)temp_cmd);
		}
	}

	xmlFreeDoc(doc);

	*cmd_list = temp_cmd_list;

	__vc_cmd_parser_print_commands(temp_cmd_list);

	return 0;
}

int vc_cmd_parser_append_commands(int pid, vc_cmd_type_e type, vc_cmd_list_h vc_cmd_list)
{
	/* Check file */
	char* filepath = NULL;
	__vc_cmd_parser_make_filepath(pid, type, &filepath);

	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *key;

	doc = xmlParseFile(filepath);
	if (doc == NULL) {
		SECURE_SLOG(LOG_WARN, vc_info_tag(), "[WARNING] Fail to parse file error : %s", filepath);
		return -1;
	}
	if (NULL != filepath)	free(filepath);

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_CMD_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] The wrong type, root node is NOT '%s'", VC_TAG_CMD_BASE_TAG);
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	vc_cmd_h temp_command = NULL;

	while (cur != NULL) {
		cur = cur->next;

		if (NULL == cur) {
			break;
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)"text")) {
			continue;
		}

		xmlNodePtr command_node = NULL;
		command_node = cur->xmlChildrenNode;
		command_node = command_node->next;

		if (0 != vc_cmd_create(&temp_command)) {
			SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Fail to create command!!");
			return -1;
		}

		/* ID */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_ID)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				/*SLOG(LOG_DEBUG, vc_info_tag(), "ID : %s", (char *)key); */
				vc_cmd_set_id(temp_command, atoi((char*)key));
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_ID);
				vc_cmd_destroy(temp_command);
				break;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* PID */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_PID)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				/*SLOG(LOG_DEBUG, vc_info_tag(), "PID : %s", (char *)key); */
				vc_cmd_set_pid(temp_command, atoi((char*)key));
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_PID);
				vc_cmd_destroy(temp_command);
				break;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* TYPE */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_TYPE)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				/*SLOG(LOG_DEBUG, vc_info_tag(), "Type : %s", (char *)key); */
				vc_cmd_set_type(temp_command, atoi((char*)key));
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_TYPE);
				vc_cmd_destroy(temp_command);
				break;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* FORMAT */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_FORMAT)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				/*SLOG(LOG_DEBUG, vc_info_tag(), "Format : %s", (char *)key); */
				vc_cmd_set_format(temp_command, atoi((char*)key));
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_FORMAT);
				vc_cmd_destroy(temp_command);
				break;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* DOMAIN */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_DOMAIN)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				/*SLOG(LOG_DEBUG, vc_info_tag(), "Domain : %s", (char *)key); */
				vc_cmd_set_domain(temp_command, atoi((char*)key));
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_DOMAIN);
				vc_cmd_destroy(temp_command);
				break;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* Command */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_COMMAND_TEXT)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				/*SLOG(LOG_DEBUG, vc_info_tag(), "Command : %s, size : %d", (char *)key, strlen(key)); */
				vc_cmd_set_command(temp_command, (char*)key);

				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_COMMAND_TEXT);
				vc_cmd_destroy(temp_command);
				break;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* Parameter */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_PARAMETER_TEXT)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				/*SLOG(LOG_DEBUG, vc_info_tag(), "Parameter : %s , size : %d", (char *)key, strlen(key)); */
				/*vc_cmd_set_parameter(temp_command, (char*)key); */
				vc_cmd_set_unfixed_command(temp_command, (char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_PARAMETER_TEXT);
				vc_cmd_destroy(temp_command);
				break;
			}
		}

		if (0 != vc_cmd_list_add(vc_cmd_list, temp_command)) {
			SLOG(LOG_DEBUG, vc_info_tag(), "Fail to add command to list");
			vc_cmd_destroy(temp_command);
			vc_cmd_list_destroy(vc_cmd_list, true);
			return -1;
		}
	}

	xmlFreeDoc(doc);

	vc_cmd_print_list(vc_cmd_list);

	return 0;
}

int vc_info_parser_get_demandable_clients(GSList** client_list)
{
	/* Check file */
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *key;

	doc = xmlParseFile(VC_RUNTIME_INFO_DEMANDABLE_LIST);
	if (doc == NULL) {
		SECURE_SLOG(LOG_WARN, vc_info_tag(), "[WARNING] Fail to parse file error : %s", VC_RUNTIME_INFO_FOREGROUND);
		return -1;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_DEMANDABLE_CLIENT_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] The wrong type, root node is NOT '%s'", VC_TAG_DEMANDABLE_CLIENT_BASE_TAG);
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	GSList* temp_client_list = NULL;

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_DEMANDABLE_CLIENT_APPID)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "App id : %s", (char *)key);

				vc_demandable_client_s* temp_client;
				temp_client = (vc_demandable_client_s*)calloc(1, sizeof(vc_demandable_client_s));

				if (NULL == temp_client) {
					SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Memory alloc error!!");
					return -1;
				}

				if (0 < xmlStrlen(key)) {
					temp_client->appid = strdup((char*)key);
				} else {
					/* NULL for appid is available */
					temp_client->appid = NULL;
				}
				xmlFree(key);

				temp_client_list = g_slist_append(temp_client_list, temp_client);
			} else {
				SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] enable is NULL");
			}
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);

	*client_list = temp_client_list;

	remove(VC_RUNTIME_INFO_DEMANDABLE_LIST);

	return 0;
}

int vc_info_parser_set_demandable_client(const char* filepath)
{
	if (NULL == filepath) {
		remove(VC_RUNTIME_INFO_DEMANDABLE_LIST);
		return 0;
	}

	/* Check file */
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *key;

	doc = xmlParseFile(filepath);
	if (doc == NULL) {
		SECURE_SLOG(LOG_WARN, vc_info_tag(), "[WARNING] Fail to parse file error : %s", filepath);
		return -1;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_DEMANDABLE_CLIENT_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] The wrong type, root node is NOT '%s'", VC_TAG_DEMANDABLE_CLIENT_BASE_TAG);
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	while (cur != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_DEMANDABLE_CLIENT_APPID)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "App id : %s", (char *)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] enable is NULL");
				xmlFreeDoc(doc);
				return -1;
			}
		}
		cur = cur->next;
	}

	int ret = xmlSaveFormatFile(VC_RUNTIME_INFO_DEMANDABLE_LIST, doc, 1);
	SLOG(LOG_DEBUG, vc_info_tag(), "Save demandable file info : %d", ret);

	return 0;
}

int vc_info_parser_set_result(const char* result_text, int event, const char* msg, vc_cmd_list_h vc_cmd_list, bool exclusive)
{
	char filepath[256] = {'\0',};

	if (false == exclusive) {
		snprintf(filepath, 256, "%s", VC_RUNTIME_INFO_RESULT);
	} else {
		snprintf(filepath, 256, "%s", VC_RUNTIME_INFO_EX_RESULT);
	}

	SLOG(LOG_DEBUG, vc_info_tag(), "Result file path : %s", filepath);

	/* Check file */
	remove(filepath);

	xmlDocPtr doc;
	xmlNodePtr root_node;
	xmlNodePtr cmd_node;
	xmlNodePtr tmp_node;
	char temp[16];
	int ret = 0;

	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->encoding = (const xmlChar*)"utf-8";
	doc->charset = 1;

	root_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_RESULT_BASE_TAG);
	xmlDocSetRootElement(doc, root_node);

	tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_RESULT_TEXT);
	xmlNodeSetContent(tmp_node, (const xmlChar *)result_text);
	xmlAddChild(root_node, tmp_node);

	memset(temp, 0, 16);
	snprintf(temp, 16, "%d", event);

	tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_RESULT_EVENT);
	xmlNodeSetContent(tmp_node, (const xmlChar *)temp);
	xmlAddChild(root_node, tmp_node);

	tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_RESULT_MESSAGE);
	xmlNodeSetContent(tmp_node, (const xmlChar *)msg);
	xmlAddChild(root_node, tmp_node);

	/* Make client list node */
	vc_cmd_h vc_command = NULL;

	vc_cmd_list_first(vc_cmd_list);

	while (VC_ERROR_ITERATION_END != ret) {
		if (0 != vc_cmd_list_get_current(vc_cmd_list, &vc_command)) {
			LOG(LOG_ERROR, vc_info_tag(), "[ERROR] Fail to get command");
			break;
		}

		if (NULL == vc_command) {
			break;
		}

		vc_cmd_s* temp_cmd = NULL;
		temp_cmd = (vc_cmd_s*)vc_command;

		/* Make new command node */
		cmd_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_COMMAND);

		SLOG(LOG_DEBUG, vc_info_tag(), "[Result info] ID(%d) PID(%d) type(%d) format(%d) domain(%d) cmd(%s) param(%s)",
			 temp_cmd->id, temp_cmd->pid, temp_cmd->type, temp_cmd->format, temp_cmd->domain, temp_cmd->command, temp_cmd->parameter);


		/* ID */
		memset(temp, 0, 16);
		snprintf(temp, 16, "%d", temp_cmd->id);

		tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_ID);
		xmlNodeSetContent(tmp_node, (const xmlChar *)temp);
		xmlAddChild(cmd_node, tmp_node);

		/* PID */
		memset(temp, 0, 16);
		snprintf(temp, 16, "%d", temp_cmd->pid);

		tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_PID);
		xmlNodeSetContent(tmp_node, (const xmlChar *)temp);
		xmlAddChild(cmd_node, tmp_node);

		/* TYPE */
		memset(temp, 0, 16);
		snprintf(temp, 16, "%d", temp_cmd->type);

		tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_TYPE);
		xmlNodeSetContent(tmp_node, (const xmlChar *)temp);
		xmlAddChild(cmd_node, tmp_node);

		/* FORMAT */
		memset(temp, 0, 16);
		snprintf(temp, 16, "%d", temp_cmd->format);

		tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_FORMAT);
		xmlNodeSetContent(tmp_node, (const xmlChar *)temp);
		xmlAddChild(cmd_node, tmp_node);

		/* DOMAIN */
		memset(temp, 0, 16);
		snprintf(temp, 16, "%d", temp_cmd->domain);

		tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_DOMAIN);
		xmlNodeSetContent(tmp_node, (const xmlChar *)temp);
		xmlAddChild(cmd_node, tmp_node);

		/* COMMAND */
		tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_COMMAND_TEXT);
		xmlNodeSetContent(tmp_node, (const xmlChar *)temp_cmd->command);
		xmlAddChild(cmd_node, tmp_node);

		/* PARAMETER */
		tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CMD_PARAMETER_TEXT);
		xmlNodeSetContent(tmp_node, (const xmlChar *)temp_cmd->parameter);
		xmlAddChild(cmd_node, tmp_node);

		xmlAddChild(root_node, cmd_node);

		ret = vc_cmd_list_next(vc_cmd_list);
	}

	ret = xmlSaveFormatFile(filepath, doc, 1);
	if (0 >= ret) {
		SLOG(LOG_DEBUG, vc_info_tag(), "[ERROR] Fail to save result command file : %d", ret);
		return -1;
	}

	SLOG(LOG_DEBUG, vc_info_tag(), "[Success] Save result command file");

	return 0;
}

int vc_info_parser_get_result(char** result_text, int* event, char** result_message, int pid, vc_cmd_list_h vc_cmd_list, bool exclusive)
{
	if (NULL == result_text || NULL == event || NULL == vc_cmd_list) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Input parameter is NULL");
		return -1;
	}

	char filepath[256] = {'\0',};

	if (false == exclusive) {
		snprintf(filepath, 256, "%s", VC_RUNTIME_INFO_RESULT);
	} else {
		snprintf(filepath, 256, "%s", VC_RUNTIME_INFO_EX_RESULT);
	}

	SLOG(LOG_DEBUG, vc_info_tag(), "Result file path : %s", filepath);

	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *key;

	doc = xmlParseFile(filepath);
	if (doc == NULL) {
		SLOG(LOG_ERROR, vc_info_tag(), "[WARNING] Fail to parse file error : %s", filepath);
		return -1;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_RESULT_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] The wrong type, root node is NOT '%s'", VC_TAG_RESULT_BASE_TAG);
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->next;
	if (NULL == cur) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	/* Result text */
	if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_RESULT_TEXT)) {
		key = xmlNodeGetContent(cur);
		if (NULL != key) {
			SLOG(LOG_DEBUG, vc_info_tag(), "Result text : %s", (char *)key);
			*result_text = strdup((char*)key);
			xmlFree(key);
		} else {
			SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_RESULT_TEXT);
			return -1;
		}
	}

	cur = cur->next;
	cur = cur->next;
	if (NULL == cur) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	/* Result event */
	if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_RESULT_EVENT)) {
		key = xmlNodeGetContent(cur);
		if (NULL != key) {
			SLOG(LOG_DEBUG, vc_info_tag(), "Result event : %s", (char *)key);
			*event = atoi((char*)key);
			xmlFree(key);
		} else {
			SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_RESULT_EVENT);
			return -1;
		}
	}

	cur = cur->next;
	cur = cur->next;
	if (NULL == cur) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	/* Result Message */
	if (result_message != NULL) {
		if (0 == xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_RESULT_MESSAGE)) {
			key = xmlNodeGetContent(cur);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "Result message : %s", (char *)key);
				*result_message = strdup((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_RESULT_MESSAGE);
				return -1;
			}
		}
	}

	vc_cmd_h vc_command = NULL;

	while (cur != NULL) {

		cur = cur->next;
		if (NULL == cur) {
			break;
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)"text")) {
			continue;
		}

		SLOG(LOG_ERROR, vc_info_tag(), "111 : %s", cur->name);

		/* Check Command tag */
		if (0 != xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_CMD_COMMAND)) {
			break;
		}

		if (0 != vc_cmd_create(&vc_command)) {
			SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Fail to create command!!");
			return -1;
		}

		vc_cmd_s* temp_cmd = NULL;
		temp_cmd = (vc_cmd_s*)vc_command;

		if (NULL == temp_cmd) {
			SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Memory alloc error!!");
			return -1;
		}

		xmlNodePtr command_node = NULL;
		command_node = cur->xmlChildrenNode;
		command_node = command_node->next;


		SLOG(LOG_ERROR, vc_info_tag(), "222 : %s", command_node->name);

		/* ID */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_ID)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "ID : %s", (char *)key);
				temp_cmd->id = atoi((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_ID);
				free(temp_cmd);
				break;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* PID */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_PID)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "PID : %s", (char *)key);
				temp_cmd->pid = atoi((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_PID);
				free(temp_cmd);
				break;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* Type */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_TYPE)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "Type : %s", (char *)key);
				temp_cmd->type = atoi((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_TYPE);
				free(temp_cmd);
				return -1;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* Format */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_FORMAT)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "Format : %s", (char *)key);
				temp_cmd->format = atoi((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_FORMAT);
				free(temp_cmd);
				return -1;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* Domain */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_DOMAIN)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "Command domain : %s", (char *)key);
				temp_cmd->domain = atoi((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_DOMAIN);
				free(temp_cmd);
				return -1;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* Command */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_COMMAND_TEXT)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "Command text : %s, size : %d", (char *)key, xmlStrlen(key));
				if (0 < xmlStrlen(key)) {
					temp_cmd->command = strdup((char*)key);
				} else {
					temp_cmd->command = NULL;
				}
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_COMMAND_TEXT);
				free(temp_cmd);
				return -1;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* Parameter */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_PARAMETER_TEXT)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "Parameter text : %s , size : %d", (char *)key, xmlStrlen(key));
				if (0 < xmlStrlen(key)) {
					temp_cmd->parameter = strdup((char*)key);
				} else {
					temp_cmd->parameter = NULL;
				}
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_PARAMETER_TEXT);
				if (NULL != temp_cmd->command)	free(temp_cmd->command);
				free(temp_cmd);
				return -1;
			}
		}

		if (0 < pid && pid != temp_cmd->pid) {
			SLOG(LOG_DEBUG, vc_info_tag(), "Current command is NOT valid");
			vc_cmd_destroy(vc_command);
		} else {
			if (0 != vc_cmd_list_add(vc_cmd_list, vc_command)) {
				SLOG(LOG_DEBUG, vc_info_tag(), "Fail to add command to list");
				vc_cmd_destroy(vc_command);
				return -1;
			}
		}
	}

	xmlFreeDoc(doc);

	return 0;
}

int vc_info_parser_set_nlp_info(const char* nlp_info)
{
	if (NULL == nlp_info) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] nlp info is NULL");
		return -1;
	}

	remove(VC_RUNTIME_INFO_NLP_INFO);

	FILE* fp = NULL;
	int write_size = -1;

	fp = fopen(VC_RUNTIME_INFO_NLP_INFO, "w+");
	if (NULL == fp) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Fail to open file %s", VC_RUNTIME_INFO_NLP_INFO);
		return -1;
	}

	/* Write size */
	fprintf(fp, "size(%d)\n", (int)strlen(nlp_info));

	write_size = fwrite(nlp_info, 1, strlen(nlp_info), fp);
	fclose(fp);

	if (0 >= write_size) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Fail to write file");
		return -1;
	}

	if (0 != __vc_info_parser_set_file_mode(VC_RUNTIME_INFO_NLP_INFO)) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Fail to set file mode - %s", VC_RUNTIME_INFO_NLP_INFO);
	}

	SLOG(LOG_DEBUG, vc_info_tag(), "[SUCCESS] Write file (%s) size (%d)", VC_RUNTIME_INFO_NLP_INFO, strlen(nlp_info));

	return 0;
}

int vc_info_parser_get_nlp_info(char** nlp_info)
{
	if (NULL == nlp_info) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] nlp info is NULL");
		return -1;
	}

	FILE* fp = NULL;
	int readn = 0;

	fp = fopen(VC_RUNTIME_INFO_NLP_INFO, "r");
	if (NULL == fp) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Fail to open file %s", VC_RUNTIME_INFO_NLP_INFO);
		return -1;
	}

	int ret;
	ret = fscanf(fp, "size(%d)\n", &readn);
	if (ret <= 0) {
		SLOG(LOG_DEBUG, vc_info_tag(), "[ERROR] Fail to get buffer size");
		fclose(fp);
		return -1;
	}

	SLOG(LOG_DEBUG, vc_info_tag(), "[DEBUG] buffer size (%d)", readn);
    if (10000000 < readn || 0 > readn) {
        SLOG(LOG_DEBUG, vc_info_tag(), "[ERROR] Invalid buffer size");
        fclose(fp);
        return -1;
    }
    int tmp_readn = readn + 10;

	*nlp_info = (char*)calloc(tmp_readn, sizeof(char));
    if (NULL == *nlp_info) {
        SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Out of memory");
        fclose(fp);
        return -1;
    }

	readn = fread(*nlp_info, 1, readn, fp);
	fclose(fp);

	SLOG(LOG_DEBUG, vc_info_tag(), "[DEBUG] Read buffer (%d)", readn);

	/* remove(VC_RUNTIME_INFO_NLP_INFO); */

	return 0;
}


int vc_info_parser_unset_result(bool exclusive)
{
	if (false == exclusive) {
		remove(VC_RUNTIME_INFO_RESULT);
	} else {
		remove(VC_RUNTIME_INFO_EX_RESULT);
	}

	return 0;
}

int vc_info_parser_get_result_pid_list(GSList** pid_list)
{
	char filepath[256] = {'\0', };
	snprintf(filepath, 256, "%s", VC_RUNTIME_INFO_RESULT);

	SLOG(LOG_DEBUG, vc_info_tag(), "Result file path : %s", filepath);

	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *key;

	doc = xmlParseFile(filepath);
	if (doc == NULL) {
		SLOG(LOG_ERROR, vc_info_tag(), "[WARNING] Fail to parse file error : %s", filepath);
		return -1;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_RESULT_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] The wrong type, root node is NOT '%s'", VC_TAG_RESULT_BASE_TAG);
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	cur = cur->next;

	cur = cur->next;
	cur = cur->next;

	cur = cur->next;
	cur = cur->next;

	GSList* iter = NULL;
	vc_cmd_s* temp_cmd = NULL;
	vc_cmd_s* check_cmd = NULL;

	while (cur != NULL) {

		cur = cur->next;
		if (NULL == cur) {
			break;
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)"text")) {
			continue;
		}

		/* Check Command tag */
		if (0 != xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_CMD_COMMAND)) {
			break;
		}

		temp_cmd = (vc_cmd_s*)calloc(1, sizeof(vc_cmd_s));
		if (NULL == temp_cmd) {
			SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Fail to alloc command");
			break;
		}

		xmlNodePtr command_node = NULL;
		command_node = cur->xmlChildrenNode;
		command_node = command_node->next;

		/* ID */
		if (0 != xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_ID)) {
			SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_ID);
			free(temp_cmd);
			break;
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* PID */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_PID)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "PID : %s", (char *)key);
				temp_cmd->pid = atoi((char*)key);
				xmlFree(key);
			} else {
				SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_PID);
				free(temp_cmd);
				break;
			}
		}

		command_node = command_node->next;
		command_node = command_node->next;

		/* Type */
		if (0 == xmlStrcmp(command_node->name, (const xmlChar *)VC_TAG_CMD_TYPE)) {
			key = xmlNodeGetContent(command_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "Type : %s", (char *)key);
				temp_cmd->type = atoi((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_TYPE);
				free(temp_cmd);
				return -1;
			}
		}

		/* check pid in gslist */
		iter = g_slist_nth(*pid_list, 0);
		while (NULL != iter) {
			check_cmd = iter->data;

			if (NULL == check_cmd) {
				free(temp_cmd);
				temp_cmd = NULL;
				break;
			}

			if (check_cmd->pid == temp_cmd->pid && check_cmd->type == temp_cmd->type) {
				free(temp_cmd);
				temp_cmd = NULL;
				break;
			}
			iter = g_slist_next(iter);
		}

		if (NULL != temp_cmd) {
			/* add pid to gslist */
			*pid_list = g_slist_append(*pid_list, temp_cmd);
		}
	}

	xmlFreeDoc(doc);

	return 0;
}

int vc_info_parser_set_client_info(GSList* client_info_list)
{
	if (0 >= g_slist_length(client_info_list)) {
		SLOG(LOG_WARN, vc_info_tag(), "[WARNING] client list is empty");
		return 0;
	}

	/* Remove file */
	remove(VC_RUNTIME_INFO_CLIENT);

	xmlDocPtr doc;
	xmlNodePtr root_node;
	xmlNodePtr client_node;
	xmlNodePtr tmp_node;

	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->encoding = (const xmlChar*)"utf-8";
	doc->charset = 1;

	root_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CLIENT_BASE_TAG);
	xmlDocSetRootElement(doc, root_node);

	GSList *iter = NULL;
	vc_client_info_s *client = NULL;

	int i;
	int count = g_slist_length(client_info_list);
	iter = g_slist_nth(client_info_list, 0);

	SLOG(LOG_DEBUG, vc_info_tag(), "client count : %d", count);
	char temp[16] = {0, };

	for (i = 0; i < count; i++) {
		if (NULL == iter)
			break;

		client = iter->data;

		if (NULL != client) {
			SLOG(LOG_DEBUG, vc_info_tag(), "[%dth] pid(%d) fgcmd(%d) bgcmd(%d) excmd(%d)",
				 i, client->pid, client->fg_cmd, client->bg_cmd, client->exclusive_cmd);

			/* Make new client node */
			client_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CLIENT_CLIENT);

			memset(temp, 0, 16);
			snprintf(temp, 16, "%d", client->pid);

			tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CLIENT_PID);
			xmlNodeSetContent(tmp_node, (const xmlChar *)temp);
			xmlAddChild(client_node, tmp_node);

			memset(temp, 0, 16);
			snprintf(temp, 16, "%d", client->fg_cmd);

			tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CLIENT_FGCMD);
			xmlNodeSetContent(tmp_node, (const xmlChar *)temp);
			xmlAddChild(client_node, tmp_node);

			memset(temp, 0, 16);
			snprintf(temp, 16, "%d", client->bg_cmd);

			tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CLIENT_BGCMD);
			xmlNodeSetContent(tmp_node, (const xmlChar *)temp);
			xmlAddChild(client_node, tmp_node);

			memset(temp, 0, 16);
			snprintf(temp, 16, "%d", client->exclusive_cmd);

			tmp_node = xmlNewNode(NULL, (const xmlChar*)VC_TAG_CLIENT_EXCMD);
			xmlNodeSetContent(tmp_node, (const xmlChar *)temp);
			xmlAddChild(client_node, tmp_node);

			xmlAddChild(root_node, client_node);
		}
		iter = g_slist_next(iter);
	}

	int ret = xmlSaveFormatFile(VC_RUNTIME_INFO_CLIENT, doc, 1);
	/*xmlFreeDoc(doc); */
	if (0 >= ret) {
		SLOG(LOG_DEBUG, vc_info_tag(), "[ERROR] Fail to save client file : %d", ret);
		return -1;
	}

	SLOG(LOG_DEBUG, vc_info_tag(), "[Success] Save client file");

	return 0;
}

int vc_info_parser_get_client_info(GSList** client_info_list)
{
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlChar *key;

	doc = xmlParseFile(VC_RUNTIME_INFO_CLIENT);
	if (doc == NULL) {
		SLOG(LOG_WARN, vc_info_tag(), "[WARNING] Fail to parse file error : %s", VC_RUNTIME_INFO_CLIENT);
		return -1;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *)VC_TAG_CLIENT_BASE_TAG)) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] The wrong type, root node is NOT '%s'", VC_TAG_CLIENT_BASE_TAG);
		xmlFreeDoc(doc);
		return -1;
	}

	cur = cur->xmlChildrenNode;
	if (cur == NULL) {
		SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Empty document");
		xmlFreeDoc(doc);
		return -1;
	}

	GSList* temp_client_list = NULL;

	while (cur != NULL) {
		cur = cur->next;

		if (NULL == cur) {
			break;
		} else if (0 == xmlStrcmp(cur->name, (const xmlChar *)"text")) {
			continue;
		}

		xmlNodePtr client_node = NULL;
		client_node = cur->xmlChildrenNode;
		client_node = client_node->next;

		vc_client_info_s *client = NULL;
		client = (vc_client_info_s*)calloc(1, sizeof(vc_client_info_s));

		if (NULL == client) {
			SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] Memory alloc error!!");
			return -1;
		}

		/* PID */
		if (0 == xmlStrcmp(client_node->name, (const xmlChar *)VC_TAG_CLIENT_PID)) {
			key = xmlNodeGetContent(client_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "pid : %s", (char *)key);
				client->pid = atoi((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CLIENT_PID);
				free(client);
				break;
			}
		}

		client_node = client_node->next;
		client_node = client_node->next;

		/* Foreground command */
		if (0 == xmlStrcmp(client_node->name, (const xmlChar *)VC_TAG_CLIENT_FGCMD)) {
			key = xmlNodeGetContent(client_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "fgcmd : %s", (char *)key);
				client->fg_cmd = atoi((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CLIENT_FGCMD);
				free(client);
				break;
			}
		}

		client_node = client_node->next;
		client_node = client_node->next;

		/* Background command */
		if (0 == xmlStrcmp(client_node->name, (const xmlChar *)VC_TAG_CLIENT_BGCMD)) {
			key = xmlNodeGetContent(client_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "bgcmd : %s", (char *)key);
				client->bg_cmd = atoi((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CLIENT_BGCMD);
				free(client);
				break;
			}
		}

		client_node = client_node->next;
		client_node = client_node->next;

		/* Text */
		if (0 == xmlStrcmp(client_node->name, (const xmlChar *)VC_TAG_CMD_COMMAND_TEXT)) {
			key = xmlNodeGetContent(client_node);
			if (NULL != key) {
				SLOG(LOG_DEBUG, vc_info_tag(), "excmd : %s", (char *)key);
				client->exclusive_cmd = atoi((char*)key);
				xmlFree(key);
			} else {
				SECURE_SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] <%s> has no content", VC_TAG_CMD_COMMAND_TEXT);
				free(client);
				break;
			}
		}

		temp_client_list = g_slist_append(temp_client_list, client);
	}

	xmlFreeDoc(doc);

	*client_info_list = temp_client_list;

	return 0;
}

int __vc_cmd_parser_print_commands(GSList* cmd_list)
{
	int count = g_slist_length(cmd_list);
	int i ;
	GSList *iter = NULL;
	vc_cmd_s *temp_cmd;

	iter = g_slist_nth(cmd_list, 0);

	for (i = 0; i < count; i++) {
		if (NULL == iter)
			break;

		temp_cmd = iter->data;

		if (NULL == temp_cmd) {
			SLOG(LOG_ERROR, vc_info_tag(), "[ERROR] NULL data from command list");
			iter = g_slist_next(iter);
			continue;
		}

		SLOG(LOG_DEBUG, vc_info_tag(), "  [%d][%p] PID(%d) ID(%d) Type(%d) Format(%d) Domain(%d)  Command(%s) Param(%s)",
			 i, temp_cmd, temp_cmd->pid, temp_cmd->index, temp_cmd->type, temp_cmd->format, temp_cmd->domain,
			 temp_cmd->command, temp_cmd->parameter);

		iter = g_slist_next(iter);
	}

	return 0;
}
