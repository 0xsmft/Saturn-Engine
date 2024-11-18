/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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
#include "ContentBrowserPanel.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/AssetImportPopups.h"

#include "Saturn/Asset/MaterialAsset.h"
#include "Saturn/Asset/PhysicsMaterialAsset.h"
#include "Saturn/Asset/AssetImporter.h"

#include "Saturn/Serialisation/AssetSerialisers.h"
#include "Saturn/Serialisation/SceneSerialiser.h"

#include "Saturn/Core/App.h"
#include "Saturn/Core/Process.h"

#include "Saturn/Project/Project.h"
#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Vulkan/Mesh.h"

#include "Saturn/Serialisation/AssetRegistrySerialiser.h"

#include "Saturn/Asset/Prefab.h"
#include "Saturn/Audio/Sound.h"

#include "Saturn/Premake/Premake.h"

#include "Saturn/GameFramework/Core/SourceFileTemplateHelper.h"
#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"
#include "Saturn/GameFramework/Core/GameModule.h"

#include "ContentBrowserThumbnailGenerator.h"

#include <imgui_internal.h>
#include <ranges>

namespace Saturn {
	
	static inline ImVec2 operator+( const ImVec2& lhs, const ImVec2& rhs ) { return ImVec2( lhs.x + rhs.x, lhs.y + rhs.y ); }

	ContentBrowserPanel::ContentBrowserPanel()
		: ContentBrowserBase()
	{
		m_ViewMode = CBViewMode::Assets;
		ContentBrowserThumbnailGenerator::Init();
	}

	ContentBrowserPanel::ContentBrowserPanel( const std::string& rName )
		: ContentBrowserBase()
	{
	}

	ContentBrowserPanel::~ContentBrowserPanel()
	{
		ContentBrowserThumbnailGenerator::Terminate();
	}

