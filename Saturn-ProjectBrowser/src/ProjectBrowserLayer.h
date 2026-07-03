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

#include <Saturn/ImGui/TitleBar.h>

#include <Saturn/Vulkan/Texture.h>
#include <Saturn/Core/Layer.h>

namespace Saturn {

	struct ProjectInformation
	{
		std::string Name;
		std::filesystem::path Filepath;
		std::filesystem::path AssetPath;
		std::filesystem::path ThumbnailPath;
		Ref<Texture2D> ThumbnailTexture = nullptr;

		std::string LastWriteTime;
	
		uint64_t Version = SAT_CURRENT_VERSION;
	};

	class ProjectBrowserLayer : public Layer
	{
	public:
		ProjectBrowserLayer();
		~ProjectBrowserLayer();

		void OnUpdate( Timestep time ) override;
		void OnImGuiRender() override;
		void OnEvent( Event& rEvent ) override;
		void OnAttach() override;
		void OnDetach() override;

	private:
		void ShowAboutWindow();

		void OpenEditorWithProject( const ProjectInformation& rProject );
		void CreateProject( const std::filesystem::path& rPath );
		void DrawRecentProject( const ProjectInformation& rProject );

		void ImportExternalProject( const std::filesystem::path& rPath );

	private:
		TitleBar m_TitleBar;

		Ref<Texture2D> m_NoIconTexture = nullptr;

		char* m_SaturnDirBuffer = new char[ 1024 ];
		std::filesystem::path m_SaturnDir;

		char* m_ProjectNameBuffer = new char[ 1024 ];
		std::filesystem::path m_ProjectFilePath;

		bool m_ShowNewProjectPopup = false;
		bool m_ShouldThreadTerminate = false;
		bool m_CreateHelpfulFolders = true;
		bool m_HasSaturnDir = false;
		bool m_OpenAboutWindow = false;

		std::vector<ProjectInformation> m_RecentProjects;
		std::thread m_RecentProjectThread;
	};
}
