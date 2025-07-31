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

namespace Saturn {

	using KeyCode = int;

	enum RubyKey
	{
		UnknownKey,

		// Alphabetic keys (match with Ansi)
		A = 0x41, B = 0x42, C = 0x43, D = 0x44, E = 0x45, F = 0x46, G = 0x47, H = 0x48,
		I = 0x49, J = 0x4A, K = 0x4B, L = 0x4C, M = 0x4D, N = 0x4E, O = 0x4F, P = 0x50,
		Q = 0x51, R = 0x52, S = 0x53, T = 0x54, U = 0x55, V = 0x56, W = 0x57, X = 0x58,
		Y = 0x59, Z = 0x5A,

		// Numeric keys
		Num0 = 0x30, Num1 = 0x31, Num2 = 0x32, Num3 = 0x33, Num4 = 0x34, Num5 = 0x35,
		Num6 = 0x36, Num7 = 0x37, Num8 = 0x38, Num9 = 0x39,

		// Special keys
		Space = 0x20, Enter = 0x0D, Tab = 0x09,
		Esc = 0x1B, Backspace = 0x08, CapsLock = 0x14, Shift = 0x10, Ctrl = 0x11, Alt = 0x12,
		OSKey = 0x5B,
		Insert = 0x2D, Delete = 0x2E, Home = 0x24, End = 0x23, PageUp = 0x21, PageDown = 0x22,

		Numpad0 = 0x60, Numpad1 = 0x61, Numpad2 = 0x62, Numpad3 = 0x63,
		Numpad4 = 0x64, Numpad5 = 0x65, Numpad6 = 0x66, Numpad7 = 0x67,
		Numpad8 = 0x68, Numpad9 = 0x69,
		NumpadAdd = 0x6B, NumpadSubtract = 0x6D, NumpadMultiply = 0x6A,
		NumpadDivide = 0x6F, NumpadEnter = 0x0D,

		// Arrow keys
		LeftArrow = 0x25,
		UpArrow = 0x26,
		RightArrow = 0x27,
		DownArrow = 0x28,

		// Function keys
		F1 = 0x70, F2 = 0x71, F3 = 0x72, F4 = 0x73, F5 = 0x74, F6 = 0x75, F7 = 0x76, F8 = 0x77,
		F9 = 0x78, F10 = 0x79, F11 = 0x7A, F12 = 0x7B,

		// Modifier keys
		RightCtrl = 0xA3,
		RightShift = 0xA1,
		RightAlt = 0xA5,

		EnumSize
	};

	enum class RubyMouseButton : unsigned int
	{
		Unknown = 6,

		Left = 0,
		Right = 1,
		Middle = 2,
		Extra1 = 3,
		Extra2 = 4
	};
}
