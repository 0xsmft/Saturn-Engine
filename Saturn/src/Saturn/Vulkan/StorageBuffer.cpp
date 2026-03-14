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

#include "sppch.h"
#include "StorageBuffer.h"

#include "VulkanAllocator.h"
#include "VulkanContext.h"

namespace Saturn {

	StorageBuffer::StorageBuffer( uint32_t set, uint32_t binding, bool gpuOnly )
		: m_Set( set ), m_Binding( binding ), m_GPUOnly( gpuOnly )
	{
		Create();
	}

	StorageBuffer::~StorageBuffer()
	{
	}

	void StorageBuffer::Create()
	{
		// Create the buffer however leave the size at zero.
		VkBufferCreateInfo BufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		BufferInfo.size = 1;
		BufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		auto pAllocator = VulkanContext::Get()->GetVulkanAllocator();
		pAllocator->AllocateBuffer( BufferInfo, m_GPUOnly ? VMA_MEMORY_USAGE_GPU_ONLY : VMA_MEMORY_USAGE_CPU_TO_GPU, &m_Buffer );
	}

	void StorageBuffer::Resize( uint32_t newSize )
	{
		m_Size = newSize;

		auto pAllocator = VulkanContext::Get()->GetVulkanAllocator();

		pAllocator->DestroyBuffer( m_Buffer );

		VkBufferCreateInfo BufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		BufferCreateInfo.size = newSize;
		BufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		pAllocator->AllocateBuffer( BufferCreateInfo, m_GPUOnly ? VMA_MEMORY_USAGE_GPU_ONLY : VMA_MEMORY_USAGE_CPU_TO_GPU, &m_Buffer );

		m_BufferInfo.buffer = m_Buffer;
		m_BufferInfo.range = m_Size;
	}
}
