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

#include "SingletonStorage.h"

#include <deque>

namespace Saturn {

	// TODO: Upcoming API for text editor registration.
	struct TextEditorApplication
	{
		std::string FriendlyName;
		std::filesystem::path Path;
	};

	enum class EditorFont : uint8_t
	{
		// The default font prior to Saturn Version 0.2.5,
		// this font is good because it supports Latin, Greek and Cyrillic letters!
		// see: https://fonts.google.com/noto/specimen/Noto+Sans
		NotoSans,

		// Atkinson Hyperlegible Next
		// NB: This font does not support Greek and Cyrillic letters!
		// see: https://fonts.google.com/specimen/Atkinson+Hyperlegible+Next
		Atkinson
	};

	class EngineSettings
	{
	public:
		SAT_SINGLETON_LAZY( EngineSettings )

	public:
		EngineSettings() = default;
		~EngineSettings() = default;

		void AddRecentProject( const std::filesystem::path& rPath );
		void ClearAllRecentProjects();

		std::deque<std::filesystem::path> GetAllRecentProjects() { return m_RecentProjects; }
		const std::deque<std::filesystem::path> GetAllRecentProjects() const { return m_RecentProjects; }

		void SetEditorFont( EditorFont font ) { m_EditorFont = font; }
		EditorFont GetEditorFont() const { return m_EditorFont; }

	public:
		// The startup project name i.e. (MyProject)
		std::string StartupProjectName;
		
		// The project file dir i.e. (D:\Projects\MyProject\MyProject.sproject) [Serialised]
		std::filesystem::path StartupProject;

	private:
		std::deque< std::filesystem::path > m_RecentProjects; // [Serialised]
		EditorFont m_EditorFont = EditorFont::NotoSans; // [Serialised]

	private:
		friend class EngineSettingsSerialiser;
	};
}
