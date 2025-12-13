/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2025 BEAST                                                           *
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
#include "GameModule.h"

#include "ClassMetadataHandler.h"

#include "Saturn/Scene/Entity.h"
#include "Saturn/Project/Project.h"
#include "Saturn/Core/Profiler.h"

namespace Saturn {

	GameModule::GameModule()
	{
		SingletonStorage::AddSingleton( this );

		LoadModule();
	}

	GameModule::~GameModule()
	{
		Unload();
	}

#if defined(SAT_DEBUG) || defined(SAT_RELEASE)

	void GameModule::BeginHotReload()
	{
		m_ModuleHandle = nullptr;
	}

	void GameModule::EndHotReload()
	{
		LoadModule( true );
	}

#endif

	void GameModule::LoadModule( bool wasHotReloaded /*=false*/ )
	{
#if !defined(SAT_DIST)
		// We are the editor, load game DLL.
		const auto binDir = Project::GetActiveProject()->GetBinDir();
		
		std::filesystem::path timestampFile = binDir / "Timestamp";

		if( wasHotReloaded )
		{
			timestampFile = binDir / "Timestamp.hot";
		}

		if( std::filesystem::exists( timestampFile ) )
		{
			std::ifstream stream( timestampFile );
			std::stringstream buffer;
			buffer << stream.rdbuf();
			stream.close();

			m_LastTimestamp = buffer.str();

			// If /HOTRELOAD was suggested when this module was compiled, the Build Tool will output
			// {ProjectName}_{BuildTimestamp}.dll instead of {ProjectName}.dll
			std::string dllFilename;
			if( wasHotReloaded )
			{
				dllFilename = std::format( "Saturn-{0}_{1}.dll", Project::GetActiveConfig().Name, m_LastTimestamp );
			}
			else
				dllFilename = std::format( "Saturn-{0}.dll", Project::GetActiveConfig().Name );

			const auto DllPath = binDir / dllFilename;

			//////////////////////////////////////////////////////////////////////////

			m_ModuleHandle = Ref<Module>::Create( DllPath, Project::GetActiveConfig().Name );
			m_ModuleHandle->Load();

			//////////////////////////////////////////////////////////////////////////

			// Find the init function.
			InitModuleFn initModFn = ( InitModuleFn ) m_ModuleHandle->m_Library.GetSymbol( "InitializeModule" );

			SAT_CORE_VERIFY( initModFn, "Failed to find \"InitializeModule\" function in Game Module!" );

			( initModFn ) ( Project::GetActiveProject().Get(), tracy::GetProfilerDataPtr() );
		}
		else
			SAT_CORE_WARN( "Timestamp file does not exists! Please rebuild the game in your IDE." );
#endif
	}

	void GameModule::Unload()
	{
#if !defined(SAT_DIST)
		m_ModuleHandle = nullptr;
#endif
	}

	void GameModule::Reload() 
	{
#if !defined(SAT_DIST)
		Unload();

		LoadModule();
#endif
	}
}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( GameModule );
