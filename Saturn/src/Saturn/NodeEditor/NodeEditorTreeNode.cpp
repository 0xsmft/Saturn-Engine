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
#include "NodeEditorTreeNode.h"

#include "UI/NodeEditor.h"
#include <imgui_internal.h>

namespace Saturn {

	NodeEditorTreeNode::NodeEditorTreeNode( const std::string& rName ) 
		: NodeEditorNodeBase( rName ) 
	{
	}

	NodeEditorTreeNode::~NodeEditorTreeNode()
	{
	}

	void NodeEditorTreeNode::Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder, NodeEditorBase* pBase )
	{
		const float rounding = 5.0f;
		const float padding = 12.0f;

		const auto pinBackground = ed::GetStyle().Colors[ ed::StyleColor_NodeBg ];

		ed::PushStyleColor( ed::StyleColor_NodeBg, ImColor( 128, 128, 128, 200 ) );
		ed::PushStyleColor( ed::StyleColor_NodeBorder, ImColor( 32, 32, 32, 200 ) );
		ed::PushStyleColor( ed::StyleColor_PinRect, ImColor( 60, 180, 255, 150 ) );
		ed::PushStyleColor( ed::StyleColor_PinRectBorder, ImColor( 60, 180, 255, 150 ) );

		ed::PushStyleVar( ed::StyleVar_NodePadding, ImVec4( 0, 0, 0, 0 ) );
		ed::PushStyleVar( ed::StyleVar_NodeRounding, rounding );
		ed::PushStyleVar( ed::StyleVar_SourceDirection, ImVec2( 0.0f, 1.0f ) );
		ed::PushStyleVar( ed::StyleVar_TargetDirection, ImVec2( 0.0f, -1.0f ) );
		ed::PushStyleVar( ed::StyleVar_LinkStrength, 0.0f );
		ed::PushStyleVar( ed::StyleVar_PinBorderWidth, 1.0f );
		ed::PushStyleVar( ed::StyleVar_PinRadius, 5.0f );
		ed::BeginNode( ed::NodeId( ID ) );

		auto* pDrawList = ImGui::GetWindowDrawList();

		///////////////////////////////
		// Evaluation Order

		ImVec2 headerEndPos = ImGui::GetCursorScreenPos();
		ImVec2 badgePos = headerEndPos + Size;
		badgePos.y -= 20.0f + ImGui::GetStyle().FramePadding.x;

		ImVec2 badgeSize = ImVec2( 20.0f, 20.0f );

		// Draw background
		ImU32 badgeColor = IM_COL32( 255, 100, 100, 255 );
		pDrawList->AddRectFilled( badgePos, ImVec2( badgePos.x + badgeSize.x, badgePos.y + badgeSize.y ), badgeColor, 5.0f );

		// Draw border
		ImU32 borderColor = IM_COL32( 0, 0, 0, 255 );
		pDrawList->AddRect( badgePos, ImVec2( badgePos.x + badgeSize.x, badgePos.y + badgeSize.y ), borderColor, 5.0f );

		// Draw text centered in the badge
		std::string text = std::to_string( EvaluationOrder );

		ImVec2 textSize = ImGui::CalcTextSize( text.data() );
		ImVec2 textPos = ImVec2( badgePos.x + ( badgeSize.x - textSize.x ) * 0.5f, badgePos.y + ( badgeSize.y - textSize.y ) * 0.5f );
		pDrawList->AddText( textPos, IM_COL32( 255, 255, 255, 255 ), text.data() );

		ImGui::BeginVertical( (int)ID );
		ImGui::BeginHorizontal( "inputs" );
		ImGui::Spring( 0, padding * 2 );

		ImRect inputRect;
		uint32_t pinIndex = 0;
		for( auto& rInput : Inputs )
		{
			ImGui::Dummy( ImVec2( 0.0f, padding ) );
			ImGui::Spring( 1.0f, 0.0f );
		
			inputRect = ImRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );

			rInput->Render( rBuilder, pBase->IsLinked( rInput->ID ), pinIndex );
			pinIndex++;
		}

		// Dummy if no inputs
		if( Inputs.size() == 0 )
		{
			ImGui::Dummy( ImVec2( 0.0F, padding ) );
		}

		ImGui::Spring( 0, padding * 2 );
		ImGui::EndHorizontal();

		ImGui::BeginHorizontal( "content_frame" );
		ImGui::Spring( 1, padding );

		ImGui::BeginVertical( "content", ImVec2( 0.0f, 0.0f ) );
		ImGui::Dummy( ImVec2( 160, 0 ) );
		ImGui::Spring( 1 );
		ImGui::TextUnformatted( Name.c_str() );
