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
#include "AssetImportPopups.h"

#include "Saturn/Core/App.h"
#include "Saturn/Core/JobSystem.h"

#include "Saturn/Asset/Asset.h"
#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Animation/SkeletonAsset.h"
#include "Saturn/Asset/TextureSourceAsset.h"

#include "Saturn/Serialisation/YAML/AssetManagerSerialiser.h"

#include "Saturn/Audio/Sound.h"

#include "Saturn/Vulkan/Mesh.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Alura/AluraFont.h"

#include "Saturn/Core/Renderer/RenderThread.h"

#include "ImGuiAuxiliary.h"
#include "EditorIcons.h"

#include <imgui.h>

namespace Saturn {
	
	//////////////////////////////////////////////////////////////////////////
	// BASE

	void AssetImportPopupBase::DrawErrorTextAndDescription()
	{
		switch( m_Error )
		{
			default:
			case AssetImportPopupError::None:
				break;
	
			case AssetImportPopupError::MeshNoMaterials:
			{
				ImGui::Text( "Error: MeshNoMaterials (0x%08x)", m_Error );
				ImGui::Text( "No materials exist in the Mesh. A mesh cannot have no materials but it can have no MaterialAssets. In your DCC tool ensure that you have created at least one material slot in the scene!" );
			} break;
		
			case AssetImportPopupError::MeshAssimpInternalError:
			{
				ImGui::Text( "Error: MeshAssimpInternalError (0x%08x)", m_Error );
				ImGui::Text( "An internal Assimp error occurred while importing the mesh." );
			} break;			
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// AUXILIARY

	std::shared_ptr<Saturn::AssetImportPopupBase> AssetImportPopupAuxiliary::CreatePopupFromAssetTypeReimport( Ref<Asset> asset, const std::filesystem::path& rDestinationPath )
	{
		const auto& id = asset->ID;
		const auto type = asset->Type;

		switch( type )
		{
			case AssetType::Texture:
			{
				Ref<TextureSourceAsset> textureAsset = AssetManager::Get()->GetAssetAs<TextureSourceAsset>( id );
				
				return std::make_shared<Saturn::TextureSourceAssetImportPopup>( textureAsset->GetTextureAbsolutePath(), rDestinationPath );
			}

			case AssetType::StaticMesh:
			{
				Ref<StaticMesh> staticMesh = AssetManager::Get()->GetAssetAs<StaticMesh>( id );
				return std::make_shared<Saturn::MeshImportPopup>( staticMesh->FilePath(), rDestinationPath );
			}

			case AssetType::SkeletalMesh:
			{
				Ref<SkeletalMesh> skMesh = AssetManager::Get()->GetAssetAs<SkeletalMesh>( id );
				return std::make_shared<Saturn::MeshImportPopup>( skMesh->FilePath(), rDestinationPath );
			}

			case AssetType::Material:
			case AssetType::MaterialInstance:
				break;

			case AssetType::Sound:
			{
				Ref<SoundSpecification> soundSpec = AssetManager::Get()->GetAssetAs<SoundSpecification>( id );
				return std::make_shared<Saturn::SoundImportPopup>( soundSpec->SoundSourcePath, rDestinationPath );
			}

			case AssetType::GraphSound:
			case AssetType::Scene:
			case AssetType::Prefab:
			case AssetType::Skeleton:
			case AssetType::PhysicsMaterial:
			case AssetType::BehaviourTree:
			case AssetType::BehaviourTreeMemory:
			case AssetType::SkeletalAnimation:
			case AssetType::AnimationController:
				break;

			case AssetType::Font:
			{
				Ref<AluraFont> font = AssetManager::Get()->GetAssetAs<AluraFont>( id );
				return std::make_shared<Saturn::FontImportPopup>( font->GetFontFilepath(), rDestinationPath );
			}	

			case AssetType::StyleProfile:
				break;
			
			case AssetType::Unknown:
			case AssetType::COUNT:
				SAT_CORE_ASSERT( false, "Invalid type!" );
				break;

			default:
				SAT_CORE_ASSERT( false, "Unhandled case!" );
				break;
		}

		return std::make_shared<Saturn::UnknownImportPopup>( "" );
	}

	//////////////////////////////////////////////////////////////////////////
	// UNKNWON POPUP

	UnknownImportPopup::UnknownImportPopup( const std::filesystem::path& rAssetToImportPath )
		: AssetImportPopupBase( rAssetToImportPath, rAssetToImportPath )
	{
	}

	void UnknownImportPopup::Initialise()
	{
		m_Open = true;
		m_IsReady.store( true );
	}

	void UnknownImportPopup::OnImGuiRender()
	{
		if( m_Open )
			ImGui::OpenPopup( "Failed to find a suitable importer##IMPORTX" );

		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );
		if( ImGui::BeginPopupModal( "Failed to find a suitable importer##IMPORTX", &m_Open, ImGuiWindowFlags_NoSavedSettings ) )
		{
			bool PopupModified = false;

			const std::string originPath = m_AssetToImportPath.string();

			ImGui::Text( "Saturn is unable to import %s as it was unable to find a suitable importer!", originPath.c_str() );
			ImGui::Text( "Importers are found based on the file extension!" );

			ImGui::Separator();
			ImGui::BeginHorizontal( "##actionsH" );

			if( ImGui::Button( "OK" ) )
			{
				PopupModified = true;
			}

			ImGui::EndHorizontal();

			if( PopupModified )
			{
				Close();
				m_ModificationState = AssetImportModificationState::NotModified;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// TEXTURE SOURCE IMPORT POPUP

	TextureSourceAssetImportPopup::TextureSourceAssetImportPopup( const std::filesystem::path& rAssetToImportPath, const std::filesystem::path& rDestinationPath )
		: AssetImportPopupBase( rAssetToImportPath, rDestinationPath )
	{
	}

	TextureSourceAssetImportPopup::~TextureSourceAssetImportPopup()
	{
	}

	void TextureSourceAssetImportPopup::Initialise()
	{
		// TODO: In order to show a preview we need to ensure that we destroy the texture
		//       after the frame is done...
		// I'll work on that later...

		// Load preview texture on job system.
//		JobSystem::Get().QueueJob( [ this ]()
		{
//			m_PreviewTexture = Ref<Texture2D>::Create( m_AssetToImportPath, AddressingMode::Repeat );

			// By default textures are flipped.
			m_ImportBehaviour = TextureLoadFlags_FlipVertically;

			m_Open = true;
			m_IsReady.store( true );
		}
//			);
	}

	void TextureSourceAssetImportPopup::OnImGuiRender()
	{
		if( m_Open )
			ImGui::OpenPopup( "Import Texture##IMPORT_Texture" );

		ImGui::SetNextWindowSize( { 350.0F, 0.0F } );
		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );

		if( ImGui::BeginPopupModal( "Import Texture##IMPORT_Texture", &m_Open, ImGuiWindowFlags_NoSavedSettings ) )
		{
			bool PopupModified = false;

			ImGui::BeginVertical( "##inputv" );

			ImGui::Text( "Path:" );

			ImGui::BeginHorizontal( "##inputH" );

			ImGui::InputText( "##path", ( char* ) m_AssetToImportPath.string().c_str(), 1024 );

			if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
			{
				m_AssetToImportPath = Application::Get()->OpenFile( L"Supported asset types (*.png *.jpg *.jpeg)|*.png; *.jpg; *.jpeg" );
			}

			ImGui::EndHorizontal();

			ImGui::EndVertical();

			ImGui::SeparatorText( "Flags" );

			auto hasFlag = [ this ]( TextureLoadFlags flag ) -> bool
			{
				return ( m_ImportBehaviour & flag ) != 0;
			};

			ImGui::BeginHorizontal( "##importOption_flip" );

			bool flip = hasFlag( TextureLoadFlags_FlipVertically );
			ImGui::Text( "Flip Vertically on load" );
			ImGui::Spring();

			ImGui::SetNextItemWidth( 130.0f );
			if( ImGui::Checkbox( "##FlipVert", &flip ) )
			{
				if( hasFlag( TextureLoadFlags_FlipVertically ) )
					m_ImportBehaviour = TextureLoadFlags_None;
				else
					m_ImportBehaviour = TextureLoadFlags_FlipVertically;
			}

			ImGui::EndHorizontal();

#if SAT_PREVIEW_TEXTURE_FIXED
			ImGui::SeparatorText( "Preview Image" );

			const ImVec2 UV0 = ( flip ? ImVec2( 0.0F, 1.0F ) : ImVec2( 0.0F, 0.0F ) );
			const ImVec2 UV1 = ( flip ? ImVec2( 1.0F, 0.0F ) : ImVec2( 1.0F, 1.0F ) );
			Auxiliary::Image( m_PreviewTexture, { 64.0f, 64.0f }, UV0, UV1 );
#endif

			ImGui::BeginHorizontal( "##actionsH" );

			if( ImGui::Button( "Import" ) )
			{
				if( !m_IsReimport )
				{
					CreateNew();
				}
				else
				{
					Reimport();
				}

				PopupModified = true;
			}

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

	void TextureSourceAssetImportPopup::CreateNew()
	{
		const auto id = AssetManager::Get()->CreateAsset( AssetType::Texture );
		auto asset = AssetManager::Get()->FindAsset( id );
		const auto assetPath = m_DestinationPath / m_AssetToImportPath.filename();

		// Copy the source:
		std::filesystem::copy_file( m_AssetToImportPath, assetPath, std::filesystem::copy_options::overwrite_existing );

		// Replace Extension for texture asset
		auto stxPath = assetPath;
		stxPath.replace_extension( ".stx" );
		asset->SetAbsolutePath( stxPath );

		// Create the asset.
		const auto texAsset = Ref<TextureSourceAsset>::Create( asset, assetPath, ( TextureLoadFlags ) m_ImportBehaviour );

		// Save the asset
		TextureSourceAssetSerialiser tsas;
		tsas.Serialise( texAsset );
	}

	void TextureSourceAssetImportPopup::Reimport()
	{
		auto asset = AssetManager::Get()->GetAssetAs<TextureSourceAsset>( m_ReimportID );

		// Act as if we deleted the asset (so it can delete it's source file)
		asset->OnDelete();

		// Copy the source:
		const auto assetPath = m_DestinationPath / m_AssetToImportPath.filename();
		std::filesystem::copy_file( m_AssetToImportPath, assetPath, std::filesystem::copy_options::overwrite_existing );

		asset->OnReimport( assetPath );

		// Save the asset
		TextureSourceAssetSerialiser tsas;
		tsas.Serialise( asset );
	}

	//////////////////////////////////////////////////////////////////////////
	// MESH IMPORT POPUP
	
	MeshImportPopup::MeshImportPopup( const std::filesystem::path& rAssetToImportPath, const std::filesystem::path& rDestinationPath )
		: AssetImportPopupBase( rAssetToImportPath, rDestinationPath )
	{
	}

	void MeshImportPopup::Initialise()
	{
		// Import on job system.
		JobSystem::Get().QueueJob( [this]() 
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

			if( md.CheckResult( MeshDeterminerResult_SkeletalMesh ) || md.CheckResult( MeshDeterminerResult_Animations ) )
			{
				m_IsSkeletal = true;
				m_IsReady.store( true );
			}

			// TODO: We don't currently use MeshDeterminerResult_Materials
			m_Open = true;
		} );

		// However, on the main thread check if we are an GLTF file
		// If so, we may need to get the path to the .glb file
		if( m_AssetToImportPath.extension() == ".gltf" || m_AssetToImportPath.extension() == ".glb" )
		{
			m_GLTFBinPath = m_AssetToImportPath;
			// Default to .bin, user can change it
			m_GLTFBinPath.replace_extension( ".bin" );

			m_UseBinFile = m_GLTFBinFileExists = std::filesystem::exists( m_GLTFBinPath );
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

			Auxiliary::DisabledFlag disabledIfReimport( m_IsReimport );

			ImGui::BeginVertical( "##inputv" );

			ImGui::Text( "Path:" );

			ImGui::BeginHorizontal( "##inputH" );

			ImGui::InputText( "##path", ( char* ) m_AssetToImportPath.string().c_str(), 1024 );

			if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
			{
				m_AssetToImportPath = Application::Get()->OpenFile( L"Supported asset types (*.fbx *.gltf *.glb)|*.fbx; *.gltf; *.glb" );
			}

			ImGui::EndHorizontal();

			ImGui::EndVertical();

			DrawGLTFOptions();

			if( m_IsSkeletal )
				DrawSkeletalMeshOptions();
			
			disabledIfReimport.Pop();

			DrawAndHandleImportBehaviour();

			ImGui::Separator();

			ImGui::BeginHorizontal( "##actionsH" );

			Auxiliary::DisabledFlag disabledIf( ( m_UseBinFile && m_GLTFBinPath.empty() ) || m_AssetToImportPath.empty() );

			if( ImGui::Button( "Create" ) )
			{
				if( const auto error = FullyImportMesh(); error != AssetImportPopupError::None ) 
				{
					m_Error = error;
					m_ModificationState = AssetImportModificationState::Failed;
					ImGui::CloseCurrentPopup();
				}
				else
				{
					PopupModified = true;
				}
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
				m_GLTFBinPath = Application::Get()->OpenFile( L"Supported asset types (*.glb *.bin)|*.glb; *.bin" );

				m_UseBinFile = m_GLTFBinFileExists = std::filesystem::exists( m_GLTFBinPath );
			}

			ImGui::EndHorizontal();

			{
				Auxiliary::ScopedDisabledFlag disabled( !m_GLTFBinFileExists );
				Auxiliary::DrawBoolControl( "Use binary file (check file ext!)", m_UseBinFile );
			}

			ImGui::EndVertical();
		}
	}

	void MeshImportPopup::DrawSkeletalMeshOptions()
	{
		// use skeleton asset
		// import animations
		ImGui::BeginHorizontal( "##importOption_skim" );

		auto hasFlag = [ this ]( MeshImportBehaviour flag ) -> bool
		{
			return ( m_ImportBehaviour & flag ) != 0;
		};

		bool excludeTextures = hasFlag( MeshImportBehaviour_SK_ImportMesh );
		ImGui::Text( "Import Mesh" );
		ImGui::Spring();

		ImGui::SetNextItemWidth( 130.0f );
		if( ImGui::Checkbox( "##SK_ImportMesh", &excludeTextures ) )
		{
			if( hasFlag( MeshImportBehaviour_SK_ImportMesh ) )
				m_ImportBehaviour &= ~MeshImportBehaviour_SK_ImportMesh;
			else
				m_ImportBehaviour |= MeshImportBehaviour_SK_ImportMesh;
		}

		ImGui::EndHorizontal();

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
			ImGui::Text( "todo | MeshImportPopup::DrawAndHandleImportBehaviour" );
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

		if( !noMaterials )
		{
			ImGui::BeginHorizontal( "##importOption_aum" );

			bool allowUnnamedMaterials = hasFlag( MeshImportBehaviour_AllowUnnamedMaterials );
			ImGui::Text( "Allow Unnamed Materials" );

			if( ImGui::BeginItemTooltip() )
			{
				ImGui::Text( "Enable this option if you know that the mesh you are importing will have unnamed materials." );
				ImGui::Text( "If this option is on Saturn will generate material names such as 'UnnamedMaterials[RandomID]'." );

				ImGui::Text( "NOTE: If this flag is not enabled and there is a material with no name the engine will refuse to import it!" );

				ImGui::EndTooltip();
			}

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
		}

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

#if SAT_HAS_IMPORT_SUB_MESH_AS_ASSET
		ImGui::BeginHorizontal( "##importOption_sbm" );

		bool treatSubmeshAsOwnAsset = hasFlag( MeshImportBehaviour_ImportSubMeshAsAsset );
		ImGui::Text( "Import all Submeshes independent" );
		ImGui::Spring();

		ImGui::SetNextItemWidth( 130.0f );
		if( ImGui::Checkbox( "##TreatSubmeshAsOwnAsset", &treatSubmeshAsOwnAsset ) )
		{
			if( hasFlag( MeshImportBehaviour_ImportSubMeshAsAsset ) )
				m_ImportBehaviour &= ~MeshImportBehaviour_ImportSubMeshAsAsset;
			else
				m_ImportBehaviour |= MeshImportBehaviour_ImportSubMeshAsAsset;
		}

		ImGui::EndHorizontal();
#endif

		ImGui::BeginHorizontal( "##importOption_glbsc" );

		bool globalScale = hasFlag( MeshImportBehaviour_GlobalScale );
		ImGui::Text( "Global Scale" );

		if( ImGui::BeginItemTooltip() )
		{
			ImGui::Text( "Converts meters to centimeters." );
			ImGui::EndTooltip();
		}

		ImGui::Spring();

		ImGui::SetNextItemWidth( 130.0f );
		if( ImGui::Checkbox( "##GlobalScale", &globalScale ) )
		{
			if( hasFlag( MeshImportBehaviour_GlobalScale ) )
				m_ImportBehaviour &= ~MeshImportBehaviour_GlobalScale;
			else
				m_ImportBehaviour |= MeshImportBehaviour_GlobalScale;
		}

		ImGui::EndHorizontal();
	}

	AssetImportPopupError MeshImportPopup::FullyImportMesh()
	{
		return ( m_IsSkeletal ? ImportDynamic() : ImportStatic() );
	}

	AssetImportPopupError MeshImportPopup::ImportDynamic()
	{
		if( ( m_ImportBehaviour & MeshImportBehaviour_SK_ImportMesh ) != 0 )
		{
			// Copy the raw mesh file:
			std::filesystem::copy_file( m_AssetToImportPath, m_DestinationPath / m_AssetToImportPath.filename(), std::filesystem::copy_options::overwrite_existing );

			if( m_UseBinFile )
				std::filesystem::copy_file( m_GLTFBinPath, m_DestinationPath / m_GLTFBinPath.filename(), std::filesystem::copy_options::overwrite_existing );
		}

		//////////////////////////////////////////////////////////////////////////
		// Preform import

		if( m_CurrentAssetIDForSkeleton )
			m_ImportBehaviour |= MeshImportBehaviour_SK_MergeWithExistingSK;

		SkeletalMeshImporter meshImporter( m_AssetToImportPath, m_DestinationPath, m_ImportBehaviour, m_CurrentAssetIDForSkeleton );
		
#if !defined(SAT_DIST)
		// No need to find materials when the user has already selected the one the user wants.
		if( m_CurrentAssetIDForMaterial != 0 )
			meshImporter.DisableMaterialSearching();

		if( const auto err = meshImporter.TryImport(); err != AssetImportPopupError::None )
		{
			SAT_CORE_ERROR( "Unable to import dynamic mesh!" );
			return err;
		}
#endif

		//////////////////////////////////////////////////////////////////////////
		// Create the Skeletal Mesh
#if !defined(SAT_DIST)
		if( ( m_ImportBehaviour & MeshImportBehaviour_SK_ImportMesh ) != 0 )
		{
			auto assetPath = m_DestinationPath / m_AssetToImportPath.filename();
			assetPath.replace_extension( ".skmesh" );

			const auto id = AssetManager::Get()->CreateAsset( AssetType::SkeletalMesh );
			auto asset = AssetManager::Get()->FindAsset( id );
			asset->SetAbsolutePath( assetPath );

			auto skeletalMesh = asset.As<SkeletalMesh>();
			skeletalMesh = Ref<SkeletalMesh>::Create();
			skeletalMesh->ID = asset->ID;
			skeletalMesh->Path = asset->Path;

			auto& meshPath = assetPath.replace_extension( m_AssetToImportPath.extension() );
			skeletalMesh->SetFilepath( meshPath );
			skeletalMesh->Import_InitMaterialRegistry();
			skeletalMesh->Import_InitSkeleton( m_CurrentAssetIDForSkeleton == 0 ? meshImporter.GetCreatedSkeletonID() : m_CurrentAssetIDForSkeleton );
			skeletalMesh->Import_SetImportBehaviour( m_ImportBehaviour );

#if !defined(SAT_DIST)
			if( m_CurrentAssetIDForMaterial )
			{
				// NOTE: The size of MaterialAssets will be correct however there will be NO data in the array.
				for( size_t i = 0; i < meshImporter.GetMeshInformation().MaterialAssets.size(); ++i )
				{
					skeletalMesh->GetMaterialRegistry()->AddAsset( AssetManager::Get()->GetAssetAs<MaterialAsset>( m_CurrentAssetIDForMaterial ) );
				}
			}
			else
			{
				// TOOD: Unload the material assets!! (Textures could be loaded!)
				for( uint64_t materialID : meshImporter.GetMeshInformation().MaterialAssets )
				{
					skeletalMesh->GetMaterialRegistry()->AddAsset( AssetManager::Get()->GetAssetAs<MaterialAsset>( materialID ) );
				}
			}
#endif

			// Serialise the mesh asset
			SkeletalMeshAssetSerialiser sma;
			sma.Serialise( skeletalMesh );

			skeletalMesh->SetAbsolutePath( assetPath );
		}
#endif

		return AssetImportPopupError::None;
	}

	AssetImportPopupError MeshImportPopup::ImportStatic()
	{
		// NB: Copy even if this is a reimport, the purpose of a reimport is reload the raw mesh!
		// Copy the raw mesh file:
		std::filesystem::copy_file( m_AssetToImportPath, m_DestinationPath / m_AssetToImportPath.filename(), std::filesystem::copy_options::overwrite_existing );

		if( m_UseBinFile )
			std::filesystem::copy_file( m_GLTFBinPath, m_DestinationPath / m_GLTFBinPath.filename(), std::filesystem::copy_options::overwrite_existing );

		//////////////////////////////////////////////////////////////////////////
		// Preform import

		StaticMeshImporter meshImporter( m_AssetToImportPath, m_DestinationPath, m_ImportBehaviour );

#if !defined(SAT_DIST)
		// No need to find materials when the user has already selected the one she wants.
		if( m_CurrentAssetIDForMaterial != 0 )
			meshImporter.DisableMaterialSearching();

		if( const auto err = meshImporter.TryImport(); err != AssetImportPopupError::None ) 
		{
			SAT_CORE_ERROR( "Unable to import mesh!" );
			return err;
		}
#endif

		if( m_IsReimport )
		{
			auto staticMesh = AssetManager::Get()->GetAssetAs<StaticMesh>( m_ReimportID );

			// Serialise the mesh asset
			StaticMeshAssetSerialiser sma;
			sma.Serialise( staticMesh );

			return AssetImportPopupError::None;
		}

		//////////////////////////////////////////////////////////////////////////
		// Create the mesh asset.

		if( ( m_ImportBehaviour & MeshImportBehaviour_ImportSubMeshAsAsset ) == 0 )
		{
			const auto id = AssetManager::Get()->CreateAsset( AssetType::StaticMesh );
			auto asset = AssetManager::Get()->FindAsset( id );
			auto assetPath = m_DestinationPath / m_AssetToImportPath.filename();
			assetPath.replace_extension( ".stmesh" );
			asset->SetAbsolutePath( assetPath );

			auto staticMesh = asset.As<StaticMesh>();
			staticMesh = Ref<StaticMesh>::Create();
			staticMesh->ID = asset->ID;
			staticMesh->Path = asset->Path;

			auto& meshPath = assetPath.replace_extension( m_AssetToImportPath.extension() );
			staticMesh->SetFilepath( meshPath );
			staticMesh->Import_InitMaterialRegistry();
			staticMesh->Import_SetImportBehaviour( m_ImportBehaviour );

#if !defined(SAT_DIST)
			if( m_CurrentAssetIDForMaterial )
			{
				// NOTE: The size of MaterialAssets will be correct however there will be NO data in the array.
				for( size_t i = 0; i < meshImporter.GetMeshInformation().MaterialAssets.size(); ++i )
				{
					staticMesh->GetMaterialRegistry()->AddAsset( AssetManager::Get()->GetAssetAs<MaterialAsset>( m_CurrentAssetIDForMaterial ) );
				}
			}
			else
			{
				// TOOD: Unload the material assets!! (Textures could be loaded!)
				for( uint64_t materialID : meshImporter.GetMeshInformation().MaterialAssets )
				{
					staticMesh->GetMaterialRegistry()->AddAsset( AssetManager::Get()->GetAssetAs<MaterialAsset>( materialID ) );
				}
			}
#endif

			// Serialise the mesh asset
			StaticMeshAssetSerialiser sma;
			sma.Serialise( staticMesh );

			staticMesh->SetAbsolutePath( assetPath );
		}

		return AssetImportPopupError::None;
	}

	//////////////////////////////////////////////////////////////////////////
	// SOUND IMPORT POPUP

	SoundImportPopup::SoundImportPopup( const std::filesystem::path& rAssetToImportPath, const std::filesystem::path& rDestinationPath )
		: AssetImportPopupBase( rAssetToImportPath, rDestinationPath )
	{
	}

	void SoundImportPopup::Initialise()
	{
		m_Open = true;
		m_IsReady.store( true );
	}

	void SoundImportPopup::OnImGuiRender()
	{
		if( m_Open )
			ImGui::OpenPopup( "Import Sound##IMPORT_SOUND" );

		ImGui::SetNextWindowSize( { 350.0F, 0.0F } );
		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );

		if( ImGui::BeginPopupModal( "Import Sound##IMPORT_SOUND", &m_Open, ImGuiWindowFlags_NoSavedSettings ) )
		{
			bool PopupModified = false;

			ImGui::BeginVertical( "##inputv" );

			ImGui::Text( "Path:" );

			ImGui::BeginHorizontal( "##inputH" );

			ImGui::InputText( "##path", ( char* ) m_AssetToImportPath.string().c_str(), 1024 );

			if( ImGui::Button( "Browse" ) )
			{
				m_AssetToImportPath = Application::Get()->OpenFile( L"Supported asset types (*.wav *.mp3)|*.wav; *.mp3" );
			}

			ImGui::EndHorizontal();
			ImGui::EndVertical();

			ImGui::BeginHorizontal( "##actionsH" );

			if( ImGui::Button( "Create" ) )
			{
				if( !m_IsReimport )
				{
				}
				else
				{

				}

				PopupModified = true;
			}

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
	
	void SoundImportPopup::CreateNew()
	{
		const auto id = AssetManager::Get()->CreateAsset( AssetType::Sound );
		auto asset = AssetManager::Get()->FindAsset( id );
		auto assetPath = m_DestinationPath / m_AssetToImportPath.filename();

		// Copy the audio source.
		std::filesystem::copy_file( m_AssetToImportPath, assetPath, std::filesystem::copy_options::overwrite_existing );

		// Replace Extension for sound asset
		assetPath.replace_extension( ".snd" );
		asset->SetAbsolutePath( assetPath );

		// Create the asset.
		auto sound = Ref<SoundSpecification>::Create( asset );

		sound->OriginalImportPath = m_AssetToImportPath;
		sound->SoundSourcePath = m_DestinationPath / m_AssetToImportPath.filename();

#if !defined(SAT_DIST)
		sound->LastWriteTime = GetLastModificationDate();
#endif

		// Save the asset
		SoundSpecificationAssetSerialiser s2d;
		s2d.Serialise( sound );
	}

	void SoundImportPopup::Reimport()
	{
		auto soundSpec = AssetManager::Get()->GetAssetAs<SoundSpecification>( m_ReimportID );
		if( soundSpec )
		{
			soundSpec->SoundSourcePath = m_AssetToImportPath;

			// Save the asset
			SoundSpecificationAssetSerialiser s2d;
			s2d.Serialise( soundSpec );
		}
	}

	std::string SoundImportPopup::GetLastModificationDate()
	{
		// Currently the date is YYYY-MM-DD HH-MM-SS however all we want is YYYY-MM-DD
		std::string fullTime = std::format( "{0}", std::filesystem::last_write_time( m_AssetToImportPath ) );
		const auto pos = fullTime.find_first_of( " " );
		if( pos != std::string::npos )
			fullTime.resize( fullTime.find_first_of( " " ) );

		return fullTime;
	}

	//////////////////////////////////////////////////////////////////////////
	// FONT IMPORT POPUP

	FontImportPopup::FontImportPopup( const std::filesystem::path& rAssetToImportPath, const std::filesystem::path& rDestinationPath )
		: AssetImportPopupBase( rAssetToImportPath, rDestinationPath )
	{
	}

	void FontImportPopup::Initialise()
	{
		m_Open = true;
		m_IsReady.store( true );
	}

	void FontImportPopup::OnImGuiRender()
	{
		if( m_Open )
			ImGui::OpenPopup( "Import Font##IMPORT_FONT" );

		ImGui::SetNextWindowSize( { 350.0F, 0.0F } );
		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2( 0.5f, 0.5f ) );

		if( ImGui::BeginPopupModal( "Import Font##IMPORT_FONT", &m_Open, ImGuiWindowFlags_NoSavedSettings ) )
		{
			bool PopupModified = false;

			Auxiliary::DisabledFlag disabledIfReimport( m_IsReimport );

			ImGui::Text( "Path:" );
			ImGui::BeginHorizontal( "##inputH" );

			auto pathStr = m_AssetToImportPath.string();
			ImGui::InputText( "##path", ( char* ) pathStr.c_str(), pathStr.size(), ImGuiInputTextFlags_ReadOnly );

			if( ImGui::Button( "Change" ) )
			{
				m_AssetToImportPath = Application::Get()->OpenFile( L"Supported asset types (*.ttf)|*.ttf" );
			}

			ImGui::EndHorizontal();

			disabledIfReimport.Pop();

			if( m_IsReimport )
			{
				ImGui::TextWrapped( "You are unable to change the font itself! To do that you must import a new asset!" );
				ImGui::TextWrapped( "\"Reimport\" is meant if the source file has changed and you'd like to reload it. As fonts are baked once they are imported." );
			}

			ImGui::BeginHorizontal( "##actionsH" );

			// Yes, this is our third time checking m_IsReimport, we could be more smart
			// and do a single check and create separate functions but I don't want to do that...
			if( ImGui::Button( m_IsReimport ? "Reimport" : "Create" ) )
			{
				// ... now it's 4 times.
				if( !m_IsReimport )
				{
					CreateNew();
				}
				else
				{
					Reimport();
				}

				PopupModified = true;
			}

			if( ImGui::Button( "Cancel" ) )
			{
				Close();
				PopupModified = false;
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

	void FontImportPopup::CreateNew() 
	{
		const auto id = AssetManager::Get()->CreateAsset( AssetType::Font );
		auto asset = AssetManager::Get()->FindAsset( id );
		auto assetPath = m_DestinationPath / m_AssetToImportPath.filename();

		// Replace Extension for font asset
		assetPath.replace_extension( ".saf" );
		asset->SetAbsolutePath( assetPath );

		// Create the asset.
		// NB: No serialisation done by this popup! The asset does that when it creates the font atlas.
		auto font = Ref<AluraFont>::Create( m_AssetToImportPath, asset );
	}

	void FontImportPopup::Reimport()
	{
		auto font = AssetManager::Get()->GetAssetAs<AluraFont>( m_ReimportID );
		if( font )
		{
			font->OnReimport( m_AssetToImportPath );
		}
	}

}
