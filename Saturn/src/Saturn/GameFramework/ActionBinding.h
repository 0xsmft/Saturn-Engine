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

#include "Saturn/Core/UUID.h"

#include <string>
#include <functional>
#include "Saturn/Core/Ruby/RubyEventType.h"

namespace Saturn {

	enum class ActionBindingTriggerState
	{
		Pressed = BIT( 0 ),
		Released = BIT( 1 ),
	};

	enum class ActionBindingType 
	{
		Key,
		Mouse
	};

	// @class ActionBindingData
	//
	// @breif This struct can be thought of as a specification for the real ActionBindings it contains the name, type, Key, MouseButton, ActionBindingData are owned and created by the current Project.
	//
	// In Development configurations:
	// - This struct will hold a special RenderID, and an ActionName
	// In Distribution configurations:
	// - RenderID and ActionName are stripped out.
	struct ActionBindingData
	{
		std::string Name = "";
		ActionBindingType Type = ActionBindingType::Key;

		RubyKey Key = RubyKey::UnknownKey;
		RubyMouseButton MouseButton = RubyMouseButton::Unknown;

#if !defined(SAT_DIST)
		// The name of the Key/Mouse button
		std::string ActionName = "";
		// RenderID
		UUID ID;
#endif

		bool operator==( const ActionBindingData& rOther ) 
		{
			return Name == rOther.Name
				&& Type == rOther.Type
				&& Key == rOther.Key
				&& MouseButton == rOther.MouseButton
#if !defined(SAT_DIST)
				&& ID == rOther.ID;
#else
				;
#endif
		}
	};

	// @class ActionBinding
	//
	// @breif An ActionBinding is an event that triggers based on certain conditions for example, if the 'A' key is pressed, the project will search for any ActionBinding that matches. Unlike ActionBindingData, ActionBindings are created by the PlayerInputController. A ActionBinding is defined by it's ActionBindingData.
	//
	struct ActionBinding
	{
		template<typename F>
		ActionBinding( const ActionBindingData& rActionBindingData, ActionBindingTriggerState state, F&& rrFunc )
			: Type( rActionBindingData.Type ), 
			Key( rActionBindingData.Key ), 
			MouseButton( rActionBindingData.MouseButton ), State( state ), Function( std::move( rrFunc ) )
		{
		}

		ActionBindingType Type = ActionBindingType::Key;
		RubyKey Key = RubyKey::UnknownKey;
		RubyMouseButton MouseButton = RubyMouseButton::Unknown;
		ActionBindingTriggerState State = ActionBindingTriggerState::Pressed;
		std::function<void()> Function = nullptr;
	};
}
