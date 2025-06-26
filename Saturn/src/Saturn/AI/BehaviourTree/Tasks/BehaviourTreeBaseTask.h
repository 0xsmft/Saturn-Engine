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

#include "Saturn/Core/Base.h"
#include "Saturn/GameFramework/Core/GameScript.h"
#include "Saturn/Core/UUID.h"

namespace Saturn {

	class BehaviourTreeNodeBase;
	class BehaviourTreeNodeEditor;

	enum class BehaviourTreeTaskState 
	{
		Unknown,
		Starting,
		Running,
		Completed, // success flag
		Failed
	};

	class BehaviourTreeBaseTask : public RefTarget
	{
		SAT_DECLARE_CLASS_NO_INTER( BehaviourTreeBaseTask, RefTarget )
	public:
		BehaviourTreeBaseTask() = default;
		virtual ~BehaviourTreeBaseTask() = default;

		virtual void InitialiseTask( BehaviourTreeNodeEditor* pEditor, BehaviourTreeNodeBase* pNode ) {}
		virtual BehaviourTreeTaskState Tick( Timestep ts ) { return BehaviourTreeTaskState::Unknown; }
		virtual void Reset() {}

		Saturn::UUID GetNodeID() const { return m_NodeID; }
		BehaviourTreeTaskState GetState() const { return m_CurrentState; }

		void SetBlackboard( Ref<BehaviourTreeMemory> mem );

	protected:
		template<typename CppType>
		std::optional<CppType> TryRetrieveBBKey( Saturn::UUID variableID )
		{
			return m_Blackboard->Get<CppType>( variableID );
		}

	protected:
		Saturn::UUID m_NodeID = 0;
		BehaviourTreeTaskState m_CurrentState = BehaviourTreeTaskState::Unknown;

		Ref<BehaviourTreeMemory> m_Blackboard;
		Saturn::UUID m_BlackboardVariableID = 0;
	};
}
