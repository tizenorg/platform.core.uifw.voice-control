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


#ifndef __VCD_ENGINE_AGENT_H_
#define __VCD_ENGINE_AGENT_H_

#include "vcd_main.h"
#include "voice_control_plugin_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
* Constants & Structures	
*/

#define	ENGINE_PATH_SIZE 256

typedef void (*result_callback)(vcp_result_event_e event, int* result_id, int count, 
				const char* all_result, const char* non_fixed_result, const char* msg, void *user_data);

typedef void (*silence_dectection_callback)(void *user_data);



/** Init engine agent */
int vcd_engine_agent_init(result_callback result_cb);

/** Release engine agent */
int vcd_engine_agent_release();

bool vcd_engine_is_available_engine();

/** Set current engine */
int vcd_engine_agent_initialize_current_engine();

/** load current engine */
int vcd_engine_agent_load_current_engine();

/** Unload current engine */
int vcd_engine_agent_unload_current_engine();

/** test for language list */
int vcd_print_enginelist();


int vcd_engine_get_audio_format(const char* audio_id, vcp_audio_type_e* types, int* rate, int* channels);

int vcd_engine_supported_langs(GList** lang_list);

int vcd_engine_get_current_language(char** lang);

int vcd_engine_set_current_language(const char* language);

int vcd_engine_set_commands();

/* normal recognition */
int vcd_engine_recognize_start(bool silence);

int vcd_engine_recognize_audio(const void* data, unsigned int length, vcp_speech_detect_e* speech_detected);

int vcd_engine_recognize_stop();

int vcd_engine_recognize_cancel();



#ifdef __cplusplus
}
#endif

#endif /* __VCD_ENGINE_AGENT_H_ */


