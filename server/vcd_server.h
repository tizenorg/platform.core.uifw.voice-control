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


#ifndef __VCD_SERVER_H_
#define __VCD_SERVER_H_

#include "vcd_main.h"
#include "vcd_client_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
* Daemon functions
*/
int vcd_initialize();

void vcd_finalize();

Eina_Bool vcd_cleanup_client(void *data);

int vcd_server_get_service_state();


/*
* For manager
*/
int vcd_server_mgr_initialize(int pid);

int vcd_server_mgr_finalize(int pid);

int vcd_server_mgr_set_command(int pid);

int vcd_server_mgr_unset_command(int pid);

int vcd_server_mgr_set_demandable_client(int pid);

int vcd_server_mgr_set_audio_type(int pid, const char* audio_type);

int vcd_server_mgr_get_audio_type(int pid, char** audio_type);

int vcd_server_mgr_set_client_info(int pid);

int vcd_server_mgr_start(vcd_recognition_mode_e recognition_mode, bool exclusive_cmd, bool start_by_client);

int vcd_server_mgr_stop();

int vcd_server_mgr_cancel();

int vcd_server_mgr_result_select();

/*
* For client
*/
int vcd_server_initialize(int pid);

int vcd_server_finalize(int pid);

int vcd_server_set_command(int pid, vc_cmd_type_e cmd_type);

int vcd_server_unset_command(int pid, vc_cmd_type_e cmd_type);

int vcd_server_set_exclusive_command(int pid, bool value);

#if 0
int vcd_server_request_start(int pid, bool stop_by_silence);

int vcd_server_request_stop(int pid);

int vcd_server_request_cancel(int pid);
#endif

/*
* For widget
*/
int vcd_server_widget_initialize(int pid);

int vcd_server_widget_finalize(int pid);

int vcd_server_widget_start_recording(int pid, bool widget_command);

int vcd_server_widget_start(int pid, bool stop_by_silence);

int vcd_server_widget_stop(int pid);

int vcd_server_widget_cancel(int pid);


#ifdef __cplusplus
}
#endif

#endif /* __VCD_SERVER_H_ */
