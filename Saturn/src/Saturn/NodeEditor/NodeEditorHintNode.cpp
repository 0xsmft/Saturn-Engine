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
#include "NodeEditorHintNode.h"

#include "UI/NodeEditor.h"
#include <imgui_internal.h>

namespace Saturn {

	NodeEditorHintNode::NodeEditorHintNode( const std::string& rName )
		: NodeEditorNodeBase( rName )
	{
		Type = NodeRenderType::Comment;
		ExecutionType = NodeExecutionType::HintNode;
	}

	NodeEditorHintNode::~NodeEditorHintNode()
	{
	}

	void NodeEditorHintNode::Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, NodeEditorBase* pBase )
	{
		constexpr float HINT_ALPHA = 0.75f;

		ImGui::PushStyleVar( ImGuiStyleVar_Alpha, HINT_ALPHA );

		ed::PushStyleColor( ed::StyleColor_NodeBg, ImColor( 255, 255, 255, 64 ) );
		ed::PushStyleColor( ed::StyleColor_NodeBorder, ImColor( 255, 255, 255, 64 ) );

		ed::BeginNode( ed::NodeId( ID ) );
		ImGui::PushID( ( int ) ID );

		ImGui::BeginVertical( "content" );
		ImGui::BeginHorizontal( "horizontal" );

		ImGui::Spring( 1 );
		ImGui::TextUnformatted( Name.c_str() );
		ImGui::Spring( 1 );

		ImGui::EndHorizontal();
		
		ed::Group( Size );
		ImGui::EndVertical();
		ImGui::PopID();

		ed::EndNode();
		ed::PopStyleColor( 2 );

		ImGui::PopStyleVar();

		if( ed::BeginGroupHint( ed::NodeId( ID ) ) )
		{
			auto bgAlpha = static_cast< int >( ImGui::GetStyle().Alpha * 255 );
			auto min = ed::GetGroupMin();

			ImGui::SetCursorScreenPos( min - ImVec2( -8.0f, ImGui::GetTextLineHeightWithSpacing() + 4.0f ) );
			ImGui::BeginGroup();
			ImGui::TextUnformatted( Name.c_str() );
			ImGui::EndGroup();

			auto* pDrawList = ed::GetHintBackgroundDrawList();

			auto hintBounds = ImRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
			hintBounds.Expand( ImVec2( 8.0f, 4.0f ) );

			pDrawList->AddRectFilled(
				hintBounds.GetTL(),
				hintBounds.GetBR(),
				IM_COL32( 255, 255, 255, 64 * bgAlpha / 255 ), 4.0f );

			pDrawList->AddRect(
				hintBounds.GetTL(),
				hintBounds.GetBR(),
				IM_COL32( 255, 255, 255, 128 * bgAlpha / 255 ), 4.0f );
		}
		ed::EndGroupHint();
	}

	NodeEvaluationState NodeEditorHintNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		return NodeEvaluationState::NeverEvaluated;
	}

	//////////////////////////////////////////////////////////////////////////
	// STATIC API

	Ref<NodeEditorHintNode> NodeEditorHintNode::SpawnHintNode( Ref<NodeEditorBase> nodeEditor )
	{
		Ref<NodeEditorHintNode> node = Ref<NodeEditorHintNode>::Create();
		nodeEditor->AddNode( node );

		return node;
	}

}
