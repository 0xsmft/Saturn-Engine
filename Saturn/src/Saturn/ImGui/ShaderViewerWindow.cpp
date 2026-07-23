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
#include "ShaderViewerWindow.h"

#include <imgui.h>
#include <ImGuiColorTextEdit/TextEditor.h>

namespace Saturn {

	ShaderViewerWindow::ShaderViewerWindow( const std::filesystem::path& rShaderPath )
		: ImGuiWindow()
	{
		m_Name = rShaderPath.filename().string();
		m_Editor = std::make_unique<ImGuiColorTextEdit::TextEditor>();

		InitEditorFromFile( rShaderPath );
	}

	ShaderViewerWindow::~ShaderViewerWindow()
	{
	}

	void ShaderViewerWindow::OnImGuiRender()
	{
		if( ImGui::Begin( m_Name.c_str(), &m_Open, ImGuiWindowFlags_MenuBar ) )
		{
			m_Editor->Render( m_Name.c_str() );
		}

		ImGui::End();
	}

	void ShaderViewerWindow::OnUpdate( Timestep ts )
	{
	}

	void ShaderViewerWindow::OnEvent( Event& rEvent )
	{
	}

	void ShaderViewerWindow::InitEditorFromFile( const std::filesystem::path& rShaderPath )
	{
		// Load the file.
		std::ifstream stream( rShaderPath );

		std::string text;

		stream.seekg( 0, std::ios::end );
		text.reserve( stream.tellg() );
		stream.seekg( 0, std::ios::beg );

		text.assign( std::istreambuf_iterator<char>( stream ), std::istreambuf_iterator<char>() );
		stream.close();

		m_Editor->SetText( text );
	}

}
