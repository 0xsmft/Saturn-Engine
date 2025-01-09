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
#include "IndexBuffer.h"

#include "VulkanContext.h"

#include "VulkanDebug.h"

#include <cassert>

namespace Saturn {

	IndexBuffer::IndexBuffer( void* pData, size_t Size )
	{
		m_pData = pData;
		m_Size = Size;

		CreateBuffer();
	}

	IndexBuffer::~IndexBuffer()
	{
		Destroy();
	}

	void IndexBuffer::Bind( VkCommandBuffer CommandBuffer )
	{
		vkCmdBindIndexBuffer( CommandBuffer, m_Buffer, 0, VK_INDEX_TYPE_UINT32 );
	}

	void IndexBuffer::Draw( VkCommandBuffer CommandBuffer )
	{
		vkCmdDrawIndexed( CommandBuffer, (uint32_t)m_Size / sizeof(uint32_t), 1, 0, 0, 0 );
	}

	void IndexBuffer::CreateBuffer()
	{
		assert( m_Size >= 3 && "Index count must be above 3!" );

		uint32_t BufferSize = (uint32_t)m_Size;

		VkBuffer StagingBuffer;
		
		auto pAllocator = VulkanContext::Get().GetVulkanAllocator();

		VkBufferCreateInfo BufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		BufferCreateInfo.size = BufferSize;
		BufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		auto rBufferAlloc = pAllocator->AllocateBuffer( BufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, &StagingBuffer );

		uint8_t* dstData = pAllocator->MapMemory< uint8_t >( rBufferAlloc );

		memcpy( dstData, m_pData, BufferSize );

		pAllocator->UnmapMemory( rBufferAlloc );

		//////////////////////////////////////////////////////////////////////////

		// Create the vertex buffer.
		VkBufferCreateInfo IndexBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		IndexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		IndexBufferCreateInfo.size = BufferSize;

		m_Allocation = pAllocator->AllocateBuffer( IndexBufferCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, &m_Buffer );
		
		SetDebugUtilsObjectName( "Index Buffer", ( uint64_t ) m_Buffer, VK_OBJECT_TYPE_BUFFER );

		// Copy buffer
		{
			auto CommandBuffer = VulkanContext::Get().BeginSingleTimeCommands();

			VkBufferCopy CopyRegion{};
			CopyRegion.size = BufferSize;

			vkCmdCopyBuffer( CommandBuffer, StagingBuffer, m_Buffer, 1, &CopyRegion );

			VulkanContext::Get().EndSingleTimeCommands( CommandBuffer );
		}

		pAllocator->DestroyBuffer( StagingBuffer );
	}

	void IndexBuffer::Destroy()
	{
		if( m_Buffer != nullptr )
			VulkanContext::Get().GetVulkanAllocator()->DestroyBuffer( m_Buffer );

		m_Buffer = nullptr;
	}

}