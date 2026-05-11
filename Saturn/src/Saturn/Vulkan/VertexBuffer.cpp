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
#include "VertexBuffer.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"

#include <cassert>

namespace Saturn {

	VertexBuffer::VertexBuffer( void* pData, VkDeviceSize Size, VkBufferUsageFlags Usage /*= 0 */ )
		: m_pData( pData )
	{
		m_Size = Size;

		CreateBuffer();
	}

	VertexBuffer::VertexBuffer( VkDeviceSize Size, VkBufferUsageFlags Usage /*= 0 */ )
	{
		m_Size = Size;
		m_pData = nullptr;

		auto pAllocator = VulkanContext::Get()->GetVulkanAllocator();

		// Create the vertex buffer.
		VkBufferCreateInfo VertexBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		VertexBufferCreateInfo.size = Size;
		VertexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		m_Allocation = pAllocator->AllocateBuffer( VertexBufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, &m_Buffer );
	
		// Always set a name regardless if we are dist or not!
		SetDebugUtilsObjectName( "Vertex Buffer", ( uint64_t ) m_Buffer, VK_OBJECT_TYPE_BUFFER );
	}

	VertexBuffer::~VertexBuffer()
	{
		Destroy();
	}

	void VertexBuffer::BindAndDraw( VkCommandBuffer CommandBuffer )
	{
		Bind( CommandBuffer );

		vkCmdDraw( CommandBuffer, ( uint32_t )m_Size, 1, 0, 0 );
	}
	
	void VertexBuffer::Bind( VkCommandBuffer CommandBuffer, uint32_t binding )
	{
		VkDeviceSize Offsets[] ={ 0 };

		vkCmdBindVertexBuffers( CommandBuffer, binding, 1, &m_Buffer, Offsets );
	}

	void VertexBuffer::Bind( VkCommandBuffer CommandBuffer, uint32_t binding, VkDeviceSize* Offsets )
	{
		vkCmdBindVertexBuffers( CommandBuffer, binding, 1, &m_Buffer, Offsets );
	}

	void VertexBuffer::SetData( void* pData, uint32_t size, uint32_t offset /*= 0 */ )
	{
		auto pAllocator = VulkanContext::Get()->GetVulkanAllocator();

		void* dstData = pAllocator->MapMemory< void >( m_Allocation );

		std::memcpy( dstData, (uint8_t*)pData + offset, size );
		m_pData = dstData;

		pAllocator->UnmapMemory( m_Allocation );
	}

	void VertexBuffer::Draw( VkCommandBuffer CommandBuffer )
	{
		vkCmdDraw( CommandBuffer, ( uint32_t )m_Size, 1, 0, 0 );
	}

	void VertexBuffer::CreateBuffer()
	{
		assert( m_Size >= 3 && "Vertex count must be above 3!" );
		
		VkDeviceSize BufferSize = m_Size;

		VkBuffer StagingBuffer;
		
		auto pAllocator = VulkanContext::Get()->GetVulkanAllocator();
		
		VkBufferCreateInfo BufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		BufferCreateInfo.size = BufferSize;
		BufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		auto rBufferAlloc = pAllocator->AllocateBuffer( BufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, &StagingBuffer );
		
		void* dstData = pAllocator->MapMemory< void >( rBufferAlloc );

		memcpy( dstData, m_pData, BufferSize );

		pAllocator->UnmapMemory( rBufferAlloc );

		//////////////////////////////////////////////////////////////////////////
		
		// Create the vertex buffer.
		VkBufferCreateInfo VertexBufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		VertexBufferCreateInfo.size = BufferSize;
		VertexBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		
		m_Allocation = pAllocator->AllocateBuffer( VertexBufferCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, &m_Buffer );
		 
		// Always set a name regardless if we are dist or not! 
		SetDebugUtilsObjectName( "Vertex Buffer", ( uint64_t ) m_Buffer, VK_OBJECT_TYPE_BUFFER );

		// Copy buffer
		{
			auto CommandBuffer = VulkanContext::Get()->BeginSingleTimeCommands();

			VkBufferCopy CopyRegion{};
			CopyRegion.size = BufferSize;

			vkCmdCopyBuffer( CommandBuffer, StagingBuffer, m_Buffer, 1, &CopyRegion );

			VulkanContext::Get()->EndSingleTimeCommands( CommandBuffer );
		}

		pAllocator->DestroyBuffer( StagingBuffer );
	}

	void VertexBuffer::Destroy()
	{
		if ( m_Buffer != nullptr )
			VulkanContext::Get()->GetVulkanAllocator()->DestroyBuffer( m_Buffer );
		
		m_Buffer = nullptr;
	}

#if !defined(SAT_DIST)
	void VertexBuffer::SetDebugName( const std::string& rName )
	{
		SetDebugUtilsObjectName( rName.c_str(), ( uint64_t ) m_Buffer, VK_OBJECT_TYPE_BUFFER );
	}
#endif

	//////////////////////////////////////////////////////////////////////////
}
