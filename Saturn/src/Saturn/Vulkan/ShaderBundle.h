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

#pragma once

#include "Saturn/Core/Ref.h"
#include "SingletonStorage.h"

namespace Saturn {

	enum class ShaderBundleResult 
	{
		Success,
		FileNotFound,
		InvalidShaderHeader,
		Failed
	};

	class ShaderBundle
	{
	public:
		[[nodiscard]] static ShaderBundleResult BundleShaders();
		[[nodiscard]] static ShaderBundleResult ReadBundle();
	};
}

namespace std {

	template<>
	struct formatter<Saturn::ShaderBundleResult>
	{
		constexpr auto parse( format_parse_context& ctx )
		{
			return ctx.begin();
		}

		template<typename FormatContext>
		auto format( Saturn::ShaderBundleResult result, FormatContext& ctx ) const
		{
			std::string_view errorName = "Unknown Error";

			switch( result )
			{
				default:
				case Saturn::ShaderBundleResult::Success:             errorName = "Success"; break;
				case Saturn::ShaderBundleResult::FileNotFound:        errorName = "Shader Bundle file not found"; break;
				case Saturn::ShaderBundleResult::InvalidShaderHeader: errorName = "Invalid shader header"; break;
				case Saturn::ShaderBundleResult::Failed:              errorName = "Failed, internal error"; break;
			}

			return std::format_to( ctx.out(), "{}", errorName );
		}
	};

}
