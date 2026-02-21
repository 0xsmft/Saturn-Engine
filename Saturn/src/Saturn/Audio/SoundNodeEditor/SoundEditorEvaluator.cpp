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
#include "SoundEditorEvaluator.h"

#include "Saturn/Audio/Sound.h"
#include "Saturn/Audio/SoundGroup.h"
#include "Saturn/Audio/AudioSystem.h"

#include "Nodes/SoundOutputNode.h"
#include "Nodes/SoundPlayerNode.h"
#include "Nodes/SoundRandomSoundNode.h"

#if !defined(SAT_DIST)
#include "Saturn/NodeEditor/UI/NodeEditor.h"
#include "Saturn/Asset/AssetManager.h"
#else
#include "Saturn/NodeEditor/NodeEditorBase.h"
#endif

namespace Saturn {

	SoundEditorEvaluator::SoundEditorEvaluator( const SoundEdEvaluatorInfo& rInfo )
		: m_Info( rInfo )
	{
		if( !m_Info.SoundGroup )
			m_Info.SoundGroup = AudioSystem::Get().GetMasterSoundGroup();
	}

	SoundEditorEvaluator::~SoundEditorEvaluator()
	{
		DestroyAliveSounds();
	}

	void SoundEditorEvaluator::SetTargetNodeEditor( SharedPtr<NodeEditorBase> nodeEditor )
	{
		m_NodeEditor = nodeEditor;
	}

	NodeEditorCompilationStatus SoundEditorEvaluator::EvaluateEditor()
	{
		m_Completed = false;

#if !defined( SAT_DIST )
		if( !m_NodeEditor )
			return NodeEditorCompilationStatus::Failed;

		SharedPtr<NodeEditor> uiEditor = m_NodeEditor.As<NodeEditor>();

		SharedPtr<NodeEditorNodeBase> OutputNode = m_NodeEditor->FindNode( m_Info.OutputNodeID );
		if( !OutputNode ) 
		{
			uiEditor->ThrowError( "Output node was not found!" );
			return NodeEditorCompilationStatus::Failed;
		}

		const UUID FinalSoundPinID = OutputNode->Inputs[ 0 ]->ID;

		// We must have something linked to the final output.
		if( !m_NodeEditor->IsLinked( FinalSoundPinID ) ) 
		{
			uiEditor->ThrowError( "No links are linked to the output node!" );
			return NodeEditorCompilationStatus::Failed;
		}

		// Clear pure dependencies
		AssetManager::Get()->UnregisterAllAssetDependencies( uiEditor->GetAssetID() );
#endif

		DestroyAliveSounds();

		return EvalNoChecks();
	}

	NodeEditorCompilationStatus SoundEditorEvaluator::EvalNoChecks()
	{
		SharedPtr<NodeEditorNodeBase> OutputNode = m_NodeEditor->FindNode( m_Info.OutputNodeID );

		// Stacks are last in first out, so our output node will be evaluated last which is what we want.
		std::stack<UUID> order;
		m_NodeEditor->TraverseFromStart( OutputNode, NodeEditorFlowDirection::Right,
			[ & ]( const UUID id )
		{
			order.push( id );
		} );

#if !defined( SAT_DIST )
		SharedPtr<NodeEditor> uiEditor = m_NodeEditor.As<NodeEditor>();
		if( order.size() <= 1 )
		{
			uiEditor->ThrowWarning( "There is no other nodes to compile! (The only node that exists is the output node!)" );
			return NodeEditorCompilationStatus::Failed;
		}

		size_t index = 0;
#endif

		auto compileResult = NodeEditorCompilationStatus::Success;
		while( !order.empty() )
		{
			const UUID currentNodeID = order.top();
			order.pop();

			SharedPtr<NodeEditorNodeBase> currentNode = m_NodeEditor->FindNode( currentNodeID );

			auto result = currentNode->EvaluateNode( this );
			if( result != NodeEvaluationState::Evaluated )
			{
				compileResult = NodeEditorCompilationStatus::Failed;
				break;
			}

#if defined( SAT_DEBUG )
			currentNode->EvaluationOrder = index++;
#endif
		}

		m_NodeEditor->SetState( compileResult == NodeEditorCompilationStatus::Success ? NodeEditorState::Simulating : NodeEditorState::Editing );

#if !defined(SAT_DIST)
		for( const auto& [id, state] : EvaluatedPath )
		{
			if( state == NodeEvaluationState::WasEvaluated )
			{
				PropagateNotEvaluated( m_NodeEditor->FindLink( id ), state );
			}
		}
#endif

		return compileResult;
	}

