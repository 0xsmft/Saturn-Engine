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

#include "Base.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Saturn {

	class Log : RefTarget
	{
	public:
		SAT_SINGLETON_LAZY( Log )

	public:
		Log();
		~Log();

		void Init();
		void Clear();

		std::shared_ptr<spdlog::logger>& CoreLogger() { return m_CoreLogger; }
		std::shared_ptr<spdlog::logger>& ClientLogger() { return m_ClientLogger; }

	private:
		std::shared_ptr<spdlog::logger> m_CoreLogger;
		std::shared_ptr<spdlog::logger> m_ClientLogger;
	};

	template<typename T>
	static void LogInfo( const T& msg )
	{
		Log::Get().CoreLogger()->info( msg );
	}

	template<typename T>
	static void LogTrace( const T& msg )
	{
		Log::Get().CoreLogger()->trace( msg );
	}

	template<typename T>
	static void LogWarn( const T& msg )
	{
		Log::Get().CoreLogger()->warn( msg );
	}

	template<typename T>
	static void LogError( const T& msg )
	{
		Log::Get().CoreLogger()->error( msg );
	}

	template<typename T>
	static void LogFatal( const T& msg )
	{
		Log::Get().CoreLogger()->critical( msg );
	}
}

template<typename OStream>
OStream& operator<<( OStream& os, const glm::vec2& vec )
{
	return os << '(' << vec.x << ", " << vec.y << ')';
}

template<typename OStream>
OStream& operator<<( OStream& os, const glm::vec3& vec )
{
	return os << '(' << vec.x << ", " << vec.y << ", " << vec.z << ')';
}

template<typename OStream>
OStream& operator<<( OStream& os, const glm::quat& vec )
{
	return os << '(' << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ')';
}

template<typename OStream>
OStream& operator<<( OStream& os, glm::vec4& vec )
{
	return os << '(' << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ')';
}

template<typename OStream>
OStream& operator<<( OStream& os, glm::mat4& matrix )
{
	return os << matrix[ 0 ] << matrix[ 1 ] << matrix[ 2 ] << matrix[ 3 ];
}

template<typename OStream>
OStream& operator<<( OStream& os, const glm::vec4& vec )
{
	return os << '(' << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ')';
}

template<typename OStream>
OStream& operator<<( OStream& os, const glm::mat4& matrix )
{
	return os << matrix[ 0 ] << matrix[ 1 ] << matrix[ 2 ] << matrix[ 3 ];
}

// Core log macros
#define SAT_CORE_TRACE(...)				Saturn::Log::Get().CoreLogger()->trace(__VA_ARGS__)
#define SAT_CORE_INFO(...)				Saturn::Log::Get().CoreLogger()->info(__VA_ARGS__)
#define SAT_CORE_WARN(...)				Saturn::Log::Get().CoreLogger()->warn(__VA_ARGS__)
#define SAT_CORE_ERROR(...)				Saturn::Log::Get().CoreLogger()->error(__VA_ARGS__)
#define SAT_CORE_FATAL(...)				Saturn::Log::Get().CoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define SAT_TRACE(...)					//Saturn::Log::Get().ClientLogger()->trace(__VA_ARGS__)
#define SAT_INFO(...)					//Saturn::Log::Get().ClientLogger()->info(__VA_ARGS__)
#define SAT_WARN(...)					//Saturn::Log::Get().ClientLogger()->warn(__VA_ARGS__)
#define SAT_ERROR(...)					//Saturn::Log::Get().ClientLogger()->error(__VA_ARGS__)
#define SAT_FATAL(...)					//Saturn::Log::Get().ClientLogger()->critical(__VA_ARGS__)