	void ContentBrowserPanel::DrawFolderTree( const std::filesystem::path& rPath )
	{
		for( const auto& entry : std::filesystem::directory_iterator( rPath ) )
		{
			if( !entry.is_directory() )
				continue;

			const std::filesystem::path& entryPath = entry.path();
			const std::string entryName = entryPath.filename().string();

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;

			// I don't know, what happens when we open a folder that is two subfolders down the folder tree will display the assets folder.
			if( m_CurrentPath == entryPath )
				flags |= ImGuiTreeNodeFlags_DefaultOpen;

			if( ImGui::TreeNodeEx( entryName.c_str(), flags ) )
			{
				DrawFolderTree( entryPath );

				ImGui::TreePop();
			}

			if( ImGui::BeginDragDropTarget() )
			{
				auto data = ImGui::AcceptDragDropPayload( "CB_ITEM_MOVE", ImGuiDragDropFlags_None );

				if( data )
				{
					std::filesystem::directory_entry& entry = *( std::filesystem::directory_entry* ) data->Data;

					std::filesystem::path srcPath = entry.path();
					std::filesystem::path dstPath = entryPath / srcPath.filename();

					std::filesystem::path assetPath = std::filesystem::relative( srcPath, Project::GetActiveProject()->GetRootDir() );
					
					std::filesystem::copy_file( entry, dstPath );
					std::filesystem::remove( entry );

					// Find and update the asset that is linked to this path.
					Ref<Asset> target = AssetManager::Get().FindAsset( assetPath );
					target->SetAbsolutePath( dstPath );

					AssetManager::Get().Save();

					// TODO: Change this when we support moving multiple items.
					m_SelectedItems[ 0 ]->Deselect();

					UpdateFiles( true );
				}
			}

			if( ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
			{
				// TODO: Think about this...

				auto path = std::filesystem::relative( entry.path(), m_CurrentViewModeDirectory );

				m_CurrentPath = m_RootPath;
				m_CurrentPath /= path;

				m_ChangeDirectory = true;
			}
		}
	}
	
	void ContentBrowserPanel::DrawAssetsFolderTree()
	{
		DrawFolderTree( m_CurrentViewModeDirectory );
	}

	void ContentBrowserPanel::DrawScriptsFolderTree()
	{
		DrawFolderTree( m_ScriptPath );
	}

	void ContentBrowserPanel::DrawRootFolder( CBViewMode type, bool open/* = false*/ )
	{
		switch( type )
		{
			case CBViewMode::Assets: 
			{
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
				flags |= ImGuiTreeNodeFlags_DefaultOpen;

				ImGui::PushID( "PrjAssets" );

				bool opened = ImGui::TreeNodeEx( "Assets##PrjAssets", flags );

				if( ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
				{
					ClearSelection();

					// Switch and set path to the game content.
					m_ViewMode = CBViewMode::Assets;
					ResetPath( Project::GetActiveProject()->GetRootDir() );
				}

				if( opened )
				{
					DrawAssetsFolderTree();

					ImGui::TreePop();
				}

				ImGui::PopID();
			} break;

			case CBViewMode::Scripts: 
			{
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;

				flags |= ImGuiTreeNodeFlags_DefaultOpen;

				ImGui::PushID( "PrjScripts" );

				bool opened = ImGui::TreeNodeEx( "Source##PrjScripts", flags );

				if( ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
				{
					ClearSelection();
				
					// Switch and set path to the game source.
					m_ViewMode = CBViewMode::Scripts;
					ResetPath( Project::GetActiveProject()->GetRootDir() );
				}

				if( opened )
				{
					DrawScriptsFolderTree();

					ImGui::TreePop();
				}

				ImGui::PopID();
			} break;

			default:
				break;
		}
	}

	void ContentBrowserPanel::DrawBaseContextMenu() 
	{
		// Theses actions are only going to be used when one item is selected.
		// SELECTED ITEMS ACTIONS (FOR FOLDERS AND ASSETS)
		if( m_SelectedItems.size() )
		{
			// Common Actions
			if( ImGui::MenuItem( "Rename" ) )
			{
				m_SelectedItems[ 0 ]->Rename();
			}

			// Folder Actions
			if( m_SelectedItems[ 0 ]->IsDirectory() )
			{
				if( ImGui::MenuItem( "Show In Explorer" ) )
				{
					std::wstring CommandLine = L"";
					std::filesystem::path AssetPath = m_SelectedItems[ 0 ]->Path();
					CommandLine = std::format( L"explorer.exe \"{0}\"", AssetPath.wstring() );

					DeatchedProcess dp( CommandLine );
				}
			}
			else
			{
				if( ImGui::MenuItem( "Delete" ) )
				{
					for( auto& rItem : m_SelectedItems )
					{
						rItem->Delete();
					}
				}
			}
		}
		else
			AssetsPopupContextMenu();
	}

	void ContentBrowserPanel::AssetsPopupContextMenu()
	{
		// NON-SELECTED ITEMS ACTIONS (WHEN RIGHT CLICKING ON PANEL, ONLY WHEN VIEWING ASSETS)
		if( ImGui::BeginMenu( "Import" ) )
		{
			if( ImGui::MenuItem( "Starter Assets" ) )
			{
				auto ActiveProject = Project::GetActiveProject();
				auto AssetPath = ActiveProject->GetAssetPath();

				std::filesystem::copy_file( "content/Templates/Meshes/Cube.fbx", AssetPath / "Meshes" / "Cube.fbx" );
				std::filesystem::copy_file( "content/Templates/Meshes/Plane.fbx", AssetPath / "Meshes" / "Plane.fbx" );
			}

			if( ImGui::MenuItem( "Browse" ) )
			{
				auto result = Application::Get().OpenFile( "Supported asset types (*.fbx *.gltf *.glb *.png *.tga *.jpeg *.jpg *wav *.ogg *.mp3)\0*.fbx; *.gltf; *.glb; *.png; *.tga; *.jpeg; *jpg; *.wav; *.ogg; *.mp3\0" );

				std::filesystem::path path = result;

				if( path.extension() == ".png" || path.extension() == ".tga" || path.extension() == ".jpeg" || path.extension() == ".jpg" )
				{
					auto id = AssetManager::Get().CreateAsset( AssetType::Texture );

					auto asset = AssetManager::Get().FindAsset( id );

					std::filesystem::copy_file( path, m_CurrentPath / path.filename() );

					asset->SetAbsolutePath( m_CurrentPath / path.filename() );

					AssetRegistrySerialiser ars;
					ars.Serialise( AssetManager::Get().GetAssetRegistry() );
				}

				// Meshes
				if( path.extension() == ".fbx" || path.extension() == ".gltf" )
				{
					m_ShowAssetImportPopup = true;
					m_ImportAssetPath = path;
					m_AssetImportType = AssetType::StaticMesh;
				}

				// Audio
				if( path.extension() == ".wav" || path.extension() == ".mp3" || path.extension() == ".ogg" )
				{
					m_ShowAssetImportPopup = true;
					m_ImportAssetPath = path;
					m_AssetImportType = AssetType::Sound;
				}
			}

			ImGui::EndMenu();
		}

		if( ImGui::BeginMenu( "Create" ) )
		{
			if( ImGui::MenuItem( "New Folder" ) )
			{
				auto newPath = m_CurrentPath / "New Folder";
				int32_t count = GetFilenameCount( "New Folder" );

				if( count >= 1 )
				{
					newPath = std::format( "{0}\\{1} ({2})", m_CurrentPath.string(), "New Folder", count );
				}

				std::filesystem::create_directories( newPath );

				UpdateFiles( true );
				FindAndRenameItem( newPath.stem() );
			}

			if( ImGui::MenuItem( "New Material" ) )
			{
				auto id = AssetManager::Get().CreateAsset( AssetType::Material );
				auto asset = AssetManager::Get().FindAsset( id );
				auto newPath = m_CurrentPath / "Untitled Material.smaterial";
				int32_t count = GetFilenameCount( "Untitled Material.smaterial" );

				if( count >= 1 )
				{
					newPath = std::format( "{0}\\{1} ({2}).smaterial", m_CurrentPath.string(), "Untitled Material", count );
				}

				asset->SetAbsolutePath( newPath );
				Ref<MaterialAsset> material = asset;
				material = Ref<MaterialAsset>::Create( nullptr );

				// TODO: (Asset) Fix this.
				struct
				{
					UUID ID;
					AssetType Type;
					uint32_t Flags;
					std::filesystem::path Path;
					std::string Name;
				} OldAssetData = {};

				OldAssetData.ID = asset->ID;
				OldAssetData.Type = asset->Type;
				OldAssetData.Flags = asset->Flags;
				OldAssetData.Path = asset->Path;
				OldAssetData.Name = asset->Name;

				asset = material;
				asset->ID = OldAssetData.ID;
				asset->Type = OldAssetData.Type;
				asset->Flags = OldAssetData.Flags;
				asset->Path = OldAssetData.Path;
				asset->Name = OldAssetData.Name;

				MaterialAssetSerialiser mas;
				mas.Serialise( asset );

				AssetRegistrySerialiser ars;
				ars.Serialise( AssetManager::Get().GetAssetRegistry() );

				UpdateFiles( true );
				
				FindAndRenameItem( asset->Name );
			}

			if( ImGui::MenuItem( "New Physics Material" ) )
			{
				auto id = AssetManager::Get().CreateAsset( AssetType::PhysicsMaterial );
				auto asset = AssetManager::Get().FindAsset( id );
				auto newPath = m_CurrentPath / "Untitled Physics Material.sphymaterial";

				int32_t count = GetFilenameCount( "Untitled Physics Material.sphymaterial" );

				if( count >= 1 )
				{
					newPath = std::format( "{0}\\{1} ({2}).sphymaterial", m_CurrentPath.string(), "Untitled Physics Material", count );
				}

				asset->SetAbsolutePath( newPath );

				auto materialAsset = asset.As<PhysicsMaterialAsset>();

				PhysicsMaterialAssetSerialiser mas;
				mas.Serialise( materialAsset );

				AssetRegistrySerialiser ars;
				ars.Serialise( AssetManager::Get().GetAssetRegistry() );

				UpdateFiles( true );
				FindAndRenameItem( asset->Name );
			}

			if( ImGui::MenuItem( "New Scene" ) )
			{
				auto id = AssetManager::Get().CreateAsset( AssetType::Scene );
				auto asset = AssetManager::Get().FindAsset( id );
				auto newPath = m_CurrentPath / "Empty Scene.scene";
				int32_t count = GetFilenameCount( "Empty Scene.scene" );

				if( count >= 1 )
				{
					newPath = std::format( "{0}\\{1} ({2}).scene", m_CurrentPath.string(), "Empty Scene", count );
				}

				asset->SetAbsolutePath( newPath );

				// Only set id and path, temporary asset
				Ref<Scene> newScene = Ref<Scene>::Create();
				newScene->SetAbsolutePath( newPath );
				newScene->ID = id;

				Scene* CurrentScene = GActiveScene;
				Scene::SetActiveScene( newScene.Get() );

				SceneSerialiser ss( newScene );
				ss.Serialise();

				Scene::SetActiveScene( CurrentScene );

				AssetManager::Get().Save();

				UpdateFiles( true );
				FindAndRenameItem( newScene->Name );
			}

			if( ImGui::MenuItem( "New Graph Sound" ) )
			{
				auto id = AssetManager::Get().CreateAsset( AssetType::GraphSound );
				auto asset = AssetManager::Get().FindAsset( id );
				auto newPath = m_CurrentPath / "New Sound Editor.gsnd";
				int32_t count = GetFilenameCount( "New Sound Editor.gsnd" );

				if( count >= 1 )
					newPath = std::format( "{0}\\{1} ({2}).gsnd", m_CurrentPath.string(), "New Sound Editor", count );

				asset->SetAbsolutePath( newPath );

				std::ofstream fout( newPath );
				fout.close();

				AssetManager::Get().Save();

				UpdateFiles( true );
				FindAndRenameItem( asset->Name );
			}

			if( ImGui::MenuItem( "New Class Instance" ) ) 
			{
				m_OpenClassInstancePopup = true;
			}

			ImGui::EndMenu();
		}

		if( ImGui::MenuItem( "Show folder in explorer" ) )
		{
			std::wstring CommandLine = L"";
			CommandLine = std::format( L"explorer.exe \"{0}\"", m_CurrentPath.wstring() );

			DeatchedProcess dp( CommandLine );
		}
	}

	void ContentBrowserPanel::ScriptsPopupContextMenu()
	{
		if( ImGui::BeginMenu( "Create" ) )
		{
			if( ImGui::MenuItem( "New Class" ) )
			{
				m_OpenScriptsPopup = true;
			}

			ImGui::EndMenu();
		}
	}

	void ContentBrowserPanel::OnFilewatchEvent( const std::wstring& rPath, const filewatch::Event Event )
	{
		switch( Event )
		{
			case filewatch::Event::added: 
			case filewatch::Event::removed:
			{
				ClearSearchQuery();
				UpdateFiles( true );
			} break;

			case filewatch::Event::modified:
			{
			} break;

			case filewatch::Event::renamed_new:
			case filewatch::Event::renamed_old:
			{
				ClearSearchQuery();
				UpdateFiles( true );
			} break;

			default:
				break;
		}
	}

	Ref<ContentBrowserItem> ContentBrowserPanel::GetActiveHoveredItem()
	{
		const auto Itr = std::find_if( m_Files.begin(), m_Files.end(), []( auto& rItem ) { return rItem->IsHovered(); } );

		if( Itr != m_Files.end() )
			return *Itr;

		return nullptr;
	}

	void ContentBrowserPanel::BuildSearchList()
	{
		if( m_ValidSearchFiles.size() )
			m_ValidSearchFiles.clear();

		for( const auto& entry : std::filesystem::recursive_directory_iterator( m_CurrentViewModeDirectory ) )
		{
			const std::filesystem::path& rPath = entry.path();

			if( m_TextFilter.PassFilter( rPath.filename().string().c_str() ) )
			{
				Ref<ContentBrowserItem> item = Ref<ContentBrowserItem>::Create( entry, ContentBrowserItemType::Asset );
				item->SetSelectedFn( SAT_BIND_EVENT_FN( ContentBrowserPanel::OnItemSelected ) );

				if( auto Itr = std::find( m_ValidSearchFiles.begin(), m_ValidSearchFiles.end(), item ); Itr != m_ValidSearchFiles.end() )
				{
					if( !std::filesystem::exists( entry ) )
						m_ValidSearchFiles.erase( Itr, m_ValidSearchFiles.end() );

					continue;
				}

				if( !entry.is_directory() )
				{
					if( ExtensionToAssetType( rPath.extension().string() ) == AssetType::Unknown )
						continue;
				}

				m_ValidSearchFiles.push_back( item );
			}
		}
	}

	void ContentBrowserPanel::Draw()
	{
		if( ImGui::Begin( "Content Browser", &m_Open ) ) 
		{
			if( m_ChangeDirectory )
			{
				ClearSearchQuery();
				UpdateFiles( true );

				m_ChangeDirectory = false;
			}

			ImGui::PushStyleColor( ImGuiCol_ChildBg, ImVec4( 0.0f, 0.0f, 0.0f, 0.0f ) );

			ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
			ImGui::BeginChild( "Top Bar", ImVec2( 0, 30 ), false, flags );

			DrawTopBar();

			ImGui::EndChild();

			ImGui::BeginChild( "Folder Tree", ImVec2( 200, 0 ), false );

			if( Auxiliary::TreeNode( Project::GetActiveProject()->GetConfig().Name.c_str() ) )
			{
				DrawRootFolder( CBViewMode::Assets );
				DrawRootFolder( CBViewMode::Scripts );

				Auxiliary::EndTreeNode();
			}

			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild( "Folder Contents", ImVec2( 0, 0 ), false );

			// Search
			ImGui::BeginHorizontal( "##cbfinder" );

			if( m_TextFilter.DrawWithHint( "##contentfinder", "Search for content", 436.0f ) )
			{
				m_Searching = m_TextFilter.IsActive();
				BuildSearchList();
			}

			ImGui::EndHorizontal();

			ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.0f, 0.0f, 0.0f, 0.0f ) );
			ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.3f, 0.3f, 0.3f, 0.35f ) );

			constexpr float padding = 16.0f;
			constexpr int thumbnailSizeX = 180;
			constexpr int thumbnailSizeY = 180;
			constexpr int cellSize = thumbnailSizeX + static_cast< int >( padding );
			float panelWidth = ImGui::GetContentRegionAvail().x - 20.0f + ImGui::GetStyle().ScrollbarSize;

			int columnCount = ( int ) ( panelWidth / cellSize );
			if( columnCount < 1 ) columnCount = 1;

			ImGui::Columns( columnCount, 0, false );

			if( m_Searching )
			{
				DrawItems( m_ValidSearchFiles, { thumbnailSizeX, thumbnailSizeY }, padding );

				// No longer searching, means that user has clicked on a file/folder.
				if( !m_Searching )
					ClearSearchQuery();
			}
			else
			{
				DrawItems( m_Files, { thumbnailSizeX, thumbnailSizeY }, padding );
			}

			if( !m_Searching && m_ValidSearchFiles.size() )
				m_ValidSearchFiles.clear();

			// Get the first folder in the current directory.
			m_FirstFolder = "";
			for( auto& rEntry : std::filesystem::directory_iterator( m_CurrentPath ) )
			{
				if( rEntry.is_directory() )
				{
					m_FirstFolder = rEntry.path();
					break;
				}
			}

			if( ImGui::IsMouseDown( 0 ) && ImGui::IsWindowHovered() )
			{
				auto& map = m_Searching ? m_ValidSearchFiles : m_Files;

				auto hoveredItems = std::count_if( map.begin(), map.end(),
					[]( const auto& rItem )
					{
						return rItem->IsHovered();
					} );

				if( m_SelectedItems.size() && hoveredItems == 0 )
				{
					ClearSelection();
				}
			}

			ImGui::Columns( 1 );

			ImGui::PopStyleColor( 2 );

			// CONTEXT MENU (RIGHT CLICK MENU)
			if( ImGui::BeginPopupContextWindow( "CB_ItemAction", ImGuiPopupFlags_MouseButtonRight ) )
			{
				switch( m_ViewMode )
				{
					case CBViewMode::Assets:
						DrawBaseContextMenu();
						break;

					case CBViewMode::Scripts:
						ScriptsPopupContextMenu();
						break;
				}

				ImGui::EndPopup();
			}

			DrawImportSoundPopup();
			DrawImportMeshPopup();

			if( m_OpenScriptsPopup )
				ImGui::OpenPopup( "Create A New Class##Create_Script" );

			ImGui::SetNextWindowSize( { 350.0F, 0.0F } );
			if( ImGui::BeginPopupModal( "Create A New Class##Create_Script", &m_OpenScriptsPopup, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings ) )
			{
				bool PopupModified = false;

				ImGui::BeginVertical( "##inputv" );
				ImGui::BeginHorizontal( "##inputh" );

				ImGui::Text( "Name:" );
				char buffer[ 256 ];
				memset( buffer, 0, 256 );
				memcpy( buffer, m_NewClassName.data(), m_NewClassName.length() );

				if( ImGui::InputText( "##n", buffer, 256 ) )
				{
					m_NewClassName = std::string( buffer );
				}

				ImGui::EndHorizontal();

				ImGuiIO& rIO = ImGui::GetIO();
				auto boldFont = rIO.Fonts->Fonts[ 1 ];

				ImGui::PushFont( boldFont );
				ImGui::Text( "Choose a parent class" );
				ImGui::PopFont();

				if( ImGui::BeginListBox( "##classes", ImVec2( -FLT_MIN, 0.0f ) ) )
				{
					// Root Tree
					DrawClassHierarchy( "SClass", ClassMetadataHandler::Get().GetSClassMetadata() );

					ImGui::EndListBox();
				}

				ImGui::EndVertical();

				auto drawDisabledBtn = [&]( const char* n ) -> bool
					{
						ImGui::PushItemFlag( ImGuiItemFlags_Disabled, true );
						ImGui::PushStyleVar( ImGuiStyleVar_Alpha, 0.5f );
						bool value = ImGui::Button( n );
						ImGui::PopStyleVar( 1 );
						ImGui::PopItemFlag();

						return value;
					};

				bool Pressed = false;
				if( m_NewClassName.empty() || m_SelectedMetadata.Name.empty() )
					Pressed = drawDisabledBtn( "Create" );
				else
					Pressed = ImGui::Button( "Create" );

				if( Pressed )
				{
					if( !Project::GetActiveProject()->HasPremakeFile() )
					{
						Project::GetActiveProject()->CreatePremakeFile();
					}

					Project::GetActiveProject()->CreateBuildFile();

					// Update or create the project files.
					Premake::Launch( Project::GetActiveProject()->GetRootDir().wstring() );

					// TODO: Get the correct source files.
					SourceFileTemplateHelper::CreateEntitySourceFiles( m_CurrentPath, m_NewClassName.c_str() );

					AssetRegistrySerialiser ars;
					ars.Serialise( AssetManager::Get().GetAssetRegistry() );

					PopupModified = true;

					UpdateFiles( true );
				}

				if( PopupModified )
				{
					m_OpenScriptsPopup = false;

					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			if( m_OpenClassInstancePopup )
				ImGui::OpenPopup( "Create A New Class Instance (Prefab)##Create_ClassIns" );

			ImGui::SetNextWindowSize( { 350.0F, 0.0F } );
			if( ImGui::BeginPopupModal( "Create A New Class Instance (Prefab)##Create_ClassIns", &m_OpenClassInstancePopup, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings ) )
			{
				bool PopupModified = false;

				ImGui::BeginHorizontal( "##inputH" );

				ImGui::Text( "Name:" );

				// I wish that we did not have to do this. But for some reason ImGui does not work well when I use a string.
				char buffer[ 256 ];
				memset( buffer, 0, 256 );
				memcpy( buffer, m_ClassInstanceName.c_str(), m_ClassInstanceName.length() );

				if( ImGui::InputText( "##instanceName", buffer, 256 ) )
				{
					m_ClassInstanceName = std::string( buffer );
				}

				ImGui::EndHorizontal();

				ImGui::Text( "Choose Parent class" );

				if( ImGui::BeginListBox( "##CLASSES_INST", ImVec2( -FLT_MIN, 0.0f ) ) )
				{
					// Root Tree
					DrawClassHierarchy( "SClass", ClassMetadataHandler::Get().GetSClassMetadata() );

					ImGui::EndListBox();
				}

				auto drawDisabledBtn = [&]( const char* n ) -> bool
					{
						ImGui::PushItemFlag( ImGuiItemFlags_Disabled, true );
						ImGui::PushStyleVar( ImGuiStyleVar_Alpha, 0.5f );
						bool value = ImGui::Button( n );
						ImGui::PopStyleVar( 1 );
						ImGui::PopItemFlag();

						return value;
					};

				if( ImGui::Button( "Create" ) )
				{
					// First, create the asset.
					Ref<Asset> prefabAsset = AssetManager::Get().FindAsset( AssetManager::Get().CreateAsset( AssetType::Prefab ) );
					Ref<Prefab> prefab = prefabAsset.As<Prefab>();
					prefab = Ref<Prefab>::Create();

					// Setup the asset
					std::filesystem::path path = m_CurrentPath / m_ClassInstanceName;
					path.replace_extension( ".prefab" );

					prefab->SetAbsolutePath( path );
					prefab->Name = m_ClassInstanceName;
					prefab->ID = prefabAsset->ID;
					prefab->Type = prefabAsset->Type;
					prefab->Flags = prefabAsset->Flags;

					// Create the source entity.
					Ref<Entity> sourceEntity = nullptr;

					if( ClassMetadataHandler::Get().IsEngineMetadata( m_SelectedMetadata ) )
					{
						// TODO: Create the class somehow?
					}
					else
					{
						sourceEntity = GameModule::Get().CreateEntity( m_SelectedMetadata.Name );
					}

					sourceEntity->AddComponent<ScriptComponent>().ScriptName = m_SelectedMetadata.Name;

					prefab->Create( sourceEntity );

					// Delete the temporary source entity from the current scene.
					GActiveScene->DeleteEntity( sourceEntity, true, 0 );

					// Save the prefab.
					PrefabSerialiser ps;
					ps.Serialise( prefab );

					AssetManager::Get().Save();

					PopupModified = true;
				}

				if( PopupModified )
				{
					m_OpenClassInstancePopup = false;
					m_ClassInstanceName = "";

					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			ImGui::EndChild();

			ImGui::PopStyleColor();
		}

		ImGui::End();
	}

	void ContentBrowserPanel::DrawClassHierarchy( const std::string& rKeyName, const SClassMetadata& rData )
	{
		ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
		m_SelectedMetadata.Name == rKeyName ? Flags |= ImGuiTreeNodeFlags_Selected : 0;

		bool opened = ImGui::TreeNodeEx( rKeyName.c_str(), Flags );

		if( ImGui::IsItemClicked() )
		{
			m_SelectedMetadata = rData;
		}

		if( opened ) 
		{
			ClassMetadataHandler::Get().EachTreeNode(
				[&]( auto& rMetadata )
				{
					if( rMetadata.ParentClassName == rKeyName )
					{
						DrawClassHierarchy( rMetadata.Name, rMetadata );
					}
				} );

			Auxiliary::EndTreeNode();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// POPUPS

	void ContentBrowserPanel::DrawImportSoundPopup() 
	{
		if( m_AssetImportType != AssetType::Sound )
			return;

		if( m_ShowAssetImportPopup )
			ImGui::OpenPopup( "Import Sound##IMPORT_SOUND" );
		
		bool PopupModified = false;
		ImGui::SetNextWindowSize( { 350.0F, 0.0F } );
		if( ImGui::BeginPopupModal( "Import Sound##IMPORT_SOUND", &m_ShowAssetImportPopup, ImGuiWindowFlags_NoMove ) )
		{
			ImGui::BeginVertical( "##inputv" );

			ImGui::Text( "Path:" );

			ImGui::BeginHorizontal( "##inputH" );

			ImGui::InputText( "##path", ( char* ) m_ImportAssetPath.string().c_str(), 1024 );

			if( ImGui::Button( "Browse" ) )
			{
				m_ImportAssetPath = Application::Get().OpenFile( "Supported asset types (*.wav *.mp3)\0*.wav; *.mp3\0" );
			}

			ImGui::EndHorizontal();
			ImGui::EndVertical();

			ImGui::BeginHorizontal( "##actionsH" );

			if( ImGui::Button( "Create" ) )
			{
				auto id = AssetManager::Get().CreateAsset( AssetType::Sound );
				auto asset = AssetManager::Get().FindAsset( id );

				// Copy the audio source.
				std::filesystem::copy_file( m_ImportAssetPath, m_CurrentPath / m_ImportAssetPath.filename() );

				auto assetPath = m_CurrentPath / m_ImportAssetPath.filename();
				assetPath.replace_extension( ".snd" );

				assetPath = std::filesystem::relative( assetPath, Project::GetActiveProject()->GetRootDir() );

				asset->Path = assetPath;

				// Create the asset.
				auto sound = asset.As<SoundSpecification>();
				sound = Ref<SoundSpecification>::Create();
				sound->ID = asset->ID;
				sound->Path = assetPath;
				sound->Type = AssetType::Sound;

				sound->OriginalImportPath = m_ImportAssetPath;
				sound->SoundSourcePath = m_CurrentPath / m_ImportAssetPath.filename();

#if !defined(SAT_DIST)
				// Currently the date is YYYY-MM-DD HH-MM-SS however all we want is YYYY-MM-DD
				std::string fullTime = std::format( "{0}", std::filesystem::last_write_time( m_ImportAssetPath ) );
				fullTime = fullTime.substr( 0, fullTime.find_first_of( " " ) );

				sound->LastWriteTime = fullTime;
#endif

				// Save the asset
				SoundSpecificationAssetSerialiser s2d;
				s2d.Serialise( sound );

				sound->SetAbsolutePath( assetPath );

				PopupModified = true;
			}

			auto exitPopup = [&]( ) -> void
			{
				m_ImportAssetPath = "";
				m_ShowAssetImportPopup = false;
				m_AssetImportType = AssetType::Unknown;
			};

			if( ImGui::Button( "Cancel" ) )
			{
				exitPopup();

				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			if( PopupModified )
			{
				exitPopup();

				AssetRegistrySerialiser ars;
				ars.Serialise( AssetManager::Get().GetAssetRegistry() );

				UpdateFiles( true );

				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void ContentBrowserPanel::DrawImportMeshPopup()
	{
		if( m_AssetImportType != AssetType::StaticMesh )
			return;

		if( m_ShowAssetImportPopup )
			ImGui::OpenPopup( "Import Mesh##IMPORT_MESH" );
	
		bool PopupModified = false;
		ImGui::SetNextWindowSize( { 350.0F, 0.0F } );
		if( ImGui::BeginPopupModal( "Import Mesh##IMPORT_MESH", &m_ShowAssetImportPopup, ImGuiWindowFlags_NoMove ) )
		{
			static std::filesystem::path s_GLTFBinPath = "";
			static bool s_UseBinFile = false;

			ImGui::BeginVertical( "##inputv" );

			ImGui::Text( "Path:" );

			ImGui::BeginHorizontal( "##inputH" );

			ImGui::InputText( "##path", ( char* ) m_ImportAssetPath.string().c_str(), 1024 );

			if( ImGui::Button( "Browse" ) )
			{
				m_ImportAssetPath = Application::Get().OpenFile( "Supported asset types (*.fbx *.gltf *.glb)\0*.fbx; *.gltf; *.glb\0" );
			}

			ImGui::EndHorizontal();

			ImGui::EndVertical();

			// If the path a GLTF file then we need to file the bin file.
			if( m_ImportAssetPath.extension() == ".gltf" || m_ImportAssetPath.extension() == ".glb" )
			{
				// We can assume the bin file has the same name as the mesh.
				if( s_GLTFBinPath == "" )
				{
					s_GLTFBinPath = m_ImportAssetPath;
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
				std::filesystem::copy_file( m_ImportAssetPath, m_CurrentPath / m_ImportAssetPath.filename() );

				if( s_UseBinFile )
					std::filesystem::copy_file( s_GLTFBinPath, m_CurrentPath / s_GLTFBinPath.filename() );

				auto assetPath = m_CurrentPath / m_ImportAssetPath.filename();
				assetPath.replace_extension( ".stmesh" );

				asset->SetAbsolutePath( assetPath );

				// TODO: This is bad.
				// Create the mesh so we can copy over the texture (if any).
				auto mesh = Ref<MeshSource>::Create( m_ImportAssetPath, m_CurrentPath );
				mesh = nullptr;

				// Create the mesh asset.
				auto staticMesh = asset.As<StaticMesh>();
				staticMesh = Ref<StaticMesh>::Create();
				staticMesh->ID = asset->ID;
				staticMesh->Path = asset->Path;

				auto& meshPath = assetPath.replace_extension( m_ImportAssetPath.extension() );
				staticMesh->SetFilepath( meshPath.string() );

				// Save the mesh asset
				StaticMeshAssetSerialiser sma;
				sma.Serialise( staticMesh );

				staticMesh->SetAbsolutePath( assetPath );

				PopupModified = true;
			}

			auto exitPopup = [&]() -> void
			{
				m_ImportAssetPath = "";
				m_ShowAssetImportPopup = false;
				m_AssetImportType = AssetType::Unknown;
			};

			if( ImGui::Button( "Cancel" ) )
			{
				exitPopup();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			if( PopupModified )
			{
				exitPopup();

				AssetRegistrySerialiser ars;
				ars.Serialise( AssetManager::Get().GetAssetRegistry() );

				UpdateFiles( true );

				s_GLTFBinPath = "";
				s_UseBinFile = false;

				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void ContentBrowserPanel::ClearSearchQuery()
	{
		m_TextFilter.Clear();
		m_ValidSearchFiles.clear();
	}

	void ContentBrowserPanel::ResetPath( const std::filesystem::path& rProjectRootPath )
	{
		ClearSearchQuery();

		m_ScriptPath = Project::GetActiveProject()->GetSourceDir();

		switch( m_ViewMode )
		{
			case Saturn::CBViewMode::Assets: 
			{
				m_CurrentViewModeDirectory = rProjectRootPath / "Assets";
			} break;

			case Saturn::CBViewMode::Scripts: 
			{
				m_CurrentViewModeDirectory = m_ScriptPath;
			} break;
		}

		// Only create a new watcher if the root path has changed e.g. going from assets to scripts.
		// Avoid destroying and creating new threads
		// Condition m_RootPath != m_CurrentViewModeDirectory is only true if the user wants to go to the root folder in the same view mode.
		if( m_RootPath != m_CurrentViewModeDirectory )
		{
			m_RootPath = m_CurrentViewModeDirectory;

			delete m_Watcher;
			m_Watcher = new filewatch::FileWatch<std::wstring>( m_RootPath.wstring(),
				[this]( const std::wstring& path, const filewatch::Event event )
				{
					OnFilewatchEvent( path, event );
				} );
		}
		else
			m_RootPath = m_CurrentViewModeDirectory;

		m_CurrentPath = m_CurrentViewModeDirectory;
		m_FirstFolder = m_CurrentViewModeDirectory;

		UpdateFiles( true );
	}

	void ContentBrowserPanel::BrowseToItem( const std::filesystem::path& rPath, AssetID id )
	{
		if( id == 0 || rPath.empty() )
			return;

		// First, browse to where the item is located as we may need to update the files
		// Then, find item and select it.
		const auto fullPath = Project::GetActiveProject()->FilepathAbs( rPath.parent_path() );

		// Force an update here don't wait until next frame
		if( m_CurrentPath != fullPath )
		{
			m_CurrentPath /= fullPath;
		
			ClearSelection();
			ClearSearchQuery();
			m_Searching = false;

			UpdateFiles( true );
		}

		for( auto& rItem : m_Files )
		{
			if( rItem->GetAssetID() == id )
			{
				rItem->Select();
				break;
			}
		}
	}

	void ContentBrowserPanel::GetContentFiles( bool clear )
	{
		for( auto& rEntry : std::filesystem::directory_iterator( m_CurrentPath ) )
		{
			Ref<ContentBrowserItem> item = Ref<ContentBrowserItem>::Create( rEntry, ContentBrowserItemType::Asset );
			item->SetSelectedFn( SAT_BIND_EVENT_FN( ContentBrowserPanel::OnItemSelected ) );

			// Item will never exist if we have cleared the list.
			if( !clear )
			{
				if( auto Itr = std::find( m_Files.begin(), m_Files.end(), item ); Itr != m_Files.end() )
				{
					if( !std::filesystem::exists( rEntry ) )
						m_Files.erase( Itr, m_Files.end() );

					continue;
				}
			}

			if( !rEntry.is_directory() )
			{
				if( ExtensionToAssetType( rEntry.path().extension().string() ) == AssetType::Unknown )
					continue;
			}

			m_Files.push_back( item );

			m_FilesNeedSorting = true;
		}
	}

	void ContentBrowserPanel::GetSourceFiles( bool clear ) 
	{
		ClassMetadataHandler::Get().EachTreeNode( 
			[=]( const auto& rMetadata ) 
			{
				if( rMetadata.ExternalData )
				{
					Ref<ContentBrowserItem> item = Ref<ContentBrowserItem>::Create( std::filesystem::directory_entry( rMetadata.HeaderPath ), ContentBrowserItemType::SourceItem );
					item->SetSelectedFn( SAT_BIND_EVENT_FN( ContentBrowserPanel::OnItemSelected ) );

					m_Files.push_back( item );
					m_FilesNeedSorting = true;
				}
			} );
	}

	void ContentBrowserPanel::UpdateFiles( bool clear /*= false */ )
	{
		if( clear )
			m_Files.clear();

		switch( m_ViewMode )
		{
			case CBViewMode::Assets:
				GetContentFiles( clear );
				break;
			case CBViewMode::Scripts:
				GetSourceFiles( clear );
				break;
		}

		SortFiles();
	}

	void ContentBrowserPanel::OnItemSelected( ContentBrowserItem* pItem, bool clicked )
	{
		if( pItem->IsDirectory() && clicked && !pItem->MultiSelected() ) 
		{
			m_CurrentPath /= pItem->Path();

			m_ChangeDirectory = true;

			ClearSelection();

			m_Searching = false;
		}
		else
		{
			if( pItem->MultiSelected() )
			{
				m_MultiSelecting = true;

				m_SelectedItems.push_back( pItem );
			}
			else
			{
				m_MultiSelecting = false;

				ClearSelection();

				m_SelectedItems.push_back( pItem );
			}
		}
	}

	void ContentBrowserPanel::DrawItems( std::vector<Ref<ContentBrowserItem>>& rList, ImVec2 size, float padding )
	{
		for( auto& item : rList )
		{
			item->Draw( { size.x, size.y }, padding );

			// This happens if we rename a file as we then have to create the file cache again.
			if( !item )
				break;

			if( !item->IsSelected() )
			{
				// Is the item in the selection list if so and we are no longer selected then we need to remove it.
				if( const auto Itr = std::find( m_SelectedItems.begin(), m_SelectedItems.end(), item ); Itr != m_SelectedItems.end() )
				{
					item->Deselect();

					m_SelectedItems.erase( Itr );
				}
			}
		}
	}

}