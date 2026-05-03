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
#include "BehaviourTreeSequenceNode.h"

#include "Saturn/GameFramework/SClass.h"
#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"

#include "Saturn/AI/BehaviourTree/AssetViewer/BehaviourTreeNodeEditor.h"

#include "Saturn/AI/BehaviourTree/Tasks/BehaviourTreeCompositeTasks.h"

#include "Saturn/AI/BehaviourTree/Conditions/BehaviourTreeMemoryCondition.h"

#if !defined(SAT_DIST)
#include "Saturn/ImGui/ImGuiAuxiliary.h"
#endif

namespace Saturn {

	BehaviourTreeSequenceNode::BehaviourTreeSequenceNode()
		: BehaviourTreeNodeBase( "Sequence" )
	{
		CreateNode();
	}

	void BehaviourTreeSequenceNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::BehaviourTreeSequenceNode;

#if !defined(SAT_DIST)
		Color = ImColor( 48, 128, 255, 100 );
		RenderType = NodeRenderType::Tree;
#endif

		Inputs.push_back( Ref<Pin>::Create( "In", PinType::Flow, PinKind::Input ) );
		Outputs.push_back( Ref<Pin>::Create( "Out", PinType::Flow, PinKind::Output ) );

		for( auto& rOutput : Outputs )
		{
			rOutput->RenderType = PinRenderType::Tree;
		}

		for( auto& rInput : Inputs )
		{
			rInput->RenderType = PinRenderType::Tree;
		}
	}

	BehaviourTreeSequenceNode::~BehaviourTreeSequenceNode()
	{
	}

	void BehaviourTreeSequenceNode::Serialise( std::ofstream& rStream, bool isForDist ) const
	{
		BehaviourTreeNodeBase::Serialise( rStream, isForDist );

		RawSerialisation::WriteVector( m_Children, rStream );

		const bool hasNodeCondition = NodeCondition != nullptr;
		RawSerialisation::WriteObject( hasNodeCondition, rStream );

		if( hasNodeCondition )
		{
			RawSerialisation::WriteString( NodeCondition->GetClass()->GetName(), rStream );

			NodeCondition->Serialise( rStream );
		}
	}

	void BehaviourTreeSequenceNode::Deserialise( FDependentIStream& rStream )
	{
		BehaviourTreeNodeBase::Deserialise( rStream );

		RawSerialisation::ReadVector( m_Children, rStream );

		bool hadNodeCondition = false;
		RawSerialisation::ReadObject( hadNodeCondition, rStream );

		if( hadNodeCondition )
		{
			std::string className = RawSerialisation::ReadString( rStream );

			auto* pCondition = dynamic_cast<BehaviourTreeCondition*>( ClassMetadataHandler::Get().CreateClassObject( className ) );

			if( pCondition )
			{
				pCondition->Deserialise( rStream );
				NodeCondition = pCondition;
			}
		}
	}

	NodeEditorTaskBase* BehaviourTreeSequenceNode::ConvertToTask()
	{
		return NewObject<BehaviourTreeSequenceTask>( nullptr );
	}

#if !defined( SAT_DIST )
	void BehaviourTreeSequenceNode::PostDeserialise()
	{
		if( NodeCondition && GetParentAsBTNodeEditor()->GetBlackboardSpec() )
		{
			NodeCondition->SetupMemVariable( GetParentAsBTNodeEditor()->GetBlackboardSpec()->ID );
		}
	}
#endif

	void BehaviourTreeSequenceNode::Reset()
	{
		m_Children.clear();
	}

	void BehaviourTreeSequenceNode::AddChildren( const std::vector<UUID>& rChildrenID )
	{
		for( auto& rID : rChildrenID )
		{
			m_Children.emplace_back( rID );
		}
	}

#if !defined(SAT_DIST)
	void BehaviourTreeSequenceNode::RenderContextWindow()
	{
		Auxiliary::DisabledFlag disabledIfCondition( NodeCondition );

		ImGui::SeparatorText( "Condition" );

		if( ImGui::BeginMenu( "Add Condition" ) )
		{
			if( ImGui::MenuItem( "Blackboard" ) )
			{
				auto* pCond = ( BehaviourTreeMemoryCondition* )ClassMetadataHandler::Get().CreateClassObject( BehaviourTreeMemoryCondition::StaticClass() );
				
				if( GetParentAsBTNodeEditor()->GetBlackboardSpec() )
				{
					pCond->SetupMemVariable( GetParentAsBTNodeEditor()->GetBlackboardSpec()->ID );
				}

				NodeCondition = pCond;
			}

			ImGui::EndMenu();
		}

		disabledIfCondition.Pop();

		Auxiliary::DisabledFlag disabledIfNoCondition( !NodeCondition );

		if( ImGui::MenuItem( "Remove Condition" ) )
		{
			NodeCondition = nullptr;
		}

		disabledIfNoCondition.Pop();
	}

	void BehaviourTreeSequenceNode::RenderDetails()
	{
		if( NodeCondition )
			NodeCondition->RenderDetails();
	}

#endif

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( BehaviourTreeSequenceNode );
