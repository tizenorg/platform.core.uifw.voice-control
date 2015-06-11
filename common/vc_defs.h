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


#ifndef __VC_DEFS_H__
#define __VC_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************************
* Definition for Dbus
*******************************************************************************************/

#define VC_CLIENT_SERVICE_NAME         "com.samsung.voice.vcclient"
#define VC_CLIENT_SERVICE_OBJECT_PATH  "/com/samsung/voice/vcclient"
#define VC_CLIENT_SERVICE_INTERFACE    "com.samsung.voice.vcclient"

#define VC_WIDGET_SERVICE_NAME        "com.samsung.voice.vcwidget"
#define VC_WIDGET_SERVICE_OBJECT_PATH "/com/samsung/voice/vcwidget"
#define VC_WIDGET_SERVICE_INTERFACE   "com.samsung.voice.vcwidget"

#define VC_MANAGER_SERVICE_NAME        "com.samsung.voice.vcmanager"
#define VC_MANAGER_SERVICE_OBJECT_PATH "/com/samsung/voice/vcmanager"
#define VC_MANAGER_SERVICE_INTERFACE   "com.samsung.voice.vcmanager"

#define VC_SERVER_SERVICE_NAME         "service.connect.vcserver"
#define VC_SERVER_SERVICE_OBJECT_PATH  "/com/samsung/voice/vcserver"
#define VC_SERVER_SERVICE_INTERFACE    "com.samsung.voice.vcserver"


/******************************************************************************************
* Message Definition for all 
*******************************************************************************************/

#define VC_METHOD_HELLO			"vc_method_hello"

/******************************************************************************************
* Message Definition for Client
*******************************************************************************************/

#define VC_METHOD_INITIALIZE		"vc_method_initialize"
#define VC_METHOD_FINALIZE		"vc_method_finalilze"

#define VC_METHOD_SET_EXCLUSIVE_CMD	"vc_method_set_exclusive_cmd"
#define VC_METHOD_SET_COMMAND		"vc_method_set_command"
#define VC_METHOD_UNSET_COMMAND		"vc_method_unset_command"

#define VCD_METHOD_RESULT		"vcd_method_result"
#define VCD_METHOD_ERROR		"vcd_method_error"
#define VCD_METHOD_HELLO		"vcd_method_hello"

/* Authority */
#if 0
#define VC_METHOD_OBTAIN_AUTHORITY	"vc_method_obtain_authority"
#define VC_METHOD_RETURN_AUTHORITY	"vc_method_return_authority"

#define VC_METHOD_REQUEST_START		"vc_method_request_start"
#define VC_METHOD_REQUEST_STOP		"vc_method_request_stop"
#define VC_METHOD_REQUEST_CANCEL	"vc_method_request_cancel"
#endif

#define VC_METHOD_AUTH_ENABLE		"vc_method_auth_enable"
#define VC_METHOD_AUTH_DISABLE		"vc_method_auth_disable"

#define VC_METHOD_AUTH_START		"vc_method_auth_start"
#define VC_METHOD_AUTH_STOP		"vc_method_auth_stop"
#define VC_METHOD_AUTH_CANCEL		"vc_method_auth_cancel"

/******************************************************************************************
* Message Definition for widget
*******************************************************************************************/

#define VC_WIDGET_METHOD_INITIALIZE		"vc_widget_method_initialize"
#define VC_WIDGET_METHOD_FINALIZE		"vc_widget_method_finalilze"

#define VC_WIDGET_METHOD_START_RECORDING	"vc_widget_method_start_recording"

#define VC_WIDGET_METHOD_START			"vc_widget_method_start"
#define VC_WIDGET_METHOD_STOP			"vc_widget_method_stop"
#define VC_WIDGET_METHOD_CANCEL			"vc_widget_method_cancel"

