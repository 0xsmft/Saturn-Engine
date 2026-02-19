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
#include <vma/vk_mem_alloc.h>

namespace Saturn {
	
	class VulkanAllocator
	{
	public:
		VulkanAllocator();
		~VulkanAllocator();
		
		// Allocate buffer
		VmaAllocation AllocateBuffer( VkBufferCreateInfo BufferInfo, VmaMemoryUsage MemoryUsage, VkBuffer* pBuffer );

		// Allocate image
		VmaAllocation AllocateImage( VkImageCreateInfo ImageInfo, VmaMemoryUsage MemoryUsage, VkImage* pImage );

		// Destroy buffer
		void DestroyBuffer( VkBuffer Buffer );
		
		// Destroy image
		void DestroyImage( VmaAllocation Allocation, VkImage Image );
	
		template<typename Ty>
		Ty* MapMemory( VmaAllocation Allocation )
		{
			Ty* pData = nullptr;
			
			vmaMapMemory( m_Allocator, Allocation, (void**)&pData );
			
			return pData;
		}

		void UnmapMemory( VmaAllocation Allocation )
		{
			vmaUnmapMemory( m_Allocator, Allocation );
		}

		VmaAllocation GetAllocationFromBuffer( VkBuffer Buffer ) { return m_Allocations[ Buffer ]; }

	private:
		VmaAllocator m_Allocator = VK_NULL_HANDLE;

		std::unordered_map< VkBuffer, VmaAllocation > m_Allocations;
	};
}
