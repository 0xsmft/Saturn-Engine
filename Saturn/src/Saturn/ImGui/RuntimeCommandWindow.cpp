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
#include "RuntimeCommandWindow.h"

#include "ImGuiAuxiliary.h"

#include "Saturn/RuntimeConsole/ConsoleCommandManager.h"
#include "Saturn/RuntimeConsole/ConsoleCommand.h"

#include <imgui.h>

namespace Saturn {

	RuntimeCommandWindow::RuntimeCommandWindow()
		: ImGuiWindow( RuntimeCommandWindow::GetStaticName() )
	{
	}

	RuntimeCommandWindow::RuntimeCommandWindow( const std::string& rName )
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
			rArgsList.push_back( rRemainingText );
			return;
		}

		const std::string token = rRemainingText.substr( 0, pos );
		rArgsList.push_back( token );

		ParseRec( rArgsList, rRemainingText.substr( pos + 1 ) );
	}

	void RuntimeCommandWindow::OnImGuiRender()
	{
		if( ImGui::Begin( m_Name.c_str(), &m_Open ) )
		{
			if( Auxiliary::InputText( "##entercommand", &m_CommandNameBuffer, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsUppercase ) )
			{
				OnCommandEntered();
			}

			ImGui::Separator();
			if( ImGui::BeginChild( "##responsearea" ) )
			{
				auto& rMsgs = ConsoleCommandManager::Get().GetSink().GetMessages();
				for( auto itr = rMsgs.rbegin(); itr != rMsgs.rend(); ++itr )
				{
					ImGui::Text( itr->FormattedMessage.c_str() );
				}

				ImGui::EndChild();
			}
		}

		ImGui::End();
	}

	void RuntimeCommandWindow::OnCommandEntered()
	{
		auto& rCommandMgr = ConsoleCommandManager::Get();

		if( !m_CommandNameBuffer.empty() )
		{
			// Check for command if we start with /
			if( m_CommandNameBuffer.starts_with( "/" ) )
			{
				// Command name without the slash
				auto cmdName = m_CommandNameBuffer.substr( 1, m_CommandNameBuffer.size() - 1 );

				// If we have a space afterwards we only want to pass in the command name
				// e.g. a command of "add 1 2" would be "add"
				// then the args would be "1 2"
				
				auto argsText = cmdName;
				if( cmdName.contains( ' ' ) )
				{
					argsText = argsText.substr( cmdName.find_first_of( ' ' ) + 1 );
					cmdName = cmdName.substr( 0, cmdName.find_first_of( ' ' ) );
				}

				ConsoleCommandBase* pCommand = rCommandMgr.FindCommand( cmdName );
				if( !pCommand )
				{
					std::string errorMsg = std::format( "Unknown command \"{0}\"", m_CommandNameBuffer );
					rCommandMgr.GetSink().Sink( errorMsg );
				}
				else
				{
					if( pCommand->IsFlagSet( ConsoleCommandFlags_RequiresArguments ) ) 
					{
						std::vector<std::string> argList;
						ParseRec( argList, argsText );

						if( !pCommand->Verify( argList.size() ) )
						{
							std::string errorMsg = std::format( "Too many or too little commands specified into '{}'", m_CommandNameBuffer );
							rCommandMgr.GetSink().Sink( errorMsg );
						}
						else
						{
							// If we pass verification we now change the arguments into their native type.
							pCommand->PopulateArgs( argList );
						}
					}
					
					rCommandMgr.Execmd( pCommand );
				}
			}
			else
			{
				rCommandMgr.GetSink().Sink( m_CommandNameBuffer );
			}

			m_CommandNameBuffer.clear();
		}
	}

}
