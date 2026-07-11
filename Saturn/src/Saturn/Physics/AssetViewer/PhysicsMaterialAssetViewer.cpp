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
#include "PhysicsMaterialAssetViewer.h"

#include "Saturn/Physics/PhysicsSurfaceRegistryAsset.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Project/Project.h"

#include "Saturn/Serialisation/YAML/AssetSerialisers.h"
#include "Saturn/ImGui/ImGuiAuxiliary.h"

#include <imgui.h>

namespace Saturn {

	PhysicsMaterialAssetViewer::PhysicsMaterialAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::PhysicsMaterial;

		AddPhysicsMaterialAsset();
	}

	PhysicsMaterialAssetViewer::~PhysicsMaterialAssetViewer()
	{
	}

	void PhysicsMaterialAssetViewer::OnImGuiRender()
	{
		DrawInternal();
	}

	void PhysicsMaterialAssetViewer::AddPhysicsMaterialAsset()
	{
		Ref<PhysicsMaterialAsset> physMaterialAsset = AssetManager::Get()->GetAssetAs<PhysicsMaterialAsset>( m_AssetID );
		m_MaterialAsset = physMaterialAsset;

		m_Open = true;
		m_Name = std::format( "{0}##PhysicsMaterial", m_MaterialAsset->Name );

		// Load the master registry.
		m_MasterPhysRegID = Project::GetActiveProject()->GetDefaultPhysRegAsset();
		
		// And try load.
		Ref<PhysicsSurfaceRegistryAsset> asset = AssetManager::Get()->GetAssetAs<PhysicsSurfaceRegistryAsset>( m_MasterPhysRegID );
	}

	void PhysicsMaterialAssetViewer::DrawInternal()
	{
		ImGui::PushID( ( int ) m_MaterialAsset->ID );

		ImGui::Begin( m_Name.c_str(), &m_Open );

		ImGui::BeginHorizontal( "##material_settings" );

		ImGui::BeginVertical( "##material_settingsV" );

		//////////////////////////////////////////////////////////////////////////
		// Modifiable properties vvv

		Auxiliary::DisabledFlag disabledIfRo( m_IsReadOnly );

		ImGui::Text( "Friction" );

		float friction = m_MaterialAsset->GetFriction();
		if( ImGui::InputFloat( "##StaticFriction", &friction, 0.0f, 1000.0f ) ) 
		{
			m_MaterialAsset->SetFriction( friction );
		}
		
		ImGui::Spring();

		ImGui::Text( "Restitution" );

		float restitution = m_MaterialAsset->GetRestitution();
		if( ImGui::InputFloat( "##Restitution", &restitution, 0.0f, 1000.0f ) ) 
		{
			m_MaterialAsset->SetRestitution( restitution );
		}

		ImGui::Text( "Surface Name" );
		
		if( ImGui::BeginCombo( "##surfaceopts", m_MaterialAsset->GetSurfaceName().c_str() ) )
		{
			Ref<PhysicsSurfaceRegistryAsset> asset = AssetManager::Get()->GetAssetAs<PhysicsSurfaceRegistryAsset>( m_MasterPhysRegID );
			if( asset )
			{
				for( const auto& rSurface : asset->GetNamesList() )
				{
					const bool selected = rSurface.Name == m_MaterialAsset->GetSurfaceName();

					if( ImGui::Selectable( rSurface.Name.c_str(), selected ) )
					{
						m_MaterialAsset->SetSurfaceName( rSurface.Name );
					}
				}
			}
			else
			{
				ImGui::Selectable( "No master asset is set in the Project Settings." );
			}
			
			ImGui::EndCombo();
		}

		ImGui::EndVertical();

		ImGui::EndHorizontal();

		disabledIfRo.Pop();
		ImGui::End();

		ImGui::PopID();

		if( !m_Open )
		{
			PhysicsMaterialAssetSerialiser mas;
			mas.Serialise( m_MaterialAsset );
		}
	}

}
