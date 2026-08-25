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

#include "ArgumentConversions.h"

namespace Saturn {

	enum ConsoleCommandFlags : uint8_t
	{
		ConsoleCommandFlags_DefaultFlags = 0,
		ConsoleCommandFlags_RequiresArguments = BIT( 0 ),
		ConsoleCommandFlags_RuntimeOnly       = BIT( 1 ),
	};

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
		ConsoleCommandBase( const std::string& rName, ConsoleCommandFlags flags = ConsoleCommandFlags_DefaultFlags )
			: m_Name( rName ), m_Flags( ( uint8_t ) flags )
		{
		}

		virtual ~ConsoleCommandBase() 
		{
			if( auto* pCmm = ConsoleCommandManager::Get() ) 
			{
				pCmm->UnregisterCommand( this );
			}
		}

		virtual void Execute() = 0;
		
		constexpr virtual bool Verify( size_t argsCount ) { return true; }
		virtual void PopulateArgs( const std::vector<std::string>& rArgs ) {}

		ConsoleCommandFlags GetFlags() const { return ( ConsoleCommandFlags )m_Flags; }
		bool IsFlagSet( uint8_t flag ) const { return ( m_Flags & flag ) != 0; }

	protected:
		std::string m_Name;
		uint8_t m_Flags;

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
			ConsoleCommandManager::Get()->RegisterCommand( this );
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

	constexpr uint8_t MAX_NUMBER_OF_ARGS = 16u;

	//
	// A console command with N specified args
	// 
	// N cannot be over 16.
	// 
	// There is not return type for this argument -- it returns void.
	// 
	template<typename TheoreticalFunction, typename... Args>
	class ConsoleCommandArgsVoidRet : public ConsoleCommandBase
	{
	public:
		ConsoleCommandArgsVoidRet( const std::string& rFullName, TheoreticalFunction&& rrFunctor )
			: ConsoleCommandBase( rFullName, ConsoleCommandFlags_RequiresArguments ), m_Function( std::move( rrFunctor ) )
		{
			static_assert( sizeof...( Args ) < MAX_NUMBER_OF_ARGS, "Too many arguments passed into ConsoleCommandArgs<>!" );

			ConsoleCommandManager::Get()->RegisterCommand( this );
		}

		virtual ~ConsoleCommandArgsVoidRet() = default;

		virtual void Execute() override
		{
			Invoke( std::index_sequence_for<Args...>{} );
		}

		constexpr virtual bool Verify( size_t argsCount ) override
		{
			return argsCount == m_NumberOfArgs;
		}
		
		virtual void PopulateArgs( const std::vector<std::string>& rArgs ) override
		{
			PopulateArgsInternal( rArgs, std::index_sequence_for<Args...>{} );
		}

	protected:
		template<size_t... I>
		void PopulateArgsInternal( const std::vector<std::string>& rArgs, std::index_sequence<I...> )
		{
			m_Args = std::make_tuple( Auxiliary::RtConsoleCommandArgConvert<std::decay_t<Args>>( rArgs[ I ] )... );
		}

		template<size_t... I>
		void Invoke( std::index_sequence<I...> )
		{
			std::invoke( m_Function, std::get<I>( m_Args )... );
		}

	private:
		std::tuple<std::decay_t<Args>...> m_Args;
		std::decay_t<TheoreticalFunction> m_Function;
		uint8_t m_NumberOfArgs = sizeof...( Args );

	private:
		friend class CommandList;
	};

}
