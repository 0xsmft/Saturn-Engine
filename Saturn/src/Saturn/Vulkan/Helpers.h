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

#include "VulkanContext.h"
#include "VulkanDebug.h"

#include <vulkan.h>
#include <vector>

namespace Saturn::Auxiliary {

	inline void VulkanCreateImage( 
		VkImageType Type, 
		VkFormat Format, 
		VkExtent3D Extent, 
		uint32_t ArrayLevels, 
		VkImageTiling Tiling,
		VkSampleCountFlagBits Samples, 
		VkImageUsageFlags Usage,
		
		VkImage* pImage, VkDeviceMemory* pMemory )
	{
		VkImageCreateInfo ImageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		ImageCreateInfo.imageType = Type;
		ImageCreateInfo.format = Format;
		ImageCreateInfo.extent = Extent;
		ImageCreateInfo.mipLevels = 1;
		ImageCreateInfo.arrayLayers = ArrayLevels;
		ImageCreateInfo.samples = Samples;
		ImageCreateInfo.tiling = Tiling;
		ImageCreateInfo.usage = Usage;

		VK_CHECK( vkCreateImage( VulkanContext::Get().GetDevice(), &ImageCreateInfo, nullptr, pImage ) );
#if defined(SAT_DEBUG)
		SetDebugUtilsObjectName( "Image (Saturn::Auxiliary)", ( uint64_t ) *pImage, VK_OBJECT_TYPE_IMAGE );
#else
		SetDebugUtilsObjectName( "Image", ( uint64_t ) *pImage, VK_OBJECT_TYPE_IMAGE );
#endif

		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements( VulkanContext::Get().GetDevice(), *pImage, &MemoryRequirements );

		VkMemoryAllocateInfo MemoryAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
		MemoryAllocateInfo.memoryTypeIndex = VulkanContext::Get().GetMemoryType( MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

		VK_CHECK( vkAllocateMemory( VulkanContext::Get().GetDevice(), &MemoryAllocateInfo, nullptr, pMemory ) );
		VK_CHECK( vkBindImageMemory( VulkanContext::Get().GetDevice(), *pImage, *pMemory, 0 ) );
	}
}