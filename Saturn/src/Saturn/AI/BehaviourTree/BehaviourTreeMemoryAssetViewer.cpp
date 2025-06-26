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
#include "BehaviourTreeMemoryAssetViewer.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"

#include <imgui.h>

namespace Saturn {

	BehaviourTreeMemoryAssetViewer::BehaviourTreeMemoryAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::BehaviourTreeMemory;

		Ref<BehaviourTreeMemorySpecification> snd = AssetManager::Get().GetAssetAs<BehaviourTreeMemorySpecification>( m_AssetID );
		m_SpecAsset = snd;

		m_Open = true;
		m_Name = std::format( "{0}##{1}", m_SpecAsset->Name, std::to_string( m_SpecAsset->ID ) );
	}

	BehaviourTreeMemoryAssetViewer::~BehaviourTreeMemoryAssetViewer()
	{
	}

	void BehaviourTreeMemoryAssetViewer::OnImGuiRender()
	{
#if !defined(SAT_DIST)
		ImGuiWindowFlags flags = m_Dirty ? ImGuiWindowFlags_UnsavedDocument : 0;
		if( ImGui::Begin( m_Name.c_str(), &m_Open, flags ) )
		{
			for( auto& rData : m_SpecAsset->m_SpecificationData )
			{
				char buffer[ 1024 ];
				memset( buffer, 0, 256 );
				memcpy( buffer, rData->Name.data(), rData->Name.length() );

				std::string id = std::format( "##{}", ( uint64_t ) rData->RenderID );

				ImGui::SetNextItemWidth( 130.0f );
				if( ImGui::InputText( id.c_str(), buffer, 1024 ) )
				{
					rData->Name = std::string( buffer );
					m_Dirty = true;
				}

				std::string currentType = SPropertyTypeToStringInNamespace( rData->DataType );
				auto textSize = ImGui::CalcTextSize( currentType.c_str() );

				ImGui::SameLine();

				ImGui::SetNextItemWidth( 130.0f );

				std::string dataTypeID = std::format( "##DataType/{0}", ( uint64_t ) rData->RenderID );
				if( ImGui::BeginCombo( dataTypeID.c_str(), currentType.c_str() ) )
				{
					for( size_t i = 0; i < std::underlying_type_t<SPropertyType>( SPropertyType::Unknown ); i++ )
					{
						std::string name = SPropertyTypeToStringInNamespace( ( SPropertyType ) i );
						if( ImGui::Selectable( name.c_str() ) )
						{
							rData->DataType = ( SPropertyType ) i;

							m_Dirty = true;
						}
					}

					ImGui::EndCombo();
				}

				// No new line!
				if( rData->Name.empty() )
				{
					std::string text = "The variable name cannot be empty!";

					ImVec2 padding = ImGui::GetStyle().FramePadding;
					ImVec2 textPosition = ImGui::GetCursorScreenPos();
					ImVec2 textSize = ImGui::CalcTextSize( text.c_str() );

					ImVec2 min = ImVec2( textPosition.x - padding.x, textPosition.y - padding.y );
					ImVec2 max = ImVec2( textPosition.x + padding.x + textSize.x, textPosition.y + padding.y + textSize.y );

					ImGui::GetWindowDrawList()->AddRectFilled( min, max,
						IM_COL32( 200, 30, 60, 255 ), 2.0f, ImDrawFlags_RoundCornersAll );

					ImGui::TextUnformatted( text.c_str() );

					m_CanSave = false;
				}
				else
					m_CanSave = true;

				ImGui::Separator();
			}

			if( ImGui::SmallButton( "+" ) )
			{
				std::string name = "New Variable";

				int count = 0;
				for( const auto& rData : m_SpecAsset->m_SpecificationData )
				{
					if( rData->Name.contains( name ) )
						count++;
				}

				if( count >= 1 )
				{
					name += std::format( " ({0})", count );
				}

				m_SpecAsset->AddNew( name, SPropertyType::Unknown, UUID() );
				m_Dirty = true;
			}

			ImGui::End();
		}

		if( !m_Open && m_CanSave )
		{
			BehaviourTreeMemorySpecAssetSerialiser btms;
			btms.Serialise( m_SpecAsset );
		}
		else if( !m_CanSave )
		{
			// Keep open until we can save.
			m_Open = true;
		}
#endif
	}

}
