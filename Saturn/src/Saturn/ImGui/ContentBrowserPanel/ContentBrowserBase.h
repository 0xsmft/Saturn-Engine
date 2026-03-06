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

#include "Saturn/ImGui/ImGuiWindow.h"
#include "ContentBrowserItem.h"
#include "Saturn/Vulkan/Texture.h"

#include <imgui.h>
#include <filesystem>
#include <functional>
#include <map>

namespace Saturn {

	enum class CBViewMode
	{
		Assets,
		Scripts
	};

	class ContentBrowserBase : public ImGuiWindow
	{
	public:
		ContentBrowserBase();
		virtual ~ContentBrowserBase();

		virtual void ResetPath( const std::filesystem::path& rPath ) = 0;

	protected:
		void DrawTopBar();
		void SortFiles();
		
		virtual void UpdateFiles( bool clear = false ) = 0;

		virtual void OnItemSelected( ContentBrowserItem* pItem, bool clicked ) = 0;
		virtual void DrawItemsClipped( std::vector<Ref<ContentBrowserItem>>& rList, ImVec2 size, float padding, int columnCount ) = 0;

		Ref<ContentBrowserItem> FindItem( const std::filesystem::path& rPath );
		void FindAndRenameItem( const std::filesystem::path& rName );
		uint32_t GetFilenameCount( const std::string& rName, bool directoriesOnly = false );

		void ClearSelection();

	private:
		void Init();

	protected:
		// The relative current path (always relative to m_CurrentViewModeDirectory)
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

		CBViewMode m_ViewMode;

		bool m_FilesNeedSorting = false;
		bool m_ChangeDirectory = false;
		bool m_Searching = false;
		bool m_MultiSelecting = false;
	};
}