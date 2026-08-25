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
#include "EditorRuntimeCommandWindow.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

#include "Saturn/Core/StringAuxiliary.h"

#include "SharedGlobals.h"
#include "Saturn/Scene/Scene.h"

#include "Saturn/RuntimeConsole/ConsoleCommandManager.h"
#include "Saturn/RuntimeConsole/ConsoleCommand.h"

#include <imgui.h>

namespace Saturn {

	EditorRuntimeCommandWindow::EditorRuntimeCommandWindow()
		: ImGuiWindow( EditorRuntimeCommandWindow::GetStaticName() )
	{
	}

	EditorRuntimeCommandWindow::EditorRuntimeCommandWindow( const std::string& rName )
		: ImGuiWindow( rName )
	{
	}

	static void ParseRec( 
		std::vector<std::string>& rArgsList, 
		const std::string& rRemainingText )
	{
		const size_t pos = rRemainingText.find( ' ' );

		if( pos == std::string::npos )
		{
			if( !rRemainingText.empty() )
				rArgsList.push_back( rRemainingText );
			
			return;
		}

		const std::string token = rRemainingText.substr( 0, pos );
		
		if( !token.empty() )
			rArgsList.push_back( token );

		ParseRec( rArgsList, rRemainingText.substr( pos + 1 ) );
	}

	static int EnterCommandCallbackHandler( ImGuiInputTextCallbackData* pData ) 
	{
		EditorRuntimeCommandWindow* pThis = ( EditorRuntimeCommandWindow* ) pData->UserData;
		auto& rCommands = pThis->GetCommandHistory();

		if( pData->EventFlag == ImGuiInputTextFlags_CallbackHistory )
		{
			if( rCommands.size() )
			{
				if( pData->EventKey == ImGuiKey_UpArrow )
				{
					const auto index = pThis->GetCmdHistoryIndex();
					auto& rCmd = rCommands.at( index );

					// Set index to the one before this command
					// But if we are at 0 reset back to the most recent command.
					if( index == 0 )
						pThis->SetCmdHistoryIndex( rCommands.size() - 1 );
					else
						pThis->SetCmdHistoryIndex( index - 1 );

					pData->DeleteChars( 0, pData->BufTextLen );
					pData->InsertChars( 0, rCmd.data() );
				}
				else if( pData->EventKey == ImGuiKey_DownArrow )
				{
					auto index = pThis->GetCmdHistoryIndex() + 1;

					// Reset back to the oldest command if the index will cause an array out of bounds error.
					if( index >= rCommands.size() )
					{
						index = 0;
					}
					
					auto& rCmd = rCommands.at( index );

					pThis->SetCmdHistoryIndex( index );

					pData->DeleteChars( 0, pData->BufTextLen );
					pData->InsertChars( 0, rCmd.data() );
				}
			}
		}

		return 0;
	}

	void EditorRuntimeCommandWindow::OnImGuiRender()
	{
		if( ImGui::Begin( m_Name.c_str(), &m_Open ) )
		{
			if( Auxiliary::InputText( "##entercommand", &m_CommandNameBuffer, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory, EnterCommandCallbackHandler, this ) )
			{
				OnCommandEntered();
			
				ImGui::SetKeyboardFocusHere( 0 );
			}

			ImGui::Separator();
			if( ImGui::BeginChild( "##responsearea" ) )
			{
				auto& rMsgs = ConsoleCommandManager::Get()->GetSink().GetMessages();
				for( auto itr = rMsgs.rbegin(); itr != rMsgs.rend(); ++itr )
				{
					ImGui::Text( itr->FormattedMessage.c_str() );
				}

				ImGui::EndChild();
			}
		}

		ImGui::End();
	}

	void EditorRuntimeCommandWindow::OnCommandEntered()
	{
		auto* pCommandMgr = ConsoleCommandManager::Get();

		if( !m_CommandNameBuffer.empty() )
		{
			// Check for command if we start with /
			if( m_CommandNameBuffer.starts_with( "/" ) )
			{
				// Command name without the slash
				auto cmdName = m_CommandNameBuffer.substr( 1, m_CommandNameBuffer.size() - 1 );
				Auxiliary::CovertStrToUpper( cmdName );

				// If we have a space afterwards we only want to pass in the command name
				// e.g. a command of "add 1 2" would be "add"
				// then the args would be "1 2"
				
				std::string argsText;
				if( cmdName.contains( ' ' ) )
				{
					argsText = cmdName.substr( cmdName.find_first_of( ' ' ) + 1 );
					cmdName = cmdName.substr( 0, cmdName.find_first_of( ' ' ) );
				}

				m_CommandHistory.push_back( m_CommandNameBuffer );
				m_CurrentCommandHistoryIndex = m_CommandHistory.size() - 1;

				ConsoleCommandBase* pCommand = pCommandMgr->FindCommand( cmdName );
				if( !pCommand )
				{
					std::string errorMsg = std::format( "Unknown command \"{0}\"", m_CommandNameBuffer );
					pCommandMgr->GetSink().Sink( errorMsg );
				}
				else
				{
					bool canExe = true;
					if( pCommand->IsFlagSet( ConsoleCommandFlags_RequiresArguments ) ) 
					{
						std::vector<std::string> argList;
						ParseRec( argList, argsText );

						if( !pCommand->Verify( argList.size() ) )
						{
							std::string errorMsg = std::format( "Too many or too little commands specified into '{}'", m_CommandNameBuffer );
							pCommandMgr->GetSink().Sink( errorMsg );
						
							canExe = false;
						}
						else
						{
							// If we pass verification we now change the arguments into their native type.
							pCommand->PopulateArgs( argList );
						}
					}

					if( pCommand->IsFlagSet( ConsoleCommandFlags_RuntimeOnly ) )
					{
						if( !g_ActiveScene->IsRuntimeActive() )
						{
							std::string errorMsg = std::format( "The command '{}' can only be executed during runtime!", m_CommandNameBuffer );
							pCommandMgr->GetSink().Sink( errorMsg );

							canExe = false;
						}
					}
					
					if( canExe )
						pCommandMgr->Execmd( pCommand );
				}
			}
			else
			{
				pCommandMgr->GetSink().Sink( m_CommandNameBuffer );
			}

			m_CommandNameBuffer.clear();
		}
	}

}
