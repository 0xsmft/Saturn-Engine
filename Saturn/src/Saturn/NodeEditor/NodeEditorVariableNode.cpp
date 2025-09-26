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
#include "NodeEditorVariableNode.h"

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

		Outputs.push_back( Ref<FloatPin>::Create( "", PinKind::Output ) );
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

		for( auto& rOutput : Outputs )
		{
			if( rOutput->Type == PinType::Delegate )
				continue;

			rOutput->Render( rBuilder, pOuter->IsLinked( rOutput->ID ), 0 );
		}

		rBuilder.End();
	}

	NodeEvaluationState NodeEditorVariableNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		return NodeEvaluationState::NeverEvaluated;
	}

	//////////////////////////////////////////////////////////////////////////
	// Spawners

	SharedPtr<NodeEditorVariableNode> NodeEditorVariableNode::SpawnVariableNode( Ref<NodeEditorVariable> var, SharedPtr<NodeEditorBase> nodeEditor )
	{
		SClass* pClass = NodeEditorVariableNode::StaticClass();

		NodeEditorVariableNode* pNode = ClassMetadataHandler::Get().CreateClassObject<NodeEditorVariableNode>( pClass, var->GetName(), var );

		SharedPtr<NodeEditorVariableNode> sp = pNode;
		nodeEditor->AddNode( sp );
		return sp;
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( NodeEditorVariableNode );
