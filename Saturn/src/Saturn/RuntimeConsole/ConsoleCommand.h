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

#include <string>

namespace Saturn {

	//
	// ConsoleCommandBase
	// 
	// Base class for all console commands.
	// 
	// Please note that all Console commands must either be allocated on the heap or if they are allocated on the stack,
	// the object must not exit it's scope as this will cause it to de-register itself.
	//
	//
	class ConsoleCommandBase : public RefTarget
	{
	public:
		ConsoleCommandBase( const std::string& rName )
			: m_Name( rName )
		{
		}

		virtual ~ConsoleCommandBase() 
		{
			ConsoleCommandManager::Get().UnregisterCommand( this );
		}

		virtual void Execute() = 0;

	protected:
		std::string m_Name;

	protected:
		friend class ConsoleCommandManager;
	};

	//
	// ConsoleCommandVoidRetNoArgs
	// 
	// A console command with no return value and no arguments.
	//
	template<typename TheoreticalFunctionVoidReturn>
	class ConsoleCommandVoidRetNoArgs : public ConsoleCommandBase
	{
	public:
		ConsoleCommandVoidRetNoArgs( const std::string& rName, TheoreticalFunctionVoidReturn&& rrFunctor )
			: ConsoleCommandBase( rName ), m_Function( std::move( rrFunctor ) )
		{
			ConsoleCommandManager::Get().RegisterCommand( this );
		}

		virtual ~ConsoleCommandVoidRetNoArgs() = default;

		virtual void Execute() override
		{
			m_Function();
		}

	private:
		std::decay_t<TheoreticalFunctionVoidReturn> m_Function;

	private:
		friend class CommandList;
	};

}
