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

#include "RuntimeState.h"
#include "Saturn/Core/Event.h"

namespace Saturn {

	// Runtime events happen during playtime!
	// So for example a scene travel happens during runtime

	class SceneTravelEvent : public Event
	{
		SAT_DEFINE_EVENT( SceneTravel, EC_Runtime );
	public:
		SceneTravelEvent( unsigned long long id )
			: Event( EventType::SceneTravel, EC_Runtime ), m_NewSceneID( id )
		{
		}

		virtual ~SceneTravelEvent() = default;

		[[nodiscard]] inline unsigned long long GetID() const { return m_NewSceneID; }

	private:
		unsigned long long m_NewSceneID = 0;
	};

	class RuntimeStateChangedEvent : public Event
	{
		SAT_DEFINE_EVENT( RuntimeStateChanged, EC_Runtime );
	public:
		RuntimeStateChangedEvent( RuntimeState newState, RuntimeState oldState )
			: Event( EventType::RuntimeStateChanged, EC_Runtime ), m_CurrentState( newState ), m_OldState( oldState )
		{
		}

		virtual ~RuntimeStateChangedEvent() = default;

		[[nodiscard]] inline RuntimeState GetNewState() const { return m_CurrentState; }
		[[nodiscard]] inline RuntimeState GetOldState() const { return m_OldState; }

	private:
		RuntimeState m_CurrentState = RuntimeState::NoState;
		RuntimeState m_OldState = RuntimeState::NoState;
	};

}
