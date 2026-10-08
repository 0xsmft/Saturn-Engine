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

#include "Saturn/Core/App.h"

#include "ImGuiAuxiliary.h"

#include <imgui.h>
#include <ImGuiColorTextEdit/TextEditor.h>

namespace Saturn {

	ShaderViewerWindow::ShaderViewerWindow( const std::filesystem::path& rShaderPath )
		: ImGuiWindow()
	{
		m_Name = rShaderPath.filename().string();
		m_Editor = std::make_unique<ImGuiColorTextEdit::TextEditor>();
		m_Editor->SetPalette( m_Editor->GetDarkPalette() );

		InitEditorFromFile( rShaderPath );
	}

	ShaderViewerWindow::~ShaderViewerWindow()
	{
	}

	void ShaderViewerWindow::OnImGuiRender()
	{
		ImGui::SetNextWindowPos( ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Once );
		ImGui::SetNextWindowSize( ImVec2( 350.0f, 350.0f ), ImGuiCond_FirstUseEver );

		ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
		if( IsDirty() )
			flags |= ImGuiWindowFlags_UnsavedDocument;

		auto monospaceFont = ImGui::GetIO().Fonts->Fonts[ 3 ];
		if( ImGui::Begin( m_Name.c_str(), &m_Open, flags ) )
		{
			m_Editor->SetReadOnlyEnabled( m_IsReadOnly );

			if( ImGui::BeginMenuBar() )
			{
				if( ImGui::BeginMenu( "File" ) )
				{
					if( ImGui::MenuItem( "Save" ) )
					{
						if( !m_IsReadOnly )
						{
							std::ofstream stream( m_ShaderPath );
							stream << m_Editor->GetText();
							stream.close();

							m_SavedUndoIndex = m_Editor->GetUndoIndex();
						}
					}

					if( ImGui::MenuItem( "Close" ) )
					{
						m_Open = false;
					}

					ImGui::EndMenu();
				}

				if( ImGui::BeginMenu( "Edit" ) )
				{
					Auxiliary::DisabledFlag disabledIfRo( m_IsReadOnly );

					{
						Auxiliary::ScopedDisabledFlag canUndo( !m_Editor->CanUndo() );
	
						if( ImGui::MenuItem( "Undo" ) )
						{
							m_Editor->Undo();
						}
					}

					{
						Auxiliary::ScopedDisabledFlag canRedo( !m_Editor->CanRedo() );

						if( ImGui::MenuItem( "Redo" ) )
						{
							m_Editor->Redo();
						}
					}

					if( ImGui::MenuItem( "Cut" ) )
					{
						m_Editor->Cut();
					}

					if( ImGui::MenuItem( "Copy" ) )
					{
						m_Editor->Copy();
					}

					if( ImGui::MenuItem( "Paste" ) )
					{
						m_Editor->Paste();
					}

					if( ImGui::MenuItem( "Move down line" ) )
					{
						m_Editor->MoveDownLines();
					}

					if( ImGui::MenuItem( "Move up line" ) )
					{
						m_Editor->MoveUpLines();
					}

					disabledIfRo.Pop();

					ImGui::EndMenu();
				}

				if( ImGui::BeginMenu( "View" ) )
				{
					if( ImGui::MenuItem( "Show whitespace" ) )
					{
						m_Editor->SetShowWhitespacesEnabled( !m_Editor->IsShowWhitespacesEnabled() );
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}

			ImGui::BeginHorizontal( "##optsbtn" );

			{
				Auxiliary::ScopedDisabledFlag disabledIfRo( m_IsReadOnly );
			
				if( ImGui::Button( "Compile", { 0.0f, 24.0f } ) )
				{
					HandleCompilation();
				}

#if !defined(SAT_PLATFORM_LINUX)
				if( ImGui::Button( "Open in native explorer", { 0.0f, 24.0f } ) )
				{
					Application::Get()->OpenNativeFileExplorer( m_ShaderPath, true );
				}
#endif
			}

			ImGui::EndHorizontal();

			ImGui::PushFont( monospaceFont );
			m_Editor->Render( m_Name.c_str() );
			ImGui::PopFont();
		}

		ImGui::End();
	}

	void ShaderViewerWindow::OnUpdate( Timestep ts )
	{
	}

	void ShaderViewerWindow::OnEvent( Event& rEvent )
	{
	}

	bool ShaderViewerWindow::IsDirty() const
	{
		return m_Editor->GetUndoIndex() != m_SavedUndoIndex;
	}

	void ShaderViewerWindow::InitEditorFromFile( const std::filesystem::path& rShaderPath )
	{
		m_ShaderPath = rShaderPath;

		if( std::filesystem::exists( m_ShaderPath ) )
		{
			// Load the file.
			std::ifstream stream( m_ShaderPath );

			std::string text;

			stream.seekg( 0, std::ios::end );
			text.reserve( stream.tellg() );
			stream.seekg( 0, std::ios::beg );

			text.assign( std::istreambuf_iterator<char>( stream ), std::istreambuf_iterator<char>() );
			stream.close();

			m_Editor->SetText( text );
		}

		m_Editor->SetLanguage( ImGuiColorTextEdit::TextEditor::Language::Glsl() );
		m_SavedUndoIndex = m_Editor->GetUndoIndex();
	}

	void ShaderViewerWindow::HandleCompilation()
	{
		if( m_OnCompileFunction )
			( m_OnCompileFunction ) ( );
	}

}
