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

	bool EnvironmentVariablesStorage::DoesVariableExist( const std::string &rKey )
	{
        TryLoadIfNeeded();
		return m_Variables.contains( rKey );
	}

	void EnvironmentVariablesStorage::SetVariable( const std::string &rKey, const std::filesystem::path &rPath )
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

		Serialise();
	}

	std::optional<std::filesystem::path> EnvironmentVariablesStorage::GetVariable( const std::string &rKey )
	{
        TryLoadIfNeeded();
        
		const auto Itr = m_Variables.find( rKey );
		
		if( Itr != m_Variables.end() )
		{
			return m_Variables[ rKey ];
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
        m_IsLoaded = true;
	}

}
