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
#include "ContentBrowserItem.h"

// TODO: Remove this include
// TOOD: #FixSceneRendererIncludes
#include "Saturn/Vulkan/Renderer2D.h"
#include "Saturn/Vulkan/AluraRenderer.h"

#include "Saturn/Asset/AssetImporter.h"
#include "Saturn/Asset/Asset.h"

#include "Saturn/Serialisation/YAML/AssetSerialisers.h"

#include "Saturn/ImGui/EditorEvents.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/ImGuiWindowManager.h"
#include "Saturn/ImGui/PrefabViewer.h"
#include "Saturn/ImGui/StaticMeshAssetViewer.h"
#include "Saturn/ImGui/MaterialAssetViewer/MaterialAssetViewer.h"
#include "Saturn/ImGui/TextureViewer.h"
#include "Saturn/ImGui/SoundAssetViewer.h"
#include "Saturn/Audio/SoundGraph/GraphSoundAssetViewer.h"
#include "Saturn/AI/BehaviourTree/AssetViewer/BehaviourTreeAssetViewer.h"
#include "Saturn/AI/BehaviourTree/BehaviourTreeMemoryAssetViewer.h"
#include "Saturn/Animation/AssetViewer/SkeletonAssetViewer.h"
#include "Saturn/Animation/AssetViewer/SkeletalMeshAssetViewer.h"
#include "Saturn/Animation/AssetViewer/SkeletalAnimationAssetViewer.h"
#include "Saturn/Animation/AssetViewer/AnimationControllerAssetViewer.h"
#include "Saturn/Alura/AssetViewer/AluraFontAssetViewer.h"
#include "Saturn/Alura/AssetViewer/AluraStylingProfileAssetViewer.h"
#include "Saturn/Physics/AssetViewer/PhysicsMaterialAssetViewer.h"

#include "Saturn/Project/Project.h"

#include "ContentBrowserThumbnailCache.h"

#include <imgui_internal.h>
#include <regex>

namespace Saturn {

	static char s_RenameBuffer[ 1024 ];

	ContentBrowserItem::ContentBrowserItem( const std::filesystem::directory_entry& rEntry, ContentBrowserItemType type )
		: m_Entry( rEntry ), m_Type( type ), m_Path( rEntry.path() ), m_IsDirectory( rEntry.is_directory() )  
	{
		m_Filename = m_Path.stem().string();

		if( m_IsDirectory )
		{
			m_Type = ContentBrowserItemType::Directory;
		}
		else if( m_Type == ContentBrowserItemType::Asset )
		{
			const auto path = std::filesystem::relative( m_Path, Project::GetActiveProject()->GetRootDir() );
			const auto asset = AssetManager::Get()->FindAsset( path );

			if( asset )
			{
				m_Asset = asset;
			}
		}

		// Do not generate the icon in the constructor wait until render.
		m_Icon = ContentBrowserThumbnailCache::Get().GetDefault( m_IsDirectory ? CB_DIRECTORY_ICON : CB_FILE_ICON );
	}

	ContentBrowserItem::~ContentBrowserItem()
	{
		m_Asset.Reset();
		m_Icon = nullptr;
	}

