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
#include "NodeTaskCache.h"

#include "NodeEditorBase.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"
#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include "Saturn/Animation/AssetViewer/Graph/Tasks/GraphTask.h"
#include "Saturn/Animation/AssetViewer/AnimGraphTaskAndNodeInfo.h"

#include "NodeEditorTaskHandler.h"

namespace Saturn {

	NodeTaskCache::NodeTaskCache()
	{
	}

	NodeTaskCache::~NodeTaskCache()
	{
		m_Tasks.clear();
	}

#if !defined(SAT_DIST)
	void NodeTaskCache::BuildMasterList( const std::vector<SharedPtr<NodeEditorNodeBase>>& rOrder, bool cacheVariables )
	{
		Clear();

		for( const auto& rNode : rOrder )
		{
			auto* pTask = rNode->ConvertToTask();
			SAT_CORE_ASSERT( pTask, "ConvertToTask returned null! If such node does not have a task it should not be included in the task cache order list!" );

			pTask->PreInitialiseTask( ( NodeEditor* ) rNode->GetParentObject(), rNode.Get() );

			// Converted to Ref<>!
			m_Tasks.emplace_back( pTask );
		}
	}

	void NodeTaskCache::BuildMasterListForAnimGraph( NodeEditor* pEditor, const IndexedMap<UUID, AnimGraphNodeAndTaskInfo>& rOrder, bool cacheVariables )
	{
		Clear();

		for( const auto& [rID, rInfo] : rOrder )
		{
			rInfo.pGraphTask->PreInitialiseTask( pEditor, rInfo.Node.Get() );

			m_Tasks.emplace_back( rInfo.pGraphTask );
		}

		if( cacheVariables )
			CacheVariables( pEditor );
	}

	void NodeTaskCache::Clear()
	{
		m_Tasks.clear();
	}
#endif

	NodeTaskCache::NodeTaskCacheMap NodeTaskCache::InstantiateNewTaskList( NodeEditorTaskHandler* pHandler ) const
	{
		// 1. Create variables.
		pHandler->m_EditorVariables.reserve( m_EditorVariables.size() );

		for( const auto& [ID, variable] : m_EditorVariables )
		{
			// Copy variable into var.
			pHandler->m_EditorVariables.emplace( ID, Ref<NodeEditorVariable>::Create( variable.Get() ) );
		}

		// 2. Create new tasks
		NodeTaskCache::NodeTaskCacheMap map;
		map.reserve( m_Tasks.size() );

		for( const auto& rTask : m_Tasks )
		{
			NodeEditorTaskBase* pObject = ( NodeEditorTaskBase* ) ClassMetadataHandler::Get().CreateClassObject( rTask->GetClass()->GetHash(), ( SObject* ) pHandler );

			if( pHandler )
			{
				pObject->InitialiseTaskWithOther( pHandler, ( NodeEditorTaskBase* )rTask.Get() );
			}

			map.emplace_back( pObject );
		}

		return map;
	}

	void NodeTaskCache::CacheVariables( NodeEditor* pEditor )
	{
		const auto& rVariablesMap = pEditor->GetVariables();
		m_EditorVariables.reserve( rVariablesMap.size() );

		for( const auto& [id, variable] : rVariablesMap )
		{
			m_EditorVariables.emplace( id, Ref<NodeEditorVariable>::Create( variable.Get() ) );
		}
	}

}
