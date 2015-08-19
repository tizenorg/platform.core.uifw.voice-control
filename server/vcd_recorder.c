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


#include <audio_io.h>
#include <bluetooth.h>
#include <math.h>
#include <sound_manager.h>

#include "vcd_client_data.h"
#include "vcd_dbus.h"
#include "vcd_recorder.h"
#include "vcd_main.h"
#include "voice_control_plugin_engine.h"

/* Multi session enable */
/*#define AUDIO_MULTI_SESSION */

/* TV BT enable */
/*#define TV_BT_MODE */

#define FRAME_LENGTH 160
#define BUFFER_LENGTH FRAME_LENGTH * 2

#define VCP_AUDIO_ID_NONE		"VC_AUDIO_ID_NONE"		/**< None audio id */

static vcd_recorder_state_e	g_recorder_state = VCD_RECORDER_STATE_READY;

static vcd_recoder_audio_cb	g_audio_cb = NULL;

static vcd_recorder_interrupt_cb	g_interrupt_cb = NULL;

static audio_in_h	g_audio_h;

static vcp_audio_type_e g_audio_type;

static unsigned int	g_audio_rate;

static int		g_audio_channel;

static char	g_normal_buffer[BUFFER_LENGTH + 10];

static bool	g_is_valid_audio_in = false;

static bool	g_is_valid_bt_in = false;

static char*	g_current_audio_type = NULL;

#ifdef AUDIO_MULTI_SESSION
static sound_multi_session_h	g_session = NULL;
#endif

static int	g_buffer_count;

/* Sound buf save */
/*
#define BUF_SAVE_MODE
*/

#ifdef BUF_SAVE_MODE
static FILE* g_normal_file;

static int g_count = 1;
#endif

static float get_volume_decibel(char* data, int size);

#ifdef TV_BT_MODE
static int g_bt_extend_count;

#define SMART_CONTROL_EXTEND_CMD	0x03
#define SMART_CONTROL_START_CMD		0x04

static void _bt_cb_hid_state_changed(int result, bool connected, const char *remote_address, void *user_data)
{
	SLOG(LOG_DEBUG, TAG_VCD, "[Recorder] Bluetooth Event [%d] Received address [%s]", result, remote_address);
	return;
}

static void _bt_hid_audio_data_receive_cb(bt_hid_voice_data_s *voice_data, void *user_data)
{
	if (VCD_RECORDER_STATE_RECORDING != g_recorder_state) {
		/*SLOG(LOG_DEBUG, TAG_VCD, "[Recorder] Exit audio reading normal func");*/
		return;
	}

	if (NULL != g_audio_cb) {
		if (0 != g_audio_cb((void*)voice_data->audio_buf, (unsigned int)voice_data->length)) {
			SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail to read audio");
			vcd_recorder_stop();
		}
	}

	/* Set volume */
	if (0 == g_buffer_count % 30) {
		float vol_db = get_volume_decibel((char*)voice_data->audio_buf, (unsigned int)voice_data->length);
		if (0 != vcdc_send_set_volume(vcd_client_manager_get_pid(), vol_db)) {
			SLOG(LOG_ERROR, TAG_VCD, "[Recorder] Fail to send recording volume(%f)", vol_db);
		}
	}

	if (0 == g_buffer_count || 0 == g_buffer_count % 50) {
		SLOG(LOG_WARN, TAG_VCD, "[Recorder][%d] Recording... : read_size(%d)", g_buffer_count, voice_data->length);

		if (0 == g_bt_extend_count % 5) {
			const unsigned char input_data[2] = {SMART_CONTROL_EXTEND_CMD, 0x00 };
			if (BT_ERROR_NONE != bt_hid_send_rc_command(NULL, input_data, sizeof(input_data))) {
				SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail bt_hid_send_rc_command(NULL, %s, %d)", input_data, sizeof(input_data));
			} else {
				SLOG(LOG_DEBUG, TAG_VCD, "[Recorder] Extend bt audio recorder");
			}
		}
		g_bt_extend_count++;

		if (100000 == g_buffer_count) {
			g_buffer_count = 0;
		}
	}

	g_buffer_count++;

#ifdef BUF_SAVE_MODE
	/* write pcm buffer */
	fwrite(voice_data->audio_buf, 1, voice_data->length, g_normal_file);
#endif
	return;
}

