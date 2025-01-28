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
#include "AssetImportPopups.h"

#include "Saturn/Core/App.h"

#include "Saturn/Asset/Asset.h"
#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Audio/Sound.h"

#include "Saturn/Vulkan/Mesh.h"

#include "ImGuiAuxiliary.h"
#include "EditorIcons.h"

#include <imgui.h>

namespace Saturn::Auxiliary {

	bool DrawImportMeshPopup( bool* pOpen, const std::filesystem::path& rImportTargetPath )
	{
		bool PopupModified = false;

		ImGui::SetNextWindowPos( ImGui::GetWindowViewport()->GetCenter(), ImGuiCond_Appearing );
		if( ImGui::BeginPopupModal( "Import Mesh##IMPORT_MESH", pOpen, ImGuiWindowFlags_NoSavedSettings ) )
		{
			static std::filesystem::path s_GLTFBinPath = "";
			static std::filesystem::path s_OriginalMeshPath = "";
			static bool s_UseBinFile = true;
			static AssetID s_CurrentAssetID = 0;

			ImGui::BeginVertical( "##inputv" );

			ImGui::Text( "Path:" );

			ImGui::BeginHorizontal( "##inputH" );

			ImGui::InputText( "##path", ( char* ) s_OriginalMeshPath.string().c_str(), 1024 );

			if( ImGui::Button( "Browse" ) )
			{
				s_OriginalMeshPath = Application::Get().OpenFile( "Supported asset types (*.fbx *.gltf *.glb)\0*.fbx; *.gltf; *.glb\0" );
			}

			ImGui::EndHorizontal();

			ImGui::EndVertical();

			// If the path a GLTF file then we need to file the bin file.
			if( s_OriginalMeshPath.extension() == ".gltf" || s_OriginalMeshPath.extension() == ".glb" )
			{
				// We can assume the bin file has the same name as the mesh.
				if( s_GLTFBinPath == "" )
				{
					s_GLTFBinPath = s_OriginalMeshPath;
					s_GLTFBinPath.replace_extension( ".bin" );
				}

				ImGui::BeginVertical( "##gltfinput" );

				ImGui::Text( "GLTF binary file path:" );

				ImGui::BeginHorizontal( "##gltfinputH" );

				ImGui::InputText( "##binpath", ( char* ) s_GLTFBinPath.string().c_str(), 1024 );

				if( ImGui::Button( "Browse" ) )
				{
					s_GLTFBinPath = Application::Get().OpenFile( "Supported asset types (*.glb *.bin)\0*.glb; *.bin\0" );
				}

				ImGui::EndHorizontal();

				ImGui::Checkbox( "Use Binary File", &s_UseBinFile );

				ImGui::EndVertical();
			}

			ImGui::BeginHorizontal( "##actionsH" );

			if( ImGui::Button( "Create" ) )
			{
				auto id = AssetManager::Get().CreateAsset( AssetType::StaticMesh );
				auto asset = AssetManager::Get().FindAsset( id );

				// Copy the mesh source.
				std::filesystem::copy_file( s_OriginalMeshPath, rImportTargetPath / s_OriginalMeshPath.filename() );

				if( s_UseBinFile )
					std::filesystem::copy_file( s_GLTFBinPath, rImportTargetPath / s_GLTFBinPath.filename() );

				auto assetPath = rImportTargetPath / s_OriginalMeshPath.filename();
				assetPath.replace_extension( ".stmesh" );

				asset->SetAbsolutePath( assetPath );

				// TODO: This is bad.
				// Create the mesh so we can copy over the texture (if any).
				auto mesh = Ref<MeshImporter>::Create( s_OriginalMeshPath, rImportTargetPath, 0 );
				mesh = nullptr;

				// Create the mesh asset.
				auto staticMesh = asset.As<StaticMesh>();
				staticMesh = Ref<StaticMesh>::Create();
				staticMesh->ID = asset->ID;
				staticMesh->Path = asset->Path;

				auto& meshPath = assetPath.replace_extension( s_OriginalMeshPath.extension() );
				staticMesh->SetFilepath( meshPath.string() );

				// Save the mesh asset
				StaticMeshAssetSerialiser sma;
				sma.Serialise( staticMesh );

				staticMesh->SetAbsolutePath( assetPath );

				PopupModified = true;
			}

			// Don't set PopupModified to true
			// Only set PopupModified to true if we have created something.
			if( ImGui::Button( "Cancel" ) )
			{
				*pOpen = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			if( PopupModified )
			{
				*pOpen = false;

				ImGui::CloseCurrentPopup();

				s_OriginalMeshPath = "";
				s_GLTFBinPath = "";
				s_UseBinFile = false;
			}

			ImGui::EndPopup();
		}

		return PopupModified;
	}

	bool DrawImportSoundPopup( bool* pOpen, const std::filesystem::path& rImportTargetPath, std::filesystem::path& rDefaultPath )
	{
		bool PopupModified = false;
		static std::filesystem::path s_ImportSoundPath;

		if( ImGui::BeginPopupModal( "Import Sound##IMPORT_SOUND", pOpen, ImGuiWindowFlags_NoMove ) )
		{
			ImGui::BeginVertical( "##inputv" );

			ImGui::Text( "Path:" );

			ImGui::BeginHorizontal( "##inputH" );

			ImGui::InputText( "##path", ( char* ) rDefaultPath.string().c_str(), 1024 );

			if( ImGui::Button( "Browse" ) )
			{
				rDefaultPath = Application::Get().OpenFile( "Supported asset types (*.wav *.mp3)\0*.wav; *.mp3\0" );
			}

			ImGui::EndHorizontal();
			ImGui::EndVertical();

			ImGui::BeginHorizontal( "##actionsH" );

			if( ImGui::Button( "Create" ) )
			{
				auto id = AssetManager::Get().CreateAsset( AssetType::Sound );
				auto asset = AssetManager::Get().FindAsset( id );

				// Copy the audio source.
				std::filesystem::copy_file( rDefaultPath, rImportTargetPath / rDefaultPath.filename() );

				auto assetPath = rImportTargetPath / rDefaultPath.filename();
				assetPath.replace_extension( ".snd" );

				assetPath = std::filesystem::relative( assetPath, Project::GetActiveProject()->GetRootDir() );

				asset->Path = assetPath;

				// Create the asset.
				auto sound = asset.As<SoundSpecification>();
				sound = Ref<SoundSpecification>::Create();
				sound->ID = asset->ID;
				sound->Path = assetPath;
				sound->Type = AssetType::Sound;

				sound->OriginalImportPath = rDefaultPath;
				sound->SoundSourcePath = rImportTargetPath / rDefaultPath.filename();

#if !defined(SAT_DIST)
				// Currently the date is YYYY-MM-DD HH-MM-SS however all we want is YYYY-MM-DD
				std::string fullTime = std::format( "{0}", std::filesystem::last_write_time( rDefaultPath ) );
				fullTime = fullTime.substr( 0, fullTime.find_first_of( " " ) );

				sound->LastWriteTime = fullTime;
#endif

				// Save the asset
				SoundSpecificationAssetSerialiser s2d;
				s2d.Serialise( sound );

				sound->SetAbsolutePath( assetPath );

				PopupModified = true;
			}

			if( ImGui::Button( "Cancel" ) )
			{
				*pOpen = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			if( PopupModified )
			{
				*pOpen = false;

				ImGui::CloseCurrentPopup();

				s_ImportSoundPath = "";
			}

			ImGui::EndPopup();
		}

		return PopupModified;
	}
}