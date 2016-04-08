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


#ifndef __VCD_RECORDER_H__
#define __VCD_RECORDER_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "voice_control_plugin_engine.h"

#ifndef EXPORT_API
#define EXPORT_API
#endif // EXPORT_API


typedef enum {
	VCD_RECORDER_STATE_READY,	/**< Recorder is ready to start */
	VCD_RECORDER_STATE_RECORDING,	/**< In the middle of recording */
} vcd_recorder_state_e;


typedef int (*vcd_recoder_audio_cb)(const void* data, const unsigned int length);

typedef void (*vcd_recorder_interrupt_cb)();


EXPORT_API int vcd_recorder_create(vcd_recoder_audio_cb audio_cb, vcd_recorder_interrupt_cb interrupt_cb);

EXPORT_API int vcd_recorder_destroy();

EXPORT_API int vcd_recorder_set(const char* audio_type, vcp_audio_type_e type, int rate, int channel);

EXPORT_API int vcd_recorder_get(char** audio_type);

EXPORT_API int vcd_recorder_start();

EXPORT_API int vcd_recorder_read();

EXPORT_API int vcd_recorder_stop();

EXPORT_API int vcd_recorder_get_state();


#ifdef __cplusplus
}
#endif

#endif	/* __VCD_RECORDER_H__ */