#endif

#ifdef AUDIO_MULTI_SESSION
void __vcd_recorder_sound_interrupted_cb(sound_interrupted_code_e code, void *user_data)
{
	SLOG(LOG_DEBUG, TAG_VCD, "[Recorder] Get the interrupt code from sound mgr : %d", code);

	if (SOUND_INTERRUPTED_BY_CALL == code) {
		if (NULL != g_interrupt_cb) {
			g_interrupt_cb();
		}
	}

	return;
}
#endif

int vcd_recorder_create(vcd_recoder_audio_cb audio_cb, vcd_recorder_interrupt_cb interrupt_cb)
{
	if (NULL == audio_cb || NULL == interrupt_cb) {
		SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Input param is NOT valid");
		return VCD_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
#ifdef AUDIO_MULTI_SESSION
	g_session = NULL;
	ret = sound_manager_multi_session_create(SOUND_MULTI_SESSION_TYPE_VOICE_RECOGNITION, &g_session);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail to create multi session : %d", ret);
		return VCD_ERROR_OPERATION_FAILED;
	}

	ret = sound_manager_set_interrupted_cb(__vcd_recorder_sound_interrupted_cb, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail to set interrupt callback : %d", ret);
		sound_manager_multi_session_destroy(g_session);
		return VCD_ERROR_OPERATION_FAILED;
	}

	ret = sound_manager_multi_session_set_mode(g_session, SOUND_MULTI_SESSION_MODE_VR_NORMAL);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail to set mode : %d", ret);
		sound_manager_unset_interrupted_cb();
		sound_manager_multi_session_destroy(g_session);
		return VCD_ERROR_OPERATION_FAILED;
	}
#endif
	/* set init value */
	g_is_valid_audio_in = false;
	g_is_valid_bt_in = false;
	g_current_audio_type = NULL;

	g_audio_type = VCP_AUDIO_TYPE_PCM_S16_LE;
	g_audio_rate = 16000;
	g_audio_channel = 1;

	audio_channel_e audio_ch;
	audio_sample_type_e audio_type;

	switch (g_audio_channel) {
	case 1:	audio_ch = AUDIO_CHANNEL_MONO;		break;
	case 2:	audio_ch = AUDIO_CHANNEL_STEREO;	break;
	default:
		SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Input channel is not supported");
		return VCD_ERROR_OPERATION_FAILED;
		break;
	}

	switch (g_audio_type) {
	case VCP_AUDIO_TYPE_PCM_S16_LE:	audio_type = AUDIO_SAMPLE_TYPE_S16_LE;	break;
	case VCP_AUDIO_TYPE_PCM_U8:	audio_type = AUDIO_SAMPLE_TYPE_U8;	break;
	default:
		SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Invalid Audio Type");
		return VCD_ERROR_OPERATION_FAILED;
		break;
	}

	/*
	ret = audio_in_create_ex(sample_rate, audio_ch, audio_type, &g_audio_h, AUDIO_IO_SOURCE_TYPE_VOICECONTROL);
	*/
	ret = audio_in_create(g_audio_rate, audio_ch, audio_type, &g_audio_h);
	if (AUDIO_IO_ERROR_NONE == ret) {
		g_is_valid_audio_in = true;
		/*
		ret = audio_in_ignore_session(g_audio_h);
		if (AUDIO_IO_ERROR_NONE != ret) {
			SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail to ignore session : %d", ret);
		}
		*/
	} else {
		SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Rate(%d) Channel(%d) Type(%d)", g_audio_rate, audio_ch, audio_type);
		SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail to create audio handle : %d", ret);
		g_is_valid_audio_in = false;
	}

	g_audio_cb = audio_cb;
	g_interrupt_cb = interrupt_cb;
	g_recorder_state = VCD_RECORDER_STATE_READY;

#ifdef TV_BT_MODE

	bool is_bt_failed = false;

	if (false == is_bt_failed && BT_ERROR_NONE != bt_initialize()) {
		SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail to init bt");
		is_bt_failed = true;
	}

	if (false == is_bt_failed && BT_ERROR_NONE != bt_hid_host_initialize(_bt_cb_hid_state_changed, NULL)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail bt_hid_host_initialize()");
		is_bt_failed = true;
	}

	if (false == is_bt_failed && BT_ERROR_NONE != bt_hid_set_audio_data_receive_cb(_bt_hid_audio_data_receive_cb, NULL)) {
		SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail bt_hid_set_audio_data_receive_cb()");
		is_bt_failed = true;
	}


	if (false == is_bt_failed) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Recorder] Bluetooth is available");
		g_is_valid_bt_in = true;
	}
