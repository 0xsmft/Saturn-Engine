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
#include "AluraStylingProfileAssetViewer.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace Saturn {

	AluraStylingProfileAssetViewer::AluraStylingProfileAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::StyleProfile;
		m_Open = true;

		m_StylingProfile = AssetManager::Get()->GetAssetAs<AluraStylingProfile>( m_AssetID );
		m_Name = std::format( "{0} - Alura Styling Profile##{1}", m_StylingProfile->Name, ( uint64_t ) m_AssetID );
	}

	AluraStylingProfileAssetViewer::~AluraStylingProfileAssetViewer()
	{
	}

	void AluraStylingProfileAssetViewer::OnImGuiRender()
	{
		if( ImGui::Begin( m_Name.c_str(), &m_Open, ImGuiWindowFlags_MenuBar ) )
		{
			if( ImGui::BeginMenuBar() )
			{
				if( ImGui::BeginMenu( "File" ) )
				{
					if( ImGui::MenuItem( "Close" ) )
					{
						m_Open = false;
					}

					if( ImGui::MenuItem( "Save" ) )
					{
						if( m_Dirty )
						{
							AluraStylingProfileAssetSerialiser ssp;
							ssp.Serialise( m_StylingProfile );

							m_Dirty = false;
						}
					}

					ImGui::EndMenu();
				}

				if( ImGui::BeginMenu( "Styling Profile" ) )
				{
					if( ImGui::MenuItem( "Reset to Defaults" ) )
					{
						if( !m_IsReadOnly )
						{
							m_StylingProfile->GetStyle().Default();
						}
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}

			ImGui::Text( "Alura Styling Profile" );

			{
				Auxiliary::ScopedDisabledFlag disabledIfReadOnly( m_IsReadOnly );
			
				ImGui::SeparatorText( "General Style" );
				auto& rStyle = m_StylingProfile->GetStyle();

				Auxiliary::DrawFloatControl( "Alpha", rStyle.Alpha );
				Auxiliary::DrawFloatControl( "Disabled Alpha", rStyle.DisabledAlpha );
				Auxiliary::DrawFloatControl( "Region Rounding", rStyle.RegionRounding );

				Auxiliary::DrawVec2Control( "Window Padding", rStyle.WindowPadding );
				Auxiliary::DrawVec2Control( "Item Spacing", rStyle.ItemSpacing );

				Auxiliary::DrawFloatControl( "Indent Spacing", rStyle.IndentSpacing );
				Auxiliary::DrawFloatControl( "Window Border Size", rStyle.WindowBorderSize );
				Auxiliary::DrawFloatControl( "Font Size", rStyle.CurrentFontSize );

				ImGui::SeparatorText( "Colors" );
				ImGui::ColorEdit4( "Text", glm::value_ptr( rStyle.Colors[ 0 ] ) );
				ImGui::ColorEdit4( "Text Disabled", glm::value_ptr( rStyle.Colors[ 1 ] ) );
				ImGui::ColorEdit4( "Button", glm::value_ptr( rStyle.Colors[ 2 ] ) );
				ImGui::ColorEdit4( "Button Hovered", glm::value_ptr( rStyle.Colors[ 3 ] ) );
				ImGui::ColorEdit4( "Border", glm::value_ptr( rStyle.Colors[ 4 ] ) );
				ImGui::ColorEdit4( "Frame Background", glm::value_ptr( rStyle.Colors[ 5 ] ) );
				ImGui::ColorEdit4( "Progress bar Color", glm::value_ptr( rStyle.Colors[ 6 ] ) );
				ImGui::ColorEdit4( "Separator", glm::value_ptr( rStyle.Colors[ 7 ] ) );
				ImGui::ColorEdit4( "Region Background", glm::value_ptr( rStyle.Colors[ 8 ] ) );
			}
		}

		ImGui::End();
	
		if( !m_Open && !m_IsReadOnly )
		{
			AluraStylingProfileAssetSerialiser ssp;
			ssp.Serialise( m_StylingProfile );
		}
	}

}
