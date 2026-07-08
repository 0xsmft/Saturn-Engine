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

#include "ContentBrowserItem.h"

#include "Saturn/Vulkan/Texture.h"
#include "Saturn/Asset/Asset.h"
#include "Saturn/GameFramework/SClass.h"

#include "Saturn/ImGui/ImGuiWindow.h"

#include <imgui.h>
#include <filesystem>
#include <functional>
#include <map>

#include <filewatch/filewatch.h>

namespace Saturn {

	enum class CBViewMode : uint8_t
	{
		Assets,
		Scripts
	};

	struct SClassExtendedMetadata;

	struct ContentBrowserQuickAction
	{
		std::filesystem::path OldPath;
		std::filesystem::path NewPath;
	};

	class Event;
	class AssetImportPopupBase;

	class ContentBrowserPanel : public Saturn::ImGuiWindow
	{
	public:
		ContentBrowserPanel();
		ContentBrowserPanel( const std::string& rName );

		virtual ~ContentBrowserPanel();

		virtual void OnImGuiRender() override;
		virtual void OnEvent( Event& rEvent ) override;
		virtual void OnUpdate( Timestep ts ) {}

		static inline const char* GetStaticName()
		{
			return "Content Browser Panel";
		}

		void ResetPath( const std::filesystem::path& rPath );
		void BrowseToItem( const std::filesystem::path& rPath, AssetID id );

	public:
		const std::vector<Ref<ContentBrowserItem>>& GetSelectedItems() const { return m_SelectedItems; }

	private:
		void DrawTopBar();
		void SortFiles();

		void UpdateFiles( bool clear = false );
		void ChangeDirectoryAndAddQuickAction( const std::filesystem::path& rPath );
		void OnItemSelected( ContentBrowserItem* pItem, bool clicked );
		void DrawItemsClipped( std::vector<Ref<ContentBrowserItem>>& rList, ImVec2 size, float padding, int columnCount );

		Ref<ContentBrowserItem> FindItem( const std::filesystem::path& rPath );
		void FindAndRenameItem( const std::filesystem::path& rName );
		uint32_t GetFilenameCount( const std::string& rName, bool directoriesOnly = false );

		void AddSelected( Ref<ContentBrowserItem> item );
		void ClearSelection();

		void Init();

	private:
		void OnKeyPressed( RubyKeyEvent& rEvent );
		void DrawItemsUnclipped( std::vector<Ref<ContentBrowserItem>>& rList, ImVec2 size, float padding );

		void BuildSearchList();

		void DrawFolderTree( const std::filesystem::path& rPath );

		void DrawAssetsFolderTree();
		void DrawScriptsFolderTree();

		void DrawRootFolder( CBViewMode type, bool open = false );

		void DrawAssetOpenRenamePopup();

		void DrawBaseContextMenu();
		void AssetsPopupContextMenu();
		void ScriptsPopupContextMenu();

		void OnFilewatchEvent( const std::wstring& rPath, const filewatch::Event Event );

		Ref<ContentBrowserItem> GetActiveHoveredItem();

		void DrawClassHierarchy( const std::string& rKeyName, const SClass* pClass );

		void ClearSearchQuery();

		void DrawNotReadyImportPopup();
		void DrawErrorImportPopup();
		void DrawDeleteAssetPopup();

		void GetContentFiles( bool clear );
		void GetSourceFiles( bool clear );
		void UpdateFirstFolder();
		bool ItemIsNotInSelectionList( const Ref<ContentBrowserItem>& rItem );

		void UndoQuickAction();
		void RedoQuickAction();
		void ClearQuickActions();
		void AddQuickAction( const std::filesystem::path& rOldPath, const std::filesystem::path& rNewPath );

		void DuplicateAsset( Ref<Asset> asset );

		void MoveItemToItem( ContentBrowserItem* pSrc, ContentBrowserItem* pDst );
		void MoveItemToFolder( ContentBrowserItem* pSrc, const std::filesystem::path& rDst );

		void DrawCreateNewClassPopupModal();
		bool CheckIllegalClassName();

		void ResolveAssetImporterBasedOnExt( const std::filesystem::path& rPath );

	private:
		// The absolute current path
		// Used for finding/creating assets.
		std::filesystem::path m_CurrentPath;

		// The first folder in the current directory.
		std::filesystem::path m_FirstFolder;

		// The absolute path to the current folder we are looking at.
		std::filesystem::path m_CurrentViewModeDirectory;

		// The absolute path root path for the current view mode.
		std::filesystem::path m_RootPath;

		Ref< Texture2D > m_BackIcon;
		Ref< Texture2D > m_ForwardIcon;

		// Files and folder, sorted by folders then files.
		std::vector<Ref<ContentBrowserItem>> m_Files;
		std::vector<Ref<ContentBrowserItem>> m_ValidSearchFiles;
		std::vector<Ref<ContentBrowserItem>> m_SelectedItems;

		CBViewMode m_ViewMode = CBViewMode::Assets;

		bool m_FilesNeedSorting = false;
		bool m_ChangeDirectory = false;
		bool m_Searching = false;
		bool m_MultiSelecting = false;

	private:
		std::filesystem::path m_ScriptPath;

		ImGuiTextFilter m_TextFilter;

		struct AssetInfo
		{
			AssetType Type;
			std::filesystem::path Path;
		};

		// This used to be SClassExtendedMetadata hence the name
		const SClass* m_pSelectedMetadata = SObject::StaticClass();

		std::unique_ptr<filewatch::FileWatch<std::wstring>> m_Watcher;

	private:
		std::vector<ContentBrowserQuickAction> m_QuickActionUndo;
		std::vector<ContentBrowserQuickAction> m_QuickActionRedo;

	private:		
		// Popup data
		std::string m_ClassInstanceName;
		std::string m_NewClassName;

		std::vector<Ref<ContentBrowserItem>> m_ItemsToDelete;

		std::shared_ptr<AssetImportPopupBase> m_CurrentImportPopup;

		// Pending asset paths to import.
		std::vector<std::filesystem::path> m_PendingAssetPathsToImport;

		// Should be handled by the editor.
		bool m_OpenRenameAssetOpenPopup = false;
		bool m_OpenScriptsPopup = false;
		bool m_OpenClassInstancePopup = false;
		bool m_ShowDeleteAssetPopup = false;
		bool m_RenderUnclipped = false;
		bool m_RenderCreateWindow = false;
		bool m_ShowFolderPopupMenu = false;
		bool m_WindowFocused = false;
		
		// NewClass popup items... which should be in their own class...
		bool m_OpenIDEAfterNewClass = false;
		bool m_HotReloadAfterNewClass = false;
		bool m_IsSimpleClassLayout = true;
		bool m_IllegalClassName = false;
	};

}
