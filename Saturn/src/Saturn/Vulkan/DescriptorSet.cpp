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
#include "DescriptorSet.h"

#include "VulkanContext.h"

namespace Saturn {

	DescriptorPool::DescriptorPool( std::vector< VkDescriptorPoolSize > PoolSizes, uint32_t MaxSets )
	{
		VkDescriptorPoolCreateInfo PoolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		PoolCreateInfo.poolSizeCount = (uint32_t)PoolSizes.size();
		PoolCreateInfo.pPoolSizes = PoolSizes.data();
		PoolCreateInfo.maxSets = MaxSets;
		PoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VK_CHECK( vkCreateDescriptorPool( VulkanContext::Get()->GetDevice(), &PoolCreateInfo, nullptr, &m_Pool ) );
	}

	DescriptorPool::~DescriptorPool()
	{
		Terminate();
	}

	void DescriptorPool::Terminate()
	{
		if( m_Pool )
			vkDestroyDescriptorPool( VulkanContext::Get()->GetDevice(), m_Pool, nullptr );

		m_Pool = nullptr;
	}

	//////////////////////////////////////////////////////////////////////////

	DescriptorSet::DescriptorSet( DescriptorSetSpecification Spec )
		: m_Specification( Spec )
	{
		Allocate();
	}

	DescriptorSet::~DescriptorSet()
	{
		Terminate();
	}

	void DescriptorSet::Terminate()
	{
		if( m_Set )
			vkFreeDescriptorSets( VulkanContext::Get()->GetDevice(), *m_Specification.Pool.Get(), 1, &m_Set );

		m_Set = nullptr;
		m_Specification = {};
	}

	void DescriptorSet::Write( VkDescriptorBufferInfo BufferInfo, VkDescriptorImageInfo ImageInfo )
	{
		VkWriteDescriptorSet WriteDescriptorSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		WriteDescriptorSet.dstSet = m_Set;
		WriteDescriptorSet.dstBinding = 0;
		WriteDescriptorSet.dstArrayElement = 0;
		WriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		WriteDescriptorSet.descriptorCount = 1;
		
		if( BufferInfo.buffer != VK_NULL_HANDLE )
			WriteDescriptorSet.pBufferInfo = &BufferInfo;
		else
			WriteDescriptorSet.pBufferInfo = nullptr;

		if( ImageInfo.imageView != VK_NULL_HANDLE )
			WriteDescriptorSet.pImageInfo = &ImageInfo;
		else
			WriteDescriptorSet.pImageInfo = nullptr;

		vkUpdateDescriptorSets( VulkanContext::Get()->GetDevice(), 1, &WriteDescriptorSet, 0, nullptr );
	}

	void DescriptorSet::Write( std::vector< VkWriteDescriptorSet > WriteDescriptorSets )
	{
		vkUpdateDescriptorSets( VulkanContext::Get()->GetDevice(), (uint32_t)WriteDescriptorSets.size(), WriteDescriptorSets.data(), 0, nullptr );
	}

	void DescriptorSet::Bind( VkCommandBuffer CommandBuffer, VkPipelineLayout PipelineLayout )
	{
		vkCmdBindDescriptorSets( CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0, 1, &m_Set, 0, nullptr );
	}

	void DescriptorSet::Allocate()
	{
		VkDescriptorSetAllocateInfo AllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		AllocateInfo.descriptorPool = *m_Specification.Pool.Get();
		AllocateInfo.descriptorSetCount = 1;
		AllocateInfo.pSetLayouts = &m_Specification.Layout;
		
		VK_CHECK( vkAllocateDescriptorSets( VulkanContext::Get()->GetDevice(), &AllocateInfo, &m_Set ) );
	}
}