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

 
#ifndef __VC_MAIN_H_
#define __VC_MAIN_H_

#include <dbus/dbus.h>
#include <dlog.h>
#include <Ecore.h>
#include <Ecore_X.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "vc_defs.h"
#include "voice_control_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_VCC		"vcc"		/* Normal client log tag */
#define TAG_VCW		"vcw"		/* Widget client log tag */
#define TAG_VCM		"vcm"		/* Manager client log tag */
#define TAG_VCS		"vcsetting"	/* Setting client log tag */
#define TAG_VCINFO	"vcinfo"	/* info lib log tag */
#define TAG_VCCONFIG	"vcinfo"	/* config lib log tag */
#define TAG_VCCMD	"vccmd"		/* Command log tag */

/** 
* @brief A structure of handle for identification
*/
struct vc_s {
	int handle;
};

typedef struct vc_s *vc_h;

#ifdef __cplusplus
}
#endif

#endif /* __VC_CLIENT_H_ */
