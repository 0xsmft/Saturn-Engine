/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 BEAST                                                           *
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

#include "Saturn/Core/Event.h"
#include "RubyEventType.h"

namespace Saturn {

	class RubyWindowResizeEvent : public Event
	{
		SAT_DEFINE_EVENT( Resize, EC_Ruby )
	public:
		RubyWindowResizeEvent() = default;
		RubyWindowResizeEvent( EventType type, uint32_t width, uint32_t height )
			: Event( type, EC_Ruby ), m_Width( width ), m_Height( height )
		{
		}

		~RubyWindowResizeEvent() {}

		uint32_t GetWidth() { return m_Width; }
		uint32_t GetHeight() { return m_Height; }

		const uint32_t GetWidth() const { return m_Width; }
		const uint32_t GetHeight() const { return m_Height; }

	private:
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
	};

	class RubyMouseMoveEvent : public Event
	{
		SAT_DEFINE_EVENT( MouseMoved, EC_Ruby )
	public:
		RubyMouseMoveEvent() = default;
		RubyMouseMoveEvent( EventType type, float x, float y )
			: Event( type, EC_Ruby ), X( x ), Y( y )
		{
		}

		~RubyMouseMoveEvent() {}

		float GetX() const { return X; }
		float GetY() const { return Y; }

	private:
		float X, Y;
	};

	class RubyMouseEvent : public Event
	{
	public:
		RubyMouseEvent() = default;
		RubyMouseEvent( EventType type, int ButtonCode ) : Event( type, EC_Ruby ), m_ButtonCode( ButtonCode ) {}
		~RubyMouseEvent() {}

		int GetButton() const { return m_ButtonCode; }

	private:
		int m_ButtonCode = 0;
	};

	class RubyMouseScrollEvent : public Event
	{
		SAT_DEFINE_EVENT( MouseScroll, EC_Ruby )
	public:
		RubyMouseScrollEvent() = default;
		RubyMouseScrollEvent( EventType type, int x, int y ) : Event( type, EC_Ruby ), m_MouseOffsetX( x ), m_MouseOffsetY( y ) {}
		~RubyMouseScrollEvent() {}

		int GetOffsetX() const { return m_MouseOffsetX; }
		int GetOffsetY() const { return m_MouseOffsetY; }

	private:
		int m_MouseOffsetX = 0;
		int m_MouseOffsetY = 0;
	};

	class RubyKeyEvent : public Event
	{
	public:
		RubyKeyEvent() = default;
		RubyKeyEvent( EventType Type, RubyKey keyCode, uint32_t scanCode, int modifiers ) 
			: Event( Type, EC_Ruby ), m_KeyCode( keyCode ), m_ScanCode( scanCode ), m_Modifiers( modifiers ) 
		{
		}
		~RubyKeyEvent() = default;

		RubyKey GetKeycode() const { return m_KeyCode; }
		int GetScancode() const { return m_ScanCode; }
		int GetModifers() const { return m_Modifiers; }

	private:
		RubyKey m_KeyCode = RubyKey_UnknownKey;
		
		uint32_t m_ScanCode = 0;
		int m_Modifiers = 0;
	};

	class RubyCharacterEvent : public Event
	{
		SAT_DEFINE_EVENT( InputCharacter, EC_Ruby )
	public:
		RubyCharacterEvent() = default;
		RubyCharacterEvent( EventType Type, wchar_t wc ) : Event( Type, EC_Ruby ), m_WideCharacter( wc ) {}

		~RubyCharacterEvent() = default;

		wchar_t GetCharacter() const { return m_WideCharacter; }
		char GetCharacterA()   const { return (char)m_WideCharacter; }

	private:
		wchar_t m_WideCharacter;
	};

	class RubyMaximizeEvent : public Event
	{
		SAT_DEFINE_EVENT( WindowMaximized, EC_Ruby )
	public:
		RubyMaximizeEvent() = default;
		RubyMaximizeEvent( EventType Type, bool state ) : Event( Type, EC_Ruby ), m_State( state ) {}

		~RubyMaximizeEvent() = default;

		bool GetState() const { return m_State; }

	private:
		// State meaning: true is maximized, false no longer maximized.
		bool m_State = false;
	};

	class RubyFocusEvent : public Event
	{
		SAT_DEFINE_EVENT( WindowFocus, EC_Ruby )
	public:
		RubyFocusEvent() = default;
		RubyFocusEvent( EventType Type, bool state ) : Event( Type, EC_Ruby ), m_State( state ) {}

		~RubyFocusEvent() = default;

		bool GetState() const { return m_State; }

	private:
		bool m_State = false;
	};

	class RubyMinimizeEvent : public Event
	{
		SAT_DEFINE_EVENT( WindowMinimized, EC_Ruby )
	public:
		RubyMinimizeEvent() = default;
		RubyMinimizeEvent( EventType Type, bool state ) : Event( Type, EC_Ruby ), m_State( state ) {}

		~RubyMinimizeEvent() = default;

		bool GetState() const { return m_State; }

	private:
		bool m_State = false;
	};

	class RubyEventTarget
	{
	public:
		virtual ~RubyEventTarget() = default;

		virtual bool OnEvent( Event& rEvent ) = 0;
	};

}