#endif
	/* Select default audio type */
	if (true == g_is_valid_bt_in) {
		g_current_audio_type = strdup(VCP_AUDIO_ID_BLUETOOTH);
	} else {
		g_current_audio_type = strdup(VCP_AUDIO_ID_NONE);
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Recorder] Audio type : %s", g_current_audio_type);

	return 0;
}

int vcd_recorder_destroy()
{
	if (VCD_RECORDER_STATE_RECORDING == g_recorder_state) {
		if (0 != strncmp(VCP_AUDIO_ID_BLUETOOTH, g_current_audio_type, strlen(VCP_AUDIO_ID_BLUETOOTH))) {
			audio_in_unprepare(g_audio_h);
		} else {
#ifdef TV_BT_MODE
			bt_hid_unset_audio_data_receive_cb();
#endif
		}
		g_recorder_state = VCD_RECORDER_STATE_READY;
	}

	audio_in_destroy(g_audio_h);

#ifdef AUDIO_MULTI_SESSION
	sound_manager_unset_interrupted_cb();
	sound_manager_multi_session_destroy(g_session);
#endif

#ifdef TV_BT_MODE
	bt_hid_unset_audio_data_receive_cb();

	bt_hid_host_deinitialize();

	bt_deinitialize();
#endif

	g_audio_cb = NULL;

	if (NULL != g_current_audio_type) {
		free(g_current_audio_type);
		g_current_audio_type = NULL;
	}

	return 0;
}

int vcd_recorder_set(const char* audio_type, vcp_audio_type_e type, int rate, int channel)
{
	if (NULL == audio_type) {
		return VCD_ERROR_INVALID_PARAMETER;
	}

	if (NULL != g_current_audio_type) {
		if (0 == strncmp(g_current_audio_type, audio_type, strlen(g_current_audio_type))) {
			SLOG(LOG_DEBUG, TAG_VCD, "[Recorder] Current audio type is already set : %s", audio_type);
			return 0;
		}
	}

	if (VCD_RECORDER_STATE_READY != g_recorder_state) {
		SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Recorder is NOT ready");
		return VCD_ERROR_INVALID_STATE;
	}

	int ret = -1;
	/* Check BT audio */
	if (0 == strncmp(VCP_AUDIO_ID_BLUETOOTH, audio_type, strlen(VCP_AUDIO_ID_BLUETOOTH))) {
		if (false == g_is_valid_bt_in) {
			SLOG(LOG_ERROR, TAG_VCD, "[Recorder] BT audio is NOT valid");
			return VCD_ERROR_OPERATION_REJECTED;
		}

		if (NULL != g_current_audio_type) {
			free(g_current_audio_type);
			g_current_audio_type = NULL;
		}

		g_current_audio_type = strdup(audio_type);
	} else {
		if (false == g_is_valid_audio_in) {
			SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Audio-in is NOT valid");
			return VCD_ERROR_OPERATION_REJECTED;
		}

		if (g_audio_type != type || g_audio_rate != rate || g_audio_channel != channel) {
			audio_in_destroy(g_audio_h);

			audio_channel_e audio_ch;
			audio_sample_type_e audio_type;

			switch (channel) {
			case 1:	audio_ch = AUDIO_CHANNEL_MONO;		break;
			case 2:	audio_ch = AUDIO_CHANNEL_STEREO;	break;
			default:
				SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Input channel is not supported");
				return VCD_ERROR_OPERATION_FAILED;
				break;
			}

			switch (type) {
			case VCP_AUDIO_TYPE_PCM_S16_LE:	audio_type = AUDIO_SAMPLE_TYPE_S16_LE;	break;
			case VCP_AUDIO_TYPE_PCM_U8:	audio_type = AUDIO_SAMPLE_TYPE_U8;	break;
			default:
				SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Invalid Audio Type");
				return VCD_ERROR_OPERATION_FAILED;
				break;
			}

			ret = audio_in_create(rate, audio_ch, audio_type, &g_audio_h);
			if (AUDIO_IO_ERROR_NONE != ret) {
				SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail to create audio handle : %d", ret);
				g_is_valid_audio_in = false;
				return VCD_ERROR_OPERATION_FAILED;
			}

			g_audio_type = type;
			g_audio_rate = rate;
			g_audio_channel = channel;
		}

#ifdef TV_BT_MODE
		char* temp_type = NULL;
		temp_type = strdup(audio_type);

		ret = audio_in_set_device(g_audio_h, temp_type);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_VCD, "[Recorder] Audio type is NOT valid :%s", temp_type);
			if (NULL != temp_type)
				free(temp_type);
			return VCD_ERROR_OPERATION_REJECTED;
		}

		if (NULL != temp_type)
			free(temp_type);

		if (NULL != g_current_audio_type) {
			free(g_current_audio_type);
			g_current_audio_type = NULL;
		}

		g_current_audio_type = strdup(audio_type);
