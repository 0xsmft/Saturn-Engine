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

#pragma once

#include "ContentBrowserBase.h"

#include "Saturn/Vulkan/Texture.h"

#include "Saturn/Asset/Asset.h"

#include "ContentBrowserItem.h"

#include "Saturn/GameFramework/SClass.h"

#include <imgui.h>
#include <filesystem>
#include <functional>
#include <map>

#include <filewatch/filewatch.h>

namespace Saturn {

	struct SClassExtendedMetadata;

	struct ContentBrowserQuickAction
	{
		std::filesystem::path OldPath;
		std::filesystem::path NewPath;
	};

	class Event;
	class AssetImportPopupBase;

	class ContentBrowserPanel : public ContentBrowserBase
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

		virtual void ResetPath( const std::filesystem::path& rPath ) override;

		void BrowseToItem( const std::filesystem::path& rPath, AssetID id );

	private:
		virtual void UpdateFiles( bool clear = false ) override;
		virtual void OnItemSelected( ContentBrowserItem* pItem, bool clicked ) override;
		virtual void DrawItemsClipped( std::vector<Ref<ContentBrowserItem>>& rList, ImVec2 size, float padding, int columnCount ) override;
		void DrawItemsUnclipped( std::vector<Ref<ContentBrowserItem>>& rList, ImVec2 size, float padding );

		void BuildSearchList();

		void DrawFolderTree( const std::filesystem::path& rPath );

		void DrawAssetsFolderTree();
		void DrawScriptsFolderTree();

		void DrawRootFolder( CBViewMode type, bool open = false );

		void DrawBaseContextMenu();
		void AssetsPopupContextMenu();
		void ScriptsPopupContextMenu();

		void OnFilewatchEvent( const std::wstring& rPath, const filewatch::Event Event );

		Ref<ContentBrowserItem> GetActiveHoveredItem();

		void DrawClassHierarchy( const std::string& rKeyName, const SClass* pClass );

		void ClearSearchQuery();

		void DrawNotReadyImportPopup();
		void DrawDeleteAssetPopup();

		void GetContentFiles( bool clear );
		void GetSourceFiles( bool clear );
		void UpdateFirstFolder();
		bool ItemIsNotInSelectionList( const Ref<ContentBrowserItem>& rItem );

		void UndoQuickAction();
		void RedoQuickAction();
		void ClearQuickActions();
		void AddQuickAction( const std::filesystem::path& rOldPath, const std::filesystem::path& rNewPath );

	private:
		std::filesystem::path m_ScriptPath;

		ImGuiTextFilter m_TextFilter;

		struct AssetInfo
		{
			AssetType Type;
			std::filesystem::path Path;
		};

		// This used to be SClassExtendedMetadata hence the name
		const SClass* m_SelectedMetadata = SObject::StaticClass();

		filewatch::FileWatch<std::wstring>* m_Watcher = nullptr;

	private:
		std::vector<ContentBrowserQuickAction> m_QuickActionUndo;
		std::vector<ContentBrowserQuickAction> m_QuickActionRedo;

	private:		
		// Popup data
		std::string m_ClassInstanceName;
		std::string m_NewClassName;

		Ref<ContentBrowserItem> m_ItemToDelete = nullptr;

		std::unique_ptr<AssetImportPopupBase> m_CurrentImportPopup;

		bool m_OpenScriptsPopup = false;
		bool m_OpenClassInstancePopup = false;
		bool m_ShowDeleteAssetPopup = false;
		bool m_RenderUnclipped = false;
		bool m_RenderCreateWindow = false;
		bool m_ShowFolderPopupMenu = false;
		bool m_WindowFocused = false;
	};
}
