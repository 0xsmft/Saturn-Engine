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
#include "BehaviourTree.h"

#include "AssetViewer/Nodes/BehaviourTreeSequenceNode.h"
#include "AssetViewer/BehaviourTreeNodeEditor.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"
#include "Saturn/NodeEditor/UI/NodeEditor.h"
#include "Saturn/NodeEditor/Serialisation/NodeCache.h"

namespace Saturn {

	BehaviourTree::BehaviourTree( AssetID id )
		: m_BehaviourTreeAsset( AssetManager::Get().FindAsset( id ) )
	{
	}

	BehaviourTree::~BehaviourTree()
	{
		if( m_NodeEditor )
			m_NodeEditor->SetRuntime( nullptr );

		m_Runtime = nullptr;
		m_NodeEditor = nullptr;
	}

	void BehaviourTree::Initialise( Ref<AIAgentEntity> entity )
	{
#if defined(SAT_DIST)
		m_NodeEditor = Ref<BehaviourTreeNodeEditor>::Create( m_BehaviourTreeAsset->ID );
#else
		m_NodeEditor = Ref<BehaviourTreeNodeEditor>::Create( m_BehaviourTreeAsset->ID );
		m_NodeEditor->SetPrivileges( NodeEditorPrivileges::All, false );
		m_NodeEditor->SetPrivileges( NodeEditorPrivileges::ReadOnly, true );
		m_NodeEditor->SetPrivileges( NodeEditorPrivileges::Editing, false );
#endif

		std::string filename = std::format( "{0}.sbt", m_BehaviourTreeAsset->Name );
		if( NodeCacheEditor::ReadNodeEditorCache( m_NodeEditor, m_BehaviourTreeAsset->ID, filename ) )
		{
			m_OutputNodeID = m_NodeEditor->FindNode( "Root Node" )->ID;
		}
		else
		{
			SAT_CORE_WARN( "Failed to read node editor, using empty behaviour tree" );
			SAT_CORE_ASSERT( false );
		}

		BehaviourTreeEditorEvaluator::BehaviourTreeEdEvaluatorInfo info{};
		info.OutputNodeID = m_OutputNodeID;

		m_Runtime = Ref<BehaviourTreeEditorEvaluator>::Create( info );
		m_Runtime->SetTargetNodeEditor( m_NodeEditor );

		m_NodeEditor->SetRuntime( m_Runtime );

		m_AIAgentEntity = entity;
		m_NodeEditor->SetTargetAgent( m_AIAgentEntity );

		// Convert nodes into tasks.
		m_NodeEditor->InitBehaviourTree();
	
#if !defined(SAT_DIST)
		m_NodeEditor->SetState( NodeEditorState::Simulating );
#endif
	}

	void BehaviourTree::FirstEvaluate()
	{
		m_NodeEditor->Evaluate();
	}

	void BehaviourTree::Tick( Timestep ts )
	{
		m_NodeEditor->Tick( ts );
	}

}
