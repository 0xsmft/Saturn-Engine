/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2025 BEAST                                                           *
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
#include <string>

namespace Saturn {

	inline std::string_view RubyKeyToString( RubyKey Key )
	{
		switch( Key )
		{
			case A: return "A";
			case B: return "B";
			case C: return "C";
			case D: return "D";
			case E: return "E";
			case F: return "F";
			case G: return "G";
			case H: return "H";
			case I: return "I";
			case J: return "J";
			case K: return "K";
			case L: return "L";
			case M: return "M";
			case N: return "N";
			case O: return "O";
			case P: return "P";
			case Q: return "Q";
			case R: return "R";
			case S: return "S";
			case T: return "T";
			case U: return "U";
			case V: return "V";
			case W: return "W";
			case X: return "X";
			case Y: return "Y";
			case Z: return "Z";

			case Num0: return "0";
			case Num1: return "1";
			case Num2: return "2";
			case Num3: return "3";
			case Num4: return "4";
			case Num5: return "5";
			case Num6: return "6";
			case Num7: return "7";
			case Num8: return "8";
			case Num9: return "9";

			case Space: return "Space";
			case Enter: return "Enter";
			case Tab: return "Tab";
			case Esc: return "Esc";
			case Backspace: return "Backspace";
			case CapsLock: return "CapsLock";
			case Shift: return "Shift";
			case Ctrl: return "Ctrl";
			case Alt: return "Alt";
			case OSKey: return "OSKey";
			case Insert: return "Insert";
			case Delete: return "Delete";
			case Home: return "Home";
			case End: return "End";
			case PageUp: return "PageUp";
			case PageDown: return "PageDown";

			case Numpad0: return "Numpad0";
			case Numpad1: return "Numpad1";
			case Numpad2: return "Numpad2";
			case Numpad3: return "Numpad3";
			case Numpad4: return "Numpad4";
			case Numpad5: return "Numpad5";
			case Numpad6: return "Numpad6";
			case Numpad7: return "Numpad7";
			case Numpad8: return "Numpad8";
			case Numpad9: return "Numpad9";
			case NumpadAdd: return "NumpadAdd";
			case NumpadSubtract: return "NumpadSubtract";
			case NumpadMultiply: return "NumpadMultiply";
			case NumpadDivide: return "NumpadDivide";

			case LeftArrow: return "LeftArrow";
			case UpArrow: return "UpArrow";
			case RightArrow: return "RightArrow";
			case DownArrow: return "DownArrow";

			case F1: return "F1";
			case F2: return "F2";
			case F3: return "F3";
			case F4: return "F4";
			case F5: return "F5";
			case F6: return "F6";
			case F7: return "F7";
			case F8: return "F8";
			case F9: return "F9";
			case F10: return "F10";
			case F11: return "F11";
			case F12: return "F12";

			case RightCtrl: return "RightCtrl";
			case RightShift: return "RightShift";
			case RightAlt: return "RightAlt";

			case EnumSize:
			case UnknownKey:
			default:
				return "";
		}

		return "";
	}

	inline std::string_view RubyMouseButtonToString( RubyMouseButton Button )
	{
		switch( Button )
		{
			case RubyMouseButton::Left:
				return "Left";
			case RubyMouseButton::Right:
				return "Right";
			case RubyMouseButton::Middle:
				return "Middle";
			case RubyMouseButton::Extra1:
				return "Extra1";
			case RubyMouseButton::Extra2:
				return "Extra2";

			case RubyMouseButton::Unknown:
			default:
				return "";
		}

		return "";
	}

}