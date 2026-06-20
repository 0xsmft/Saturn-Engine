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
#include "NodeEditorTaskHandler.h"

#include "NodeTaskCache.h"

#include "NodeEditorBase.h"

namespace Saturn {

	void NodeEditorTaskHandler::ReleaseAll()
	{
		m_CurrentTask = nullptr;
		m_CurrentTaskIndex = 0;

		m_Tasks.clear();
		m_Locators.clear();
		m_EditorVariables.clear();
	}

	NodeEditorTaskHandler::~NodeEditorTaskHandler()
	{
		ReleaseAll();
	}

	void NodeEditorTaskHandler::Init( const NodeTaskCache& rCache )
	{
		m_Tasks = rCache.InstantiateNewTaskList( this );

		OnInit();
	}

	Ref<NodeEditorVariable> NodeEditorTaskHandler::GetVariable( UUID id )
	{
		auto itr = m_EditorVariables.find( id );
		return itr == m_EditorVariables.end() ? nullptr : itr->second;
	}

	Ref<NodeEditorVariable> NodeEditorTaskHandler::GetVariable( const std::string& rName )
	{
		auto itr = std::find_if( m_EditorVariables.begin(), m_EditorVariables.end(),
			[ rName ]( const auto& rKV )
		{
			return rKV.second->GetName() == rName;
		} );

		return itr == m_EditorVariables.end() ? nullptr : itr->second;
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
			if( ( m_CurrentTask->GetNodeFlags() & NodeFlags_ConstantEvaluated ) == 0 )
			{
				const auto status = m_CurrentTask->Tick( ts );
				if( status == NodeEditorTaskState::Completed )
				{
					m_CurrentTask = nullptr;
				}
			}
			else
				m_CurrentTask = nullptr;
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
