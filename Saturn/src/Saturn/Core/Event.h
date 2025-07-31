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

#define SAT_DEFINE_EVENT( Type, Category ) \
public: \
static inline EventType GetStaticType()     { return EventType::Type; } \
static inline EventCategory GetStaticCategory() { return EventCategory::Category; } \
	   inline EventCategory GetCategory()       { return EventCategory::Category; }

	enum class EventType
	{
		None,

		//////////////////////////////////////////////////////////////////////////
		// RubyEventType

		Resize,
		Close,
		MouseMoved,
		MousePressed,
		MouseReleased,
		MouseEnterWindow,
		MouseLeaveWindow,
		MouseScroll,
		KeyReleased,
		KeyPressed,
		KeyHeld, // Dispatches as KeyPressed
		InputCharacter,
		WindowMaximized,
		WindowMinimized,
		WindowRestored,
		WindowMoved,
		WindowFocus,
		DisplayChanged,

		//////////////////////////////////////////////////////////////////////////
		// Editor

		HotReload,

		//////////////////////////////////////////////////////////////////////////
		// Runtime

		SceneTravel,
	};

	enum EventCategory
	{
		EC_None    = 1 << 0,
		EC_Ruby    = 1 << 1, // Window, mouse, keyboard events
		EC_Editor  = 1 << 2,
		EC_Runtime = 1 << 3,
		EC_Scene   = 1 << 4
	};

	class Event 
	{
	public:
		Event() = default;
		Event( EventType type, EventCategory eventCategory ) : Type( type ), Category( eventCategory ) {}
		virtual ~Event() = default;

		EventType Type = EventType::None;
		EventCategory Category = EC_None;
		bool Handled = false;
	};
}
