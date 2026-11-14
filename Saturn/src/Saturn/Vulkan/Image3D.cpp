/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 0xsmft https://0xsmft.dev                                       *
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
#include "Image3D.h"

#include "VulkanAllocator.h"
#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "VulkanImageAux.h"

namespace Saturn {

	Image3D::Image3D( 
		ImageFormat Format, 
		uint32_t Width, 
		uint32_t Height, 
		uint32_t Depth, 
		uint32_t MipCount /*= 1u*/, 
		ImageTiling Tiling /*= ImageTiling::Optimal*/, 
		bool storage /*= false */ 
	)
		: m_Format( Format ),
		m_Width( Width ),
		m_Height( Height ),
		m_Depth( Depth ),
		m_MipCount( MipCount ),
		m_Tiling( Tiling ),
		m_Storage( storage )
	{
		Create();
	}

	Image3D::~Image3D()
	{
		VkImage img = m_Image;
		VkImageView imgView = m_ImageView;
		VkSampler sampler = m_Sampler;
		VkDeviceMemory mem = m_Memory;
		VulkanContext::Get()->SubmitResourceDestory( [ img, imgView, sampler, mem ]()
		{
			vkDestroyImage( VulkanContext::Get()->GetDevice(), img, nullptr );
			vkDestroySampler( VulkanContext::Get()->GetDevice(), sampler, nullptr );
			vkDestroyImageView( VulkanContext::Get()->GetDevice(), imgView, nullptr );
			vkFreeMemory( VulkanContext::Get()->GetDevice(), mem, nullptr );
		} );

		m_Image = nullptr;
		m_Sampler = nullptr;
		m_Memory = nullptr;
	}

	void Image3D::SetDebugName( const std::string& rName ) const
	{
		SetDebugUtilsObjectName( rName, ( uint64_t ) m_Image, VK_OBJECT_TYPE_IMAGE );
	}

	void Image3D::Resize( uint32_t w, uint32_t h, uint32_t d )
	{
		VkImage img = m_Image;
		VkImageView imgView = m_ImageView;
		VkSampler sampler = m_Sampler;
		VkDeviceMemory mem = m_Memory;
		VulkanContext::Get()->SubmitResourceDestory( [ img, imgView, sampler, mem ]()
		{
			vkDestroyImage( VulkanContext::Get()->GetDevice(), img, nullptr );
			vkDestroySampler( VulkanContext::Get()->GetDevice(), sampler, nullptr );
			vkDestroyImageView( VulkanContext::Get()->GetDevice(), imgView, nullptr );
			vkFreeMemory( VulkanContext::Get()->GetDevice(), mem, nullptr );
		} );

		m_Width = w;
		m_Height = h;
		m_Depth = d;

		Create();
	}

