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

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Serialisation/YAML/AssetSerialisers.h"

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
	}

	void PhysicsMaterialAssetViewer::DrawInternal()
	{
		ImGui::PushID( (int)m_MaterialAsset->ID );

		ImGui::Begin( m_Name.c_str(), &m_Open );

		ImGui::BeginHorizontal( "##material_settings" );

		ImGui::BeginVertical( "##material_settingsV" );

		ImGui::Text( "Static Friction" );
		ImGui::InputFloat( "##StaticFriction", &m_MaterialAsset->m_StaticFriction, 0.0f, 1000.0f );

		ImGui::Spring();

		ImGui::Text( "Dynamic Friction" );
		ImGui::InputFloat( "##DynamicFriction", &m_MaterialAsset->m_DynamicFriction, 0.0f, 1000.0f );
		
		ImGui::Spring();

		ImGui::Text( "Restitution" );
		ImGui::InputFloat( "##Restitution", &m_MaterialAsset->m_Restitution, 0.0f, 1000.0f );
		
		// TODO: Flags

		ImGui::EndVertical();

		ImGui::EndHorizontal();

		ImGui::End();

		ImGui::PopID();

		if( !m_Open )
		{
			PhysicsMaterialAssetSerialiser mas;
			mas.Serialise( m_MaterialAsset );
		}
	}

}