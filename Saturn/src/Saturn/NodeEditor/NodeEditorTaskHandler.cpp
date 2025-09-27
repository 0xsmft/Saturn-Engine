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
#include "NodeEditorTaskHandler.h"

#include "NodeEditorBase.h"

namespace Saturn {

	void NodeEditorTaskHandler::Init( SharedPtr<NodeEditorBase> nodeEditor )
	{
		for( const auto& [id, rNode] : nodeEditor->GetNodes() )
		{
			NodeEditorTaskBase* pTask = rNode->ConvertToTask();
			if( pTask )
			{
				pTask->InitialiseTask( nodeEditor.Get(), rNode.Get() );

				m_Tasks.push_back( pTask );
			}
		}
	}

	void NodeEditorTaskHandler::ResetAllTasks()
	{
		for( auto& pTask : m_Tasks )
		{
			pTask->Reset();
		}

		m_CurrentTask = nullptr;
		m_CurrentTaskIndex = 0;
	}

	void NodeEditorTaskHandler::Tick( Timestep ts )
	{
		if( m_CurrentTask )
		{
			const auto status = m_CurrentTask->Tick( ts );
			if( status == NodeEditorTaskState::Completed ) 
			{
				m_CurrentTask = nullptr;
			}
		}

		if( m_CurrentTask == nullptr )
		{
			if( m_CurrentTaskIndex + 1 > m_Tasks.size() )
			{
				// At end, restart from the root.
				ResetAllTasks();
			}
			else
			{
				m_CurrentTask = m_Tasks.at( m_CurrentTaskIndex++ );
			}
		}
	}

}
