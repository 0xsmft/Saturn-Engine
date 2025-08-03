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

#include "sppch.h"
#include "PlayerInputController.h"

#include "Saturn/Core/OptickProfiler.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Core/App.h"
#include "Saturn/Core/Ruby/RubyWindow.h"
#include "Saturn/Core/Ruby/RubyAuxiliary.h"

namespace Saturn {

	PlayerInputController::PlayerInputController()
	{
	}

	PlayerInputController::~PlayerInputController()
	{
		m_ActionMap.clear();
	}

	static std::unordered_map<EventType, ActionBindingTriggerState> s_EventTypeToActionType
	{
		{ EventType::KeyPressed,    ActionBindingTriggerState::Pressed  },
		{ EventType::KeyReleased,   ActionBindingTriggerState::Released },
		{ EventType::MousePressed,  ActionBindingTriggerState::Pressed  },
		{ EventType::MouseReleased, ActionBindingTriggerState::Released }
	};

	void PlayerInputController::UpdateKeyState( const RubyKeyEvent& rEvent )
	{
		if( !m_ActionMap.size() )
			return;

		std::vector<ActionBinding> EventsToFire;

		// Traverse the action map directly.
		for( const auto& [name, bindings] : m_ActionMap )
		{
			for( const auto& rBinding : bindings )
			{
				if( rBinding.Type != ActionBindingType::Key ) continue;

				if( rBinding.Key == rEvent.GetKeycode() && rBinding.State == s_EventTypeToActionType[ rEvent.Type ] )
				{
					EventsToFire.push_back( rBinding );
				}
			}
		}

		// Trigger events.
		for( const auto& rAction : EventsToFire )
		{
			if( rAction.Function )
				( rAction.Function )();
		}
	}

	void PlayerInputController::UpdateMouseState( const RubyMouseEvent& rEvent )
	{
		if( !m_ActionMap.size() )
			return;

		std::vector<ActionBinding> EventsToFire;

		// Traverse the action map directly.
		for( const auto& [name, bindings] : m_ActionMap )
		{
			for( const auto& rBinding : bindings )
			{
				if( rBinding.Type != ActionBindingType::Mouse ) continue;

				if( rBinding.MouseButton == ( RubyMouseButton ) rEvent.GetButton() && rBinding.State == s_EventTypeToActionType[ rEvent.Type ] )
				{
					EventsToFire.push_back( rBinding );
				}
			}
		}

		// Trigger events.
		for( const auto& rAction : EventsToFire )
		{
			if( rAction.Function )
				( rAction.Function )( );
		}
	}

	void PlayerInputController::BindAction( const std::string& rBindingName, ActionBindingTriggerState state, const ActionFunction& rFunction )
	{
		Ref<Project> project = Project::GetActiveProject();
		const auto& bindings = project->GetActionBindings();

		// Check if the binding exists in the project
		const auto it = std::find_if( bindings.begin(), bindings.end(),
			[rBindingName]( const auto& binding )
			{
				return binding.Name == rBindingName;
			} );

		if( it != bindings.end() )
		{
			const auto& rBindingData = *( it );
			
			m_ActionMap[ rBindingName ].emplace_back( rBindingData, state, rFunction );
		}
	}

	void PlayerInputController::RemoveAction( const std::string& rBindingName )
	{
		m_ActionMap.erase( rBindingName );
	}

}