	void SoundEditorEvaluator::PropagateNotEvaluated( Ref<Link> node, NodeEvaluationState state )
	{
#if !defined(SAT_DIST)
		if( EvaluatedPath[ node->ID ] == NodeEvaluationState::WasEvaluated )
			return;

		EvaluatedPath[ node->ID ] = NodeEvaluationState::WasEvaluated;

		auto links = m_NodeEditor->FindLinksByPin( node->EndPinID );
		for( const auto& link : links )
		{
			auto endNode = m_NodeEditor->FindNodeByPin( link->StartPinID );
			if( endNode )
			{
				PropagateNotEvaluated( link, state );
			}
		}
#endif
	}

	void SoundEditorEvaluator::AddNewSound( UUID id, bool spatialisation )
	{
		Ref<Sound> snd;
		if( spatialisation )
		{
			snd = AudioSystem::Get().PlaySoundAtLocation( id, UUID(), { 0.0f, 0.0f, 0.0f }, false, nullptr );
		}
		else
		{
			snd = AudioSystem::Get().RequestNewSound( id, UUID(), false, nullptr );
		}

		snd->AddOnCompleteFunction( SAT_BIND_EVENT_FN( SoundEditorEvaluator::OnSoundCompleted ) );
		snd->WaitUntilLoaded();

		AliveSounds.push_back( snd );

#if !defined( SAT_DIST )
		AssetManager::Get()->RegisterAssetDependency( m_NodeEditor->GetAssetID(), id );
#endif
	}

	void SoundEditorEvaluator::RegisterSound( size_t id )
	{
		SoundsPlaying.insert( id );

#if defined(SAT_DEBUG) || defined(SAT_RELEASE)
		SAT_CORE_INFO( "[SoundEditorEvaluator] RegisterSound new sound with playing index/{0}", id );
#endif
	}

	void SoundEditorEvaluator::UnregisterSound( size_t id )
	{
		SoundsPlaying.erase( id );

#if defined(SAT_DEBUG) || defined(SAT_RELEASE)
		SAT_CORE_INFO( "[SoundEditorEvaluator] UnregisterSound sound with playing index/{0}", id );
#endif
	}

	void SoundEditorEvaluator::OnSoundCompleted( UUID PlayerID )
	{
		auto Itr = std::find_if( AliveSounds.begin(), AliveSounds.end(), 
		[PlayerID]( const auto& rSound ) 
		{
			return rSound->GetPlayerID() == PlayerID;
		} );

		if( Itr != AliveSounds.end() )
		{
			AliveSounds.erase( Itr );
		}

		if( SoundsPlaying.size() == 0 )
		{
			m_NodeEditor->SetState( NodeEditorState::Evaluating );
			if( m_Looping )
				EvalNoChecks();
			else
				m_Completed = true;
		}
	}

	void SoundEditorEvaluator::TerminateEvaluation()
	{
		DestroyAliveSounds();
		m_NodeEditor->SetState( NodeEditorState::Editing );
	}

	void SoundEditorEvaluator::TraceEvaluationPath()
	{
#if !defined(SAT_DIST)
		for( const auto& [id, state] : EvaluatedPath )
		{
			if( state == NodeEvaluationState::Evaluated )
			{
				m_NodeEditor->ShowFlow( id );
			}
		}
#endif
	}

	void SoundEditorEvaluator::DestroyAliveSounds()
	{
		for( auto& rSound : AliveSounds )
		{
			rSound->Stop();
			AudioSystem::Get().UnloadSound( rSound );

			rSound = nullptr;
		}

		AliveSounds.clear();
		SoundsPlaying.clear();

#if !defined(SAT_DIST)
		EvaluatedPath.clear();
#endif
	}

}
