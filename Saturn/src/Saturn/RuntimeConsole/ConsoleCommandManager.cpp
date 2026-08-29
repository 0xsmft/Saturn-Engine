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
#include "ConsoleCommandManager.h"

#include "Saturn/Core/App.h"
#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Scene/Entity.h"
#include "Saturn/Scene/Scene.h"

#include "Saturn/Runtime/RuntimeEvents.h"

#include "ConsoleCommand.h"

namespace Saturn {

	ConsoleCommandManager::ConsoleCommandManager()
	{
#if !defined(SAT_DIST)
		SAT_CORE_ASSERT( !SingletonStorage::GetSingleton<ConsoleCommandManager>(), "A command manager already exists!" );

		SingletonStorage::AddSingleton<ConsoleCommandManager>( this );
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// Builtin commands

	static void CmmCmd_Help() 
	{
		ConsoleCommandManager::Get()->GetSink().Sink( "Type / to enter commands, wow so helpful." );
	}

	static void CmmCmd_Info()
	{
		ConsoleCommandManager::Get()->GetSink().SinkFormatted( "Saturn Engine, version: {}, internal number: {} ident: {}", ConsoleCommandMessageType::Info, SAT_CURRENT_VERSION_STRING, SAT_CURRENT_VERSION, SAT_CURRENT_VERSION_BUILD_TAG );
	}

	static void CmmCmd_Abort()
	{
		Application::Get()->Close();
	}

	static void CmmCmd_SaveAssetManager()
	{
		AssetManager::Get()->Save();
	}

	static void CmmCmd_Question()
	{
		// NOTE: Sink order is reversed.
		auto& rSink = ConsoleCommandManager::Get()->GetSink();
		rSink.Sink( "====================" );

		for( const auto& [name, rCommand] : ConsoleCommandManager::Get()->GetAllCommands() )
		{
			rSink.Sink( name );
		}
	
		rSink.Sink( "=== All Commands ===" );
	}

	static void CmmCmd_Stat( uint64_t id ) 
	{
		auto& rSink = ConsoleCommandManager::Get()->GetSink();

		const auto entity = g_ActiveScene->FindEntityByID( id );
		if( entity )
		{
			rSink.Sink( entity->GetName() );
		}
		else
		{
			rSink.Sink( "Entity not found" );
		}
	}

	static void CmmCmd_Tp( uint64_t id, glm::vec3 rPosition ) 
	{
		auto& rSink = ConsoleCommandManager::Get()->GetSink();

		const auto entity = g_ActiveScene->FindEntityByID( id );
		if( entity )
		{
			entity->SetPosition( rPosition );
		}
	}

	// obfuscated -> ke == scene renderer options
	// example: ke ao gtao
	// example: ke fps true
	static void CmmCmd_Ke( std::string key, std::string value )
	{
		Application::Get()->DispatchEvent<RuntimeSceneRendererKeCommand>( 
			std::move( key ), std::move( value ) );
	}

	//////////////////////////////////////////////////////////////////////////

	void ConsoleCommandManager::RegisterEngineDefaultCommands()
	{
		static const ConsoleCommandVoidRetNoArgs helpCommand( "help", CmmCmd_Help );
		static const ConsoleCommandVoidRetNoArgs infoCommand( "info", CmmCmd_Info );
		static const ConsoleCommandVoidRetNoArgs abrtCommand( "abort", CmmCmd_Abort );
		static const ConsoleCommandVoidRetNoArgs asstCommand( "saveassetman", CmmCmd_SaveAssetManager );
		static const ConsoleCommandVoidRetNoArgs questCommand( "?", CmmCmd_Question );

		static const ConsoleCommandArgsVoidRet<decltype( CmmCmd_Stat ), uint64_t> statCommand( "stat", CmmCmd_Stat );
		static const ConsoleCommandArgsVoidRet<decltype( CmmCmd_Tp ), uint64_t, glm::vec3> tpCommand( "tp", CmmCmd_Tp );
		
		static const ConsoleCommandArgsVoidRet<decltype( CmmCmd_Ke ), std::string, std::string> keCommand( "ke", CmmCmd_Ke );
	}

	void ConsoleCommandManager::ClearAllCommands()
	{
		m_Commands.clear();
	}

	ConsoleCommandManager::~ConsoleCommandManager()
	{
		ClearAllCommands();

#if !defined(SAT_DIST)
		SingletonStorage::RemoveSingleton( this );
#endif
	}

	void ConsoleCommandManager::RegisterCommand( ConsoleCommandBase* pCmd )
	{
		SAT_CORE_ASSERT( pCmd );

		auto nameUpper = pCmd->m_Name;
		std::transform( nameUpper.cbegin(), nameUpper.cend(), nameUpper.begin(), ::toupper );

		m_Commands[ nameUpper ] = pCmd;
	}

	void ConsoleCommandManager::UnregisterCommand( ConsoleCommandBase* pCmd )
	{
		auto nameUpper = pCmd->m_Name;
		std::transform( nameUpper.cbegin(), nameUpper.cend(), nameUpper.begin(), ::toupper );

		const auto itr = m_Commands.find( nameUpper );
		if( itr != m_Commands.end() )
		{
			m_Commands.erase( itr );
		}
	}

	bool ConsoleCommandManager::Execmd( const std::string& rCommandName )
	{
		const auto itr = m_Commands.find( rCommandName );
		if( itr != m_Commands.end() )
		{
			itr->second->Execute();
			return true;
		}

		return false;
	}

	void ConsoleCommandManager::Execmd( ConsoleCommandBase* pCommand )
	{
		SAT_CORE_ASSERT( pCommand );

		pCommand->Execute();
	}

	ConsoleCommandBase* ConsoleCommandManager::FindCommand( const std::string& rCommandName )
	{
		const auto itr = m_Commands.find( rCommandName );
		return itr == m_Commands.end() ? nullptr : itr->second;
	}

}
