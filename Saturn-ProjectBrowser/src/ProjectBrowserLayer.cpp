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
#include "ProjectBrowserLayer.h"

#include <Saturn/ImGui/ImGuiAuxiliary.h>
#include <Saturn/ImGui/EditorAboutWindowContents.h>

#include <Saturn/Core/Ruby/RubyWindow.h>
#include <Saturn/Core/EnvironmentVariables.h>
#include <Saturn/Core/Process.h>
#include <Saturn/Core/Profiler.h>

#include <Saturn/Serialisation/YAML/ProjectSerialiser.h>
#include <Saturn/Serialisation/YAML/EngineSettingsSerialiser.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h> 
#include <imgui_internal.h>

namespace Saturn {

	static inline ImVec2  operator*( const ImVec2& lhs, const float rhs ) { return ImVec2( lhs.x * rhs, lhs.y * rhs ); }
	static inline ImVec2  operator/( const ImVec2& lhs, const float rhs ) { return ImVec2( lhs.x / rhs, lhs.y / rhs ); }
	static inline ImVec2  operator+( const ImVec2& lhs, const ImVec2& rhs ) { return ImVec2( lhs.x + rhs.x, lhs.y + rhs.y ); }
	static inline ImVec2  operator-( const ImVec2& lhs, const ImVec2& rhs ) { return ImVec2( lhs.x - rhs.x, lhs.y - rhs.y ); }

	static void ReplaceToken( std::string& str, const char* token, const std::string& value )
	{
		size_t pos = 0;
		while( ( pos = str.find( token, pos ) ) != std::string::npos )
		{
			str.replace( pos, strlen( token ), value );
			pos += strlen( token );
		}
	}

	ProjectBrowserLayer::ProjectBrowserLayer()
	{
#if defined(SAT_PROFILER_ENABLE)
		tracy::StartupProfiler();
#endif

		m_HasSaturnDir = Auxiliary::HasEnvironmentVariable( "SATURN_DIR" );

		if( m_HasSaturnDir )
			m_SaturnDir = Auxiliary::GetEnvironmentVariable( "SATURN_DIR" );

		std::memset( m_SaturnDirBuffer, 0, 1024 );
		std::memset( m_ProjectNameBuffer, 0, 1024 );

		EngineSettingsSerialiser::Deserialise();

		m_RecentProjectThread = std::thread( [this]() 
		{
#if defined(SAT_PLATFORM_WINDOWS)
			::SetThreadDescription( ::GetCurrentThread(), L"RecentProjectThread" );
#endif
			auto& rUserSettings = EngineSettings::Get();

			bool projectsNeedSorting = false;

			while( !m_ShouldThreadTerminate )
			{
				for( auto& path : rUserSettings.GetAllRecentProjects() )
				{
					auto Itr = std::find_if( m_RecentProjects.begin(), m_RecentProjects.end(), 
						[path](const auto& rInfo)
						{
							return rInfo.Filepath == path;
						} );

					if( Itr == m_RecentProjects.end() )
					{
						// Deserialise the project.
						ProjectSerialiser ps;
						ps.Deserialise( path );

						Ref<Project> project = Project::GetActiveProject();
						
						if( project )
						{
							ProjectInformation info{};
							info.Filepath = path;
							info.Name = project->GetConfig().Name;
							info.AssetPath = project->GetFullAssetPath();
							info.LastWriteTime = std::format( "{0}", std::filesystem::last_write_time( path ) );
							info.LastWriteTime = info.LastWriteTime.substr( 0, info.LastWriteTime.find_first_of( " " ) );
					
							info.ThumbnailTexture = m_NoIconTexture;

							if( project->HasThumbnail() )
							{
								info.ThumbnailPath = project->GetThumbnailPath();
								info.ThumbnailTexture = Ref<Texture2D>::Create( info.ThumbnailPath );
							}

							m_RecentProjects.push_back( info );

							// Reset.
							Project::SetActiveProject( nullptr );
						
							projectsNeedSorting = true;
						}
					}

					if( projectsNeedSorting )
					{
						std::sort( m_RecentProjects.begin(), m_RecentProjects.end(), 
							[]( const auto& rA, const auto& rB )
							{
								if( rA.LastWriteTime > rB.LastWriteTime )
									return true;
								else
									return false;
							} );

						projectsNeedSorting = false;
					}
				}

				// Sleep to avoid exhausting this thread.
				std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
			}
		} );

		m_TitleBar.AddMenuBarFunction( [this]() 
		{
			if( ImGui::BeginMenu( "File" ) )
			{
				if( ImGui::MenuItem( "Close", "Alt+F4" ) ) Application::Get()->Close();

				if( ImGui::MenuItem( "About" ) ) m_OpenAboutWindow ^= 1;

				ImGui::EndMenu();
			}

			if( ImGui::BeginMenu( "Projects" ) )
			{
				if( ImGui::MenuItem( "Clear All" ) ) EngineSettings::Get().ClearAllRecentProjects();

				ImGui::EndMenu();
			}
		} );

		Application::Get()->GetWindow()->ChangeTitle( "Saturn Project Browser" );
		Application::Get()->GetWindow()->Show();
	}

