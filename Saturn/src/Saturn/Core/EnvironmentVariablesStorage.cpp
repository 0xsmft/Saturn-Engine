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
#include "EnvironmentVariablesStorage.h"

#include "EnvironmentVariables.h"

#include "Saturn/Serialisation/YAML/EnvironmentVariablesSerialiser.h"

namespace Saturn {

	EnvironmentVariablesStorage::EnvironmentVariablesStorage()
	{
	}

	void EnvironmentVariablesStorage::TryLoadIfNeeded()
	{
		if( !m_IsLoaded )
			Deserialise();
	}

	EnvironmentVariablesStorage::~EnvironmentVariablesStorage()
	{
	}

	void EnvironmentVariablesStorage::SetVariable( const std::string& rKey, const std::filesystem::path& rPath )
	{
		const auto Itr = m_Variables.find( rKey );

		if( Itr == m_Variables.end() )
		{
			m_Variables.emplace( rKey, rPath );
		}
		else
		{
			m_Variables[ rKey ] = rPath;
		}

		// And we make sure to tell the system to set it as well.
		Auxiliary::System_SetEnvironmentVariable( rKey, rPath.string().c_str() );

		Serialise();
	}

	std::optional<std::filesystem::path> EnvironmentVariablesStorage::GetVariable( const std::string& rKey )
	{
		TryLoadIfNeeded();
		
		const auto Itr = m_Variables.find( rKey );
		
		if( Itr != m_Variables.end() )
		{
			return m_Variables[ rKey ];
		}
		else
		{
			// If we do not find it in our local list, let's check if the system has it
			// because this is a new feature the system may have the variable and we may not.
			const auto systemEnvVar = Auxiliary::System_GetEnvironmentVariable( rKey );
			if( systemEnvVar )
			{
				SAT_CORE_INFO( "[EnvironmentVariablesStorage]: Loaded variable \"{0}\" from system registry!", rKey );

				m_Variables[ rKey ] = *systemEnvVar;
				return *systemEnvVar;
			}
		}
		
		return std::nullopt;
	}
	
	void EnvironmentVariablesStorage::Serialise()
	{
		TryLoadIfNeeded();
		EnvironmentVariablesSerialiser::Serialise();
	}

	void EnvironmentVariablesStorage::Deserialise()
	{
		EnvironmentVariablesSerialiser::Deserialise();

#if defined(SAT_PLATFORM_MACOS)
		// On platforms that do not have persistent environment variables
		// we need to set them every time, we load them.
		for( const auto& [ rKey, rValue ] : m_Variables )
		{
			Auxiliary::System_SetEnvironmentVariable( rKey, rValue.string() );
		}
#endif

		m_IsLoaded = true;
	}

}
