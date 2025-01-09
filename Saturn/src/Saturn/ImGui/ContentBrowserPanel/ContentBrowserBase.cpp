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
#include "ContentBrowserBase.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Saturn {

	ContentBrowserBase::ContentBrowserBase()
	{
		Init();
	}

	void ContentBrowserBase::Init()
	{		
		m_BackIcon      = Ref<Texture2D>::Create( "content/textures/editor/Left.png", AddressingMode::Repeat );
		m_ForwardIcon   = Ref<Texture2D>::Create( "content/textures/editor/Right.png", AddressingMode::Repeat );
	}

	ContentBrowserBase::~ContentBrowserBase()
	{
		m_BackIcon      = nullptr;
		m_ForwardIcon   = nullptr;
	}

	void ContentBrowserBase::DrawTopBar()
	{
		// Back button.
		if( m_CurrentPath != m_CurrentViewModeDirectory )
		{
			if( Auxiliary::ImageButton( m_BackIcon, { 24, 24 } ) )
			{
				m_CurrentPath = m_CurrentPath.parent_path();

				ClearSelection();
				UpdateFiles( true );
			}
		}
		else
		{
			ImGui::PushItemFlag( ImGuiItemFlags_Disabled, true );
			ImGui::PushStyleVar( ImGuiStyleVar_Alpha, 0.5f );
			Auxiliary::ImageButton( m_BackIcon, { 24, 24 } );
			ImGui::PopStyleVar( 1 );
			ImGui::PopItemFlag();
		}

		ImGui::SameLine();

		// Forward button.
		if( !m_FirstFolder.empty() || std::filesystem::exists( m_FirstFolder ) )
		{
			if( Auxiliary::ImageButton( m_ForwardIcon, { 24, 24 } ) )
			{
				m_CurrentPath /= std::filesystem::relative( m_FirstFolder, m_RootPath );

				ClearSelection();
				UpdateFiles( true );
			}
		}
		else
		{
			ImGui::PushItemFlag( ImGuiItemFlags_Disabled, true );
			ImGui::PushStyleVar( ImGuiStyleVar_Alpha, 0.5f );
			Auxiliary::ImageButton( m_ForwardIcon, { 24, 24 } );
			ImGui::PopStyleVar( 1 );
			ImGui::PopItemFlag();
		}

		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.0f, 0.0f, 0.0f, 0.0f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.3f, 0.3f, 0.3f, 0.35f ) );

		ImGui::SameLine();

		int i = 0;
		for( auto& pFolder : m_CurrentPath )
		{
			const char* name = m_ViewMode == CBViewMode::Assets ? "Assets" : "Source";

			if( i == 0 && pFolder != name )
				continue;

			i++;

			if( pFolder != name )
			{
				ImGui::Text( "/" );
			}

			ImGui::SameLine();

			std::string filename = pFolder.string();

			float size = strlen( filename.c_str() ) + ImGui::CalcTextSize( filename.c_str() ).x;

			ImGui::Selectable( filename.c_str(), false, 0, ImVec2( size, 22.0f ) );

			ImGui::SameLine();
		}

		ImGui::PopStyleColor( 2 );
	}

	void ContentBrowserBase::SortFiles()
	{
		if( m_FilesNeedSorting )
		{
			auto Fn = []( Ref<ContentBrowserItem>& a, Ref<ContentBrowserItem>& b ) -> bool
				{
					if( a->IsDirectory() && !b->IsDirectory() )
						return true; // sort first if a is directory.
					else if( !a->IsDirectory() && b->IsDirectory() )
						return false;
					else
						return a->Filename() < b->Filename();
				};

			std::sort( m_Files.begin(), m_Files.end(), Fn );
			m_FilesNeedSorting = false;
		}
	}

	Ref<ContentBrowserItem> ContentBrowserBase::FindItem( const std::filesystem::path& rPath )
	{
		const auto Itr = std::find_if( m_Files.begin(), m_Files.end(), [rPath]( auto& rItem ) { return rItem->Path() == rPath; } );

		if( Itr != m_Files.end() )
			return *Itr;

		return nullptr;
	}

	void ContentBrowserBase::FindAndRenameItem( const std::filesystem::path& rName )
	{
		const auto Itr = std::find_if( m_Files.begin(), m_Files.end(), 
			[rName]( auto& rItem ) 
			{
				return rItem->Filename() == rName;
			} );

		if( Itr != m_Files.end() )
			return (*Itr)->Rename();
	}

	int ContentBrowserBase::GetFilenameCount( const std::string& rName, bool directoriesOnly )
	{
		int count = 0;

		for( const auto& rEntry : std::filesystem::directory_iterator( m_CurrentPath ) )
		{
			if( directoriesOnly && !rEntry.is_directory() )
				continue;

			std::string filename = rEntry.path().stem().string();

			if( filename.find( rName ) != std::string::npos )
				count++;
		}

		return count;
	}

	void ContentBrowserBase::ClearSelection()
	{
		for( auto&& rrItem : m_SelectedItems )
		{
			rrItem->Deselect();
		}

		m_SelectedItems.clear();
	}
}