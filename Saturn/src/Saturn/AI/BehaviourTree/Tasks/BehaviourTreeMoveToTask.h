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

#include "BehaviourTreeBaseTask.h"

#include "Saturn/AI/Navigation/StraightNavPath.h"

namespace Saturn {

	class AIAgentEntity;

	SCLASS()
	class BehaviourTreeMoveToTask : public BehaviourTreeBaseTask
	{
		SAT_DECLARE_CLASS_MOVE( BehaviourTreeMoveToTask, BehaviourTreeBaseTask )
	public:
		BehaviourTreeMoveToTask() = default;

		BehaviourTreeMoveToTask( const glm::vec3& rTargetPosition );
		virtual ~BehaviourTreeMoveToTask();

		virtual void InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pEditor, NodeEditorNodeBase* pNode ) override;
		virtual NodeEditorTaskState Tick( Timestep ts ) override;
		virtual void Reset() override;

#if !defined(SAT_DIST)
		[[nodiscard]] virtual bool IsSpawnableNode() const { return true; }
		virtual const char* GetTaskName() const { return "Move To"; }
#endif

	private:
		// #ReplaceRawPtrOrRefWithWeakRef, for now it's a raw ptr
		AIAgentEntity* m_Agent = nullptr;

		glm::vec3 m_TargetPosition{};
		StraightNavPath m_Path{};

	private:
		[[nodiscard]] NodeEditorTaskState InitPathTo();
		[[nodiscard]] NodeEditorTaskState WalkToNextWaypoint( Timestep ts );
	};
	
}