	void ProjectBrowserLayer::OnAttach()
	{	
		m_TitleBar.AddOnExitFunction( [this]() -> bool
		{
			m_ShouldThreadTerminate = true;

			using namespace std::literals::chrono_literals;

			std::this_thread::sleep_for( 1ms );

			m_RecentProjectThread.join();

			return true;
		} );

		m_NoIconTexture = Ref<Texture2D>::Create( "content/textures/NoIcon.png" );

		// Centre window.
		const auto& rMonitorSize = RubyLibrary::Get().GetPrimaryMonitor().WorkSize;
		const auto& rWindowSize = Application::Get()->GetWindow()->GetSize();

		Application::Get()->GetWindow()->SetPosition( ( rMonitorSize.x - rWindowSize.x ) * 0.5f, ( rMonitorSize.y - rWindowSize.y ) * 0.5f );
	}

	ProjectBrowserLayer::~ProjectBrowserLayer()
	{
		m_NoIconTexture = nullptr;
	
#if defined(SAT_PROFILER_ENABLE)
		tracy::ShutdownProfiler();
#endif
	}
	
	void ProjectBrowserLayer::OnDetach()
	{
		m_ShouldThreadTerminate = true;
	
		if( m_RecentProjectThread.joinable() )
			m_RecentProjectThread.join();

		EngineSettingsSerialiser uss;
		uss.Serialise();
	}

	void ProjectBrowserLayer::OnUpdate( Timestep time )
	{
	}

