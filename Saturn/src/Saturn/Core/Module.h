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

#include "Ref.h"
#include "DynamicLinkLibrary.h"
#include "Log.h"

#include <filesystem>
#include <unordered_map>

namespace Saturn {
	
	class Project;
	class Scene;

	// This class is really only here for one reason:
	// We need to way to set globals in the Game DLL, we have solved this problem by creating "Shared Storage" however, for some globals they do not change or are tied to the lifetime of the game.
	// TODO: I want this class to be only created in the game and to hold function pointers when we want to create a class.
	class Module : public RefTarget
	{
	public:
		Module( const std::filesystem::path& rPath, const std::string& rName );
		~Module();

		void Load();
		
		// Where Ty, must be a valid function pointer type
		template<typename Ty>
		Ty GetOrFindFunction( const std::string& rName )
		{
			auto result = m_Library.GetSymbol( rName.c_str() );
			if( result )
			{
				return reinterpret_cast<Ty>( result );
			}
			else
				SAT_CORE_ERROR( "Could not find funtion {0} looking in module DLL: {1}", rName, m_Name );

			return nullptr;
		}

	private:
		void Terminate();

	private:
		DynamicLinkLibrary m_Library;

		std::filesystem::path m_Path;
		std::string m_Name;

	private:
		friend class GameModule;
	};

	// Default Module registration function.
	typedef void ( __stdcall* InitModuleFn )( Project* /* active project */, const void* /* tracy profiler data */ );
}
