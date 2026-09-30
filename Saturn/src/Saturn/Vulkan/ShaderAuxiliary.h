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

namespace Saturn::Auxiliary {

#if defined(SAT_PLATFORM_WINDOWS)
	static constexpr std::string_view s_ShaderLineEndingToken = "\r\n";
	static constexpr size_t s_ShaderLineEndingTokenSize = s_ShaderLineEndingToken.size();
#else
	static constexpr std::string_view ShaderLineEndingToken = "\n";
	static constexpr size_t s_ShaderLineEndingTokenSize = s_ShaderLineEndingToken.size();
#endif

	static ShaderType VulkanStageToSaturn( VkShaderStageFlags Flags )
	{
		switch( Flags )
		{
			case VK_SHADER_STAGE_VERTEX_BIT:
				return ShaderType::Vertex;
			case VK_SHADER_STAGE_FRAGMENT_BIT:
				return ShaderType::Fragment;
			case VK_SHADER_STAGE_GEOMETRY_BIT:
				return ShaderType::Geometry;
			case VK_SHADER_STAGE_COMPUTE_BIT:
				return ShaderType::Compute;
			default:
				return ShaderType::All;
		}
	}

	static VkShaderStageFlags ShaderTypeToVulkan( ShaderType type )
	{
		switch( type )
		{
			case Saturn::ShaderType::None:
				return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			case Saturn::ShaderType::Vertex:
				return VK_SHADER_STAGE_VERTEX_BIT;
			case Saturn::ShaderType::Fragment:
				return VK_SHADER_STAGE_FRAGMENT_BIT;
			case Saturn::ShaderType::Geometry:
				return VK_SHADER_STAGE_GEOMETRY_BIT;
			case Saturn::ShaderType::Compute:
				return VK_SHADER_STAGE_COMPUTE_BIT;
			case Saturn::ShaderType::All:
				return VK_SHADER_STAGE_ALL;
		}

		return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	}
	
	static ShaderType ShaderTypeFromString( const std::string& rTypeString )
	{
		if( rTypeString == "vertex" )
		{
			return ShaderType::Vertex;
		}
		else if( rTypeString == "fragment" )
		{
			return ShaderType::Fragment;
		}
		else if( rTypeString == "compute" )
		{
			return ShaderType::Compute;
		}
		else if( rTypeString == "geometry" )
		{
			return ShaderType::Geometry;
		}
		else if( rTypeString == "header" )
		{
			return ShaderType::Resource;
		}
		else
		{
			return ShaderType::None;
		}
	}

	static std::string ShaderTypeToString( ShaderType Type )
	{
		switch( Type )
		{
			case Saturn::ShaderType::Vertex:
				return "Vertex";
			case Saturn::ShaderType::Fragment:
				return "Fragment";
			case Saturn::ShaderType::Geometry:
				return "Geometry";
			case Saturn::ShaderType::Compute:
				return "Compute";
			case Saturn::ShaderType::Resource:
				return "Resource";
			default:
				break;
		}

		return "";
	}

	static shaderc_shader_kind SaturnShaderTypeToShaderc( ShaderType type )
	{
		switch( type )
		{
			case ShaderType::Vertex:
				return shaderc_glsl_default_vertex_shader;

			case ShaderType::Fragment:
				return shaderc_glsl_default_fragment_shader;

			case ShaderType::Geometry:
				return shaderc_glsl_default_geometry_shader;

			case ShaderType::Compute:
				return shaderc_glsl_default_compute_shader;

			case ShaderType::None:
			case ShaderType::Resource:
			case ShaderType::All:
			default:
				break;
		}

		SAT_CORE_ASSERT( false, "Invalid type passed into SaturnShaderTypeToShaderc!" );
		return ( shaderc_shader_kind ) -1;
	}

}
