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
#include "MaterialGraphColorPin.h"

#include "Saturn/NodeEditor/NodeEditorNodeBase.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace Saturn {

	MaterialViewerColorPin::MaterialViewerColorPin( UUID id, const std::string& rName, PinType type, UUID nodeID )
		: Pin( id, rName, type, nodeID )
	{
	}

	MaterialViewerColorPin::MaterialViewerColorPin( const std::string& rName, PinKind kind, bool readonly, bool accpetOnlyTextures )
		: Pin( rName, accpetOnlyTextures ? PinType::Material_TextureColor : PinType::Material_Color, kind ), m_ReadOnly( readonly ), m_AccpetOnlyTextures( accpetOnlyTextures )
	{
		if( accpetOnlyTextures )
			Type = PinType::Material_TextureColor;
	}

	MaterialViewerColorPin::~MaterialViewerColorPin()
	{
	}

	void MaterialViewerColorPin::OnRenderOutput()
	{
		if( m_ReadOnly ) return;

		bool OpenAssetColorPicker = false;

		ImGui::BeginHorizontal( "PickerH" );

		ImVec2 buttonSize = { ImGui::GetFrameHeight(), ImGui::GetFrameHeight() };
		ImRect boundingBox = { ImGui::GetCursorPos(), ImGui::GetCursorPos() + buttonSize };

		bool hovered, held;

		ImGui::ButtonBehavior( boundingBox, ImGui::GetID( &ID ), &hovered, &held );

		// TODO: Ruby and ImGui do not match! so ImGuiButtonFlags_None = 0 and RubyMouseButton_Left = 0
		if( hovered && ImGui::IsMouseClicked( ImGuiButtonFlags_None ) )
		{
			OpenAssetColorPicker = true;
		}

		Auxiliary::DrawColoredRect( buttonSize, ImVec4( Data.x, Data.y, Data.z, 255.0f ) );

		ImGui::EndHorizontal();

		ed::Suspend();

		if( OpenAssetColorPicker )
		{
			ImGui::OpenPopup( "AssetColorPicker" );
		}

		ImGui::SetNextWindowSize( { 350.0f, 0.0f } );
		if( ImGui::BeginPopup( "AssetColorPicker", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize ) )
		{
			ImVec4 color = ImVec4( Data.x, Data.y, Data.z, 255.0f );

			if( ImGui::ColorPicker3( "Color Picker", ( float* ) &color ) )
			{
				Data = glm::vec3( color.x, color.y, color.z );
			}

			ImGui::EndPopup();
		}

		ed::Resume();
	}

	//////////////////////////////////////////////////////////////////////////

	void MaterialViewerColorPin::Serialise( std::ofstream& rStream ) const
	{
		Pin::Serialise( rStream );

		RawSerialisation::WriteVec3( Data, rStream );
	}

	void MaterialViewerColorPin::Deserialise( FDependentIStream& rStream )
	{
		Pin::Deserialise( rStream );

		RawSerialisation::ReadVec3( Data, rStream );
	}

}
