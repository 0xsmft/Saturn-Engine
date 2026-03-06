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

#include "Saturn/GameFramework/SProperty.h"

#include <filesystem>
#include <map>

namespace Saturn {

	enum class HeaderToolConfigKind
	{
		Debug,
		Release,
		Dist,
		Unknown
	};

	struct GClass
	{
		std::string ClassName;
		std::string BaseClass;
		uint64_t ClassFlags = 0 /* SClassFlags::None */;
		uint32_t LineNumberForGeneratedBody = 0;

		std::map<uint32_t, SProperty> Properties;
	};

	struct HeaderToolCommand
	{
		HeaderToolCommand( const std::filesystem::path& rFilepath, HeaderToolConfigKind config ) 
			: Filepath( rFilepath ), ConfigKind( config )
		{
		}

		std::filesystem::path Filepath;
		HeaderToolConfigKind ConfigKind;

#if SAT_HT_VER > 5
		std::string ClassName;
		std::string BaseClass;
		uint32_t ClassFlags = 0 /* SClassFlags::None */;
		uint32_t LineNumberForGeneratedBody = 0;

		std::map<uint32_t, SProperty> Properties;
#else
		//				HASH ->		CLASS
		std::unordered_map<uint64_t, GClass> GClassInfos;
#endif
	};
}
