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
#include "BehaviourTree.h"

#include "BehaviourTreeTaskHandler.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/NodeEditor/Serialisation/NodeCache.h"
#include "Saturn/NodeEditor/GlobalNodeEditorTaskCache.h"

namespace Saturn {

	BehaviourTree::BehaviourTree( AssetID id )
		: m_BehaviourTreeAsset( AssetManager::Get()->FindAsset( id ) )
	{
	}

	BehaviourTree::~BehaviourTree()
	{
		m_TaskHandler->ReleaseAll();
		m_TaskHandler = nullptr;
	}

	void BehaviourTree::Initialise( SharedPtr<AIAgentEntity> entity )
	{
		m_TaskHandler = Ref<BehaviourTreeTaskHandler>::Create();

		// Try to load without touching the disk...
		auto& rCache = GlobalNodeEditorTaskCache::Get().GetOrCreateTaskCache( m_BehaviourTreeAsset->ID );
		if( rCache.IsListEmpty() )
		{
			// ...otherwise load it from disk.
			const std::string filename = std::format( "{0}.sbt", m_BehaviourTreeAsset->Name );
			NodeCacheEditor::ReadNodeTaskCacheOnly( rCache, m_BehaviourTreeAsset->ID, filename );
		}

		m_TaskHandler->SetAgent( entity.Get() );
		m_TaskHandler->Init( rCache );

#if !defined(SAT_DIST)
		m_DebugName = std::format( "{0}/{1} ({2})", m_BehaviourTreeAsset->Name, entity->GetName(), ( uint64_t ) entity->GetUUID() );
#endif
	}

	void BehaviourTree::Tick( Timestep ts )
	{
		m_TaskHandler->Tick( ts );
	}

}
