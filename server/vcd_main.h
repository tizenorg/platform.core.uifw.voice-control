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


#ifndef __VCD_MAIN_H_
#define __VCD_MAIN_H_

#include <Ecore.h>
#include <dlog.h>
#include <errno.h>
#include <glib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <tizen.h>
#include <unistd.h>

#include "vc_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
* Daemon Define
*/

#define TAG_VCD "vcd"

/* for debug message */
/* #define RECORDER_DEBUG */
#define CLIENT_DATA_DEBUG
/* #define COMMAND_DATA_DEBUG */

typedef enum {
	VCD_ERROR_NONE			= TIZEN_ERROR_NONE,			/**< Successful */
	VCD_ERROR_OUT_OF_MEMORY		= TIZEN_ERROR_OUT_OF_MEMORY,		/**< Out of Memory */
	VCD_ERROR_IO_ERROR		= TIZEN_ERROR_IO_ERROR,			/**< I/O error */
	VCD_ERROR_INVALID_PARAMETER	= TIZEN_ERROR_INVALID_PARAMETER,	/**< Invalid parameter */
	VCD_ERROR_TIMED_OUT		= TIZEN_ERROR_TIMED_OUT,		/**< No answer from service */
	VCD_ERROR_RECORDER_BUSY		= TIZEN_ERROR_RESOURCE_BUSY,		/**< Busy recorder */
	VCD_ERROR_PERMISSION_DENIED	= TIZEN_ERROR_PERMISSION_DENIED,	/**< Permission denied */
	VCD_ERROR_NOT_SUPPORTED		= TIZEN_ERROR_NOT_SUPPORTED,		/**< VC NOT supported */
	VCD_ERROR_INVALID_STATE		= TIZEN_ERROR_VOICE_CONTROL | 0x011,	/**< Invalid state */
	VCD_ERROR_INVALID_LANGUAGE	= TIZEN_ERROR_VOICE_CONTROL | 0x012,	/**< Invalid language */
	VCD_ERROR_ENGINE_NOT_FOUND	= TIZEN_ERROR_VOICE_CONTROL | 0x013,	/**< No available engine */
	VCD_ERROR_OPERATION_FAILED	= TIZEN_ERROR_VOICE_CONTROL | 0x014,	/**< Operation failed */
	VCD_ERROR_OPERATION_REJECTED	= TIZEN_ERROR_VOICE_CONTROL | 0x015,	/**< Operation rejected */
	VCD_ERROR_SERVICE_RESET		= TIZEN_ERROR_VOICE_CONTROL | 0x018	/**< Daemon Service reset */
} vcd_error_e;

typedef enum {
	VCD_STATE_NONE = 0,
	VCD_STATE_READY = 1,
	VCD_STATE_RECORDING = 2,
	VCD_STATE_PROCESSING = 3
} vcd_state_e;


#ifdef __cplusplus
}
#endif

#endif	/* __VCD_MAIN_H_ */
