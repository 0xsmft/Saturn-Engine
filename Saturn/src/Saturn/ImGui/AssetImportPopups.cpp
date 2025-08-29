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
#include "Saturn/Core/JobSystem.h"

#include "Saturn/Asset/Asset.h"
#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Serialisation/YAML/AssetManagerSerialiser.h"

#include "Saturn/Audio/Sound.h"

#include "Saturn/Vulkan/Mesh.h"

#include "Saturn/Project/Project.h"

#include "ImGuiAuxiliary.h"
#include "EditorIcons.h"

#include <imgui.h>

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// MESH IMPORT POPUP
	
	MeshImportPopup::MeshImportPopup( const std::filesystem::path& rAssetToImportPath, const std::filesystem::path& rDestinationPath )
		: AssetImportPopupBase( rAssetToImportPath, rDestinationPath )
	{
	}

	void MeshImportPopup::Initialise()
	{
		// Import on job system.
		JobSystem::Get().AddJob( [this]() 
		{
			MeshDeterminer md;
			md.ImportAndDetermine( m_AssetToImportPath );
		
			if( md.CheckResult( MeshDeterminerResult_Undetermined ) )
			{
				// TODO: Handle error.
			}

			if( md.CheckResult( MeshDeterminerResult_StaticMesh ) )
			{
				m_IsSkeletal = false;
				m_IsReady.store( true );
			}

			if( md.CheckResult( MeshDeterminerResult_SkeletalMesh ) )
			{
				m_IsSkeletal = true;
				m_IsReady.store( true );
			}

			// TODO: We don't currently use MeshDeterminerResult_Materials or MeshDeterminerResult_Animations
			m_Open = true;
		} );

		// However, on the main thread check if we are an GLTF file
		// If so, we may need to get the path to the .glb file
		if( m_AssetToImportPath.extension() == ".gltf" || m_AssetToImportPath.extension() == ".glb" )
		{
			m_GLTFBinPath = m_AssetToImportPath;
			// Default to .bin, user can change it
			m_GLTFBinPath.replace_extension( ".bin" );
			
			m_UseBinFile = std::filesystem::exists( m_GLTFBinPath );
		}
	}

	void MeshImportPopup::OnImGuiRender()
	{
		if( m_Open )
			ImGui::OpenPopup( "Import Mesh##IMPORT_MESH" );

		ImGui::SetNextWindowSize( { 350.0F, 0.0F } );
		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );

		if( ImGui::BeginPopupModal( "Import Mesh##IMPORT_MESH", &m_Open, ImGuiWindowFlags_NoSavedSettings ) )
		{
			bool PopupModified = false;

			ImGui::BeginVertical( "##inputv" );

			ImGui::Text( "Path:" );

			ImGui::BeginHorizontal( "##inputH" );

			ImGui::InputText( "##path", ( char* ) m_AssetToImportPath.string().c_str(), 1024 );

			if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
			{
				m_AssetToImportPath = Application::Get().OpenFile( "Supported asset types (*.fbx *.gltf *.glb)\0*.fbx; *.gltf; *.glb\0" );
			}

			ImGui::EndHorizontal();

			ImGui::EndVertical();

			DrawGLTFOptions();

			if( m_IsSkeletal )
				DrawSkeletalMeshOptions();

			DrawAndHandleImportBehaviour();

			ImGui::Separator();

			ImGui::BeginHorizontal( "##actionsH" );

			Auxiliary::DisabledFlag disabledIf( ( m_UseBinFile && m_GLTFBinPath.empty() ) || m_AssetToImportPath.empty() );

			if( ImGui::Button( "Create" ) )
			{
				FullyImportMesh();

				PopupModified = true;
			}

			disabledIf.Pop();

			if( ImGui::Button( "Cancel" ) )
			{
				Close();

				m_ModificationState = AssetImportModificationState::NotModified;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			if( PopupModified )
			{
				Close();
				m_ModificationState = AssetImportModificationState::Modified;

				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

	}

	void MeshImportPopup::DrawGLTFOptions()
	{
		// If the path a GLTF file then we need to file the bin file.
		if( m_AssetToImportPath.extension() == ".gltf" || m_AssetToImportPath.extension() == ".glb" )
		{
			ImGui::BeginVertical( "##gltfinput" );

			ImGui::Text( "GLTF binary file path:" );

			ImGui::BeginHorizontal( "##gltfinputH" );

			ImGui::InputText( "##binpath", ( char* ) m_GLTFBinPath.string().c_str(), 1024, ImGuiInputTextFlags_ReadOnly );

			if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
			{
				m_GLTFBinPath = Application::Get().OpenFile( "Supported asset types (*.glb *.bin)\0*.glb; *.bin\0" );

				m_UseBinFile = std::filesystem::exists( m_GLTFBinPath );
			}

			ImGui::EndHorizontal();

			Auxiliary::DrawBoolControl( "Use binary file (check file ext!)", m_UseBinFile );

			ImGui::EndVertical();
		}
	}

	void MeshImportPopup::DrawSkeletalMeshOptions()
	{
		// use skeleton asset
		// import animations

		ImGui::Text( "DrawSkeletalMeshOptions..." );

		m_CurrentAssetIDForSkeleton == 0 ? ImGui::Text( "NOTE: No Skeleton is selected, a new one will be created!" ) : ImGui::Text( std::to_string( m_CurrentAssetIDForSkeleton ).c_str() );

//		ImGui::Spring();

		bool open = false;
		if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
			open = true;

		Auxiliary::DrawAssetFinder( AssetType::Skeleton, &open, m_CurrentAssetIDForSkeleton );
	}

	void MeshImportPopup::DrawAndHandleImportBehaviour()
	{
		ImGui::BeginHorizontal( "##defMaterial" );

		bool open = false;

		ImGui::Text( "Default Material:" );

		if( ImGui::BeginItemTooltip() )
		{
			ImGui::Text( "todo" );
			ImGui::EndTooltip();
		}

		m_CurrentAssetIDForMaterial == 0 ? ImGui::Text( "None -- create new" ) : ImGui::Text( std::to_string( m_CurrentAssetIDForMaterial ).c_str() );

		ImGui::Spring();

		if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
			open = true;

		Auxiliary::DrawAssetFinder( AssetType::Material, &open, m_CurrentAssetIDForMaterial );

		ImGui::EndHorizontal();

		ImGui::Text( "Import Behaviour:" );

		auto hasFlag = [ this ]( MeshImportBehaviour flag ) -> bool
		{
			return ( m_ImportBehaviour & flag ) != 0;
		};

		ImGui::BeginHorizontal( "##importOption_aum" );

		bool allowUnnamedMaterials = hasFlag( MeshImportBehaviour_AllowUnnamedMaterials );
		ImGui::Text( "Allow Unnamed Materials" );
		ImGui::Spring();

		ImGui::SetNextItemWidth( 130.0f );
		if( ImGui::Checkbox( "##AllowUnnamedMaterials", &allowUnnamedMaterials ) )
		{
			if( hasFlag( MeshImportBehaviour_AllowUnnamedMaterials ) )
				m_ImportBehaviour &= ~MeshImportBehaviour_AllowUnnamedMaterials;
			else
				m_ImportBehaviour |= MeshImportBehaviour_AllowUnnamedMaterials;
		}

		ImGui::EndHorizontal();

		ImGui::BeginHorizontal( "##importOption_nomat" );

		bool noMaterials = hasFlag( MeshImportBehaviour_CreateNoMaterials );
		ImGui::Text( "Don't Create Materials" );
		ImGui::Spring();

		ImGui::SetNextItemWidth( 130.0f );
		if( ImGui::Checkbox( "##NoMaterials", &noMaterials ) )
		{
			if( hasFlag( MeshImportBehaviour_CreateNoMaterials ) )
				m_ImportBehaviour &= ~MeshImportBehaviour_CreateNoMaterials;
			else
				m_ImportBehaviour |= MeshImportBehaviour_CreateNoMaterials;
		}

		ImGui::EndHorizontal();

		ImGui::BeginHorizontal( "##importOption_ext" );

		bool excludeTextures = hasFlag( MeshImportBehaviour_ExcludeTextures );
		ImGui::Text( "Exclude Textures" );
		ImGui::Spring();

		ImGui::SetNextItemWidth( 130.0f );
		if( ImGui::Checkbox( "##ExcludeTextures", &excludeTextures ) )
		{
			if( hasFlag( MeshImportBehaviour_ExcludeTextures ) )
				m_ImportBehaviour &= ~MeshImportBehaviour_ExcludeTextures;
			else
				m_ImportBehaviour |= MeshImportBehaviour_ExcludeTextures;
		}

		ImGui::EndHorizontal();
	}

	void MeshImportPopup::FullyImportMesh()
	{
		if( m_IsSkeletal )
			ImportDynamic();
		else
			ImportStatic();
	}

	void MeshImportPopup::ImportDynamic()
	{
		const auto id = AssetManager::Get().CreateAsset( AssetType::SkeletalMesh );
		auto asset = AssetManager::Get().FindAsset( id );

		// Copy the raw mesh file:
		std::filesystem::copy_file( m_AssetToImportPath, m_DestinationPath / m_AssetToImportPath.filename(), std::filesystem::copy_options::overwrite_existing );

		if( m_UseBinFile )
			std::filesystem::copy_file( m_GLTFBinPath, m_DestinationPath / m_GLTFBinPath.filename(), std::filesystem::copy_options::overwrite_existing );

		auto assetPath = m_DestinationPath / m_AssetToImportPath.filename();
		assetPath.replace_extension( ".skmesh" );

		asset->SetAbsolutePath( assetPath );

		SkeletalMeshImporter meshImporter( m_AssetToImportPath, m_DestinationPath, m_ImportBehaviour );

		//////////////////////////////////////////////////////////////////////////
		// Create the Skeletal Mesh
	}

	void MeshImportPopup::ImportStatic()
	{
		const auto id = AssetManager::Get().CreateAsset( AssetType::StaticMesh );
		auto asset = AssetManager::Get().FindAsset( id );

		// Copy the raw mesh file:
		std::filesystem::copy_file( m_AssetToImportPath, m_DestinationPath / m_AssetToImportPath.filename(), std::filesystem::copy_options::overwrite_existing );

		if( m_UseBinFile )
			std::filesystem::copy_file( m_GLTFBinPath, m_DestinationPath / m_GLTFBinPath.filename(), std::filesystem::copy_options::overwrite_existing );

		auto assetPath = m_DestinationPath / m_AssetToImportPath.filename();
		assetPath.replace_extension( ".stmesh" );

		asset->SetAbsolutePath( assetPath );

		StaticMeshImporter meshImporter( m_AssetToImportPath, m_DestinationPath, m_ImportBehaviour );

		//////////////////////////////////////////////////////////////////////////
		// Create the mesh asset.
		
		auto staticMesh = asset.As<StaticMesh>();
		staticMesh = Ref<StaticMesh>::Create();
		staticMesh->ID = asset->ID;
		staticMesh->Path = asset->Path;

		auto& meshPath = assetPath.replace_extension( m_AssetToImportPath.extension() );
		staticMesh->SetFilepath( meshPath );
		staticMesh->Import_InitMaterialRegistry();

		// TOOD: Unload the material assets!! (Textures could be loaded!)
		for( uint64_t materialID : meshImporter.GetMeshInformation().MaterialAssets )
		{
			staticMesh->GetMaterialRegistry()->AddAsset( AssetManager::Get().GetAssetAs<MaterialAsset>( materialID ) );
		}

		// Serialise the mesh asset
		StaticMeshAssetSerialiser sma;
		sma.Serialise( staticMesh );

		staticMesh->SetAbsolutePath( assetPath );
	}

}