#define VCD_WIDGET_METHOD_RESULT		"vcd_widget_method_result"
#define VCD_WIDGET_METHOD_ERROR			"vcd_widget_method_error"
#define VCD_WIDGET_METHOD_HELLO			"vcd_widget_method_hello"
#define VCD_WIDGET_METHOD_SHOW_TOOLTIP		"vcd_widget_method_show_tooltip"


/******************************************************************************************
* Message Definition for manager
*******************************************************************************************/

#define VC_MANAGER_METHOD_INITIALIZE		"vc_manager_method_initialize"
#define VC_MANAGER_METHOD_FINALIZE		"vc_manager_method_finalilze"

#define VC_MANAGER_METHOD_SET_COMMAND		"vc_manager_method_set_command"
#define VC_MANAGER_METHOD_UNSET_COMMAND		"vc_manager_method_unset_command"
#define VC_MANAGER_METHOD_SET_DEMANDABLE	"vc_manager_method_set_demandable_client"
#define VC_MANAGER_METHOD_SET_AUDIO_TYPE	"vc_manager_method_set_audio_type"
#define VC_MANAGER_METHOD_GET_AUDIO_TYPE	"vc_manager_method_get_audio_type"
#define VC_MANAGER_METHOD_SET_CLIENT_INFO	"vc_manager_method_set_client_info"

#define VC_MANAGER_METHOD_START			"vc_manager_method_request_start"
#define VC_MANAGER_METHOD_STOP			"vc_manager_method_request_stop"
#define VC_MANAGER_METHOD_CANCEL		"vc_manager_method_request_cancel"
#define VC_MANAGER_METHOD_RESULT_SELECTION	"vc_manager_method_result_selection"

#define VCD_MANAGER_METHOD_HELLO		"vcd_manager_method_hello"
#define VCD_MANAGER_METHOD_SPEECH_DETECTED	"vcd_manager_method_speech_detected"
#define VCD_MANAGER_METHOD_ALL_RESULT		"vcd_manager_method_all_result"
#define VCD_MANAGER_METHOD_RESULT		"vcd_manager_method_result"

#define VCD_MANAGER_METHOD_ERROR		"vcd_manager_method_error"


/******************************************************************************************
* Defines for configuration
*******************************************************************************************/

#define VC_DAEMON_PATH			"/usr/bin/vc-daemon"

#define VC_CONFIG_DEFAULT		"/usr/lib/voice/vc/1.0/vc-config.xml"

#define VC_CONFIG			"/opt/home/app/.voice/vc-config.xml"

#define VC_DEFAULT_ENGINE_INFO		"/usr/lib/voice/vc/1.0/engine-info"

#define VC_NO_FOREGROUND_PID		0

#define VC_BASE_LANGUAGE		"en_US"

#define VC_RETRY_COUNT		5


#define VC_RUNTIME_INFO_ROOT		"/opt/home/app/.voice/vc"

#define VC_RUNTIME_INFO_AUDIO_VOLUME	VC_RUNTIME_INFO_ROOT"/vc_vol"

#define VC_RUNTIME_INFO_FOREGROUND	VC_RUNTIME_INFO_ROOT"/vc-info-foreground.xml"

#define VC_RUNTIME_INFO_SERVICE_STATE	VC_RUNTIME_INFO_ROOT"/vc-info-state.xml"

#define VC_RUNTIME_INFO_DEMANDABLE_LIST	VC_RUNTIME_INFO_ROOT"/vc-demandable-client.xml"

#define VC_RUNTIME_INFO_RESULT		VC_RUNTIME_INFO_ROOT"/vc-result.xml"

#define VC_RUNTIME_INFO_EX_RESULT	VC_RUNTIME_INFO_ROOT"/vc-ex-result.xml"

#define VC_RUNTIME_INFO_CLIENT		VC_RUNTIME_INFO_ROOT"/vc-client-info.xml"


#ifdef __cplusplus
}
#endif

#endif /* __VC_DEFS_H__ */
