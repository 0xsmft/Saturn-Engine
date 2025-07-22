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
#include "NodeEditorBlueprintNode.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"

#include "builders.h"

namespace Saturn {

	NodeEditorBlueprintNode::NodeEditorBlueprintNode( const std::string& rName )
		: NodeEditorNodeBase( rName )
	{
	}

	NodeEditorBlueprintNode::~NodeEditorBlueprintNode()
	{
	}

	void NodeEditorBlueprintNode::Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder )
	{
#if !defined(SAT_DIST)
		rBuilder.Begin( ed::NodeId( ID ) );

		rBuilder.Header( Color );

		ImGui::Spring( 0 );
		ImGui::TextUnformatted( Name.c_str() );
		ImGui::Spring( 1 );
		ImGui::Dummy( ImVec2( 0, 28 ) );
		ImGui::Spring( 0 );

#if defined( SAT_DEBUG )
		// Draw debug evaluation order
		auto* pDrawlist = ImGui::GetWindowDrawList();

		ImVec2 headerEndPos = ImGui::GetCursorScreenPos();
		ImVec2 badgePos = headerEndPos;
		badgePos.y -= 20; // Move upward to be above header
		badgePos.x -= 30; // Align at the end

		ImVec2 badgeSize = ImVec2( 20.0f, 20.0f );

		// Draw background
		ImU32 badgeColor = IM_COL32( 255, 100, 100, 255 );
		pDrawlist->AddRectFilled( badgePos, ImVec2( badgePos.x + badgeSize.x, badgePos.y + badgeSize.y ), badgeColor, 5.0f );

		// Draw border
		ImU32 borderColor = IM_COL32( 0, 0, 0, 255 );
		pDrawlist->AddRect( badgePos, ImVec2( badgePos.x + badgeSize.x, badgePos.y + badgeSize.y ), borderColor, 5.0f );

		// Draw text centered in the badge
		std::string text = std::to_string( EvaluationOrder );

		ImVec2 textSize = ImGui::CalcTextSize( text.data() );
		ImVec2 textPos = ImVec2( badgePos.x + ( badgeSize.x - textSize.x ) * 0.5f, badgePos.y + ( badgeSize.y - textSize.y ) * 0.5f );
		pDrawlist->AddText( textPos, IM_COL32( 255, 255, 255, 255 ), text.data() );
#endif

		rBuilder.EndHeader();

		uint32_t pinIndex = 0;
		for( auto& rInput : Inputs )
		{
			rInput->Render( rBuilder, pOuter->IsLinked( rInput->ID ), pinIndex );
			pinIndex++;
		}

		for( auto& rOutput : Outputs )
		{
			if( rOutput->Type == PinType::Delegate )
				continue;

			rOutput->Render( rBuilder, pOuter->IsLinked( rOutput->ID ), 0 );
		}

		rBuilder.End();
#endif
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( NodeEditorBlueprintNode );
