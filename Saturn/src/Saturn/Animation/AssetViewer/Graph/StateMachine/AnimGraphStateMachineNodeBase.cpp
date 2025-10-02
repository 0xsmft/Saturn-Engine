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

#include "Saturn/Animation/AssetViewer/Graph/Animation/AnimGraph.h"

#include "builders.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"

#include <imgui_internal.h>

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
		ImVec4 normalisedColor = Color;
		normalisedColor.x /= 2.0f;
		normalisedColor.y /= 2.0f;
		normalisedColor.z /= 2.0f;

		ed::PushStyleColor( ed::StyleColor_NodeBg, normalisedColor );

		rBuilder.Begin( ed::NodeId( ID ) );
		rBuilder.Middle();

		ImGui::BeginHorizontal( "##state" );

		// TODO: Much better to store the ID rather than the Node itself!
		auto* AG = dynamic_cast< AnimGraph* >( pOuter );
		if( ImGui::RadioButton( "##markasentry", AG->GetEntryNode() == SharedFromThis() ) ) 
		{
			if( AG )
			{
				AG->MarkNodeAsEntry( SharedFromThis() );
			}
		}

		const ImVec2 textSize = ImGui::CalcTextSize( Name.c_str() );
		ImGui::SetNextItemWidth( textSize.x );
		ImGui::TextUnformatted( Name.c_str() );

		ImGui::EndHorizontal();
		rBuilder.End();

		ed::PopStyleColor();
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( AnimGraphStateMachineNodeBase );
