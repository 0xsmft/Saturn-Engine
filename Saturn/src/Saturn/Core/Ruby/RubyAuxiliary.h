/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2023 BEAST                                                           		*
*                                                                                           *
* Permission is hereby granted, free of charge, to any person obtaining a copy              *
* of this software and associated documentation files (the "Software"), to deal             *
* in the Software without restriction, including without limitation the rights              *
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell                 *
* copies of the Software, and to permit persons to whom the Software is                     *
* furnished to do so, subject to the following conditions:                                  *
*                                                                                           *
* The above copyright notice and this permission notice shall be included in all            *
* copies or substantial portions of the Software.                                           *
*                                                                                           *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR                *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,                  *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE               *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER                    *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,             *
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE             *
* SOFTWARE.                                                                                 *
*********************************************************************************************
*/

#pragma once

#include "RubyEventType.h"

namespace Saturn {

	inline std::string_view RubyKeyToString( RubyKey Key )
	{
		switch( Key )
		{
			case RubyKey_A: return "A";
			case RubyKey_B: return "B";
			case RubyKey_C: return "C";
			case RubyKey_D: return "D";
			case RubyKey_E: return "E";
			case RubyKey_F: return "F";
			case RubyKey_G: return "G";
			case RubyKey_H: return "H";
			case RubyKey_I: return "I";
			case RubyKey_J: return "J";
			case RubyKey_K: return "K";
			case RubyKey_L: return "L";
			case RubyKey_M: return "M";
			case RubyKey_N: return "N";
			case RubyKey_O: return "O";
			case RubyKey_P: return "P";
			case RubyKey_Q: return "Q";
			case RubyKey_R: return "R";
			case RubyKey_S: return "S";
			case RubyKey_T: return "T";
			case RubyKey_U: return "U";
			case RubyKey_V: return "V";
			case RubyKey_W: return "W";
			case RubyKey_X: return "X";
			case RubyKey_Y: return "Y";
			case RubyKey_Z: return "Z";

			case RubyKey_Num0: return "0";
			case RubyKey_Num1: return "1";
			case RubyKey_Num2: return "2";
			case RubyKey_Num3: return "3";
			case RubyKey_Num4: return "4";
			case RubyKey_Num5: return "5";
			case RubyKey_Num6: return "6";
			case RubyKey_Num7: return "7";
			case RubyKey_Num8: return "8";
			case RubyKey_Num9: return "9";

			case RubyKey_CapsLock: return "CapsLock";
			case RubyKey_NumLock: return "NumLock";
			case RubyKey_ScrollLock: return "ScrollLock";

			// grammar / symbols
			case RubyKey_Space: return "Space";
			case RubyKey_Apostrophe: return "Apostrophe";
			case RubyKey_Hash: return "Hash";
			case RubyKey_Comma: return "Comma";
			case RubyKey_Minus: return "Minus";
			case RubyKey_Period: return "Period";
			case RubyKey_Slash: return "ForwardSlash";
			case RubyKey_Semicolon: return "Semicolon";
			case RubyKey_Equal: return "Equal";
			case RubyKey_LeftBracket: return "LeftBracket";
			case RubyKey_Backslash: return "Backslash";
			case RubyKey_RightBracket: return "RightBracket";
			case RubyKey_Grave: return "Grave";

			case RubyKey_Enter: return "Enter";
			case RubyKey_Tab: return "Tab";
			case RubyKey_Esc: return "Esc";
			case RubyKey_Backspace: return "Backspace";
			case RubyKey_Home: return "Home";
			case RubyKey_Insert: return "Insert";
			case RubyKey_Delete: return "Delete";
			case RubyKey_End: return "End";
			case RubyKey_PageUp: return "PageUp";
			case RubyKey_PageDown: return "PageDown";

			case RubyKey_LeftShift: return "LeftShift";
			case RubyKey_LeftCtrl: return "LeftCtrl";
			case RubyKey_LeftAlt: return "LeftAlt";

			case RubyKey_OSKey: return "OSKey";
			case RubyKey_OSKeyRight: return "OSKeyRight";

			case RubyKey_Numpad0: return "Numpad0";
			case RubyKey_Numpad1: return "Numpad1";
			case RubyKey_Numpad2: return "Numpad2";
			case RubyKey_Numpad3: return "Numpad3";
			case RubyKey_Numpad4: return "Numpad4";
			case RubyKey_Numpad5: return "Numpad5";
			case RubyKey_Numpad6: return "Numpad6";
			case RubyKey_Numpad7: return "Numpad7";
			case RubyKey_Numpad8: return "Numpad8";
			case RubyKey_Numpad9: return "Numpad9";
			case RubyKey_NumpadAdd: return "NumpadAdd";
			case RubyKey_NumpadSubtract: return "NumpadSubtract";
			case RubyKey_NumpadMultiply: return "NumpadMultiply";
			case RubyKey_NumpadDivide: return "NumpadDivide";

			case RubyKey_LeftArrow: return "LeftArrow";
			case RubyKey_UpArrow: return "UpArrow";
			case RubyKey_RightArrow: return "RightArrow";
			case RubyKey_DownArrow: return "DownArrow";

			case RubyKey_F1: return "F1";
			case RubyKey_F2: return "F2";
			case RubyKey_F3: return "F3";
			case RubyKey_F4: return "F4";
			case RubyKey_F5: return "F5";
			case RubyKey_F6: return "F6";
			case RubyKey_F7: return "F7";
			case RubyKey_F8: return "F8";
			case RubyKey_F9: return "F9";
			case RubyKey_F10: return "F10";
			case RubyKey_F11: return "F11";
			case RubyKey_F12: return "F12";

			case RubyKey_RightCtrl: return "RightCtrl";
			case RubyKey_RightShift: return "RightShift";
			case RubyKey_RightAlt: return "RightAlt";

			case RubyKey_PrintScreen: return "PrintScreen";
			case RubyKey_Pause:       return "Pause";
			case RubyKey_Menu:        return "Menu";

			case RubyKey_EnumSize:
			case RubyKey_UnknownKey:
			default: return "";
		}
	}

	inline std::string_view RubyMouseButtonToString( RubyMouseButton Button )
	{
		switch( Button )
		{
			case RubyMouseButton_Left:
				return "Left";
			case RubyMouseButton_Right:
				return "Right";
			case RubyMouseButton_Middle:
				return "Middle";
			case RubyMouseButton_Extra1:
				return "Extra1";
			case RubyMouseButton_Extra2:
				return "Extra2";

			case RubyMouseButton_Unknown:
			default: return "";
		}

	}

}
