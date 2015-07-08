/*
 * Copyright (c) 2011-2014 Samsung Electronics Co., Ltd All Rights Reserved
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


#ifndef __TIZEN_UIX_VOICE_CONTROL_DOC_H__
#define __TIZEN_UIX_VOICE_CONTROL_DOC_H__

/**
 * @defgroup CAPI_UIX_VOICE_CONTROL_MODULE Voice control
 * @ingroup CAPI_UIX_FRAMEWORK
 * @brief The @ref CAPI_UIX_VOICE_CONTROL_MODULE API provides functions for registering command and getting notification when registered command is recognized.
 * 
 * @section CAPI_UIX_VOICE_CONTROL_MODULE_HEADER Required Header
 *   \#include <voice_control.h><br>
 * 
 * @section CAPI_UIX_VOICE_CONTROL_MODULE_OVERVIEW Overview
 * A main function of Voice Control API register command and gets notification for recognition result.
 * Applications can add their own commands and be provided result when their command is recognized by user voice input.
 * 
 * To use of Voice Control, use the following steps:
 * 1. Initialize <br>
 * 2. Register callback functions for notifications <br> 
 * 3. Connect to voice control service asynchronously. The state should be changed to Ready <br>
 * 4. Make command list as the following step <br>
 * 4-1. Create command list handle <br>
 * 4-2. Create command handle <br>
 * 4-3. Set command and type for command handle <br>
 * 4-4. Add command handle to command list <br>
 * Step 4 is called repeatedly for each command which an application wants <br>
 * 5. Set command list for recognition <br>
 * 6. If an application wants to finish voice control,<br>
 * 6-1. Destroy command and command list handle <br>
 * 6-2. Deinitialize <br>
 *
 * An application can obtain command handle from command list, and also get information from handle. 
 *
 *
 * The Voice Control API also notifies you (by callback mechanism) when the states of client and service are changed, 
 * command is recognized, current language is changed or error occurred.
 * An application should register callback functions: vc_state_changed_cb(), vc_service_state_changed_cb(), vc_result_cb(), 
 * vc_current_language_changed_cb(), vc_error_cb().
 *
 * @section CAPI_UIX_VOICE_CONTROL_MODULE_STATE_DIAGRAM State Diagram
 * The following diagram shows the life cycle and the states of the Voice Control.
 *
 * @image html capi_uix_voice_control_state_diagram.png "<State diagram>    "
 * The following diagram shows the states of Voice Control service.
 * @image html capi_uix_voice_control_service_state_diagram.png "<Service state diagram>"
 *
 * @section CAPI_UIX_VOICE_CONTROL_MODULE_STATE_TRANSITIONS State Transitions
 *
 * <table>
 * <tr>
 * <th>FUNCTION</th>
 * <th>PRE-STATE</th>
 * <th>POST-STATE</th>
 * <th>SYNC TYPE</th>
 * </tr>
 * <tr>
 * <td>vc_initialize()</td>
 * <td>None</td>
 * <td>Initialized</td>
 * <td>SYNC</td>
 * </tr>
 * <tr>
 * <td>vc_deinitialize()</td>
 * <td>Initialized</td>
 * <td>None</td>
 * <td>SYNC</td>
 * </tr>
 * <tr>
 * <td>vc_prepare()</td>
 * <td>Initialized</td>
 * <td>Ready</td>
 * <td>ASYNC</td>
 * </tr>
 * <tr>
 * <td>vc_unprepare()</td>
 * <td>Ready</td>
 * <td>Initialized</td>
 * <td>SYNC</td>
 * </tr>

 * </table>
 *
 * @section CAPI_UIX_VOICE_CONTROL_MODULE_STATE_DEPENDENT_FUNCTION_CALLS State Dependent Function Calls
 * The following table shows state-dependent function calls.
 * It is forbidden to call functions listed below in wrong states.
 * Violation of this rule may result in an unpredictable behavior.
 * 
 * <table>
 * <tr>
 * <th>FUNCTION</th>
 * <th>VALID STATES</th>
 * <th>DESCRIPTION</th>
 * </tr>
 * <tr>
 * <td>vc_initialize()</td>
 * <td>None</td>
 * <td>All functions must be called after vc_initialize()</td>
 * </tr>
 * <tr>
 * <td>vc_deinitialize()</td>
 * <td>Initialized, Ready</td>
 * <td>This function should be called when an application want to finalize voice control using</td>
 * </tr>
 * <tr>
 * <td>vc_prepare()</td>
 * <td>Initialized</td>
 * <td>This function works asynchronously. If service start is failed, application gets the error callback.</td>
 * </tr>
 * <tr>
 * <td>vc_unprepare()</td>
 * <td>Ready</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>vc_foreach_supported_languages()</td>
 * <td>Initialized, Ready</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>vc_get_current_language()</td>
 * <td>Initialized, Ready</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>vc_get_state()</td>
 * <td>Initialized, Ready</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>vc_get_service_state()</td>
 * <td>Initialized, Ready</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>vc_set_command_list()</td>
 * <td>Ready</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>vc_unset_command_list()</td>
 * <td>Ready</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>
 * vc_set_result_cb()<br>
 * vc_unset_result_cb()<br>
 * vc_set_state_changed_cb()<br>
 * vc_unset_state_changed_cb()<br>
 * vc_set_service_state_changed_cb()<br>
 * vc_unset_service_state_changed_cb()<br>
 * vc_set_current_language_changed_cb()<br>
 * vc_unset_current_language_changed_cb()<br>
 * vc_set_error_cb()<br>
 * vc_unset_error_cb()</td>
 * <td>Initialized</td>
 * <td> All callback function should be registered in Initialized state </td>
 * </tr>
 * </table>
 * 
 * @section CAPI_UIX_STT_MODULE_FEATURE Related Features
 * This API is related with the following features:<br>
 *  - http://tizen.org/feature/microphone<br>
 *
 * It is recommended to design feature related codes in your application for reliability.<br>
 * You can check if a device supports the related features for this API by using @ref CAPI_SYSTEM_SYSTEM_INFO_MODULE, thereby controlling the procedure of your application.<br>
 * To ensure your application is only running on the device with specific features, please define the features in your manifest file using the manifest editor in the SDK.<br>
 * More details on featuring your application can be found from <a href="../org.tizen.mobile.native.appprogramming/html/ide_sdk_tools/feature_element.htm"><b>Feature Element</b>.</a>
 *
 */

#endif /* __TIZEN_UIX_VOICE_CONTROL_DOC_H__ */