#endif
	}

	SLOG(LOG_DEBUG, TAG_VCD, "[Recorder] Current audio type is changed : %s", g_current_audio_type);

	return 0;
}

int vcd_recorder_get(char** audio_type)
{
	if (NULL == audio_type) {
		return VCD_ERROR_INVALID_PARAMETER;
	}

	if (NULL != g_current_audio_type) {
		*audio_type = strdup(g_current_audio_type);
	} else {
		SLOG(LOG_WARN, TAG_VCD, "[Recorder] Current audio type is NOT ready", audio_type);
		*audio_type = NULL;
	}

	return 0;
}

static float get_volume_decibel(char* data, int size)
{
#define MAX_AMPLITUDE_MEAN_16 32768

	int i, depthByte;
	int count = 0;

	float db = 0.0;
	float rms = 0.0;
	unsigned long long square_sum = 0;
	short pcm16 = 0;

	depthByte = 2;

	for (i = 0; i < size; i += (depthByte<<1)) {
		pcm16 = 0;
		memcpy(&pcm16, data + i, sizeof(short));
		square_sum += pcm16 * pcm16;
		count++;
	}

	if (0 == count)
		rms = 0.0;
	else
		rms = sqrt(square_sum/count);

	db = 20 * log10(rms/MAX_AMPLITUDE_MEAN_16);
	return db;
}

Eina_Bool __read_normal_func(void *data)
{
	int ret = -1;

	if (VCD_RECORDER_STATE_RECORDING != g_recorder_state) {
		SLOG(LOG_DEBUG, TAG_VCD, "[Recorder] Exit audio reading normal func");
		return EINA_FALSE;
	}

	memset(g_normal_buffer, '\0', BUFFER_LENGTH + 10);

	ret = audio_in_read(g_audio_h, g_normal_buffer, BUFFER_LENGTH);
	if (0 > ret) {
		SLOG(LOG_WARN, TAG_VCD, "[Recorder WARNING] Fail to read audio : %d", ret);
		g_recorder_state = VCD_RECORDER_STATE_READY;
		return EINA_FALSE;
	}

	if (NULL != g_audio_cb) {
		if (0 != g_audio_cb(g_normal_buffer, BUFFER_LENGTH)) {
			SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail to read audio : %d", ret);
			vcd_recorder_stop();
			return EINA_FALSE;
		}
	}

	/* Set volume */
	if (0 == g_buffer_count % 30) {
		float vol_db = get_volume_decibel(g_normal_buffer, BUFFER_LENGTH);
		if (0 != vcdc_send_set_volume(vcd_client_manager_get_pid(), vol_db)) {
			SLOG(LOG_ERROR, TAG_VCD, "[Recorder] Fail to send recording volume(%f)", vol_db);
		}
	}

	if (0 == g_buffer_count || 0 == g_buffer_count % 50) {
		SLOG(LOG_WARN, TAG_VCD, "[Recorder][%d] Recording... : read_size(%d)", g_buffer_count, ret);

		if (100000 == g_buffer_count) {
			g_buffer_count = 0;
		}
	}

	g_buffer_count++;

#ifdef BUF_SAVE_MODE
	/* write pcm buffer */
	fwrite(g_normal_buffer, 1, BUFFER_LENGTH, g_normal_file);
#endif

	return EINA_TRUE;
}

