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

#include "Saturn/Core/App.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"
#include "Saturn/ImGui/EditorEvents.h"

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
			if( m_ShowNewLanguagePopup ) DrawLanguagePopup();

			ImGui::BeginHorizontal( "##opt_hz" );

			if( ImGui::Button( "Add new language" ) )
			{
				m_ShowNewLanguagePopup = true;
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
					ImGui::TableSetupColumn( m_LanguageNameCache[ ( std::underlying_type_t< LocTextLanguage > ) rLang ].c_str() );
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

					ImGui::PushID( "typesec" );
					ImGui::BeginHorizontal( "##typeItmHz" );
					ImGui::Text( "Type: " );
					ImGui::PopID();

					ImGui::Spring();

					const std::string_view typeStr = LocalisationTableEntryTypeToString( entry.Type );
					const auto& rStyle = ImGui::GetStyle();
					const float comboWidth = glm::max(
						200.0f,
						ImGui::CalcTextSize( typeStr.data() ).x +
						rStyle.FramePadding.x * 2.0f +
						ImGui::GetFrameHeight() );

					ImGui::SetNextItemWidth( comboWidth );
					if( ImGui::BeginCombo( "##type", typeStr.data() ) )
					{
						bool selected = ( entry.Type == LocalisationTableEntryType::Text );
						if( ImGui::Selectable( "Localised Text", selected ) )
						{
							if( !selected )
								m_LocalisationTable->ChangeTypeForEntry( entry, LocalisationTableEntryType::Text );
						}

						selected = ( entry.Type == LocalisationTableEntryType::Asset );
						if( ImGui::Selectable( "Asset", selected ) )
						{
							if( !selected )
								m_LocalisationTable->ChangeTypeForEntry( entry, LocalisationTableEntryType::Asset );
						}

						ImGui::EndCombo();
					}

					ImGui::EndHorizontal();
					ImGui::PopID();

					for( auto& [langCode, data] : entry.DataPerLanguage )
					{
						ImGui::TableNextColumn();
						
						switch( data.Type )
						{
							case LocalisationTableEntryType::Asset:
							{
								bool assetFinderOpen = false;

								ImGui::PushID( ( int ) id );
								ImGui::PushID( ( int ) langCode );

								ImGui::BeginHorizontal( "##assetareaHz" );
								ImGui::Text( "%" PRIu64, data.GetAssetID() );
								
								if( ImGui::BeginItemTooltip() )
								{
									ImGui::Text( "This asset will be loaded based on the current language." );
									ImGui::EndTooltip();
								}

								ImGui::Spring();
								if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
								{
									assetFinderOpen = !assetFinderOpen;
								}
								ImGui::EndHorizontal();

								AssetID temporaryID = data.GetAssetID();
								if( Auxiliary::DrawAssetFinder( AssetType::Unknown, &assetFinderOpen, temporaryID ) )
								{
									data.SetAssetID( temporaryID );
								}

								if( Auxiliary::TreeNode( "Show details", false ) )
								{
									Ref<Asset> asset = AssetManager::Get()->FindAsset( data.GetAssetID() );
									if( asset )
									{
										ImGui::Text( "Name: %s", asset->Name.c_str() );
										ImGui::Text( "Type: %s", AssetTypeToString( asset->Type ).data() );
										
										if( ImGui::Button( "Browse To" ) )
										{
											Application::Get()->DispatchEvent<CBBrowseToItemEvent>(
												asset->Path, asset->ID );
										}
									}
									else
									{
										ImGui::TextColored( ImVec4{ 1.0f, 0.0f, 0.0f, 1.0f }, "No asset is set." );
									}

									Auxiliary::EndTreeNode();
								}
								ImGui::PopID();
								ImGui::PopID();
							} break;

							case LocalisationTableEntryType::Text:
							{
								ImGui::PushID( ( int ) id );
								ImGui::PushID( ( int ) langCode );
								Auxiliary::InputTextMultiline( "##textfield", &data.GetTextData(), ImVec2( 200.0F, 200.0F ) );
								ImGui::PopID();
								ImGui::PopID();
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
	
	void LocalisationTableAssetViewer::DrawLanguagePopup()
	{
		ImGui::OpenPopup( "Choose a new language##sltlang" );
		if( ImGui::BeginPopupModal( "Choose a new language##sltlang", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) )
		{
			ImGui::BeginHorizontal( "##langpickerHz" );

			ImGui::Text( "Language:" );
			ImGui::Spring();

			const std::string langName = Auxiliary::LocTextLanguageToString( m_ChosenLanguage );
			const auto& rStyle = ImGui::GetStyle();
			const float comboWidth = glm::max(
				200.0f,
				ImGui::CalcTextSize( langName.c_str() ).x +
				rStyle.FramePadding.x * 2.0f +
				ImGui::GetFrameHeight() );

			ImGui::SetNextItemWidth( comboWidth );
			if( ImGui::BeginCombo( "##langName", langName.c_str() ) )
			{
				for( std::underlying_type_t<LocTextLanguage> i = 1; i < ( std::underlying_type_t<LocTextLanguage> )LocTextLanguage::COUNT; ++i )
				{
					const bool selected = ( m_ChosenLanguage == ( LocTextLanguage ) i );

					const std::string& rItemName = Auxiliary::LocTextLanguageToString( ( LocTextLanguage ) i );
					if( ImGui::Selectable( rItemName.c_str() ) )
					{
						m_ChosenLanguage = ( LocTextLanguage ) i;
					}
				}

				ImGui::EndCombo();
			}

			ImGui::EndHorizontal();

			if( m_ChosenLanguage != LocTextLanguage::Unknown )
			{
				ImGui::BeginHorizontal( "##dialectHz" );

				ImGui::Text( "Dialect (variant):" );
				ImGui::Spring();

				const std::string& rVariantName = Auxiliary::LocTextLanguageVariantToString( m_ChosenLanguageVariant );
				const auto& rStyle = ImGui::GetStyle();
				const float comboWidth = glm::max(
					200.0f,
					ImGui::CalcTextSize( rVariantName.c_str() ).x +
					rStyle.FramePadding.x * 2.0f +
					ImGui::GetFrameHeight() );

				ImGui::SetNextItemWidth( comboWidth );
				if( ImGui::BeginCombo( "##variantName", rVariantName.c_str() ) )
				{
					for( std::underlying_type_t<LocTextLanguageVariant> i = 1; i < ( std::underlying_type_t<LocTextLanguageVariant> )LocTextLanguageVariant::COUNT; ++i )
					{
						const bool selected = ( m_ChosenLanguageVariant == ( LocTextLanguageVariant ) i );

						const std::string& rItemName = Auxiliary::LocTextLanguageVariantToString( ( LocTextLanguageVariant ) i );
						if( ImGui::Selectable( rItemName.c_str() ) )
						{
							m_ChosenLanguageVariant = ( LocTextLanguageVariant ) i;
						}
					}

					ImGui::EndCombo();
				}

				ImGui::EndHorizontal();
			}

			ImGui::Separator();

			ImGui::BeginHorizontal( "optionsHz" );
			
			if( ImGui::Button( "Add" ) )
			{
				m_LocalisationTable->AddNewLanguage( m_ChosenLanguage, m_ChosenLanguageVariant );

				m_LanguageNameCache[ ( std::underlying_type_t< LocTextLanguage > )m_ChosenLanguage ] = Auxiliary::LocTextLanguageToString( m_ChosenLanguage );

				m_ChosenLanguage = LocTextLanguage::Unknown;
				m_ChosenLanguageVariant = LocTextLanguageVariant::UnknownUnknown;
				m_ShowNewLanguagePopup = false;
				ImGui::CloseCurrentPopup();
			}

			if( ImGui::Button( "Cancel" ) )
			{
				m_ChosenLanguage = LocTextLanguage::Unknown;
				m_ChosenLanguageVariant = LocTextLanguageVariant::UnknownUnknown;
				m_ShowNewLanguagePopup = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}
	}

}
