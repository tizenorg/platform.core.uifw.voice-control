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


#include "vcd_dbus.h"
#include "vcd_main.h"
#include "vcd_server.h"

#define CLIENT_CLEAN_UP_TIME 500

static Ecore_Timer* g_check_client_timer = NULL;

int main(int argc, char** argv)
{
	SLOG(LOG_DEBUG, TAG_VCD, "  ");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");
	SLOG(LOG_DEBUG, TAG_VCD, "===== VC Daemon Initialize");

	if (!ecore_init()) {
		SLOG(LOG_ERROR, TAG_VCD, "[ERROR] Fail ecore_init()");
		return -1;
	}

	if (0 != vcd_dbus_open_connection()) {
		SLOG(LOG_ERROR, TAG_VCD, "[ERROR] Fail to open connection");
		return EXIT_FAILURE;
	}

	if (0 != vcd_initialize()) {
		SLOG(LOG_ERROR, TAG_VCD, "[ERROR] Fail to initialize vc-daemon");
		return EXIT_FAILURE;
	}

	g_check_client_timer = ecore_timer_add(CLIENT_CLEAN_UP_TIME, vcd_cleanup_client, NULL);
	if (NULL == g_check_client_timer) {
		SLOG(LOG_WARN, TAG_VCD, "[Main Warning] Fail to create timer of client check");
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Main] vc-daemon start...");

	SLOG(LOG_DEBUG, TAG_VCD, "=====");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	ecore_main_loop_begin();

	SLOG(LOG_DEBUG, TAG_VCD, "===== VC Daemon Finalize");

	if (NULL != g_check_client_timer) {
		ecore_timer_del(g_check_client_timer);
	}

	vcd_dbus_close_connection();

	vcd_finalize();

	ecore_shutdown();

	SLOG(LOG_DEBUG, TAG_VCD, "=====");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");
	SLOG(LOG_DEBUG, TAG_VCD, "  ");

	return 0;
}


