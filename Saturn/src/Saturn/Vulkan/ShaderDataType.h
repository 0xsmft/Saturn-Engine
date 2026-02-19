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

#include <vulkan.h>

#include <stdint.h>

namespace Saturn {

	enum class ShaderDataType
	{
		None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool, Sampler2D, SamplerCube
	};
	
	// Vulkan shader type sizes
	static uint32_t ShaderDataTypeSize( ShaderDataType type )
	{
		switch( type )
		{
			case ShaderDataType::Float:		return 4;
			case ShaderDataType::Float2:	return 4 * 2;
			case ShaderDataType::Float3:	return 4 * 3;
			case ShaderDataType::Float4:	return 4 * 4;
			case ShaderDataType::Mat3:		return 4 * 3 * 3;
			case ShaderDataType::Mat4:		return 4 * 4 * 4;
			case ShaderDataType::Int:		return 4;
			case ShaderDataType::Int2:		return 4 * 2;
			case ShaderDataType::Int3:		return 4 * 3;
			case ShaderDataType::Int4:		return 4 * 4;
			case ShaderDataType::Bool:		return 1;
		}

		return 0;
	}

	static VkFormat ShaderDataTypeToVulkan( ShaderDataType Type )
	{
		switch( Type )
		{
			case ShaderDataType::Float:
				return VK_FORMAT_R32_SFLOAT;
				break;
			case ShaderDataType::Float2:
				return VK_FORMAT_R32G32_SFLOAT;
				break;
			case ShaderDataType::Float3:
				return VK_FORMAT_R32G32B32_SFLOAT;
				break;
			case ShaderDataType::Sampler2D:
			case ShaderDataType::Mat4:
			case ShaderDataType::Float4:
				return VK_FORMAT_R32G32B32A32_SFLOAT;
				break;
			case ShaderDataType::Bool:
			case ShaderDataType::Int:
				return VK_FORMAT_R32_SINT;
				break;
			case ShaderDataType::Int2:
				return VK_FORMAT_R32G32_SINT;
				break;
			case ShaderDataType::Int3:
				return VK_FORMAT_R32G32B32_SINT;
				break;
			case ShaderDataType::Int4:
				return VK_FORMAT_R32G32B32A32_SINT;
				break;
			case ShaderDataType::Mat3:
				return VK_FORMAT_R32G32B32_SFLOAT;
				break;
			default:
				break;
		}

		return VK_FORMAT_UNDEFINED;
	}

	static ShaderDataType VulkanToShaderDataType( VkFormat Type )
	{
		switch( Type )
		{
			case VK_FORMAT_R32_SFLOAT:
				return ShaderDataType::Float;
				break;
			case VK_FORMAT_R32G32_SFLOAT:
				return ShaderDataType::Float2;
				break;
			case VK_FORMAT_R32G32B32_SFLOAT:
				return ShaderDataType::Float3;
				break;
			case VK_FORMAT_R32G32B32A32_SFLOAT:
				return ShaderDataType::Float4;
				break;
			case VK_FORMAT_R32_SINT:
				return ShaderDataType::Int;
				break;
			case VK_FORMAT_R32G32_SINT:
				return ShaderDataType::Int2;
				break;
			case VK_FORMAT_R32G32B32_SINT:
				return ShaderDataType::Int3;
				break;
			case VK_FORMAT_R32G32B32A32_SINT:
				return ShaderDataType::Int4;
				break;
			case VK_FORMAT_R8_UNORM:
				return ShaderDataType::Bool;
				break;
			default:
				break;
		}
	}

	static ShaderDataType VulkanDescriptorToShaderDataType( VkDescriptorType Type )
	{
		switch( Type )
		{
			case VK_DESCRIPTOR_TYPE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				return ShaderDataType::Sampler2D;
		}

		return ShaderDataType::None;
	}

	static std::string ShaderDataTypeToString( ShaderDataType Type )
	{
		switch( Type )
		{
			case ShaderDataType::Float:		return "float";
			case ShaderDataType::Float2:	return "vec2";
			case ShaderDataType::Float4:	return "vec3";
			case ShaderDataType::Float3:	return "vec4";
			case ShaderDataType::Bool:		return "bool";
			case ShaderDataType::Int:		return "int";
			case ShaderDataType::Int2:		return "ivec2";
			case ShaderDataType::Int3:		return "ivec3";
			case ShaderDataType::Int4:		return "ivec4";
			case ShaderDataType::Mat3:		return "mat3";
			case ShaderDataType::Mat4:		return "mat4";
			case ShaderDataType::Sampler2D:	return "sampler2D";
			case ShaderDataType::None:
			default:						return "";
		}
	}
}