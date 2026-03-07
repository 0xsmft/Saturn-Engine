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

#pragma once

#include "Saturn/Core/Base.h"

#include "Saturn/Asset/Asset.h"

#include "Saturn/Vulkan/Texture.h"

#include <imgui.h>
#include <filesystem>

namespace Saturn {

	class Asset;

	enum class ContentBrowserItemType
	{
		Asset,
		Directory,
		SourceItem
	};

	class ContentBrowserItem : public RefTarget
	{
	public:
		ContentBrowserItem( const std::filesystem::directory_entry& rEntry, ContentBrowserItemType type );
		~ContentBrowserItem();

		void Draw( ImVec2 ThumbnailSize, float Padding );

		bool operator==( const ContentBrowserItem& rOther ) 
		{
			return m_Entry == rOther.m_Entry && m_Filename == rOther.m_Filename && m_Path == rOther.m_Path;
		}

		bool IsDirectory()    const   { return m_IsDirectory;   }
		bool IsHovered()      const   { return m_IsHovered;     }
		bool IsSelected()     const   { return m_IsSelected;    }
		bool MultiSelected()  const   { return m_MultiSelected; }
		bool IsRenaming()     const   { return m_IsRenaming;    }
		AssetID GetAssetID()  const   { if( m_Asset ) return m_Asset->ID; else return 0; }
		Ref<Asset> GetAsset() const   { return m_Asset;         }
		void CanEverDrag( bool val )  { m_CanEverDrag = val;    }

		std::filesystem::path& Filename() { return m_Filename; }
		const std::filesystem::path& Filename() const { return m_Filename; }
		
		std::filesystem::path& Path()             { return m_Path; }
		const std::filesystem::path& Path() const { return m_Path; }

		void SetSelectedFn( const std::function<void( ContentBrowserItem*, bool )>&& rrFunc ) { m_OnSelected = rrFunc; }
		
		void OnRenameCommitted( const std::string& rName );
		void OnRenameCommittedFolder( const std::string& rName );

	public:
		void Select( bool forceMultiSelection = false );
		void Deselect();
		void Rename();
		bool Delete();
		void ScrollTo();

	private:
		void HandleOpenAsset();
		void HandleOpenSourceItem();
		void HandleDragDrop();

		void DrawIcon( const ImVec2& rThumbnailSize, const ImVec2& rTopLeft, const ImVec2& rBottomRight );
		void DrawIconInternal( const ImVec2& rTopLeft, const ImVec2& rBottomRight, ImDrawFlags drawFlags = ImDrawFlags_RoundCornersTop );

		void RemoveAssetDependants();
		void CloseAssetViewersBeforeDeletion();

	private:
		std::filesystem::directory_entry m_Entry;
		
		// The filename without the extension.
		std::filesystem::path m_Filename;

		// The absolute filepath of the asset/folder/source item.
		std::filesystem::path m_Path;

		Ref<Texture2D> m_Icon;

		std::function<void( ContentBrowserItem*, bool )> m_OnSelected;

		bool m_IsDirectory = false;
		bool m_IsHovered = false;
		bool m_IsSelected = false;
		bool m_MultiSelected = false;
		bool m_CanEverDrag = true;

		bool m_IsRenaming = false;
		bool m_StartingRename = false;

		bool m_PendingScrollTo = false;

		Ref<Asset> m_Asset = nullptr;
		ContentBrowserItemType m_Type = ContentBrowserItemType::Asset;
	};
}
