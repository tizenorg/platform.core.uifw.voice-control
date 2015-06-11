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


#ifndef __VOICE_CONTROL_KEY_DEFINES_H__
#define __VOICE_CONTROL_KEY_DEFINES_H__


/**
* @addtogroup VOICE_CONTROL_KEY_DEFINES
* @{
*/

#ifdef __cplusplus
extern "C" 
{
#endif

/*
* Modifier defines
*/
#define VC_MODIFIER_NONE	0x00000000
#define VC_MODIFIER_CTRL	0x01000000
#define VC_MODIFIER_ALT		0x00100000
#define VC_MODIFIER_SHIFT	0x00010000

/*
* Key defines
*/
#define VC_KEY_NONE		0x00000000

#define VC_KEY_NUMBER_BASE	0x00001000
#define VC_KEY_0		VC_KEY_NUMBER_BASE | 0x00
#define VC_KEY_1		VC_KEY_NUMBER_BASE | 0x01
#define VC_KEY_2		VC_KEY_NUMBER_BASE | 0x02
#define VC_KEY_3		VC_KEY_NUMBER_BASE | 0x03
#define VC_KEY_4		VC_KEY_NUMBER_BASE | 0x04
#define VC_KEY_5		VC_KEY_NUMBER_BASE | 0x05
#define VC_KEY_6		VC_KEY_NUMBER_BASE | 0x06
#define VC_KEY_7		VC_KEY_NUMBER_BASE | 0x07
#define VC_KEY_8		VC_KEY_NUMBER_BASE | 0x08
#define VC_KEY_9		VC_KEY_NUMBER_BASE | 0x09


#define VC_KEY_FUNCTION_BASE	0x00002000
#define VC_KEY_F1		VC_KEY_FUNCTION_BASE | 0x01
#define VC_KEY_F2		VC_KEY_FUNCTION_BASE | 0x02
#define VC_KEY_F3		VC_KEY_FUNCTION_BASE | 0x03
#define VC_KEY_F4		VC_KEY_FUNCTION_BASE | 0x04
#define VC_KEY_F5		VC_KEY_FUNCTION_BASE | 0x05
#define VC_KEY_F6		VC_KEY_FUNCTION_BASE | 0x06
#define VC_KEY_F7		VC_KEY_FUNCTION_BASE | 0x07
#define VC_KEY_F8		VC_KEY_FUNCTION_BASE | 0x08
#define VC_KEY_F9		VC_KEY_FUNCTION_BASE | 0x09
#define VC_KEY_F10		VC_KEY_FUNCTION_BASE | 0x10
#define VC_KEY_F11		VC_KEY_FUNCTION_BASE | 0x11
#define VC_KEY_F12		VC_KEY_FUNCTION_BASE | 0x12

#define VC_KEY_UP		VC_KEY_FUNCTION_BASE | 0x40
#define VC_KEY_DOWN		VC_KEY_FUNCTION_BASE | 0x41
#define VC_KEY_RIGHT		VC_KEY_FUNCTION_BASE | 0x42
#define VC_KEY_LEFT		VC_KEY_FUNCTION_BASE | 0x43
#define VC_KEY_INSERT		VC_KEY_FUNCTION_BASE | 0x44
#define VC_KEY_HOME		VC_KEY_FUNCTION_BASE | 0x45
#define VC_KEY_END		VC_KEY_FUNCTION_BASE | 0x46
#define VC_KEY_PAGE_UP		VC_KEY_FUNCTION_BASE | 0x47
#define VC_KEY_PAGE_DOWN	VC_KEY_FUNCTION_BASE | 0x48


#define VC_KEY_ALPHABET_BASE	0x00003000
#define VC_KEY_A		VC_KEY_ALPHABET_BASE | 0x01
#define VC_KEY_B		VC_KEY_ALPHABET_BASE | 0x02
#define VC_KEY_C		VC_KEY_ALPHABET_BASE | 0x03
#define VC_KEY_D		VC_KEY_ALPHABET_BASE | 0x04
#define VC_KEY_E		VC_KEY_ALPHABET_BASE | 0x05
#define VC_KEY_F		VC_KEY_ALPHABET_BASE | 0x06
#define VC_KEY_G		VC_KEY_ALPHABET_BASE | 0x07
#define VC_KEY_H		VC_KEY_ALPHABET_BASE | 0x08
#define VC_KEY_I		VC_KEY_ALPHABET_BASE | 0x09
#define VC_KEY_J		VC_KEY_ALPHABET_BASE | 0x10
#define VC_KEY_K		VC_KEY_ALPHABET_BASE | 0x11
#define VC_KEY_L		VC_KEY_ALPHABET_BASE | 0x12
#define VC_KEY_M		VC_KEY_ALPHABET_BASE | 0x13
#define VC_KEY_N		VC_KEY_ALPHABET_BASE | 0x14
#define VC_KEY_O		VC_KEY_ALPHABET_BASE | 0x15
#define VC_KEY_P		VC_KEY_ALPHABET_BASE | 0x16
#define VC_KEY_Q		VC_KEY_ALPHABET_BASE | 0x17
#define VC_KEY_R		VC_KEY_ALPHABET_BASE | 0x18
#define VC_KEY_S		VC_KEY_ALPHABET_BASE | 0x19
#define VC_KEY_T		VC_KEY_ALPHABET_BASE | 0x20
#define VC_KEY_U		VC_KEY_ALPHABET_BASE | 0x21
#define VC_KEY_V		VC_KEY_ALPHABET_BASE | 0x22
#define VC_KEY_W		VC_KEY_ALPHABET_BASE | 0x23
#define VC_KEY_X		VC_KEY_ALPHABET_BASE | 0x24
#define VC_KEY_Y		VC_KEY_ALPHABET_BASE | 0x25
#define VC_KEY_Z		VC_KEY_ALPHABET_BASE | 0x26


#define VC_KEY_SYMBOL_BASE	0x00004000
#define VC_KEY_COLON		VC_KEY_SYMBOL_BASE | 0x01
#define VC_KEY_SEMICOLON	VC_KEY_SYMBOL_BASE | 0x02
#define VC_KEY_LESS		VC_KEY_SYMBOL_BASE | 0x03
#define VC_KEY_EQUAL		VC_KEY_SYMBOL_BASE | 0x04
#define VC_KEY_GREATER		VC_KEY_SYMBOL_BASE | 0x05
#define VC_KEY_QUESTION		VC_KEY_SYMBOL_BASE | 0x06
#define VC_KEY_AT		VC_KEY_SYMBOL_BASE | 0x07


#ifdef __cplusplus
}
#endif

/**
 * @}@}
 */

#endif /* __VOICE_CONTROL_KEY_DEFINES_H__ */
