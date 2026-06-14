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
#include "BehaviourTreeBlackboardCondition.h"

#include "Saturn/AI/AIAgentEntity.h"
#include "Saturn/AI/BehaviourTree/AssetViewer/BehaviourTreeNodeEditor.h"

#if !defined( SAT_DIST )
#include "Saturn/ImGui/ImGuiAuxiliary.h"
#endif

namespace Saturn {

	BehaviourTreeBlackboardCondition::BehaviourTreeBlackboardCondition()
		: BehaviourTreeConditionTask( "Blackboard Condition", BehaviourTreeConditionType::Blackboard )
#if !defined(SAT_DIST)
		, m_VariableSpec( Ref<BlackboardVaraibleSpec>::Create() )
#endif
	{
	}

	BehaviourTreeBlackboardCondition::~BehaviourTreeBlackboardCondition()
	{
	}

	/*
	void BehaviourTreeMemoryCondition::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditor* pEditor, NodeEditorNodeBase* pNode )
	{
		BehaviourTreeNodeEditor* pBehaviourTreeNodeEditor = dynamic_cast< BehaviourTreeNodeEditor* >( pEditor );
		m_pRTBlackboard = pBehaviourTreeNodeEditor->GetBlackboard().Get();
	}
	*/

	NodeEditorTaskState BehaviourTreeBlackboardCondition::Tick( Timestep ts )
	{
		const auto key = m_pRTBlackboard->GetKey( m_RTBlackboardVariableID );
		if( !key )
			return NodeEditorTaskState::Failed;

		const bool hasValue = key->HoldsAnyValue();
		switch( m_QueryType )
		{
			case BTBlackboardConditionQueryType::Set:
				return hasValue ? NodeEditorTaskState::Completed : NodeEditorTaskState::Failed;

			case BTBlackboardConditionQueryType::NotSet:
				return hasValue ? NodeEditorTaskState::Failed : NodeEditorTaskState::Completed;

			default: return NodeEditorTaskState::Failed;
		}
	}

	void BehaviourTreeBlackboardCondition::Reset()
	{
	}

#if !defined( SAT_DIST )
	void BehaviourTreeBlackboardCondition::RenderDetails()
	{
		ImGui::Text( "Blackboard Condition" );
		ImGui::Separator();

		ImGui::Columns( 2 );
		ImGui::SetColumnWidth( 0, 125.0f );

		ImGui::Text( "Condition: " );

		ImGui::NextColumn();

		if( ImGui::BeginCombo( "##condition", BTBlackboardConditionQueryTypeToString( m_QueryType ).c_str() ) )
		{
			if( ImGui::Selectable( "Is Set" ) )
			{
				m_QueryType = BTBlackboardConditionQueryType::Set;
			}

			if( ImGui::Selectable( "Is Not Set" ) )
			{
				m_QueryType = BTBlackboardConditionQueryType::NotSet;
			}

			ImGui::EndCombo();
		}

		ImGui::Columns( 1 );

/*
		if( ImGui::BeginCombo( "##memcondkey", m_VariableSpec->Name.c_str() ) )
		{
			ImGui::EndCombo();
		}
*/
		// drop down for options
		ImGui::Text( "Variable:" );

		Auxiliary::DisabledFlag disabledIfNoBBSpec( m_BlackboardSpec == nullptr );
		
		if( ImGui::BeginCombo( "##condkey", m_VariableSpec->Name.c_str() ) )
		{
			if( auto out = m_BlackboardSpec->DrawKeyFinder( NodeEditorVariableDataType::Unknown, m_VariableSpec ); out )
			{
				m_VariableSpec = out;
				m_RTBlackboardVariableID = out->VariableID;
			}

			ImGui::EndCombo();
		}

		disabledIfNoBBSpec.Pop();
	}

	std::string BehaviourTreeBlackboardCondition::GetTitleText() const
	{
		std::string text = std::format( "{0} | If {1} {2}", m_Title, m_VariableSpec->Name.empty() ? "<NULL>" : m_VariableSpec->Name, BTBlackboardConditionQueryTypeToString( m_QueryType ) );
		return text;
	}

#endif

	void BehaviourTreeBlackboardCondition::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );

		RawSerialisation::WriteObject( ( std::underlying_type_t<BTBlackboardConditionQueryType> )m_QueryType, rStream );
	}

	void BehaviourTreeBlackboardCondition::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		RawSerialisation::ReadObject( m_QueryType, rStream );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( BehaviourTreeBlackboardCondition );
