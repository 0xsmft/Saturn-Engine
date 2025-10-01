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
#include "BehaviourTreeWaitTask.h"

#include "Saturn/AI/BehaviourTree/AssetViewer/Nodes/BehaviourTreeNodeBase.h"

#if !defined(SAT_DIST)
#include "Saturn/ImGui/ImGuiAuxiliary.h"
#endif

namespace Saturn {

	BehaviourTreeWaitTask::BehaviourTreeWaitTask( float WaitDuration )
		: m_WaitDuration( WaitDuration )
	{
	}

	BehaviourTreeWaitTask::BehaviourTreeWaitTask( UUID WaitDurationVarID )
		: m_WaitDuration( -1.0f )
	{
		m_RTBlackboardVariableID = WaitDurationVarID;
	}

	void BehaviourTreeWaitTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		if( pNode )
			m_NodeID = pNode->ID;
	}

	BehaviourTreeWaitTask::~BehaviourTreeWaitTask()
	{
	}

	NodeEditorTaskState BehaviourTreeWaitTask::Tick( Timestep ts )
	{
		if( !m_Started )
		{
			m_StartTime = std::chrono::steady_clock::now();
			m_Started = true;

			const auto waitTimeBB = TryRetrieveBBKey<float>( m_RTBlackboardVariableID );
			if( waitTimeBB.has_value() )
			{
				m_WaitDuration = waitTimeBB.value_or( m_WaitDuration );
			}

			SAT_CORE_INFO( "[BehaviourTreeWaitTask] Wait started, will be awaiting for: {0} seconds", m_WaitDuration );

			m_CurrentState = NodeEditorTaskState::Starting;
			return m_CurrentState;
		}

		const auto now = std::chrono::steady_clock::now();
		const std::chrono::duration<float> elasped = now - m_StartTime;

		if( elasped.count() >= m_WaitDuration )
		{
			m_Started = false;
			SAT_CORE_INFO( "[BehaviourTreeWaitTask] Wait ended" );

			m_CurrentState = NodeEditorTaskState::Completed;
			return m_CurrentState;
		}

		// Still waiting
		m_CurrentState = NodeEditorTaskState::Running;
		return m_CurrentState;
	}

	void BehaviourTreeWaitTask::Reset()
	{
		m_Started = false;
	}

#if !defined(SAT_DIST)
	void BehaviourTreeWaitTask::OnRenderExtra()
	{
		ImGui::Text( "%.2fs", m_WaitDuration );
	}

	void BehaviourTreeWaitTask::RenderDetails()
	{
		Auxiliary::DrawFloatControl( "Wait duration", m_WaitDuration );
	}
#endif

	void BehaviourTreeWaitTask::Serialise( std::ofstream& rStream ) const
	{
		RawSerialisation::WriteObject( m_WaitDuration, rStream );
	}

	void BehaviourTreeWaitTask::Deserialise( FDependentIStream& rStream )
	{
		RawSerialisation::ReadObject( m_WaitDuration, rStream );
	}

}
