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
#include "NodeEditorOutput.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Saturn {

	NodeEditorOutput::NodeEditorOutput()
	{
	}

	NodeEditorOutput::~NodeEditorOutput()
	{
		ClearOutput();
	}

	void NodeEditorOutput::Draw()
	{
		ImGui::Begin( "Node Editor Output", &m_ShowWindow );

		ImGui::BeginVertical( "##MessageRegionVert" );

		if( ImGui::BeginChild( "MessageRegion", ImVec2( 0.0f, 0.0f ), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar ) ) 
		{
			for( const auto& rMessage : m_Messages )
			{
				DrawMessage( rMessage );
			}

			if( ImGui::IsMouseDown( 0 ) && ImGui::IsWindowHovered() )
			{
				m_SelectedMessageID = 0;
			}
		}

		if( m_SelectedMessageID != 0 && ImGui::BeginPopupContextWindow( "NeOutMsg", ImGuiPopupFlags_MouseButtonRight ) )
		{
			if( ImGui::MenuItem( "Copy" ) )
			{
				const auto Itr = std::find_if( m_Messages.begin(), m_Messages.end(),
					[messageID = m_SelectedMessageID]( const auto& rMessage )
				{
					return rMessage.ID == messageID;
				} );

				if( Itr != m_Messages.end() )
				{
					const NodeEditorMessage msg = *Itr;
					ImGui::SetClipboardText( msg.MessageText.c_str() );
				}
			}

			if( ImGui::MenuItem( "Delete" ) )
			{
				ClearMessage( m_SelectedMessageID );
			}

			ImGui::EndPopup();
		}

		ImGui::EndChild();
		ImGui::EndVertical();
		ImGui::End();
	}

	void NodeEditorOutput::ClearOutput()
	{
		m_Messages.clear();
	}

	void NodeEditorOutput::PushMessage( const NodeEditorMessage& rMessageData )
	{
		m_Messages.push_back( rMessageData );
	}

	void NodeEditorOutput::DrawMessage( const NodeEditorMessage& rMessage )
	{
		const float height = ImGui::GetTextLineHeightWithSpacing();

		ImGui::BeginHorizontal( (int)rMessage.ID );

		ImGui::Spring( 1.0f, 1.0f );

		switch( rMessage.Type )
		{
			case NodeEditorMessageSeverity::Error:
				Auxiliary::Image( EditorIcons::GetIcon( "Error_Small" ), { height, height } );
				break;

			case NodeEditorMessageSeverity::Info:
				Auxiliary::Image( EditorIcons::GetIcon( "Information_Small" ), { height, height } );
				break;

			case NodeEditorMessageSeverity::Warning:
				Auxiliary::Image( EditorIcons::GetIcon( "Exclamation_Small" ), { height, height } );
				break;
		}

		ImGui::Spring();

		const float width = ImGui::GetContentRegionAvail().x;
		// Leave space for the bin icon
		const ImVec2 size( width - ( height * 2 ), height );
		
		if( ImGui::Selectable( rMessage.MessageText.c_str(), rMessage.ID == m_SelectedMessageID, 0, size ) )
		{
			m_SelectedMessageID = rMessage.ID;
		}

		if( ImGui::BeginItemTooltip() )
		{
			ImGui::Text( rMessage.MessageText.c_str() );
			ImGui::EndTooltip();
		}

		if( ImGui::IsItemClicked( ImGuiMouseButton_Right ) )
		{
			// Select the message so we can use it in the right click context menu
			m_SelectedMessageID = rMessage.ID;
		}

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.0f, 0.0f, 0.0f, 0.0f ) );

		if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Bin" ), { height, height } ) )
		{
			ClearMessage( rMessage.ID );
		}

		ImGui::PopStyleColor();

		ImGui::EndHorizontal();

		ImGui::Separator();
	}

	void NodeEditorOutput::ClearMessage( UUID messageID )
	{
		const auto Itr = std::find_if( m_Messages.begin(), m_Messages.end(), 
			[messageID]( const auto& rMessage )
			{
				return rMessage.ID == messageID;
			} );

		if( Itr != m_Messages.end() ) 
		{
			m_Messages.erase( Itr );
		}
	}

}
