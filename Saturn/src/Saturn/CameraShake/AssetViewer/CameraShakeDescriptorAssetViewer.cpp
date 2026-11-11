/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 0xsmft https://0xsmft.dev                                       *
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
#include "CameraShakeDescriptorAssetViewer.h"

#include "Saturn/CameraShake/CameraShakeDescriptorAsset.h"
#include "Saturn/Asset/AssetManager.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

#include "Saturn/ImGui/UndoRedo/GlobalUndoRedoGroup.h"

namespace Saturn {

	CameraShakeDescriptorAssetViewer::CameraShakeDescriptorAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::CameraShakeDescriptor;

		m_CameraShakeAsset = AssetManager::Get()->GetAssetAs<CameraShakeDescriptorAsset>( m_AssetID );
		m_OriginalCameraShakeAsset = Ref<CameraShakeDescriptorAsset>::Create( m_CameraShakeAsset );

		m_Open = true;
		m_Name = std::format( "{0}##{1}", m_CameraShakeAsset->Name, id );
	}

	CameraShakeDescriptorAssetViewer::~CameraShakeDescriptorAssetViewer()
	{
		GlobalUndoRedoGroup::Get()->RemoveIfActionHasIdentifier( m_AssetID );

		m_OriginalCameraShakeAsset = nullptr;
	}

	void CameraShakeDescriptorAssetViewer::OnImGuiRender()
	{
		const ImGuiWindowFlags flags = m_Dirty ? ImGuiWindowFlags_UnsavedDocument : 0;
		if( ImGui::Begin( m_Name.c_str(), &m_Open, flags ) )
		{
			if( m_ShowDirtyPopup ) DrawDirtyPopupModal();

			Auxiliary::ScopedDisabledFlag disabledIf( m_IsReadOnly );
			bool modifiedThisFrame = false;

			float trauma = m_CameraShakeAsset->GetTrauma();
			if( Auxiliary::DrawFloatControl( "Trauma", trauma, 0.0F, 1.0F ) ) 
			{
				Ref<UndoRedoActionModifyFloat> action = Ref<UndoRedoActionModifyFloat>::Create( "Modify Camera Shake Trauma", &m_CameraShakeAsset->m_Trauma, m_CameraShakeAsset->m_Trauma, trauma );
				GlobalUndoRedoGroup::Get()->AddAction( action, m_AssetID );

				m_CameraShakeAsset->m_Trauma = trauma;
				modifiedThisFrame = true;
			}

			float decaySpeed = m_CameraShakeAsset->GetDecaySpeed();
			if( Auxiliary::DrawFloatControl( "Decay Speed", decaySpeed ) ) 
			{
				Ref<UndoRedoActionModifyFloat> action = Ref<UndoRedoActionModifyFloat>::Create( "Modify Camera Shake Decay Speed", &m_CameraShakeAsset->m_DecaySpeed, m_CameraShakeAsset->m_DecaySpeed, decaySpeed );
				GlobalUndoRedoGroup::Get()->AddAction( action, m_AssetID );

				m_CameraShakeAsset->m_DecaySpeed = decaySpeed;
				modifiedThisFrame = true;
			}

			float pitchDeg = m_CameraShakeAsset->GetMaxPitchInDeg();
			if( Auxiliary::DrawFloatControl( "Max Pitch (degrees)", pitchDeg, 0.0f, 360.0f ) )
			{
				Ref<UndoRedoActionModifyFloat> action = Ref<UndoRedoActionModifyFloat>::Create( "Modify Camera Shake Max Pitch", &m_CameraShakeAsset->m_MaxPitchDeg, m_CameraShakeAsset->m_MaxPitchDeg, pitchDeg );
				GlobalUndoRedoGroup::Get()->AddAction( action, m_AssetID );

				m_CameraShakeAsset->m_MaxPitchDeg = pitchDeg;
				modifiedThisFrame = true;
			}

			float yawDeg = m_CameraShakeAsset->GetMaxYawInDeg();
			if( Auxiliary::DrawFloatControl( "Max Yaw (degrees)", yawDeg, 0.0f, 360.0f ) ) 
			{
				Ref<UndoRedoActionModifyFloat> action = Ref<UndoRedoActionModifyFloat>::Create( "Modify Camera Shake Max Yaw", &m_CameraShakeAsset->m_MaxYawDeg, m_CameraShakeAsset->m_MaxYawDeg, yawDeg );
				GlobalUndoRedoGroup::Get()->AddAction( action, m_AssetID );

				m_CameraShakeAsset->m_MaxYawDeg = yawDeg;
				modifiedThisFrame = true;
			}

			float rollDeg = m_CameraShakeAsset->GetMaxRollInDeg();
			if( Auxiliary::DrawFloatControl( "Max Roll (degrees)", rollDeg, 0.0f, 360.0f ) )
			{
				Ref<UndoRedoActionModifyFloat> action = Ref<UndoRedoActionModifyFloat>::Create( "Modify Camera Shake Max Roll", &m_CameraShakeAsset->m_MaxRollDeg, m_CameraShakeAsset->m_MaxRollDeg, rollDeg );
				GlobalUndoRedoGroup::Get()->AddAction( action, m_AssetID );

				m_CameraShakeAsset->m_MaxRollDeg = rollDeg;
				modifiedThisFrame = true;
			}

			if( modifiedThisFrame )
			{
				m_Dirty = m_CameraShakeAsset->CompareForDirty( m_OriginalCameraShakeAsset );
			}
		}

		if( !m_Open && m_Dirty )
		{
			m_Open = true;
			m_ShowDirtyPopup = true;
		}
	
		ImGui::End();
	}

	void CameraShakeDescriptorAssetViewer::OnUpdate( Timestep ts )
	{
	}

	void CameraShakeDescriptorAssetViewer::OnEvent( Event& rEvent )
	{
	}

	void CameraShakeDescriptorAssetViewer::DrawDirtyPopupModal()
	{
		ImGui::OpenPopup( "Camera Shake Descriptor is dirty##camshakedirty" );

		if( ImGui::BeginPopupModal( "Camera Shake Descriptor is dirty##camshakedirty", nullptr, ImGuiWindowFlags_NoSavedSettings ) )
		{
			ImGui::Text( "There are unsaved changes to this Camera Shake Descriptor, what would you like to do?" );
			ImGui::Separator();

			ImGui::BeginHorizontal( "##optsbbhz" );

			if( ImGui::Button( "Save" ) )
			{
				CameraShakeDescriptorAssetSerialiser csdas;
				csdas.Serialise( m_CameraShakeAsset );

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

}
