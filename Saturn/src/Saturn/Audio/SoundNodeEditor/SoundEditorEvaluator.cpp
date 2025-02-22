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
#include "SoundEditorEvaluator.h"

#include "Saturn/Audio/Sound.h"
#include "Saturn/Audio/SoundGroup.h"
#include "Saturn/Audio/AudioSystem.h"

#include "Nodes/SoundOutputNode.h"
#include "Nodes/SoundPlayerNode.h"
#include "Nodes/SoundRandomNode.h"

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

	void SoundEditorEvaluator::SetTargetNodeEditor( Ref<NodeEditorBase> nodeEditor )
	{
		m_NodeEditor = nodeEditor;
	}

	NodeEditorCompilationStatus SoundEditorEvaluator::EvaluateEditor()
	{
#if !defined( SAT_DIST )
		if( !m_NodeEditor )
			return NodeEditorCompilationStatus::Failed;

		Ref<NodeEditor> uiEditor = m_NodeEditor.As<NodeEditor>();

		Ref<Node> OutputNode = m_NodeEditor->FindNode( m_Info.OutputNodeID );
		if( !OutputNode ) 
		{
			uiEditor->ThrowError( "Output node was not found!" );
			return NodeEditorCompilationStatus::Failed;
		}

		UUID FinalSoundPinID = OutputNode->Inputs[ 0 ]->ID;

		// We must have something linked to the final output.
		if( !m_NodeEditor->IsLinked( FinalSoundPinID ) ) 
		{
			uiEditor->ThrowError( "No links are linked to the output node!" );
			return NodeEditorCompilationStatus::Failed;
		}

		// Clear pure dependencies
		AssetManager::Get().UnregisterAllAssetDependencies( uiEditor->GetAssetID() );
#else
		Ref<Node> OutputNode = m_NodeEditor->FindNode( m_Info.OutputNodeID );
		UUID FinalSoundPinID = OutputNode->Inputs[ 0 ]->ID;
#endif

		DestroyAliveSounds();

		// Stacks are last in first out, so our output node will be evaluated last which is what we want.
		std::stack<UUID> order;
		m_NodeEditor->TraverseFromStart( OutputNode,
			[&]( const UUID id )
			{
				order.push( id );
			} );

#if !defined( SAT_DIST )
		if( order.size() <= 1 ) 
		{
			uiEditor->ThrowWarning( "There is no other nodes to compile! (The only node that exists is the output node!)" );
			return NodeEditorCompilationStatus::Failed;
		}
#endif

		auto compileResult = NodeEditorCompilationStatus::Success;
		while( !order.empty() )
		{
			const UUID currentNodeID = order.top();
			order.pop();

			Ref<Node> currentNode = m_NodeEditor->FindNode( currentNodeID );

			if( auto result = currentNode->EvaluateNode( this ); result != NodeEditorCompilationStatus::Success )
			{
				compileResult = NodeEditorCompilationStatus::Failed;
				break;
			}	
		}

		return compileResult;
	}

	void SoundEditorEvaluator::AddNewSound( UUID id )
	{
		Ref<Sound> snd = AudioSystem::Get().RequestNewSound( id, UUID(), false, nullptr );
		snd->WaitUntilLoaded();

		AliveSounds.push_back( snd );

#if !defined( SAT_DIST )
		AssetManager::Get().RegisterAssetDependency( m_NodeEditor->GetAssetID(), id );
#endif
	}

	void SoundEditorEvaluator::RegisterSound( size_t id )
	{
		SoundsPlaying.insert( id );
	}

	void SoundEditorEvaluator::UnregisterSound( size_t id )
	{
		SoundsPlaying.erase( id );
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
	}

}