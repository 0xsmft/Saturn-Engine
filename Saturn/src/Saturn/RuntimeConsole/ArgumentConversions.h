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

namespace Saturn::Auxiliary {

	template<typename Ty>
	Ty RtConsoleCommandArgConvert( const std::string& rStringVal );

	// Convert string to int
	template<>
	inline int RtConsoleCommandArgConvert<int>( const std::string& rStringVal ) 
	{
		return std::stoi( rStringVal );
	}

	// Convert string to float
	template<>
	inline float RtConsoleCommandArgConvert<float>( const std::string& rStringVal )
	{
		return std::stof( rStringVal );
	}

	// Convert string to uint64_t
	template<>
	inline uint64_t RtConsoleCommandArgConvert<uint64_t>( const std::string& rStringVal )
	{
		return std::stoull( rStringVal );
	}

	template<>
	inline std::string RtConsoleCommandArgConvert<std::string>( const std::string& rStringVal )
	{
		return rStringVal;
	}

	template<>
	inline glm::vec3 RtConsoleCommandArgConvert<glm::vec3>( const std::string& rStringVal )
	{
		glm::vec3 result{};

		std::stringstream ss( rStringVal );
		char spc;

		ss >> result.x >> spc >> result.y >> spc >> result.z;

		return result;
	}

	template<>
	inline glm::vec2 RtConsoleCommandArgConvert<glm::vec2>( const std::string& rStringVal )
	{
		glm::vec2 result{};

		std::stringstream ss( rStringVal );
		char spc;

		ss >> result.x >> spc >> result.y;

		return result;
	}
}
