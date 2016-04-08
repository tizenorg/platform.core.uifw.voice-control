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


#ifndef __VCD_CONFIG_H_
#define __VCD_CONFIG_H_

#include "vcd_main.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EXPORT_API
#define EXPORT_API
#endif // EXPORT_API


typedef void (*vcd_config_lang_changed_cb)(const char* language, void* user_data);

typedef void (*vcd_config_foreground_changed_cb)(int previous, int current, void* user_data);


EXPORT_API int vcd_config_initialize(vcd_config_lang_changed_cb lang_cb, vcd_config_foreground_changed_cb fore_cb, void* user_data);

EXPORT_API int vcd_config_finalize();

EXPORT_API int vcd_config_get_default_language(char** language);

EXPORT_API int vcd_config_set_service_state(vcd_state_e state);

EXPORT_API vcd_state_e vcd_config_get_service_state();

EXPORT_API int vcd_config_get_foreground(int* pid);

EXPORT_API int vcd_config_set_foreground(int pid, bool value);


#ifdef __cplusplus
}
#endif

#endif /* __VCD_CONFIG_H_ */
