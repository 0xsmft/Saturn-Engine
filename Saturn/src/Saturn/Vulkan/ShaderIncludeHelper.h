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

#include <shaderc/shaderc.hpp>

namespace Saturn {

	class ShaderIncludeHelper : public shaderc::CompileOptions::IncluderInterface
	{
	public:
		ShaderIncludeHelper();
		virtual ~ShaderIncludeHelper();

		virtual shaderc_include_result* GetInclude( const char* pRequestedPath, shaderc_include_type type, const char* pInstigatorPath, size_t depth ) override;

		virtual void ReleaseInclude( shaderc_include_result* pData ) override;

		void AddIncludeDirectory( const std::filesystem::path& rPath )
		{
			m_IncludeDirectories.emplace_back( rPath );
		}

	private:
		std::filesystem::path ResolveInclude( const std::filesystem::path& rIncludeDir );

	private:
		std::vector<std::filesystem::path> m_IncludeDirectories;
	};
	
}