	void ContentBrowserItem::Draw( ImVec2 ThumbnailSize, float Padding )
	{
		ImDrawList* pDrawList = ImGui::GetWindowDrawList();
		const ImGuiStyle& style = ImGui::GetStyle();

		const float EdgeOffset = 4.0f;
		const float TextLineHeight = ImGui::GetTextLineHeightWithSpacing() * 2.0f + EdgeOffset * 2.0f;
		const float InfoPanelHeight = std::max( ThumbnailSize.x * 0.5f, TextLineHeight );
		const ImVec2 TopLeft = ImGui::GetCursorScreenPos();
		const ImVec2 ThumbnailBottomRight = ImVec2( TopLeft.x + ThumbnailSize.x, TopLeft.y + ThumbnailSize.y );
		const ImVec2 InfoTopLeft = ImVec2( TopLeft.x, TopLeft.y + ThumbnailSize.y );
		const ImVec2 BottomRight = ImVec2( TopLeft.x + ThumbnailSize.x, TopLeft.y + ThumbnailSize.y + InfoPanelHeight );

		if( m_PendingScrollTo )
		{
			ImGui::SetScrollHereY();

			m_PendingScrollTo = false;
		}

		ImGui::PushID( m_Path.c_str() );
		ImGui::BeginGroup();

		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0.0f, 0.0f ) );

		// Draw the item.
		if( m_IsDirectory )
		{
			bool Clicked = false;
			bool DoubleClicked = false;

			if( !m_IsRenaming )
				ImGui::ButtonBehavior( ImRect( TopLeft, BottomRight ), ImGui::GetID( m_Path.c_str() ), &m_IsHovered, &Clicked );

			pDrawList->AddRectFilled( TopLeft, BottomRight, ImGui::GetColorU32( ImGuiCol_Button ), 5.0f, ImDrawFlags_RoundCornersAll );

			ImGui::ItemSize( ThumbnailSize, style.FramePadding.y );
			ImGui::ItemAdd( ImRect( TopLeft, BottomRight ), ImGui::GetID( m_Path.c_str() ) );

			if( ImGui::BeginDragDropTarget() )
			{
				// Item to item moving
				auto* pData = ImGui::AcceptDragDropPayload( "CB_ITEM_MOV_XX", ImGuiDragDropFlags_None );
				if( pData && pData->Data )
				{
					ContentBrowserItem* pSrc = *( ContentBrowserItem** )pData->Data;

					Application::Get()->DispatchEvent<CBMoveItemEvent>( pSrc, this );
				}

				ImGui::EndDragDropTarget();
			}

			if( m_IsHovered && !m_IsRenaming )
			{
				// Draw a highlight around the button.
				pDrawList->AddRect( TopLeft, BottomRight, ImGui::GetColorU32( ImGuiCol_ButtonHovered ), 5.0f, ImDrawFlags_RoundCornersAll );

				if( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
				{
					DoubleClicked = true;

					m_OnSelected( this, true );
				}

				if( ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
				{
					Select();

					m_OnSelected( this, false );
				}
			}

			if( ImGui::IsItemHovered( ImGuiHoveredFlags_Stationary | ImGuiHoveredFlags_DelayNormal ) )
			{
				if( ImGui::BeginTooltip() )
				{
					ImGui::Text( "Folder" );
					ImGui::Separator();

					ImGui::Text( "%s", m_Path.string().c_str() );

					ImGui::EndTooltip();
				}
			}

			// Selected but not opened!
			// Double clicked = open (change CB folder)
			// Single clicked = selected or deselected
			if( Clicked && !DoubleClicked )
			{
				if( Input::Get().KeyPressed( RubyKey_LeftShift ) || Input::Get().KeyPressed( RubyKey_RightShift ) )
				{
					m_MultiSelected ^= 1;
				}

				m_IsSelected ^= 1;

				// Selected but not opened!
				m_OnSelected( this, false );
			}
		}
		else
		{
			// Generate new thumbnail OR return existing one in cache.
			// Returns default icon while generating.
			m_Icon = ContentBrowserThumbnailCache::Get().GetFor( m_Asset );

			// Fill background.
			pDrawList->AddRectFilled( TopLeft, ThumbnailBottomRight, ImGui::GetColorU32( ImGuiCol_Button ), 5.0f, ImDrawFlags_RoundCornersTop );

			// Fill Info area
			pDrawList->AddRectFilled( InfoTopLeft, BottomRight, IM_COL32( 47, 47, 47, 255 ), 5.0f, ImDrawFlags_RoundCornersBottom );

			// Draw line between thumbnail and info.
			if( m_Asset )
				pDrawList->AddLine( ThumbnailBottomRight, InfoTopLeft, AssetTypeToColor( m_Asset->Type ), 1.5f );
			else
				pDrawList->AddLine( ThumbnailBottomRight, InfoTopLeft, IM_COL32( 255, 0, 0, 255 ), 1.5f );

			ImGui::ItemSize( ImRect( TopLeft, BottomRight ).Min, style.FramePadding.y );
			ImGui::ItemAdd( ImRect( TopLeft, BottomRight ), ImGui::GetID( m_Path.c_str() ) );

			bool ItemClicked = false;
			bool Open = false;
			
			if( !m_IsRenaming )
				ItemClicked = Auxiliary::ButtonRd( "##CONTENT_BROWSER_ITEM_BTN", ImRect( TopLeft, BottomRight ), true );

			m_IsHovered = ImGui::IsItemHovered();

			if( m_IsHovered && !m_IsRenaming )
			{
				if( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
				{
					Open = true;
				}
				else if( ImGui::IsMouseClicked( ImGuiMouseButton_Right ) )
				{
					Select();

					m_OnSelected( this, false );
				}
			}

			if( ImGui::IsItemHovered( ImGuiHoveredFlags_Stationary | ImGuiHoveredFlags_DelayNormal ) )
			{
				if( ImGui::BeginTooltip() )
				{
					ImGui::Text( "Asset" );
					ImGui::Separator();

					ImGui::Text( "%s", m_Path.string().c_str() );

					if( m_Asset )
					{
						ImGui::Text( "Asset: %llu", m_Asset->ID );
						ImGui::Text( "Asset Name: %s", m_Asset->Name.c_str() );
						ImGui::Text( "Asset Version: %llu", m_Asset->Version );
					}

					ImGui::EndTooltip();
				}
			}

			if( ItemClicked )
			{
				if( Input::Get().KeyPressed( RubyKey_LeftShift ) || Input::Get().KeyPressed( RubyKey_RightShift ) )
				{
					m_MultiSelected ^= 1;
				}

				m_IsSelected ^= 1;
				m_OnSelected( this, m_IsSelected );
			}

			HandleDragDrop();

			if( Open )
			{
				switch( m_Type )
				{
					case ContentBrowserItemType::Asset:
					{
						HandleOpenAsset();
					} break;

					case ContentBrowserItemType::SourceItem:
					{
						HandleOpenSourceItem();
					} break;

					case ContentBrowserItemType::Directory:
					default:
						break;
				}
			}
		}

		ImGui::EndGroup();

		DrawIcon( ThumbnailSize, TopLeft, ThumbnailBottomRight );

		if( m_IsSelected )
		{
			// Draw a thicker highlight around the button when selected.
			pDrawList->AddRect( TopLeft, BottomRight, ImGui::GetColorU32( ImGuiCol_ButtonHovered ), 5.0f, ImDrawFlags_RoundCornersAll, 2.5f );
		}

		ImGui::SetCursorScreenPos( ImVec2( TopLeft.x + 2.0f, TopLeft.y + ThumbnailSize.y ) );

		// Filename
		const ImVec2 cursor = ImGui::GetCursorPos();
		ImGui::SetCursorPos( ImVec2( cursor.x + EdgeOffset + 5.0f, cursor.y + EdgeOffset + 5.0f ) );

		const std::string Filename = m_Filename.string();

		if( m_IsDirectory )
		{
			ImGui::BeginVertical( "FILENAME_PANEL", ImVec2( ThumbnailSize.x - EdgeOffset * 2.0f, InfoPanelHeight - EdgeOffset ) );

			ImGui::BeginHorizontal( m_Filename.c_str(), ImVec2( ThumbnailSize.x - 2.0f, 0.0f ) );

			ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + ( ThumbnailSize.x - EdgeOffset * 3.0f ) );

			const float textWidth = std::min( ImGui::CalcTextSize( Filename.c_str() ).x, ThumbnailSize.x );

			ImGui::SetNextItemWidth( textWidth );

			ImGui::SetCursorPosX( ImGui::GetCursorPosX() + ( ThumbnailSize.x - ImGui::CalcTextSize( Filename.c_str() ).x ) * 0.5f - EdgeOffset - 5.0f );

			if( m_IsRenaming )
			{
				if( m_StartingRename )
				{
					std::memset( s_RenameBuffer, 0, 1024 );
					std::memcpy( s_RenameBuffer, Filename.c_str(), Filename.size() );

					ImGui::SetKeyboardFocusHere( 0 );

					m_StartingRename = false;
				}

				if( ImGui::InputText( "##renamefolder", s_RenameBuffer, 1024, ImGuiInputTextFlags_EnterReturnsTrue ) )
				{
					m_IsRenaming = false;
					OnRenameCommittedFolder( s_RenameBuffer );

					std::memset( s_RenameBuffer, 0, 1024 );
				}
			}
			else
			{
				ImGui::Text( Filename.c_str() );
			}

			ImGui::PopTextWrapPos();

			ImGui::Spring();
			ImGui::EndHorizontal();
			ImGui::Spring();
			ImGui::EndVertical();
		}
		else
		{
			ImGui::BeginVertical( "FILENAME_PANEL", ImVec2( ThumbnailSize.x - EdgeOffset * 3.0f, InfoPanelHeight - EdgeOffset ) );

			ImGui::BeginHorizontal( "FILENAME_PANEL_HOR", ImVec2( 0.0f, 0.0f ) );

			ImGui::SuspendLayout();

			ImGui::PushTextWrapPos( ImGui::GetCursorPosX() + ( ThumbnailSize.x - EdgeOffset - 5.0f * 3.0f ) );

			if( m_IsRenaming )
			{
				if( m_StartingRename )
				{
					std::memset( s_RenameBuffer, 0, 1024 );
					std::memcpy( s_RenameBuffer, m_Filename.string().c_str(), m_Filename.string().size() );

					ImGui::SetKeyboardFocusHere( 0 );

					m_StartingRename = false;
				}

				if( ImGui::InputText( "##renamefile", s_RenameBuffer, 1024, ImGuiInputTextFlags_EnterReturnsTrue ) )
				{
					m_IsRenaming = false;
					OnRenameCommitted( s_RenameBuffer );

					std::memset( s_RenameBuffer, 0, 1024 );
				}

				/*
				// TODO: Check for invalid characters and follow OS rules
				// Windows does not allow \ / : ? * <> | "
				// Linux does not allow /
				// Windows does not allow files to end in a space or a dot
				// Windows does not allow files to be called CON, AUX, PRN, NUL, COM1-9, LPT1-9
				// Linux does not allow files to be called .., .
				char c = s_RenameBuffer[ 0 ];
				while(  c != '\0' )
				{
					if( c == '\\' || c == '/' || c == ':' || c == '?' || c == '*'  || c == '<' || c == '>' || c == '|' || c == '\"' )
						break;
					++c;
				}
				*/
			}
			else
			{
				ImGui::Text( Filename.c_str() );
			}

			ImGui::PopTextWrapPos();
			ImGui::ResumeLayout();

			ImGui::Spring();

			ImGui::EndHorizontal();

			ImGui::Spring();
			ImGui::EndVertical();
		}

		ImGui::PopStyleVar();

		ImGui::NextColumn();
		ImGui::PopID();
	}

	void ContentBrowserItem::OnRenameCommitted( const std::string& rName )
	{
		// TODO: Check for invalid characters and follow OS rules
		// Windows does not allow \ / : ? * <> | "
		// Linux does not allow /
		// Windows does not allow files to end in a space or a dot
		// Windows does not allow files to be called CON, AUX, PRN, NUL, COM1-9, LPT1-9
		// Linux does not allow files to be called .., .

		std::regex invalidCharacterRegex( "[\\\\/:?*<>|\\\"]" );
		if( std::regex_search( rName, invalidCharacterRegex ) )
		{
			SAT_CORE_INFO( "Invalid chars" );
			return;
		}

		m_Filename = rName;

		const std::string extension = m_Path.extension().string();

		std::filesystem::path newPath = m_Path.parent_path();
		newPath /= rName;
		newPath.replace_extension( extension );

		// Rename the file on the filesystem
		std::filesystem::rename( m_Path, newPath );

		if( m_Asset )
		{
			AssetManager::Get()->RenameAsset( m_Asset->ID, rName );
		}

		// Update our Entry.
		m_Entry = std::filesystem::directory_entry( newPath );
		m_Path = m_Entry.path();
	}

	void ContentBrowserItem::OnRenameCommittedFolder( const std::string& rName )
	{
		m_Filename = rName;

		std::filesystem::path newPath = m_Path.parent_path();
		newPath /= rName; 

		std::filesystem::rename( m_Path, newPath );

		// After the call to the FS, tell the Asset Manager to update any assets in this folder.
		AssetManager::Get()->UpdateAssetPathsOnRename( m_Path, newPath );

		// And finally update our path.
		m_Entry = std::filesystem::directory_entry( newPath );
		m_Path = m_Entry.path();
	}

	void ContentBrowserItem::Select( bool forceMultiSelection /*=false*/ )
	{
		m_IsSelected = true;
	
		if( forceMultiSelection )
			m_MultiSelected = true;
	}

	void ContentBrowserItem::Deselect()
	{
		m_IsSelected = false;
		m_MultiSelected = false;
	}

	void ContentBrowserItem::Rename()
	{
		m_IsRenaming = true;
		m_StartingRename = true;
	}

	bool ContentBrowserItem::Delete()
	{
		if( m_IsDirectory )
		{
			AssetManager::Get()->Each( [&]( Ref<Asset> asset ) 
				{
					if( asset->Path.string().contains( m_Path.string() ) ) 
					{
						AssetManager::Get()->RemoveAsset( asset->ID );
					}
				} );
		}
		else
		{
			// If this asset is a dependee, we need to tell
			// the depedant that we no longer need it, because we won't exist anymore.
			RemoveAssetDependencies();
			CloseAssetViewersBeforeDeletion();
			AssetManager::Get()->RemoveAsset( m_Asset->ID );
		}

		// Delete the file.
		std::filesystem::remove( m_Path );

		return true;
	}

	void ContentBrowserItem::ScrollTo()
	{
		m_PendingScrollTo = true;
	}

	void ContentBrowserItem::HandleOpenAsset()
	{
		if( !m_Asset )
			return;

		switch( m_Asset->Type )
		{
			case AssetType::Texture:
			{
				const auto viewer = Ref<TextureViewer>::Create( m_Asset->ID );
				ImGuiWindowManager::Get()->AddWindow( viewer, viewer->GetWindowName() );
			} break;

			case AssetType::StaticMesh:
			{
				const auto viewer = Ref<StaticMeshAssetViewer>::Create( m_Asset->ID );
				ImGuiWindowManager::Get()->AddWindow( viewer, viewer->GetWindowName() );
			} break;

			case AssetType::SkeletalMesh:
			{
				const auto viewer = Ref<SkeletalMeshAssetViewer>::Create( m_Asset->ID );
				ImGuiWindowManager::Get()->AddWindow( viewer, viewer->GetWindowName() );
			} break;

			case AssetType::Material:
			{
				// Importing the asset will happen in this function.
				const auto viewer = Ref<MaterialAssetViewer>::Create( m_Asset->ID );
				ImGuiWindowManager::Get()->AddWindow( viewer, viewer->GetWindowName() );
			} break;
			case AssetType::MaterialInstance:
				break;

			case AssetType::Prefab:
			{
				const auto viewer = Ref<PrefabViewer>::Create( m_Asset->ID );
				ImGuiWindowManager::Get()->AddWindow( viewer, viewer->GetWindowName() );
			} break;

			case AssetType::PhysicsMaterial:
			{
				const auto viewer = Ref<PhysicsMaterialAssetViewer>::Create( m_Asset->ID );
				ImGuiWindowManager::Get()->AddWindow( viewer, viewer->GetWindowName() );
			} break;

			case AssetType::Sound:
			{
				const auto viewer = Ref<SoundAssetViewer>::Create( m_Asset->ID );
				ImGuiWindowManager::Get()->AddWindow( viewer, viewer->GetWindowName() );
			} break;

			case AssetType::GraphSound:
			{
				const auto viewer = Ref<GraphSoundAssetViewer>::Create( m_Asset->ID );
				ImGuiWindowManager::Get()->AddWindow( viewer, viewer->GetWindowName() );
			} break;

			case AssetType::BehaviourTree:
			{
				const auto viewer = Ref<BehaviourTreeAssetViewer>::Create( m_Asset->ID );
				ImGuiWindowManager::Get()->AddWindow( viewer, viewer->GetWindowName() );
			} break;

			case AssetType::BehaviourTreeMemory:
			{
				const auto viewer = Ref<BehaviourTreeMemoryAssetViewer>::Create( m_Asset->ID );
				ImGuiWindowManager::Get()->AddWindow( viewer, viewer->GetWindowName() );
			} break;

			case AssetType::Skeleton:
			{
				const auto viewer = Ref<SkeletonAssetViewer>::Create( m_Asset->ID );
				ImGuiWindowManager::Get()->AddWindow( viewer, viewer->GetWindowName() );
			} break;

			case AssetType::SkeletalAnimation:
			{
				const auto viewer = Ref<SkeletalAnimationAssetViewer>::Create( m_Asset->ID );
				ImGuiWindowManager::Get()->AddWindow( viewer, viewer->GetWindowName() );
			} break;

			case AssetType::AnimationController:
			{
				const auto viewer = Ref<AnimationControllerAssetViewer>::Create( m_Asset->ID );
				ImGuiWindowManager::Get()->AddWindow( viewer, viewer->GetWindowName() );
			} break;

			case AssetType::Scene:
			{
				// Scenes have to handled via an event because this class does not have the ability to switch scenes.
				Application::Get()->DispatchEvent<CBOpenFileEvent>( m_Asset->ID );
			} break;

			case AssetType::Font:
			{
				const auto viewer = Ref<AluraFontAssetViewer>::Create( m_Asset->ID );
				ImGuiWindowManager::Get()->AddWindow( viewer, viewer->GetWindowName() );
			} break;

			case AssetType::StyleProfile:
			{
				const auto viewer = Ref<AluraStylingProfileAssetViewer>::Create( m_Asset->ID );
				ImGuiWindowManager::Get()->AddWindow( viewer, viewer->GetWindowName() );
			} break;

			case AssetType::Unknown:
			case AssetType::COUNT:
			default:
				SAT_CORE_WARN( "Unhandled type for double click on Content Browser Item!" );
				break;
		}
	}

	void ContentBrowserItem::HandleOpenSourceItem()
	{
#if !defined(SAT_DIST)
		Application::Get()->DispatchEvent<RequestOpenIDEEvent>( m_Path );
#endif
	}

	void ContentBrowserItem::HandleDragDrop()
	{
		if( m_Type == ContentBrowserItemType::SourceItem )
			return;

		// Do not change!
		// The whole Thumbnail system will break!
		// TOOD: Fix
		const auto Icon = ContentBrowserThumbnailCache::Get().GetDefault( CB_FILE_ICON );

		if( ImGui::BeginDragDropSource( ImGuiDragDropFlags_SourceAllowNullID ) )
		{
			// Tooltip
			ImGui::BeginHorizontal( "##dndinfo" );

			Auxiliary::Image( Icon, ImVec2( 24.0f, 24.0f ) );

			const void* pData = &m_Asset->ID;
			if( Input::Get().KeyPressed( RubyKey_LeftCtrl ) || Input::Get().KeyPressed( RubyKey_RightCtrl ) )
			{
				Select();

				ContentBrowserItem* pThis = this;
				ImGui::SetDragDropPayload( "CB_ITEM_MOV_XX", &pThis, sizeof( uintptr_t ), ImGuiCond_Once );

				ImGui::Text( "Moving: %s", m_Filename.string().c_str() );
			}
			else
			{
				ImGui::Text( "Importing (Ctrl to move): %s", m_Filename.string().c_str() );
			}

			if( m_MultiSelected )
			{
				ImGui::Text( " + others" );
			}

			ImGui::EndHorizontal();

			if( m_MultiSelected )
			{
				// NOTE: ImGui does not allow us to pass in no data for a payload, so we'll just pass in
				//		 the asset ID as a dummy data,
				//		 it should not be used! Use content browser instead for correct multi-selection data.
				// NOTE: NDT stands for NoDaTa
				ImGui::SetDragDropPayload( "CONTENT_BROWSER_ITEM_MULTI_NDT", pData, sizeof( UUID ), ImGuiCond_Once );
			}
			else
			{
				switch( m_Asset->Type )
				{
					case Saturn::AssetType::Texture:
						break;
					case Saturn::AssetType::StaticMesh:
					{
						ImGui::SetDragDropPayload( "CONTENT_BROWSER_ITEM_MODEL", pData, sizeof( UUID ), ImGuiCond_Once );
					}	break;
					case Saturn::AssetType::SkeletalMesh:
					{
						ImGui::SetDragDropPayload( "CONTENT_BROWSER_ITEM_SKMODEL", pData, sizeof( UUID ), ImGuiCond_Once );
					}	break;
					case Saturn::AssetType::Material:
					{
						ImGui::SetDragDropPayload( "asset_payload", pData, sizeof( UUID ), ImGuiCond_Once );
					}	break;
					case Saturn::AssetType::MaterialInstance:
						break;
					case Saturn::AssetType::Sound:
					{
						ImGui::SetDragDropPayload( "CONTENT_BROWSER_ITEM_SND", pData, sizeof( UUID ), ImGuiCond_Once );
					} break;

					case Saturn::AssetType::Scene:
					{
						ImGui::SetDragDropPayload( "CONTENT_BROWSER_ITEM_SCENE", pData, sizeof( UUID ), ImGuiCond_Once );
					} break;

					case Saturn::AssetType::Prefab:
					{
						ImGui::SetDragDropPayload( "CONTENT_BROWSER_ITEM_PREFAB", pData, sizeof( UUID ), ImGuiCond_Once );
					} break;

					case Saturn::AssetType::Unknown:
					case Saturn::AssetType::COUNT:
					default:
						break;
				}
			}

			ImGui::EndDragDropSource();
		}
	}

	void ContentBrowserItem::DrawIcon( const ImVec2& rThumbnailSize, const ImVec2& rTopLeft, const ImVec2& rBottomRight )
	{
		const ImVec2 imageSize = ImVec2( ( float ) m_Icon->Width(), ( float ) m_Icon->Height() );
		ImVec2 scaledSize = imageSize;

		// TODO: We use 512x512 images not 180x180
		const float maxSize = rThumbnailSize.x;
		if( imageSize.x > maxSize || imageSize.y > maxSize )
		{
			const float aspectRatio = imageSize.x / imageSize.y;

			if( aspectRatio > 1.0f )
			{
				// Wider than tall (rect)
				scaledSize.x = maxSize;
				scaledSize.y = maxSize / aspectRatio;

				// Align to the bottom of the item
				const ImVec2 modifiedTopLeft = ImVec2( rTopLeft.x, rTopLeft.y + ( rThumbnailSize.y - scaledSize.y ) );
				DrawIconInternal( modifiedTopLeft, rBottomRight, 256 /* <- ImDrawFlags_RoundCornersNone */ );
			}
			else
			{
				// Every thumbnail image that does not come from a texture (source asset) will always have an aspect ratio of one (as 512 / 512 = 1)
				// Now, if an image is taller than it is wider it would also come through here.
				
				// Draw normal
				DrawIconInternal( rTopLeft, rBottomRight );
			}
		}
		else
		{
			DrawIconInternal( rTopLeft, rBottomRight );
		}
	}

	void ContentBrowserItem::DrawIconInternal( const ImVec2& rTopLeft, const ImVec2& rBottomRight, ImDrawFlags drawFlags )
	{
		ImDrawList* pDrawList = ImGui::GetWindowDrawList();
		pDrawList->AddImageRounded(
			m_Icon->GetDescriptorSet(),
			rTopLeft,
			rBottomRight,
			{ 0, 1 }, { 1, 0 }, IM_COL32_WHITE, 5.0f, drawFlags );
	}

	void ContentBrowserItem::RemoveAssetDependencies()
	{
		if( m_Asset )
		{
			AssetManager::Get()->UnregisterAllAssetDependencies( m_Asset->ID );
		}
	}

	void ContentBrowserItem::CloseAssetViewersBeforeDeletion()
	{
		const std::string windowName = std::format( "{0}##{1}", m_Asset->Name, ( uint64_t ) m_Asset->ID );
		Ref<ImGuiWindow> window = ImGuiWindowManager::Get()->GetWindow<ImGuiWindow>( windowName );
		if( window )
		{
			window->CloseWindow();
		}
	}

}
