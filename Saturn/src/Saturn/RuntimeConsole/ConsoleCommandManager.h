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

#include "ConsoleOutputSink.h"

#include "Saturn/Core/Base.h"

namespace Saturn {

	class ConsoleCommandBase;

	class ConsoleCommandManager : public RefTarget
	{
	public:
		static inline ConsoleCommandManager* Get() { return SingletonStorage::GetSingleton<ConsoleCommandManager>(); }
	public:
		ConsoleCommandManager();
		~ConsoleCommandManager();

		void RegisterEngineDefaultCommands();
		void RegisterCommand( ConsoleCommandBase* pCmd );
		void UnregisterCommand( ConsoleCommandBase* pCmd );

		//
		// Execute a command.
		// 
		// @param rCommandName - command name to be executed.
		// 
		// returns true if the command is found.
		//
		bool Execmd( const std::string& rCommandName );

		//
		// Execute a command.
		// 
		// @param pCommand - command to be executed.
		//
		void Execmd( ConsoleCommandBase* pCommand );

		//
		// Find a command.
		//
		ConsoleCommandBase* FindCommand( const std::string& rCommandName );

	public:
		ConsoleOutputSink& GetSink() { return m_Sink; }

		const std::unordered_map<std::string, ConsoleCommandBase*>& GetAllCommands() const { return m_Commands; }

	private:
		void ClearAllCommands();

	private:
		ConsoleOutputSink m_Sink;
		// NAME -> COMMAND PTR (Non-owning ptr, commands are registerd on the stack)
		std::unordered_map<std::string, ConsoleCommandBase*> m_Commands;
	};

}
