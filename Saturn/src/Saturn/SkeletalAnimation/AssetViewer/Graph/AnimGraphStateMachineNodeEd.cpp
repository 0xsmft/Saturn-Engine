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
#include "AnimGraphStateMachineNodeEd.h"

namespace Saturn {

	AnimGraphStateMachineNodeEd::AnimGraphStateMachineNodeEd()
		: FDependentNodeEditorSuper()
	{
	}

	AnimGraphStateMachineNodeEd::AnimGraphStateMachineNodeEd( AssetID id )
		: FDependentNodeEditorSuper( id )
	{
	}

	AnimGraphStateMachineNodeEd::~AnimGraphStateMachineNodeEd()
	{

	}

	void AnimGraphStateMachineNodeEd::OnImGuiRender()
	{
#if !defined(SAT_DIST)
		ed::SetCurrentEditor( m_Editor );

		ImGui::Begin( m_Name.c_str(), &m_WindowOpen );

		// Hand off to imgui_node_editor and draw the actual node editor and nodes
		ed::Begin( m_InternalEditorID.c_str(), ImGui::GetContentRegionAvail() );

		const auto cursorTopLeft = ImGui::GetCursorScreenPos();

		for( auto& [id, rNode] : m_Nodes )
		{
			rNode->Render( m_Builder );
		}

		for( const auto& rLink : m_Links )
			ed::Link( ed::LinkId( rLink->ID ), ed::PinId( rLink->StartPinID ), ed::PinId( rLink->EndPinID ), rLink->Color );

		ImGui::SetCursorScreenPos( cursorTopLeft );
		ed::End();
		ImGui::End();
#endif
	}

	void AnimGraphStateMachineNodeEd::OnUpdate( Timestep ts )
	{

	}

	void AnimGraphStateMachineNodeEd::OnEvent( Event& rEvent )
	{

	}

}