	void ProjectBrowserLayer::OnImGuiRender()
	{
		// --- Title bar
		m_TitleBar.OnImGuiRender();

		// --- Check if Saturn directory is set, if not show prompt to set it
		if( !m_HasSaturnDir )
		{
			if( ImGui::BeginPopupModal( "Saturn directory not set", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) ) 
			{
				ImGui::Text( "No Saturn directory set. Please set the SATURN_DIR environment variable." );
				ImGui::Text( "The directory that you pick must point to the root directory of Saturn, it should contain /bin, /Saturn, /Saturn-Editor etc" );
				
				const auto str = m_SaturnDir.string();

				ImGui::InputText( "##enterpath", ( char* ) str.c_str(), str.size(), ImGuiInputTextFlags_ReadOnly );
				ImGui::SameLine();
				if( ImGui::Button( "...##dir" ) )
				{
					m_SaturnDir = Application::Get()->OpenFolder();
				}
				
				ImGui::Separator();
				
				ImGui::BeginHorizontal( "##options" );

				{
					Auxiliary::ScopedDisabledFlag disabledIfInvalid( m_SaturnDir.empty() && !std::filesystem::exists( m_SaturnDir ) );

					if( ImGui::Button( "Set" ) )
					{
						Auxiliary::SetEnvironmentVariable( "SATURN_DIR", m_SaturnDir.string() );
						
						m_HasSaturnDir = true;
						ImGui::CloseCurrentPopup();
					}
				}

				if( ImGui::Button( "Exit" ) )
				{
					Application::Get()->Close();
				}

				ImGui::EndHorizontal();

				ImGui::EndPopup();
			}

			ImGui::OpenPopup( "Saturn directory not set" );
		}

		if( m_OpenAboutWindow ) ShowAboutWindow();

		const ImGuiViewport* pViewport = ImGui::GetMainViewport();
		const ImGuiID dockspaceID = ImGui::DockSpaceOverViewport( pViewport, ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoDockingOverMe | ImGuiDockNodeFlags_NoUndocking );

		ImGui::Begin( "##project_browser", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar );
		ImGui::SetWindowDock( ImGui::GetCurrentWindow(), dockspaceID, ImGuiCond_FirstUseEver );

		const auto boldFont = ImGui::GetIO().Fonts->Fonts[ 1 ];
		ImGui::PushFont( boldFont );
		ImGui::Text( "Recent Projects" );
		ImGui::Separator();
		ImGui::PopFont();

		// Recent Projects
		ImGui::BeginHorizontal( "##recentProjects" );

		for( const auto& rProjectInfo : m_RecentProjects )
		{
			DrawRecentProject( rProjectInfo );
		
			ImGui::Spring();
		}

		ImGui::EndHorizontal();

		constexpr float bottomBarHeight = 48.0f;
		ImGui::Dummy( ImVec2( 0.0f, ImGui::GetContentRegionAvail().y - bottomBarHeight ) );

		ImGui::SetCursorPosY( ImGui::GetWindowHeight() - bottomBarHeight );

		ImGui::Separator();

		ImGui::BeginHorizontal( "##project_browser_bottom" );

		if( ImGui::Button( "Import", ImVec2( bottomBarHeight, bottomBarHeight ) ) ) 
		{
			const auto filePath = Application::Get()->OpenFile( "Saturn Project file (*.sproject)|*.sproject" );
			if( !filePath.empty() )
				ImportExternalProject( filePath );
		}

		ImGui::Spring();

		if( ImGui::Button( "Create New", ImVec2( bottomBarHeight, bottomBarHeight ) ) ) 
			m_ShowNewProjectPopup = true;

		ImGui::EndHorizontal();

		if( m_ShowNewProjectPopup )
		{
			ImGui::OpenPopup( "New project" );
			m_ShowNewProjectPopup = false;
		}

		const auto center = pViewport->GetCenter();
		ImGui::SetNextWindowPos( center, ImGuiCond_FirstUseEver, ImVec2( 0.5f, 0.5f ) );

		if( ImGui::BeginPopupModal( "New project", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove ) )
		{
			ImGui::InputTextWithHint( "##project_name", "Project name", m_ProjectNameBuffer, 1024 );

			ImGui::SameLine();
			ImGui::Text( ".sproject" );

			ImGui::InputTextWithHint( "##project_loc", "Project location", ( char* ) m_ProjectFilePath.string().c_str(), 1024, ImGuiInputTextFlags_ReadOnly );
			ImGui::SameLine();

			if( ImGui::SmallButton( "...##location" ) )
			{
				m_ProjectFilePath = Application::Get()->OpenFolder();
			}

			ImGui::Checkbox( "Create helpful folders", &m_CreateHelpfulFolders );

			if( ImGui::BeginItemTooltip() )
			{
				ImGui::Text( "If this option is enabled, the project will start with some helpful folders in the Assets directory, such as \"Scenes\" and \"Meshes\"" );
				ImGui::EndTooltip();
			}

			ImGui::Separator();

			auto drawDisabledBtn = [&]( const char* n )
				{
					ImGui::PushItemFlag( ImGuiItemFlags_Disabled, true );
					ImGui::PushStyleVar( ImGuiStyleVar_Alpha, 0.5f );
					ImGui::Button( n );
					ImGui::PopStyleVar( 1 );
					ImGui::PopItemFlag();
				};

			auto createButtonFunc = [&]
				{
					if( m_ProjectNameBuffer == nullptr && !m_ProjectFilePath.empty() )
					{
						drawDisabledBtn( "Create" );
					}
					else if( !std::filesystem::exists( m_ProjectFilePath ) )
					{
						drawDisabledBtn( "Create" );
					}
					else
					{
						if( ImGui::Button( "Create" ) )
						{
							// Path: C:\{dirs}\{name}\{name}.sproject
							std::filesystem::path fullPath = m_ProjectFilePath;
							fullPath /= std::string( m_ProjectNameBuffer );
							fullPath /= std::string( m_ProjectNameBuffer );
							fullPath.replace_extension( ".sproject" );

							CreateProject( fullPath );

							auto& us = EngineSettings::Get();
							us.GetAllRecentProjects().push_back( fullPath );

							EngineSettingsSerialiser uss;
							uss.Serialise();

							ImGui::CloseCurrentPopup();
						}
					}
				};

			createButtonFunc();

			ImGui::SameLine();

			if( ImGui::Button( "Cancel" ) )
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}

		ImGui::End();
	}

