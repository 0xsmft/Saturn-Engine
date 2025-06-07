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
#include "BehaviourTreeTaskNodes.h"

#include "Saturn/Audio/AudioSystem.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"

#include "Saturn/Serialisation/RawSerialisation.h"

// Tasks
#include "Saturn/AI/BehaviourTree/Tasks/BehaviourTreeWaitTask.h"
#include "Saturn/AI/BehaviourTree/Tasks/BehaviourTreePlaySoundTask.h"
#include "Saturn/AI/BehaviourTree/Tasks/BehaviourTreeMoveToTask.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// WAIT NODE

	BehaviourTreeWaitNode::BehaviourTreeWaitNode()
		: BehaviourTreeNodeBase( "Wait" )
	{
		CreateNode();
	}

	void BehaviourTreeWaitNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::BehaviourTreeWaitNode;

#if !defined(SAT_DIST)
		Color = ImColor( 48, 128, 255, 100 );
		Type = NodeRenderType::Tree;
#endif

		Inputs.push_back( Ref<Pin>::Create( "In", PinType::Flow, PinKind::Input ) );

		for( auto& rInput : Inputs )
		{
			rInput->RenderType = PinRenderType::Tree;
		}
	}

	BehaviourTreeWaitNode::~BehaviourTreeWaitNode()
	{
	}

	NodeEvaluationState BehaviourTreeWaitNode::EvaluateNode( NodeEditorRuntime* pEvaluator )
	{
		if( WaitDuration > 0.0f )
		{
			return NodeEvaluationState::Evaluated;
		}

		return NodeEvaluationState::Failed;
	}

	void BehaviourTreeWaitNode::OnSerialise( std::ofstream& rStream ) const
	{
		RawSerialisation::WriteObject( WaitDuration, rStream );
	}

	void BehaviourTreeWaitNode::OnDeserialise( IStream& rStream )
	{
		RawSerialisation::ReadObject( WaitDuration, rStream );
	}
	
	BehaviourTreeBaseTask* BehaviourTreeWaitNode::ConvertToTask()
	{
		return new BehaviourTreeWaitTask( WaitDuration );
	}

#if !defined(SAT_DIST)
	void BehaviourTreeWaitNode::RenderDetails()
	{
		Auxiliary::DrawFloatControl( "Wait duration", WaitDuration );
	}

	void BehaviourTreeWaitNode::OnRenderExtra()
	{
		ImGui::Text( "%.2fs", WaitDuration );
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// PLAY SOUND NODE

	BehaviourTreePlaySoundNode::BehaviourTreePlaySoundNode()
		: BehaviourTreeNodeBase( "Play Sound" )
	{
		CreateNode();
	}

	void BehaviourTreePlaySoundNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::BehaviourTreePlaySoundNode;

#if !defined(SAT_DIST)
		Color = ImColor( 48, 128, 255, 100 );
		Type = NodeRenderType::Tree;
#endif

		Inputs.push_back( Ref<Pin>::Create( "In", PinType::Flow, PinKind::Input ) );

		for( auto& rInput : Inputs )
		{
			rInput->RenderType = PinRenderType::Tree;
		}
	}

	BehaviourTreePlaySoundNode::~BehaviourTreePlaySoundNode()
	{
	}

	NodeEvaluationState BehaviourTreePlaySoundNode::EvaluateNode( NodeEditorRuntime* pEvaluator )
	{
		if( m_SoundID != 0 )
		{
			return NodeEvaluationState::Evaluated;
		}

		return NodeEvaluationState::Failed;
	}

	void BehaviourTreePlaySoundNode::OnSerialise( std::ofstream& rStream ) const
	{
		RawSerialisation::WriteObject( m_SoundID, rStream );
	}

	void BehaviourTreePlaySoundNode::OnDeserialise( IStream& rStream )
	{
		RawSerialisation::ReadObject( m_SoundID, rStream );
	}

	BehaviourTreeBaseTask* BehaviourTreePlaySoundNode::ConvertToTask()
	{
		return new BehaviourTreePlaySoundTask( m_SoundID );
	}

#if !defined(SAT_DIST)
	void BehaviourTreePlaySoundNode::RenderDetails()
	{
		bool open = false;

		ImGui::Text( "Sound Specification" );

		if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24, 24 ) ) )
		{
			open = true;
		}

		ImGui::SameLine();

		if( Auxiliary::DrawAssetFinder( { AssetType::GraphSound, AssetType::Sound }, &open, m_SoundID ) )
		{
			m_SoundID = m_SoundID;
		}

		ImGui::PushID( ( int ) m_SoundID );

		if( m_SoundID != 0 )
			ImGui::InputText( "##2dplayerid", ( char* ) std::to_string( m_SoundID ).c_str(), 256, ImGuiInputTextFlags_ReadOnly );
		else
			ImGui::InputText( "##2dplayerid", ( char* ) "", 256, ImGuiInputTextFlags_ReadOnly );

		ImGui::PopID();
	}

	void BehaviourTreePlaySoundNode::OnRenderExtra()
	{
		ImGui::Text( "%llu", m_SoundID );
	}
#endif

	//////////////////////////////////////////////////////////////////////////
	// MOVE TO NODE

	BehaviourTreeMoveToNode::BehaviourTreeMoveToNode()
		: BehaviourTreeNodeBase( "Move To" )
	{
		CreateNode();
	}

	void BehaviourTreeMoveToNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::BehaviourTreeMoveTo;

#if !defined(SAT_DIST)
		Color = ImColor( 48, 128, 255, 100 );
		Type = NodeRenderType::Tree;
#endif

		Inputs.push_back( Ref<Pin>::Create( "In", PinType::Flow, PinKind::Input ) );

		for( auto& rInput : Inputs )
		{
			rInput->RenderType = PinRenderType::Tree;
		}
	}

	BehaviourTreeMoveToNode::~BehaviourTreeMoveToNode()
	{
	}

	NodeEvaluationState BehaviourTreeMoveToNode::EvaluateNode( NodeEditorRuntime* pEvaluator )
	{
		return NodeEvaluationState::Evaluated;
	}

	void BehaviourTreeMoveToNode::OnSerialise( std::ofstream& rStream ) const
	{
		RawSerialisation::WriteObject( m_TargetPosition, rStream );
	}

	void BehaviourTreeMoveToNode::OnDeserialise( IStream& rStream )
	{
		RawSerialisation::ReadObject( m_TargetPosition, rStream );
	}

	BehaviourTreeBaseTask* BehaviourTreeMoveToNode::ConvertToTask()
	{
		return new BehaviourTreeMoveToTask( m_TargetPosition );
	}

#if !defined(SAT_DIST)
	void BehaviourTreeMoveToNode::RenderDetails()
	{
		Auxiliary::DrawVec3Control( "Destination", m_TargetPosition );
	}

	void BehaviourTreeMoveToNode::OnRenderExtra()
	{
		ImGui::Text( "X %.2f Y %.2f Z %.2f", m_TargetPosition.x, m_TargetPosition.y, m_TargetPosition.z );
	}
#endif

}
