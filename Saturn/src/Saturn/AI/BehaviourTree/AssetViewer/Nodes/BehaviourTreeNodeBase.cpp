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
#include "BehaviourTreeNodeBase.h"

#include "Saturn/AI/BehaviourTree/AssetViewer/BehaviourTreeNodeEditor.h"

namespace Saturn {

#if !defined(SAT_DIST)
	void BehaviourTreeNodeBase::OnRenderNextSection()
	{
		if( !NodeCondition )
			return;

		auto* pDrawList = ImGui::GetWindowDrawList();

		ImGui::BeginHorizontal( "BlackboardSection" );
		ImGui::Spring( 1.0f, 6.0f );

		ImVec2 blackboardSize = ImVec2( 160.0f * 0.5f, 40.0f * 0.5f );
		ImVec2 blackboardPos = ImGui::GetCursorScreenPos();

		const ImU32 bbBGColor = IM_COL32( 50.0f, 50.0f, 50.0f, 255.0f );
		const ImU32 bbBorderColor = IM_COL32( 0.0f, 0.0f, 0.0f, 255.0f );
		
		const std::string textTile = NodeCondition->GetTitleText();
		blackboardSize += ImGui::CalcTextSize( textTile.c_str() );

		// Draw section
		pDrawList->AddRectFilled(
			blackboardPos,
			blackboardPos + blackboardSize,
			bbBGColor,
			5.0f
		);

		// Draw border
		pDrawList->AddRect(
			blackboardPos,
			blackboardPos + blackboardSize,
			bbBorderColor,
			5.0f
		);

		// Center the text inside the blackboard box
		const ImVec2 bbTextSize = ImGui::CalcTextSize( textTile.c_str() );
		const ImVec2 bbTextPos = blackboardPos + ( blackboardSize - bbTextSize ) * 0.5f;
		pDrawList->AddText( bbTextPos, IM_COL32( 255.0f, 255.0f, 255.0f, 255.0f ), textTile.c_str() );

		ImGui::Dummy( blackboardSize );

		ImGui::Spring( 1.0f, 6.0f );
		ImGui::EndHorizontal();
	}
#endif

	BehaviourTreeNodeEditor* BehaviourTreeNodeBase::GetParent()
	{
		return dynamic_cast<BehaviourTreeNodeEditor*>( pOuter );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

constexpr std::underlying_type_t<Saturn::SClassFlags> RStaticClassFlagsBehaviourTreeNodeBase = ( Saturn::SClassFlags ) Saturn::SC_VisibleInEditor | Saturn::SC_NoExtendedMetadata | Saturn::SC_Abstract;

static Saturn::SClass* RStaticLnkBehaviourTreeNodeBase()
{
	static Saturn::SClass* pClass = nullptr;
	if( !pClass ) 
	{
		const Saturn::SClassSpecification spec
		{ 
			"BehaviourTreeNodeBase", 
			RStaticClassFlagsBehaviourTreeNodeBase,
			0, 
			sizeof( Saturn::BehaviourTreeNodeBase ), alignof( Saturn::BehaviourTreeNodeBase ),
			Saturn::FNV1A64( "BehaviourTreeNodeBase" ), 
			Saturn::BehaviourTreeNodeBase::Super::StaticClass(), nullptr, RStaticLnkBehaviourTreeNodeBase, nullptr, {}
		}; 
		
		Saturn::SClass::RConstructClass( &pClass, spec );
	} 
	
	return pClass;
}

Saturn::SClass* Saturn::BehaviourTreeNodeBase::GetStaticClassInternal()
{
	return RStaticLnkBehaviourTreeNodeBase();
} 

static Saturn::SClassRegistrar RCRBehaviourTreeNodeBase( RStaticLnkBehaviourTreeNodeBase );
