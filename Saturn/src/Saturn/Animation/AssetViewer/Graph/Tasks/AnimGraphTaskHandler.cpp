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

#include "sppch.h"
#include "AnimGraphTaskHandler.h"

#include "GraphTask.h"
#include "AnimGraphPlayAnimTask.h"

#include "Saturn/Animation/AssetViewer/Graph/Animation/AnimGraph.h"

#include "Saturn/NodeEditor/NodeEditorNodeBase.h"
#include "Saturn/NodeEditor/NodeEditorBase.h"

#include "Saturn/Animation/Animator.h"

namespace Saturn {

	AnimGraphTaskHandler::AnimGraphTaskHandler( Ref<Animator> animator )
		: m_Animator( animator )
	{
	}

	void AnimGraphTaskHandler::InitWithCustomOrder2( SharedPtr<NodeEditor> nodeEditor, const IndexedMap<UUID, SGraphTask*>& rOrder )
	{
		for( auto& [id, pTask] : rOrder )
		{
			if( pTask )
			{
				pTask->InitialiseTask( this, nodeEditor.Get(), nullptr );

				m_Tasks.push_back( pTask );
			}
		}

#if defined(SAT_DEBUG)
		SAT_CORE_INFO( "=== ALL TASKS ===" );
		for( auto& [id, pTask] : rOrder )
		{
			SAT_CORE_INFO( "ID: {0} Class Name: {1}", ( uint64_t ) id, pTask->GetClass()->GetName() );
			for( const auto& subTask : pTask->GetTasks() )
			{
				if( subTask.pTask )
				{
					SAT_CORE_INFO( " -> {0}", subTask.pTask->GetClass()->GetName() );
				}
				else
					SAT_CORE_INFO( " -> <NULL>" );
			}
		}
		SAT_CORE_INFO( "=================" );
#endif
	}

	void AnimGraphTaskHandler::Tick( Timestep ts )
	{
		if( m_CurrentTask == nullptr )
		{
			if( m_CurrentTaskIndex + 1 > m_Tasks.size() )
			{
				// At end, restart from the root.
				ResetAllTasks();

				SAT_CORE_INFO( "Task Handler, completed" );
			}
			else
			{
				m_CurrentTask = m_Tasks.at( m_CurrentTaskIndex++ );
			}
		}

		if( m_CurrentTask )
		{
			const auto status = m_CurrentTask->Tick( ts );
			switch( status )
			{
				// Completed? move on to the next sub-graph.
				case NodeEditorTaskState::Completed:
				{
					m_CurrentTask = nullptr;
				} break;

				default:
					break;
			}
		}
	}

	Ref<Animator> AnimGraphTaskHandler::GetAnimator() const
	{
		return m_Animator;
	}

}