	void ProjectBrowserLayer::DrawRecentProject( const ProjectInformation& rProject )
	{
		ImDrawList* pDrawList = ImGui::GetWindowDrawList();
		
		const std::string name = std::format( "##{0}", rProject.Name );

		const ImVec2 projectNameTextSize = ImGui::CalcTextSize( rProject.Name.c_str() );
		const ImVec2 lastWriteTextSize = ImGui::CalcTextSize( rProject.LastWriteTime.c_str() );

		const ImVec2 imageSize = ImVec2( 156.0f, 128.0f );

		ImVec2 buttonSize = ImVec2( 156.0f, 156.0f );
		const float extraSizeNeeded = projectNameTextSize.y + lastWriteTextSize.y;
		buttonSize.y += extraSizeNeeded;

		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton( name.c_str(), buttonSize );

		ImRect buttonBoundingBox( pos, ImVec2( pos.x + buttonSize.x, pos.y + buttonSize.y ) );

		bool hovered = ImGui::IsItemHovered();
		if( hovered )
		{
			pDrawList->AddRect( buttonBoundingBox.Min, buttonBoundingBox.Max, ImGui::GetColorU32( ImGuiCol_ButtonHovered ), 5.0f, ImDrawFlags_RoundCornersAll );
		
			if( ImGui::BeginTooltip() ) 
			{
				ImGui::Text( "%s", rProject.Name.c_str() );
				ImGui::Text( "Project Path: %s", rProject.Filepath.string().c_str() );
				ImGui::Text( "Asset Path: %s", rProject.AssetPath.string().c_str() );
				ImGui::Text( "Last Modified: %s", rProject.LastWriteTime.c_str() );

				ImGui::EndTooltip();
			}

			if( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
			{
				OpenEditorWithProject( rProject );
			}
		}

		// Draw the button background
		pDrawList->AddRectFilled( buttonBoundingBox.Min, buttonBoundingBox.Max, ImGui::GetColorU32( hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button ), 5.0f, ImDrawFlags_RoundCornersAll );

		ImVec2 imagePos = ImVec2( 
			buttonBoundingBox.Min.x, 
			buttonBoundingBox.Min.y );

		pDrawList->AddImage( 
			rProject.ThumbnailTexture->GetDescriptorSet(), 
			imagePos, 
			ImVec2( imagePos.x + imageSize.x, imagePos.y + imageSize.y ), { 0, 1 }, { 1, 0 } );

		float availableHeight = buttonSize.y - imageSize.y - ImGui::GetStyle().FramePadding.y * 2;
		float lineHeight = availableHeight * 0.5f;

		ImVec2 projectNameTextPos = ImVec2(
			buttonBoundingBox.Min.x + ( buttonSize.x - projectNameTextSize.x ) * 0.5f,
			imagePos.y + imageSize.y + ImGui::GetStyle().FramePadding.y * 0.5f + 4.0f
		);
		pDrawList->AddText( projectNameTextPos, ImGui::GetColorU32( ImGuiCol_Text ), rProject.Name.c_str() );

		ImVec2 lastWriteTimeTextPos = ImVec2(
			buttonBoundingBox.Min.x + ImGui::GetStyle().FramePadding.x + ( buttonSize.x - lastWriteTextSize.x ) * 0.5f,
			projectNameTextPos.y + lineHeight
		);
		pDrawList->AddText( lastWriteTimeTextPos, ImGui::GetColorU32( ImGuiCol_TextDisabled ), rProject.LastWriteTime.c_str() );
	}

	void ProjectBrowserLayer::ImportExternalProject( const std::filesystem::path& rPath )
	{
		ProjectSerialiser ps;
		ps.Deserialise( rPath );

		Ref<Project> activeProject = Project::GetActiveProject();

		if( activeProject )
		{
			ProjectInformation info{};
			info.Filepath = rPath;
			info.Name = activeProject->GetConfig().Name;
			info.AssetPath = activeProject->GetFullAssetPath();
			info.LastWriteTime = std::format( "{0}", std::filesystem::last_write_time( rPath ) );
			info.LastWriteTime = info.LastWriteTime.substr( 0, info.LastWriteTime.find_first_of( " " ) );
			info.ThumbnailTexture = m_NoIconTexture;

			m_RecentProjects.push_back( info );
		}
	}

	void ProjectBrowserLayer::CreateProject( const std::filesystem::path& rPath )
	{
		const std::string ProjectName = rPath.filename().string();
		const std::string ProjectNameNoExt = rPath.stem().string();

		// parent_path to get rid of the file
		std::filesystem::path ProjectFolderPath = rPath.parent_path();

		if( !std::filesystem::exists( ProjectFolderPath ) )
			std::filesystem::create_directories( ProjectFolderPath );

		// Copy files.
		std::filesystem::path templatesPath = m_SaturnDir;
		templatesPath /= "Saturn-Editor";
		templatesPath /= "content";
		templatesPath /= "Templates";

		{
			const auto projectFilepath = templatesPath / "Project.sproject";
			std::filesystem::copy( projectFilepath, ProjectFolderPath / "Project.sproject" );
		}
		{
			const auto premakeBuildFile = templatesPath / "premake5.lua";
			std::filesystem::copy( premakeBuildFile, ProjectFolderPath / "premake5.lua" );
		}

		// New Project ref
		Ref<Project> newProject = Ref<Project>::Create();

		// Project file
		{
			std::ifstream stream( ProjectFolderPath / "Project.sproject" );
			std::stringstream ss;
			ss << stream.rdbuf();
			stream.close();

			std::string str = ss.str();
			ReplaceToken( str, "%PROJECT_NAME%", ProjectNameNoExt );

			std::ofstream out( ProjectFolderPath / "Project.sproject" );
			out << str;
			out.close();

			newProject->GetConfig().Name = ProjectNameNoExt;
			newProject->GetConfig().Path = ProjectFolderPath.string();

			std::filesystem::rename( ProjectFolderPath / "Project.sproject", ProjectFolderPath / ProjectName );
		}

		std::filesystem::create_directory( ProjectFolderPath / "Assets" );

		if( m_CreateHelpfulFolders )
		{
			std::filesystem::create_directories( ProjectFolderPath / "Assets" / "Shaders" );
			std::filesystem::create_directories( ProjectFolderPath / "Assets" / "Textures" );
			std::filesystem::create_directories( ProjectFolderPath / "Assets" / "Meshes" );
			std::filesystem::create_directories( ProjectFolderPath / "Assets" / "Materials" );
			std::filesystem::create_directories( ProjectFolderPath / "Assets" / "Scenes" );
			std::filesystem::create_directories( ProjectFolderPath / "Assets" / "Sound" );
			std::filesystem::create_directories( ProjectFolderPath / "Assets" / "Sound" / "Source" );
		}

		std::filesystem::create_directory( ProjectFolderPath / "Source" );
		std::filesystem::create_directory( ProjectFolderPath / "Build" );
		std::filesystem::create_directory( ProjectFolderPath / "Cache" );

		{
			const auto editorScriptsPath = templatesPath / "ScriptsForCopy";

			if( std::filesystem::exists( editorScriptsPath ) )
			{
				// Copy to project directory.
				std::filesystem::copy( ProjectFolderPath / "Scripts", editorScriptsPath, std::filesystem::copy_options::overwrite_existing );
			}
		}

		std::filesystem::create_directories( ProjectFolderPath / "Source" / newProject->GetConfig().Name );

		Project::SetActiveProject( newProject );

		std::filesystem::path outputPath = newProject->GetConfig().Path;
		outputPath /= newProject->GetConfig().Name;

		ProjectSerialiser ps;
		ps.Serialise();

		Project::SetActiveProject( nullptr );
	}

	void ProjectBrowserLayer::OpenEditorWithProject( const ProjectInformation& rProject )
	{
		std::filesystem::path commandLine = m_SaturnDir;
		const std::filesystem::path workingDir = commandLine / "Saturn-Editor";

		commandLine /= "bin";
		commandLine /= std::format( "{0}-{1}", Application::Get()->GetCurrentConfigName(), SAT_PLATFORM_BINARY_FOLDER );
		commandLine /= "Saturn-Editor";
			
#if defined( SAT_PLATFORM_WINDOWS )
		commandLine /= "Saturn-Editor.exe";
#elif defined(SAT_PLATFORM_MACOS)
		commandLine /= "Saturn-Editor.app";
#else
		commandLine /= "Saturn-Editor";
#endif
		commandLine += " " + rProject.Filepath.string();

		DetachedProcess dp( commandLine, workingDir );

		EngineSettings::Get().AddRecentProject( rProject.Filepath );

		Application::Get()->Close();
	}

	void ProjectBrowserLayer::OnEvent( Event& rEvent )
	{
	}

	void ProjectBrowserLayer::ShowAboutWindow()
	{
		if( ImGui::Begin( "About", &m_OpenAboutWindow ) )
		{
			EditorAboutWindowContents::DrawContents();
			
			ImGui::End();
		}
	}

}