#if !defined(SAT_DIST)
		OnRenderExtra();
#endif
		ImGui::Spring( 1 );
		ImGui::EndVertical();

		ImRect itemRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );

		ImGui::Spring( 1, padding );
		ImGui::EndHorizontal();

		ImGui::BeginHorizontal( "outputs" );
		ImGui::Spring( 0, padding * 2 );

		ImRect outputRect;
		for( auto& rOutput : Outputs )
		{
			if( rOutput->Type == PinType::Delegate )
				continue;

			ImGui::Dummy( ImVec2( 0.0f, padding ) );
			ImGui::Spring( 1.0f, 0.0f );

			outputRect = ImRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );

			rOutput->Render( rBuilder, pBase->IsLinked( rOutput->ID ), 0 );
		}

		// Dummy if no outputs
		if( Outputs.size() == 0 )
		{
			ImGui::Dummy( ImVec2( 0.0F, padding ) );
		}

		ImGui::Spring( 0, padding * 2 );
		ImGui::EndHorizontal();

		ImGui::EndVertical();

		ed::EndNode();
		ed::PopStyleVar( 7 );
		ed::PopStyleColor( 4 );

		pDrawList = ed::GetNodeBackgroundDrawList( ed::NodeId( ID ) );

		// Draw output box
		pDrawList->AddRectFilled(
			outputRect.GetTL() - ImVec2( 0.0f, 1.0f ),
			outputRect.GetBR(),
			IM_COL32( ( int ) ( 255 * pinBackground.x ), ( int ) ( 255 * pinBackground.y ), ( int ) ( 255 * pinBackground.z ), 255 ), 4.0f, ImDrawFlags_RoundCornersTop );

		pDrawList->AddRect(
			outputRect.GetTL() - ImVec2( 0.0f, 1.0f ),
			outputRect.GetBR(),
			IM_COL32( ( int ) ( 255 * pinBackground.x ), ( int ) ( 255 * pinBackground.y ), ( int ) ( 255 * pinBackground.z ), 255 ), 4.0f, ImDrawFlags_RoundCornersTop );

		// Draw input box
		pDrawList->AddRectFilled(
			inputRect.GetTL() + ImVec2( 0.0f, 1.0f ),
			inputRect.GetBR(),
			IM_COL32( ( int ) ( 255 * pinBackground.x ), ( int ) ( 255 * pinBackground.y ), ( int ) ( 255 * pinBackground.z ), 255 ), 4.0f, ImDrawFlags_RoundCornersBottom );

		pDrawList->AddRect(
			inputRect.GetTL() + ImVec2( 0.0f, 1.0f ),
			inputRect.GetBR(),
			IM_COL32( ( int ) ( 255 * pinBackground.x ), ( int ) ( 255 * pinBackground.y ), ( int ) ( 255 * pinBackground.z ), 255 ), 4.0f, ImDrawFlags_RoundCornersBottom );

		// Draw the node backdrop for the text
		pDrawList->AddRectFilled( itemRect.GetTL(), itemRect.GetBR(), Color, 0.0f );
		pDrawList->AddRect( itemRect.GetTL(), itemRect.GetBR(), Color, 0.0f );
	}

	NodeEvaluationState NodeEditorTreeNode::EvaluateNode( NodeEditorRuntime* evaluator )
	{
		return NodeEvaluationState::Failed;
	}

}