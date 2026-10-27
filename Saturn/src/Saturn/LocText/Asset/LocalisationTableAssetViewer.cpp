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
#include "LocalisationTableAssetViewer.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

namespace Saturn {

	LocalisationTableAssetViewer::LocalisationTableAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::LocalisationTable;

		Ref<LocalisationTableAsset> table = AssetManager::Get()->GetAssetAs<LocalisationTableAsset>( m_AssetID );
		m_LocalisationTable = table;

		m_Open = true;
		m_Name = std::format( "{0}##{1}", m_LocalisationTable->Name, id );
	}

	LocalisationTableAssetViewer::~LocalisationTableAssetViewer()
	{
	}

	void LocalisationTableAssetViewer::OnImGuiRender()
	{
		const ImGuiWindowFlags flags = m_Dirty ? ImGuiWindowFlags_UnsavedDocument : 0;
		if( ImGui::Begin( m_Name.c_str(), &m_Open, flags ) )
		{
			ImGui::BeginHorizontal( "##opt_hz" );

			if( ImGui::Button( "Add new language" ) )
			{
			}

			if( ImGui::Button( "Add new entry" ) )
			{
				m_LocalisationTable->AddNewEntry();
			}

			ImGui::EndHorizontal();

			const auto& rLanguages = m_LocalisationTable->GetUsedLanguages();
			const size_t numberOfRows = 1 + rLanguages.size();

			if( ImGui::BeginTable( "##LocTbl", static_cast< int >( numberOfRows ), ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_NoBordersInBody ) )
			{
				ImGui::TableSetupColumn( "Key" );

				for( const auto& rLang : rLanguages )
				{
					std::string label = std::to_string( ( uint16_t ) rLang );
					ImGui::TableSetupColumn( label.c_str() );
				}

				ImGui::TableHeadersRow();

				const auto& rStyle = ImGui::GetStyle();
				for( auto& [id, entry] : m_LocalisationTable->GetAllEntries() )
				{
					ImGui::TableNextRow();

					ImGui::TableNextColumn();

					ImGui::PushID( ( int ) id );
					ImGui::SetNextItemWidth( 200.0F );
					Auxiliary::InputText( "##ident", &entry.EntryName );
					ImGui::PopID();

					for( auto& [langCode, data] : entry.DataPerLanguage )
					{
						ImGui::TableNextColumn();
						
						switch( data.Type )
						{
							case LocalisationTableEntryType::Asset:
							{
								ImGui::Text( "%" PRIu64, data.GetAssetID() );
							} break;

							case LocalisationTableEntryType::Text:
							{
								ImGui::Text( "%s", data.GetTextData() );
							} break;

							default:
								break;
						}
					}
				}

				ImGui::EndTable();
			}
		}

		ImGui::End();
	}
	
}
