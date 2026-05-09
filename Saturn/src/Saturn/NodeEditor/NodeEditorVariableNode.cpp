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
#include "NodeEditorVariableNode.h"

#include "NodeEditorVariableTasks.h"

#include "NodeEditorBase.h"
#include "builders.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

namespace Saturn {

	NodeEditorVariableNode::NodeEditorVariableNode()
		: Super()
	{
		CreateNode();
	}

	NodeEditorVariableNode::NodeEditorVariableNode( const std::string& rName, Ref<NodeEditorVariable> var )
		: Super( rName ), m_Variable( var )
	{
		CreateNode();
	}
	
	void NodeEditorVariableNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::VariableNode;
#if !defined(SAT_DIST)
		Color = ImColor( 147, 226, 74 );
#endif

		if( m_Variable )
		{
			InitPinsForVariable();
		}
	}

	void NodeEditorVariableNode::InitPinsForVariable()
	{
		switch( m_Variable->GetType() )
		{
			case NodeEditorVariableDataType::Float:
			{
				Outputs.push_back( Ref<FloatPin>::Create( "", PinKind::Output ) );
			} break;

			case NodeEditorVariableDataType::Int:
			{
				Outputs.push_back( Ref<IntPin>::Create( "", PinKind::Output ) );
			} break;

			case NodeEditorVariableDataType::Bool:
			{
				Outputs.push_back( Ref<BoolPin>::Create( "", PinKind::Output ) );
			} break;

			default:
			{
				SAT_CORE_WARN( "Unhandled variable data type to pin!, using default pin..." );
				Outputs.push_back( Ref<Pin>::Create( "", PinType::Class, PinKind::Output ) );
			} break;
		}
	}

	void NodeEditorVariableNode::Serialise( std::ofstream& rStream, bool isForDist ) const
	{
		// We must write the ID first, so when we deserailise we know what pin type to load back from.
		if( m_Variable )
		{
			RawSerialisation::WriteObjectChecked( m_Variable->GetUUID(), rStream );
		}
		else
			RawSerialisation::WriteObjectChecked( 0llu, rStream );

		Super::Serialise( rStream, isForDist );
	}

	void NodeEditorVariableNode::Deserialise( FDependentIStream& rStream )
	{
		UUID dataHandleID = 0llu;
		RawSerialisation::ReadObjectChecked( dataHandleID, rStream );

		if( dataHandleID )
		{
			auto* pOuter = dynamic_cast< NodeEditorBase* >( GetParentObject() );
			m_Variable = pOuter->FindVariable( dataHandleID );
			InitPinsForVariable();
		}

		Super::Deserialise( rStream );
	}

	NodeEditorVariableNode::~NodeEditorVariableNode()
	{
	}

	void NodeEditorVariableNode::Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder )
	{
		rBuilder.Begin( ed::NodeId( ID ) );
		rBuilder.Middle();

		ImGui::BeginHorizontal( "##variablenode" );

		const ImVec2 textSize = ImGui::CalcTextSize( Name.c_str() );
		ImGui::TextUnformatted( Name.c_str() );
		ImGui::EndHorizontal();

		auto* pOuter = dynamic_cast< NodeEditorBase* >( GetParentObject() );
		for( auto& rOutput : Outputs )
		{
			rOutput->RenderOutput( rBuilder, pOuter->IsLinked( rOutput->ID ) );
		}

		rBuilder.End();
	}

	NodeEditorTaskBase* NodeEditorVariableNode::ConvertToTask()
	{
		return NewObject<SNodeEditorGetVariableTask>( GetParentObject() );
	}

	//////////////////////////////////////////////////////////////////////////
	// Spawners

	SharedPtr<NodeEditorVariableNode> NodeEditorVariableNode::SpawnVariableNode( Ref<NodeEditorVariable> var, SharedPtr<NodeEditorBase> nodeEditor )
	{
		NodeEditorVariableNode* pNode = NewObject<NodeEditorVariableNode>( nodeEditor.Get(), var->GetName(), var );
		SharedPtr<NodeEditorVariableNode> sp = pNode;

		nodeEditor->AddNode( sp );
		return sp;
	}

	//////////////////////////////////////////////////////////////////////////
	// SET VARIABLE NODE

	NodeEditorSetVariableNode::NodeEditorSetVariableNode()
		: Super()
	{
		CreateNode();
	}

	NodeEditorSetVariableNode::NodeEditorSetVariableNode( const std::string& rName, Ref<NodeEditorVariable> var )
		: Super( rName ), m_Variable( var )
	{
		CreateNode();
	}

	void NodeEditorSetVariableNode::CreateNode()
	{
		ExecutionType = NodeExecutionType::VariableSetNode;
#if !defined(SAT_DIST)
		Color = ImColor( 147, 226, 74 );
#endif

		switch( m_Variable->GetType() )
		{
			case NodeEditorVariableDataType::Float:
			{
				Outputs.push_back( Ref<FloatPin>::Create( "", PinKind::Output ) );

				Inputs.push_back( Ref<FloatPin>::Create( "In Variable", PinKind::Input ) );
				Inputs.push_back( Ref<FloatPin>::Create( "In Value", PinKind::Input ) );
			} break;

			case NodeEditorVariableDataType::Int:
			{
				Outputs.push_back( Ref<IntPin>::Create( "", PinKind::Output ) );

				Inputs.push_back( Ref<IntPin>::Create( "In Variable", PinKind::Input ) );
				Inputs.push_back( Ref<IntPin>::Create( "In Value", PinKind::Input ) );
			} break;

			case NodeEditorVariableDataType::Bool:
			{
				Outputs.push_back( Ref<BoolPin>::Create( "", PinKind::Output ) );

				Inputs.push_back( Ref<BoolPin>::Create( "In Variable", PinKind::Input ) );
				Inputs.push_back( Ref<BoolPin>::Create( "In Value", PinKind::Input ) );
			} break;

			default:
			{
				Outputs.push_back( Ref<Pin>::Create( "", PinType::Class, PinKind::Output ) );

				Inputs.push_back( Ref<Pin>::Create( "In Variable", PinType::Class, PinKind::Input ) );
				Inputs.push_back( Ref<Pin>::Create( "In Value", PinType::Class, PinKind::Input ) );
			} break;
		}
	}

	NodeEditorSetVariableNode::~NodeEditorSetVariableNode()
	{
	}

	//////////////////////////////////////////////////////////////////////////
	// Spawners

	SharedPtr<NodeEditorSetVariableNode> NodeEditorSetVariableNode::SpawnSetVariableNode( Ref<NodeEditorVariable> var, SharedPtr<NodeEditorBase> nodeEditor )
	{
		NodeEditorSetVariableNode* pNode = NewObject<NodeEditorSetVariableNode>( nodeEditor.Get(), var->GetName(), var );

		SharedPtr<NodeEditorSetVariableNode> sp = pNode;
		nodeEditor->AddNode( sp );
		return sp;
	}

	//////////////////////////////////////////////////////////////////////////
}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( NodeEditorVariableNode );
SAT_X31_CREATE_AUTO_REG( NodeEditorSetVariableNode );
