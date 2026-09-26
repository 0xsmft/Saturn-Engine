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

namespace Saturn {

	//
	// EnvironmentVariablesStorage
	//
	// This class is responsible for managing
	// environment variables.
	//
	// Due to us being cross-platform I have decided against
	// using real OS environment variables and instead
	// we just hold a map of the environment variables.
	//
	// Environment variables are stored in a yaml file
	// called EnvironmentVariables.yaml
	// which can be found in the AppData folder.
	//
	class EnvironmentVariablesStorage 
	{
    public:
		SAT_SINGLETON_LAZY( EnvironmentVariablesStorage );
	public:
		EnvironmentVariablesStorage();
		~EnvironmentVariablesStorage();

		[[nodiscard]] bool DoesVariableExist( const std::string& rKey ) const;
		[[nodiscard]] void SetVariable( const std::string& rKey, const std::filesystem::path& rPath );

		//
		// Get an environment variable, returns nullopt if not found.
		//
		[[nodiscard]] std::optional<std::filesystem::path> GetVariable( const std::string& rKey );
		[[nodiscard]] const std::optional<std::filesystem::path> GetVariable( const std::string& rKey ) const;
	private:
		void Serialise();
		void Deserialise();
	
	private:
		//
		// I know not every env var is a path but in Saturn,
		// we only use environment variables to point to paths
		// so this will be fine for us.
		//
		std::unordered_map<std::string, std::filesystem::path> m_Variables;

	private:
		friend class EnvironmentVariablesSerialiser;
	};

}
