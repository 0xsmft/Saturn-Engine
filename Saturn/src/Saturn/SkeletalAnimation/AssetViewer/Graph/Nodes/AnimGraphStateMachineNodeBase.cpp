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
#include "AnimGraphStateMachineNodeBase.h"

namespace Saturn {

	AnimGraphStateMachineNodeBase::AnimGraphStateMachineNodeBase( const std::string& rName )
		: NodeEditorNodeBase( rName )
	{
	}

	AnimGraphStateMachineNodeBase::~AnimGraphStateMachineNodeBase()
	{

	}

	void AnimGraphStateMachineNodeBase::Render( ax::NodeEditor::Utilities::BlueprintNodeBuilder& rBuilder )
	{
		const auto PADDING = 12.0f;

		ed::PushStyleVar( ed::StyleVar_NodeRounding, 5.0f );
		ed::PushStyleVar( ed::StyleVar_SourceDirection, ImVec2( 1.0f, 0.0f ) );
		ed::PushStyleVar( ed::StyleVar_TargetDirection, ImVec2( 1.0f, 0.0f ) );
		ed::PushStyleVar( ed::StyleVar_LinkStrength, 0.0f );
		ed::PushStyleVar( ed::StyleVar_PinBorderWidth, 1.0f );
		ed::PushStyleVar( ed::StyleVar_PinRadius, 5.0f );
		ed::BeginNode( ed::NodeId( ID ) );

		ed::BeginPin( ed::PinId( Inputs[ 0 ]->ID ), ed::PinKind::Input );
		ImGui::Dummy( ImVec2( 0.0f, 0.0f ) );
		ed::EndPin();

		ed::BeginPin( ed::PinId( Outputs[ 0 ]->ID ), ed::PinKind::Output );

		auto* pDrawList = ImGui::GetWindowDrawList();

		ImGui::BeginHorizontal( "ContentFrame" );

		const auto textSize = ImGui::CalcTextSize( Name.c_str() );
		const auto radius = textSize.y * 0.5f;
		const auto pos = ImGui::GetCursorScreenPos();

		ImVec2 center = ImVec2( pos.x + radius, pos.y + textSize.y * 0.5f );

		pDrawList->AddCircleFilled( center, radius, IM_COL32( 255, 255, 0, 255 ), 64 );

		ImGui::Dummy( ImVec2( radius * 2.0f + 4.0f, textSize.y ) );

		ImGui::Spring( 1 );

		ImGui::BeginVertical( "main", ImVec2( 0.0F, 0.0F ) );

//		ImGui::Spring( 1 );
		ImGui::TextUnformatted( Name.c_str() );

//		ImGui::Spring( 1 );
		ImGui::EndVertical();
		ImGui::EndHorizontal();

		ed::EndPin();
		ed::EndNode();
		ed::PopStyleVar( 6 );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( AnimGraphStateMachineNodeBase );
