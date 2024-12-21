/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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
#include "Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Saturn {

	Log::Log()
	{
		SingletonStorage::AddSingleton( this );

		Init();
	}

	Log::~Log()
	{
		Clear();
		
		SingletonStorage::RemoveSingleton( this );
	}

	void Log::Init( void )
	{
		SingletonStorage::AddSingleton( this );

		spdlog::set_automatic_registration( true );

		std::vector<spdlog::sink_ptr> logSinks;
		logSinks.emplace_back( std::make_shared< spdlog::sinks::stdout_color_sink_mt >() );
#if defined(SAT_DIST)
		logSinks.emplace_back( std::make_shared< spdlog::sinks::basic_file_sink_mt >( "Application-Dist-x64.log" ) );
#endif

		for( size_t i = 0; i < logSinks.size(); i++ )
		{
			logSinks[ i ]->set_pattern( "%^[%T] %n: %v%$" );
		}

		m_CoreLogger = std::make_shared< spdlog::logger >( "Saturn", begin( logSinks ), end( logSinks ) );
		m_ClientLogger = std::make_shared< spdlog::logger >( "App", begin( logSinks ), end( logSinks ) );

		// configure the loggers
		spdlog::set_pattern( "%^[%T] %n: %v%$" );
		m_CoreLogger->set_level( spdlog::level::trace );
		m_ClientLogger->set_level( spdlog::level::trace );
	}

	void Log::Clear( void )
	{
		m_CoreLogger = nullptr;
		m_ClientLogger = nullptr;

		spdlog::shutdown();
	}

}