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

#include "Image2D.h"

namespace Saturn {

	static VkFormat VulkanFormat( ImageFormat format )
	{
		switch( format )
		{
			case Saturn::ImageFormat::RGBA8:
				return VK_FORMAT_R8G8B8A8_UNORM;

			case Saturn::ImageFormat::RGBA16:
				return VK_FORMAT_R16G16B16A16_UNORM;

			case Saturn::ImageFormat::RGBA32F:
				return VK_FORMAT_R32G32B32A32_SFLOAT;

			case Saturn::ImageFormat::RGB32F:
				return VK_FORMAT_R32G32B32_SFLOAT;

			case ImageFormat::BGRA8:
				return VK_FORMAT_B8G8R8A8_UNORM;

			case ImageFormat::RGB8:
				return VK_FORMAT_R8G8B8_UNORM;

			case ImageFormat::RG8:
				return VK_FORMAT_R8G8_UNORM;

			case ImageFormat::RED8:
				return VK_FORMAT_R8_UNORM;

			case ImageFormat::RED32F:
				return VK_FORMAT_R32_SFLOAT;

			case Saturn::ImageFormat::DEPTH24STENCIL8:
				return VK_FORMAT_D32_SFLOAT_S8_UINT;
			case Saturn::ImageFormat::DEPTH32F:
				return VK_FORMAT_D32_SFLOAT;
		}

		SAT_CORE_ASSERT( false );
		return VK_FORMAT_UNDEFINED;
	}

	static ImageFormat SaturnFormat( VkFormat format )
	{
		switch( format )
		{
			case VK_FORMAT_R8G8B8A8_UNORM:
				return ImageFormat::BGRA8;

			case VK_FORMAT_R16G16B16A16_UNORM:
				return ImageFormat::RGBA16;

			case VK_FORMAT_R32G32B32A32_SFLOAT:
				return ImageFormat::RGBA32F;

			case VK_FORMAT_B8G8R8A8_UNORM:
				return ImageFormat::BGRA8;

			case VK_FORMAT_R32G32B32_SFLOAT:
				return ImageFormat::RGB32F;

			case VK_FORMAT_R8_UNORM:
				return ImageFormat::RED8;

			case VK_FORMAT_R8G8B8_UNORM:
				return ImageFormat::RGB8;

			case VK_FORMAT_R8G8_UNORM:
				return ImageFormat::RGB8;

			case VK_FORMAT_R32_SFLOAT:
				return ImageFormat::RED32F;

			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				return ImageFormat::DEPTH24STENCIL8;

			case VK_FORMAT_D32_SFLOAT:
				return ImageFormat::DEPTH32F;
		}

		SAT_CORE_ASSERT( false );
		return ImageFormat::None;
	}

	static bool IsColorFormat( ImageFormat format )
	{
		switch( format )
		{
			case Saturn::ImageFormat::DEPTH24STENCIL8:
			case Saturn::ImageFormat::DEPTH32F:
				return false;
		}

		return true;
	}

	static bool IsColorFormat( VkFormat format )
	{
		switch( format )
		{
			case VK_FORMAT_D32_SFLOAT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				return false;
		}

		return true;
	}

	static bool IsDepthFormat( ImageFormat format )
	{
		switch( format )
		{
			case Saturn::ImageFormat::DEPTH32F:
			case Saturn::ImageFormat::DEPTH24STENCIL8:
				return true;
		}

		return false;
	}

	static void TransitionImageLayout( VkCommandBuffer CommandBuffer, 
		VkImage Image, 
		VkImageLayout OldLayout, VkImageLayout NewLayout, 
		const VkImageSubresourceRange& rSubresourceRange, 
		VkPipelineStageFlags SrcStage, VkPipelineStageFlags DstStage )
	{
		VkImageMemoryBarrier ImageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		ImageBarrier.oldLayout = OldLayout;
		ImageBarrier.newLayout = NewLayout;
		ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.image = Image;
		ImageBarrier.subresourceRange = rSubresourceRange;

		vkCmdPipelineBarrier( CommandBuffer, 
			SrcStage, 
			DstStage, 
			0, 
			0, nullptr, 
			0, nullptr, 
			1, &ImageBarrier );
	}
}