int vcd_recorder_start()
{
	int ret = -1;
	g_buffer_count = 0;

	if (VCD_RECORDER_STATE_RECORDING == g_recorder_state)	return 0;

#ifdef AUDIO_MULTI_SESSION
	ret = sound_manager_multi_session_set_option(g_session, SOUND_MULTI_SESSION_OPT_MIX_WITH_OTHERS);
	if (0 != ret) {
		if (SOUND_MANAGER_ERROR_POLICY_BLOCKED_BY_CALL == ret) {
			SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] blocked to set option by call");
		} else {
			SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail to set option : %d", ret);
		}
		return VCD_ERROR_RECORDER_BUSY;
	}
#endif
	bool started = false;
	if (NULL != g_current_audio_type) {
		if (0 == strncmp(VCP_AUDIO_ID_BLUETOOTH, g_current_audio_type, strlen(VCP_AUDIO_ID_BLUETOOTH))) {
#ifdef TV_BT_MODE
			const unsigned char input_data[2] = {SMART_CONTROL_START_CMD, 0x00 };
			if (BT_ERROR_NONE != bt_hid_send_rc_command(NULL, input_data, sizeof(input_data))) {
				SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail bt_hid_send_rc_command(NULL, %s, %d)", input_data, sizeof(input_data));
			} else {
				SLOG(LOG_DEBUG, TAG_VCD, "[Recorder] Start bt audio recorder");
			}

			started = true;

			g_bt_extend_count = 0;
#endif
		}
	}

	if (false == started) {
		ret = audio_in_prepare(g_audio_h);
		if (AUDIO_IO_ERROR_NONE != ret) {
			if (AUDIO_IO_ERROR_SOUND_POLICY == ret) {
#ifdef AUDIO_MULTI_SESSION
				sound_manager_multi_session_set_option(g_session, SOUND_MULTI_SESSION_OPT_RESUME_OTHERS);
#endif
				SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Audio is busy.");
				return VCD_ERROR_RECORDER_BUSY;
			} else {
				SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail to start audio : %d", ret);
			}
			return VCD_ERROR_OPERATION_FAILED;
		}

		/* Add ecore timer to read audio data */
		ecore_timer_add(0, __read_normal_func, NULL);
		SLOG(LOG_DEBUG, TAG_VCD, "[Recorder] Start audio in recorder");
	}

	g_recorder_state = VCD_RECORDER_STATE_RECORDING;

#ifdef BUF_SAVE_MODE
	char normal_file_name[128] = {'\0',};
	g_count++;

	snprintf(normal_file_name, sizeof(normal_file_name), "/tmp/vc_normal_%d_%d", getpid(), g_count);
	SLOG(LOG_DEBUG, TAG_VCD, "[Recorder] File normal name : %s", normal_file_name);

	/* open test file */
	g_normal_file = fopen(normal_file_name, "wb+");
	if (!g_normal_file) {
		SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] File not found!");
	}
#endif
	return 0;
}

int vcd_recorder_stop()
{
	int ret = -1;

	if (VCD_RECORDER_STATE_READY == g_recorder_state)
		return 0;

	g_recorder_state = VCD_RECORDER_STATE_READY;

#ifdef BUF_SAVE_MODE
	fclose(g_normal_file);
#endif

	bool stoped = false;

	if (NULL != g_current_audio_type) {
		if (0 == strncmp(VCP_AUDIO_ID_BLUETOOTH, g_current_audio_type, strlen(VCP_AUDIO_ID_BLUETOOTH))) {
#ifdef TV_BT_MODE
			if (BT_ERROR_NONE != bt_hid_rc_stop_sending_voice(NULL)) {
				SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail bt_hid_rc_stop_sending_voice()");
			}
			stoped = true;
#endif
		}
	}

	if (false == stoped) {
		ret = audio_in_unprepare(g_audio_h);
		if (AUDIO_IO_ERROR_NONE != ret) {
			SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail to stop audio : %d", ret);
			return VCD_ERROR_OPERATION_FAILED;
		}
	}

#ifdef AUDIO_MULTI_SESSION
	ret = sound_manager_multi_session_set_option(g_session, SOUND_MULTI_SESSION_OPT_RESUME_OTHERS);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_VCD, "[Recorder ERROR] Fail to set option : %d", ret);
		return VCD_ERROR_OPERATION_FAILED;
	}
#endif
	return 0;
}

int vcd_recorder_get_state()
{
	return g_recorder_state;
}
