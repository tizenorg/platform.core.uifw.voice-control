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


#ifndef __VC_CONFIG_PARSER_H_
#define __VC_CONFIG_PARSER_H_

#include <glib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char*	name;
	char*	uuid;
	GSList*	languages;
	bool	non_fixed_support;
} vc_engine_info_s;

typedef struct {
	char*	engine_id;
	bool	auto_lang;
	char*	language;
	bool	enabled;
} vc_config_s;


/* Get engine information */
int vc_parser_get_engine_info(const char* path, vc_engine_info_s** engine_info);

int vc_parser_free_engine_info(vc_engine_info_s* engine_info);


int vc_parser_load_config(vc_config_s** config_info);

int vc_parser_unload_config(vc_config_s* config_info);

int vc_parser_set_engine(const char* engine_id);

int vc_parser_set_auto_lang(bool value);

int vc_parser_set_language(const char* language);

int vc_parser_set_enabled(bool value);

int vc_parser_find_config_changed(int* auto_lang, char** language, int* enabled);


/* Set / Get service state */
int vc_parser_init_service_state();

int vc_parser_set_service_state(int state);

int vc_parser_get_service_state(int* state);


/* Set / Get foreground info */
int vc_parser_set_foreground(int pid, bool value);

int vc_parser_get_foreground(int* pid);


#ifdef __cplusplus
}
#endif

#endif /* __VC_CONFIG_PARSER_H_ */