	void Image3D::Create()
	{
		// Create image
		VkImageCreateInfo ImageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
		ImageCreateInfo.format = VulkanFormat( m_Format );
		ImageCreateInfo.extent = { .width = m_Width, .height = m_Height, .depth = m_Depth };
		ImageCreateInfo.mipLevels = m_MipCount;
		ImageCreateInfo.arrayLayers = 1u;
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		ImageCreateInfo.tiling = ( VkImageTiling ) m_Tiling;

		if( IsColorFormat( m_Format ) )
			ImageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		else
			ImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		if( m_Storage )
		{
			ImageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
		}

		VK_CHECK( vkCreateImage( VulkanContext::Get()->GetDevice(), &ImageCreateInfo, nullptr, &m_Image ) );
		SetDebugUtilsObjectName( "Image 3D", ( uint64_t ) m_Image, VK_OBJECT_TYPE_IMAGE );

		// Alloc memory for our newly created image.
		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements( VulkanContext::Get()->GetDevice(), m_Image, &MemoryRequirements );

		VkMemoryAllocateInfo MemoryAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
		MemoryAllocateInfo.memoryTypeIndex = VulkanContext::Get()->GetMemoryType( MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

		VK_CHECK( vkAllocateMemory( VulkanContext::Get()->GetDevice(), &MemoryAllocateInfo, nullptr, &m_Memory ) );
		VK_CHECK( vkBindImageMemory( VulkanContext::Get()->GetDevice(), m_Image, m_Memory, 0 ) );

		if( IsColorFormat( m_Format ) )
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		else
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		// If we are a storage image we need to change our layout from undefined to general.
		if( m_Storage )
		{
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			TransitionImageLayout( VulkanFormat( m_Format ), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL );
		}
		else
		{
			TransitionImageLayout(
				VulkanFormat( m_Format ),
				VK_IMAGE_LAYOUT_UNDEFINED,
				m_DescriptorImageInfo.imageLayout );
		}

		// Create base image view & sampler.
		VkImageViewCreateInfo ImageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		ImageViewCreateInfo.image = m_Image;
		ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
		ImageViewCreateInfo.format = VulkanFormat( m_Format );

		if( IsColorFormat( m_Format ) )
			ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		else
			ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1u;

		VK_CHECK( vkCreateImageView( VulkanContext::Get()->GetDevice(), &ImageViewCreateInfo, nullptr, &m_ImageView ) );
		SetDebugUtilsObjectName( "Base image view layer", ( uint64_t ) m_ImageView, VK_OBJECT_TYPE_IMAGE_VIEW );

		// Create sampler.
		VkSamplerCreateInfo SamplerCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		SamplerCreateInfo.magFilter = VK_FILTER_NEAREST;
		SamplerCreateInfo.minFilter = VK_FILTER_NEAREST;
		SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		SamplerCreateInfo.mipLodBias = 0.0f;
		SamplerCreateInfo.maxAnisotropy = 1.0f;
		SamplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		SamplerCreateInfo.minLod = 0.0f;
		SamplerCreateInfo.maxLod = 100.0f;
		SamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		VK_CHECK( vkCreateSampler( VulkanContext::Get()->GetDevice(), &SamplerCreateInfo, nullptr, &m_Sampler ) );
		SetDebugUtilsObjectName( "Base image sampler", ( uint64_t ) m_Sampler, VK_OBJECT_TYPE_SAMPLER );

		m_DescriptorImageInfo.sampler = m_Sampler;
		m_DescriptorImageInfo.imageView = m_ImageView;
	}

	void Image3D::TransitionImageLayout(
		VkFormat Format, 
		VkImageLayout OldLayout, 
		VkImageLayout NewLayout,
		VkCommandBuffer cmdBuffer /*=nullptr*/ )
	{
		VkCommandBuffer CommandBuffer = ( cmdBuffer ? cmdBuffer : VulkanContext::Get()->BeginSingleTimeCommands() );

		VkPipelineStageFlags SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkPipelineStageFlags DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

		VkImageMemoryBarrier ImageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		ImageBarrier.oldLayout = OldLayout;
		ImageBarrier.newLayout = NewLayout;
		ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.image = m_Image;
		ImageBarrier.subresourceRange.aspectMask = IsColorFormat( Format ) ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
		ImageBarrier.subresourceRange.baseMipLevel = 0;
		ImageBarrier.subresourceRange.levelCount = 1;
		ImageBarrier.subresourceRange.baseArrayLayer = 0;
		ImageBarrier.subresourceRange.layerCount = 1;

		if( Format == VK_FORMAT_D32_SFLOAT_S8_UINT )
			ImageBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		if( OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
		{
			ImageBarrier.srcAccessMask = 0;
			ImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if( OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
		{
			ImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			ImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			DstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if( OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL )
		{
			ImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			ImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			ImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			DstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		}
		else if( OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL )
		{
			ImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			ImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if( OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_GENERAL )
		{
			ImageBarrier.srcAccessMask = 0;
			ImageBarrier.dstAccessMask = 0;

			SrcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			DstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		}

		vkCmdPipelineBarrier( CommandBuffer, SrcStage, DstStage, 0, 0, nullptr, 0, nullptr, 1, &ImageBarrier );

		// We only end (and therefore submit to the queue) if there was no specified cmd buffer.
		if( !cmdBuffer )
			VulkanContext::Get()->EndSingleTimeCommands( CommandBuffer );
	}

	void Image3D::TransitionImageLayout(
		VkCommandBuffer CommandBuffer,
		VkImageLayout OldLayout, VkImageLayout NewLayout,
		VkPipelineStageFlags DstStage, VkPipelineStageFlags SrcStage ) 
	{
		VkFormat vulkanFormat = VulkanFormat( m_Format );

		VkImageMemoryBarrier ImageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.oldLayout = OldLayout;
		ImageBarrier.newLayout = NewLayout;
		ImageBarrier.image = m_Image;

		ImageBarrier.subresourceRange.aspectMask = IsColorFormat( vulkanFormat ) ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
		ImageBarrier.subresourceRange.baseMipLevel = 0;
		ImageBarrier.subresourceRange.levelCount = 1;
		ImageBarrier.subresourceRange.baseArrayLayer = 0;
		ImageBarrier.subresourceRange.layerCount = 1;

		if( vulkanFormat == VK_FORMAT_D32_SFLOAT_S8_UINT )
			ImageBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		vkCmdPipelineBarrier( CommandBuffer,
			SrcStage,
			DstStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &ImageBarrier );
	}

	void Image3D::TransitionImageLayout( VkCommandBuffer buffer, VkImageLayout NewLayout )
	{
		TransitionImageLayout( VulkanFormat( m_Format ), m_DescriptorImageInfo.imageLayout, NewLayout, buffer );
		m_DescriptorImageInfo.imageLayout = NewLayout;
	}

	void Image3D::CopyBufferToImage( VkBuffer buffer )
	{
	}

}
