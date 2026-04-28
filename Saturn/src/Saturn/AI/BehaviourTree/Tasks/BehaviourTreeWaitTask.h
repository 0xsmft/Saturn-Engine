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

#include <chrono>

namespace Saturn {

	SCLASS()
	class BehaviourTreeWaitTask : public BehaviourTreeBaseTask
	{
		SAT_DECLARE_CLASS_MOVE( BehaviourTreeWaitTask, BehaviourTreeBaseTask )
	public:
		BehaviourTreeWaitTask() = default;

		BehaviourTreeWaitTask( float WaitDuration );
		BehaviourTreeWaitTask( UUID WaitDurationVarID );

		virtual ~BehaviourTreeWaitTask();

#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode ) override;
#endif
		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

		virtual NodeEditorTaskState Tick( Timestep ts ) override;
		virtual void Reset() override;

#if !defined(SAT_DIST)
		[[nodiscard]] virtual bool IsSpawnableNode() const { return true; }
		virtual const char* GetTaskName() const { return "Wait"; }
		virtual void OnRenderExtra() override;
		virtual void RenderDetails() override;
#endif

	public:
		virtual void Serialise( std::ofstream& rStream ) const;
		virtual void Deserialise( FDependentIStream& rStream );

	private:
		// Wait time in seconds
		float m_WaitDuration = 0.0f;

		bool m_Started = false;
		std::chrono::steady_clock::time_point m_StartTime;
	};
	
}
