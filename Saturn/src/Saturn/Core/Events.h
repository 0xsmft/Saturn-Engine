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

#include <string>
#include <sstream>

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// EVENTS																//
	//////////////////////////////////////////////////////////////////////////

	enum EventType
	{
		Close,
		Resize,
		Iconify, // Minimize
		Uniconify, // Maximize
		Drag,
		Move = Drag,
		KeyPressed,
		KeyReleased,
		KeyTyped,
		KeyHeld,
		MouseButtonPressed,
		MouseButtonReleased,
		MouseMoved,
		MouseScrolled
	};

#define EVENT_CLASS_TYPE(type)                                               \
static EventType StaticType() { return EventType::type; }                    \
virtual EventType GetEventType() const override { return StaticType(); }  \
virtual const char* Name() const override { return #type; }                  \
virtual int CategoryFlags( void ) const { return 0; }

	class Event
	{
	public:
		virtual ~Event() = default;

		bool Handled = false;

		virtual EventType GetEventType( void ) const = 0;
		virtual const char* Name( void ) const = 0;
		virtual int CategoryFlags( void ) const = 0;
		virtual std::string ToString( void ) const { return Name(); }
	};

	// KEY EVENTS

	class KeyEvent : public Event
	{
	public:
		inline int KeyCode( void ) const { return m_KeyCode; }

	protected:
		KeyEvent( int keycode )
			: m_KeyCode( keycode )
		{
		}

		int m_KeyCode;
	};

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent( int keycode, int repeatCount )
			: KeyEvent( keycode ), m_RepeatCount( repeatCount )
		{
		}

		inline int RepeatCount( void ) const { return m_RepeatCount; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyPressedEvent: " << m_KeyCode << " (" << m_RepeatCount << " repeats)";
			return ss.str();
		}

		EVENT_CLASS_TYPE( KeyPressed )
	private:
		int m_RepeatCount;
	};

	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent( int keycode )
			: KeyEvent( keycode )
		{
		}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE( KeyReleased )
	};

	class KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent( int keycode )
			: KeyEvent( keycode )
		{
		}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyTypedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE( KeyTyped )
	};

	//////////////////////////////////////////////////////////////////////////

	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent( float x, float y )
			: m_MouseX( x ), m_MouseY( y )
		{
		}

		inline float X( void ) const { return m_MouseX; }
		inline float Y( void ) const { return m_MouseY; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
			return ss.str();
		}

		EVENT_CLASS_TYPE( MouseMoved )
	private:
		float m_MouseX, m_MouseY;
	};

	class MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent( float xOffset, float yOffset )
			: m_XOffset( xOffset ), m_YOffset( yOffset )
		{
		}

		inline float XOffset( void ) const { return m_XOffset; }
		inline float YOffset( void ) const { return m_YOffset; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << XOffset() << ", " << YOffset();
			return ss.str();
		}

		EVENT_CLASS_TYPE( MouseScrolled )
	private:
		float m_XOffset, m_YOffset;
	};

	class MouseButtonEvent : public Event
	{
	public:
		inline int MouseButton( void ) const { return m_Button; }

	protected:
		MouseButtonEvent( int button )
			: m_Button( button )
		{
		}

		int m_Button;
	};

	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent( int button )
			: MouseButtonEvent( button )
		{
		}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE( MouseButtonPressed )
	};

	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent( int button )
			: MouseButtonEvent( button )
		{
		}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE( MouseButtonReleased )
	};

	//////////////////////////////////////////////////////////////////////////

	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent( unsigned int width, unsigned int height )
			: m_Width( width ), m_Height( height )
		{
		}

		unsigned int Width( void )  const { return m_Width; }
		unsigned int Height( void )  const { return m_Height; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		EVENT_CLASS_TYPE( Resize )
	private:
		unsigned int m_Width, m_Height;
	};

	//////////////////////////////////////////////////////////////////////////

	class EventDispatcher
	{
	public:
		EventDispatcher( Event& event )
			: m_Event( event )
		{
		}

		// F will be deduced by the compiler
		template<typename T, typename F>
		bool Dispatch( const F& func )
		{
			if( m_Event.GetEventType() == T::StaticType() )
			{
				m_Event.Handled = func( static_cast< T& >( m_Event ) );
				return true;
			}
			return false;
		}
	private:
		Event& m_Event;
	};
}