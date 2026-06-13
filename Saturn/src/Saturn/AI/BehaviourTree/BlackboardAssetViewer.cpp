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
#include "BlackboardAssetViewer.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"

#include <imgui.h>

namespace Saturn {

	BlackboardAssetViewer::BlackboardAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::BehaviourTreeMemory;

		Ref<BlackboardSpecificationAsset> snd = AssetManager::Get()->GetAssetAs<BlackboardSpecificationAsset>( m_AssetID );
		m_SpecAsset = snd;

		m_Open = true;
		m_Name = std::format( "{0}##{1}", m_SpecAsset->Name, std::to_string( m_SpecAsset->ID ) );
	}

	BlackboardAssetViewer::~BlackboardAssetViewer()
	{
	}

	void BlackboardAssetViewer::OnImGuiRender()
	{
#if !defined(SAT_DIST)
		const ImGuiWindowFlags flags = m_Dirty ? ImGuiWindowFlags_UnsavedDocument : 0;
		if( ImGui::Begin( m_Name.c_str(), &m_Open, flags ) )
		{
			if( m_ShowDirtyPopup ) DrawDirtyPopupModal();

			if( Auxiliary::TreeNode( "Variables" ) )
			{
				for( auto& rData : m_SpecAsset->m_SpecificationData )
				{
					const std::string id = std::format( "##{}", ( uint64_t ) rData->RenderID );

					ImGui::SetNextItemWidth( 130.0f );
					if( Auxiliary::InputText( id.c_str(), &rData->Name ) )
					{
						m_Dirty = true;
					}

					const std::string currentType = NodeEditorVariableDataTypeToString( rData->DataType );
					const auto textSize = ImGui::CalcTextSize( currentType.c_str() );

					ImGui::SameLine();

					ImGui::SetNextItemWidth( 130.0f );

					const std::string dataTypeID = std::format( "##DataType/{0}", ( uint64_t ) rData->RenderID );
					if( ImGui::BeginCombo( dataTypeID.c_str(), currentType.c_str() ) )
					{
						for( size_t i = 0; i < std::underlying_type_t<NodeEditorVariableDataType>( NodeEditorVariableDataType::Unknown ); ++i )
						{
							std::string name = NodeEditorVariableDataTypeToString( ( NodeEditorVariableDataType ) i );
							if( ImGui::Selectable( name.c_str() ) )
							{
								rData->DataType = ( NodeEditorVariableDataType ) i;

								m_Dirty = true;
							}
						}

						ImGui::EndCombo();
					}

					ImGui::SameLine();

					ImGui::PushID( ( int ) rData->RenderID );

					// Remove button
					if( ImGui::SmallButton( "-" ) )
					{
						// This is not the best way to do this,
						// we really should be using begin/end for our loop
						// and then we'd call m_SpecAsset->m_SpecificationData.erase
						// but doing it this way allows to make this code a little less 
						// complex.
						std::erase_if( m_SpecAsset->m_SpecificationData,
							[ id = rData->RenderID ]( const auto& rCandidate ) -> bool
						{
							return rCandidate->RenderID == id;
						} );

						m_Dirty = true;

						// Break because our ranges are now invalid...
						// ...and make sure to pop the id.
						ImGui::PopID();
						break;
					}

					ImGui::PopID();

					// No new line!
					if( rData->Name.empty() )
					{
						m_CanSave = false;
						DisplayErrorSection( "The variable name cannot be empty!" );
					}
					else
						m_CanSave = true;

					// Variable names can have a space but I want it to match with C++ laws
					// so that means they cannot contain a space.
					if( rData->Name.contains( ' ' ) )
					{
						m_CanSave = false;
						DisplayErrorSection( "The variable name cannot contain a space!" );
					}
					else
						m_CanSave = true;

					ImGui::Separator();
				}

				if( ImGui::SmallButton( "+" ) )
				{
					std::string name = "NewVariable";

					uint64_t count = 0llu;
					for( const auto& rData : m_SpecAsset->m_SpecificationData )
					{
						if( rData->Name.contains( name ) )
							++count;
					}

					if( count >= 1 )
					{
						name += std::format( "{0}", count );
					}

					m_SpecAsset->AddNew( name, NodeEditorVariableDataType::Unknown, UUID() );
					m_Dirty = true;
				}

				Auxiliary::EndTreeNode();
			}

			ImGui::End();
		}

		if( !m_Open && m_Dirty )
		{
			m_Open = true;
			m_ShowDirtyPopup = true;
		}
#endif
	}

	void BlackboardAssetViewer::DisplayErrorSection( const char* pText )
	{
		const ImVec2 padding = ImGui::GetStyle().FramePadding;
		const ImVec2 textPosition = ImGui::GetCursorScreenPos();
		const ImVec2 textSize = ImGui::CalcTextSize( pText );

		const ImVec2 min = ImVec2( textPosition.x - padding.x, textPosition.y - padding.y );
		const ImVec2 max = ImVec2( textPosition.x + padding.x + textSize.x, textPosition.y + padding.y + textSize.y );

		ImGui::GetWindowDrawList()->AddRectFilled( min, max, IM_COL32( 200, 30, 60, 255 ), 2.0f, ImDrawFlags_RoundCornersAll );

		ImGui::TextUnformatted( pText );
	}

	void BlackboardAssetViewer::DrawDirtyPopupModal()
	{
		ImGui::OpenPopup( "Blackboard is dirty##bbdirtypopup" );

		if( ImGui::BeginPopupModal( "Blackboard is dirty##bbdirtypopup", nullptr, ImGuiWindowFlags_NoSavedSettings ) )
		{
			if( m_CanSave )
				ImGui::Text( "There are unsaved changes to this Blackboard, what would you like to do?" );
			else
				ImGui::Text( "You cannot save, there are errors that need to be fixed before saving." );

			ImGui::Separator();

			ImGui::BeginHorizontal( "##optsbbhz" );

			Auxiliary::DisabledFlag disabledIfUnableToSave( !m_CanSave );
			
			if( ImGui::Button( "Save" ) ) 
			{
				BlackboardAssetSerialiser btms;
				btms.Serialise( m_SpecAsset );

				m_Open = m_Dirty = m_ShowDirtyPopup = false;

				ImGui::CloseCurrentPopup();
			}

			disabledIfUnableToSave.Pop();

			ImGui::Spring();

			if( ImGui::Button( "Discard Changes" ) )
			{
				m_Open = m_Dirty = m_ShowDirtyPopup = false;

				ImGui::CloseCurrentPopup();
			}

			ImGui::Spring();

			if( ImGui::Button( "Cancel" ) )
			{
				m_Open = true;
				m_ShowDirtyPopup = false;

				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}
	}

}
