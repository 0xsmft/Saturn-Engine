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
#include "PhysicsSurfaceRegistryAssetViewer.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Serialisation/YAML/AssetSerialisers.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

#include <imgui.h>

namespace Saturn {
	
	PhysicsSurfaceRegistryAssetViewer::PhysicsSurfaceRegistryAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		AddAsset();
	}

	PhysicsSurfaceRegistryAssetViewer::~PhysicsSurfaceRegistryAssetViewer()
	{
	}

	void PhysicsSurfaceRegistryAssetViewer::OnImGuiRender()
	{
		const ImGuiWindowFlags flags = m_Dirty ? ImGuiWindowFlags_UnsavedDocument : 0;
		if( ImGui::Begin( m_Name.c_str(), &m_Open ) )
		{
			if( m_ShowDirtyPopup ) DrawDirtyPopupModal();

			ImGui::Text( "Physics Surface Registry" );
			ImGui::Separator();

			for( auto itr = m_RegistryAsset->m_Surfaces.begin(); itr != m_RegistryAsset->m_Surfaces.end(); )
			{
				auto& rSurface = *itr;
				auto& rName = rSurface.Name;

				ImGui::PushID( ( int ) rSurface.RenderID );

				bool modified = Auxiliary::InputText( "##entername", &rName );
				ImGui::SameLine();

				if( ImGui::SmallButton( "-" ) )
				{
					itr = m_RegistryAsset->m_Surfaces.erase( itr );
					m_Dirty = true;
				}
				else
				{
					++itr;
				}

				if( rName.empty() )
				{
					DisplayErrorSection( "The surface name cannot be empty!" );
				}

				if( rName.contains( ' ' ) )
				{
					DisplayErrorSection( "The surface name cannot contain a space!" );
				}
				
				const auto count = std::count_if( m_RegistryAsset->m_Surfaces.begin(), m_RegistryAsset->m_Surfaces.end(),
					[ rName ]( const auto& rCandidate )
				{
					return rCandidate.Name == rName;
				} );

				if( count >= 2 )
				{
					DisplayErrorSection( "The surface name already exists!" );
				}
			
				ImGui::Separator();

				ImGui::PopID();
			}

			if( ImGui::SmallButton( "+" ) )
			{
				const auto count = std::count_if( m_RegistryAsset->m_Surfaces.begin(), m_RegistryAsset->m_Surfaces.end(), 
					[]( const auto& rCandidate ) 
				{
					return rCandidate.Name.contains( "NewSurface" );
				} );

				if( count >= 1 )
				{
					const std::string name = std::format( "NewSurface{0}", count );
					m_RegistryAsset->AddSurfaceType( name );
				}
				else
				{
					m_RegistryAsset->AddSurfaceType( "NewSurface" );
				}

				m_Dirty = true;
			}
		}

		ImGui::End();

		if( !m_Open && m_Dirty )
		{
			// Keep open until user decides what to do.
			m_Open = true;
			m_ShowDirtyPopup = true;
		}
	}

	void PhysicsSurfaceRegistryAssetViewer::DisplayErrorSection( const char* pText )
	{
		const ImVec2 padding = ImGui::GetStyle().FramePadding;
		const ImVec2 textPosition = ImGui::GetCursorScreenPos();
		const ImVec2 textSize = ImGui::CalcTextSize( pText );

		const ImVec2 min = ImVec2( textPosition.x - padding.x, textPosition.y - padding.y );
		const ImVec2 max = ImVec2( textPosition.x + padding.x + textSize.x, textPosition.y + padding.y + textSize.y );

		ImGui::GetWindowDrawList()->AddRectFilled( min, max, IM_COL32( 200, 30, 60, 255 ), 2.0f, ImDrawFlags_RoundCornersAll );

		ImGui::TextUnformatted( pText );
	}

	void PhysicsSurfaceRegistryAssetViewer::DrawDirtyPopupModal()
	{
		ImGui::OpenPopup( "Surface registry is dirty##spsrdirty" );

		if( ImGui::BeginPopupModal( "Surface registry is dirty##spsrdirty", nullptr, ImGuiWindowFlags_NoSavedSettings ) )
		{
			ImGui::Text( "There are unsaved changes to this Surface Registry, what would you like to do?" );

			ImGui::Separator();

			ImGui::BeginHorizontal( "##optsbbhz" );

			if( ImGui::Button( "Save" ) )
			{
				PhysicsSurfaceRegistryAssetSerialiser serialiser;
				serialiser.Serialise( m_RegistryAsset );

				m_Open = m_Dirty = m_ShowDirtyPopup = false;

				ImGui::CloseCurrentPopup();
			}

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

	void PhysicsSurfaceRegistryAssetViewer::AddAsset()
	{
		Ref<PhysicsSurfaceRegistryAsset> asset = AssetManager::Get()->GetAssetAs<PhysicsSurfaceRegistryAsset>( m_AssetID );
		m_RegistryAsset = asset;

		m_Open = true;
		m_Name = std::format( "{0}##PhysSurfaceReg", m_RegistryAsset->Name );
	}

}
