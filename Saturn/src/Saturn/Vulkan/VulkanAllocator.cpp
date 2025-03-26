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

#include "sppch.h"
#include "VulkanAllocator.h"
#include "Base.h"

#include "VulkanContext.h"

namespace Saturn {

	VulkanAllocator::VulkanAllocator()
	{
		// Create Allocator.
		VmaAllocatorCreateInfo AllocatorInfo = {};
		AllocatorInfo.physicalDevice = VulkanContext::Get().GetPhysicalDevice();
		AllocatorInfo.device = VulkanContext::Get().GetDevice();
		AllocatorInfo.instance = VulkanContext::Get().GetInstance();
		AllocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;

		VK_CHECK( vmaCreateAllocator( &AllocatorInfo, &m_Allocator ) );
	}

	VulkanAllocator::~VulkanAllocator()
	{		
		for ( auto& [ VulkanBuffer, Allocation ] : m_Allocations )
		{
			vmaDestroyBuffer( m_Allocator, VulkanBuffer, Allocation );
		}

		m_Allocations.clear();

		vmaDestroyAllocator( m_Allocator );
	}

	VmaAllocation VulkanAllocator::AllocateBuffer( VkBufferCreateInfo BufferInfo, VmaMemoryUsage MemoryUsage, VkBuffer* pBuffer )
	{
		VmaAllocation Allocation;

		VmaAllocationCreateInfo AllocationInfo = {};
		AllocationInfo.usage = MemoryUsage;
		
		VK_CHECK( vmaCreateBuffer( m_Allocator, &BufferInfo, &AllocationInfo, pBuffer, &Allocation, nullptr ) );

		m_Allocations[ *pBuffer ] = Allocation;

		return Allocation;
	}

	VmaAllocation VulkanAllocator::AllocateImage( VkImageCreateInfo ImageInfo, VmaMemoryUsage MemoryUsage, VkImage* pImage )
	{
		VmaAllocation Allocation;

		VmaAllocationCreateInfo AllocationInfo = {};
		AllocationInfo.usage = MemoryUsage;

		VK_CHECK( vmaCreateImage( m_Allocator, &ImageInfo, &AllocationInfo, pImage, &Allocation, nullptr ) );

		return Allocation;
	}
	
	void VulkanAllocator::DestroyBuffer( VkBuffer Buffer )
	{
		vmaDestroyBuffer( m_Allocator, Buffer, m_Allocations[ Buffer ] );
		m_Allocations.erase( Buffer );
	}

	void VulkanAllocator::DestroyImage( VmaAllocation Allocation, VkImage Image )
	{
		vmaDestroyImage( m_Allocator, Image, Allocation );
	}
}