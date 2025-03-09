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

#include "Saturn/Core/Ref.h"
#include "Saturn/Core/Ruby/RubyEventType.h"

#include "ActionBinding.h"

#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace Saturn {

	class RubyKeyEvent;
	class RubyMouseEvent;

    // PlayerInputController
	// Handles player input actions and key/mouse state updates.
    // 
    // This class manages input bindings from the project and allows functions to be assigned to specific actions.
	class PlayerInputController : public RefTarget
	{
	public:
		// Type alias for an action function. (std::function<void()>)
		using ActionFunction = std::function<void()>;
	public:
		PlayerInputController();
		~PlayerInputController();

		// Binds an action to a function with a specific trigger state.
		//
		// @param rBindingName The name of the action binding, this much match with the one in the project settings
		// @param state The trigger state of the action.
		// @param rFunction The function to execute when the action is triggered.
		void BindAction( const std::string& rBindingName, ActionBindingTriggerState state, const ActionFunction& rFunction );

		// Removes a previously bound action.
		//
		// @param rBindingName The name of the action binding, this much match with the one in the project settings
		void RemoveAction( const std::string& rBindingName );

	public:
		// Public internal functions
		// Do not call
		void UpdateKeyState( const RubyKeyEvent& rEvent );
		void UpdateMouseState( const RubyMouseEvent& rEvent );

	private:
		// BINDING NAME -> BINDINGS
		std::unordered_map<std::string, std::vector<ActionBinding>> m_ActionMap;

	private:
		friend class Character;
	};
}