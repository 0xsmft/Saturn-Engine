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
#include "Image2D.h"

#include "VulkanAllocator.h"
#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "VulkanImageAux.h"

namespace Saturn {

	Image2D::Image2D( ImageFormat Format, uint32_t Width, uint32_t Height, uint32_t ArrayLevels /*= 1*/, uint32_t MipCount /*= 1*/, uint32_t MSAASamples /*= 1*/, ImageTiling Tiling /*= ImageTiling::Optimal*/, bool storage )
		: m_Format( Format ), m_Width( Width ), m_Height( Height ), m_ArrayLevels( ArrayLevels ), m_MipCount( MipCount ), m_MSAASamples( ( VkSampleCountFlagBits ) MSAASamples ), m_Tiling( Tiling ), m_Storage( storage )
	{
		m_ImageViews.resize( m_ArrayLevels );

		Create();
	}

	Image2D::~Image2D()
	{
		vkDestroyImage( VulkanContext::Get()->GetDevice(), m_Image, nullptr );
		vkDestroySampler( VulkanContext::Get()->GetDevice(), m_Sampler, nullptr );
		vkFreeMemory( VulkanContext::Get()->GetDevice(), m_Memory, nullptr );
		vkDestroyImageView( VulkanContext::Get()->GetDevice(), m_ImageView, nullptr );

		for( size_t i = 0; i < m_ArrayLevels; i++ )
		{
			vkDestroyImageView( VulkanContext::Get()->GetDevice(), m_ImageViews[ i ], nullptr );
			m_ImageViews[ i ] = nullptr;
		}

		m_Image = nullptr;
		m_Sampler = nullptr;
		m_Memory = nullptr;
	}

	void Image2D::SetDebugName( const std::string& rName ) const
	{
		SetDebugUtilsObjectName( rName, ( uint64_t ) m_Image, VK_OBJECT_TYPE_IMAGE );
	}

	void Image2D::Resize( uint32_t Width, uint32_t Height )
	{
		vkDestroyImage( VulkanContext::Get()->GetDevice(), m_Image, nullptr );
		vkDestroySampler( VulkanContext::Get()->GetDevice(), m_Sampler, nullptr );
		vkDestroyImageView( VulkanContext::Get()->GetDevice(), m_ImageView, nullptr );
		vkFreeMemory( VulkanContext::Get()->GetDevice(), m_Memory, nullptr );

		for( size_t i = 0; i < m_ArrayLevels; i++ )
			vkDestroyImageView( VulkanContext::Get()->GetDevice(), m_ImageViews[ i ], nullptr );

		m_Width = Width;
		m_Height = Height;

		Create();
	}

	Buffer Image2D::CopyToBuffer()
	{
		VkDeviceSize ImageSize = ( uint64_t ) m_Width * ( uint64_t ) m_Height * 4;

		// Copy image to vulkan buffer
		VkBufferCreateInfo BufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		BufferCreateInfo.size = ImageSize;
		BufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer ImgBuffer;

		auto pAllocator = VulkanContext::Get()->GetVulkanAllocator();
		auto BufferAlloc = pAllocator->AllocateBuffer( BufferCreateInfo, VMA_MEMORY_USAGE_CPU_ONLY, &ImgBuffer );

		VkCommandBuffer CommandBuffer = VulkanContext::Get()->BeginSingleTimeCommands();

		// TRANSITION: DescriptorImageInfo layout to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		TransitionImageLayout(
			CommandBuffer,
			m_DescriptorImageInfo.imageLayout,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT );

		VkBufferImageCopy Region = {};
		Region.bufferOffset = 0;
		Region.bufferRowLength = 0;
		Region.bufferImageHeight = 0;
		Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		Region.imageSubresource.mipLevel = 0;
		Region.imageSubresource.baseArrayLayer = 0;
		Region.imageSubresource.layerCount = 1;
		Region.imageOffset = { 0, 0, 0 };
		Region.imageExtent = { m_Width, m_Height, 1 };

		vkCmdCopyImageToBuffer( CommandBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ImgBuffer, 1, &Region );

		// TRANSITION: VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL back to DescriptorImageInfo layout
		TransitionImageLayout(
			CommandBuffer,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_DescriptorImageInfo.imageLayout,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT );

		VulkanContext::Get()->EndSingleTimeCommands( CommandBuffer );

		void* pMappedData = pAllocator->MapMemory<void*>( BufferAlloc );

		Buffer buf = Buffer::Copy( ( const void* ) pMappedData, ImageSize );

		pAllocator->UnmapMemory( BufferAlloc );
		pAllocator->DestroyBuffer( ImgBuffer );

		return buf;
	}

	Buffer Image2D::CopyToBufferPixel( int x, int y )
	{
		VkDeviceSize ImageSize = ( uint64_t ) m_Width * ( uint64_t ) m_Height * 4;

		// Copy image to vulkan buffer
		VkBufferCreateInfo BufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		BufferCreateInfo.size = ImageSize;
		BufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer ImgBuffer;

		auto pAllocator = VulkanContext::Get()->GetVulkanAllocator();
		auto BufferAlloc = pAllocator->AllocateBuffer( BufferCreateInfo, VMA_MEMORY_USAGE_CPU_ONLY, &ImgBuffer );

		VkCommandBuffer CommandBuffer = VulkanContext::Get()->BeginSingleTimeCommands();

		// TRANSITION: DescriptorImageInfo layout to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		TransitionImageLayout(
			CommandBuffer,
			m_DescriptorImageInfo.imageLayout,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT );

		VkBufferImageCopy Region = {};
		Region.bufferOffset = 0;
		Region.bufferRowLength = 0;
		Region.bufferImageHeight = 0;
		Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		Region.imageSubresource.mipLevel = 0;
		Region.imageSubresource.baseArrayLayer = 0;
		Region.imageSubresource.layerCount = 1;
		Region.imageOffset.x = x;
		Region.imageOffset.y = y;
		Region.imageExtent = { 1, 1, 1 };

		vkCmdCopyImageToBuffer( CommandBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ImgBuffer, 1, &Region );

		// TRANSITION: VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL back to DescriptorImageInfo layout
		TransitionImageLayout(
			CommandBuffer,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_DescriptorImageInfo.imageLayout,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT );

		VulkanContext::Get()->EndSingleTimeCommands( CommandBuffer );

		void* pMappedData = pAllocator->MapMemory<void*>( BufferAlloc );

		Buffer buf = Buffer::Copy( ( const void* ) pMappedData, ImageSize );

		pAllocator->UnmapMemory( BufferAlloc );
		pAllocator->DestroyBuffer( ImgBuffer );

		return buf;
	}

	void Image2D::Create()
	{
		// Create image
		VkImageCreateInfo ImageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ImageCreateInfo.format = VulkanFormat( m_Format );
		ImageCreateInfo.extent = { .width = m_Width, .height = m_Height, .depth = 1 };
		ImageCreateInfo.mipLevels = m_MipCount;
		ImageCreateInfo.arrayLayers = m_ArrayLevels;
		ImageCreateInfo.samples = m_MSAASamples;
		ImageCreateInfo.tiling = ( VkImageTiling )m_Tiling;

		if( IsColorFormat( m_Format ) )
			ImageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		else
			ImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		if( m_MSAASamples > VK_SAMPLE_COUNT_1_BIT )
		{
			ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

			if( IsColorFormat( m_Format ) )
				ImageCreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			else
				ImageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}

		if( m_Storage ) 
		{
			ImageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
		}

		VK_CHECK( vkCreateImage( VulkanContext::Get()->GetDevice(), &ImageCreateInfo, nullptr, &m_Image ) );
		SetDebugUtilsObjectName( "Image", ( uint64_t ) m_Image, VK_OBJECT_TYPE_IMAGE );

		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements( VulkanContext::Get()->GetDevice(), m_Image, &MemoryRequirements );

		VkMemoryAllocateInfo MemoryAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
		MemoryAllocateInfo.memoryTypeIndex = VulkanContext::Get()->GetMemoryType( MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

		VK_CHECK( vkAllocateMemory( VulkanContext::Get()->GetDevice(), &MemoryAllocateInfo, nullptr, &m_Memory ) );
		VK_CHECK( vkBindImageMemory( VulkanContext::Get()->GetDevice(), m_Image, m_Memory, 0 ) );

		/*
		if( false ) 
		{
			// TRANSITION: Initial layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			TransitionImageLayout( VulkanFormat( m_Format ), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

			VkBuffer ImgBuffer;

			VkDeviceSize ImageSize = m_Width * m_Height * 4;

			VkBufferCreateInfo BufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			BufferCreateInfo.size = m_DataSize;
			BufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			auto pAllocator = VulkanContext::Get()->GetVulkanAllocator();
			auto BufferAlloc = pAllocator->AllocateBuffer( BufferCreateInfo, VMA_MEMORY_USAGE_CPU_ONLY, &ImgBuffer );

			void* pDstData = pAllocator->MapMemory<void*>( BufferAlloc );

			memcpy( pDstData, m_pData, m_DataSize );

			pAllocator->UnmapMemory( BufferAlloc );

			CopyBufferToImage( ImgBuffer );

			// TRANSITION: VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to descriptor image layout.
			TransitionImageLayout( VulkanFormat( m_Format ), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_DescriptorImageInfo.imageLayout );
		}
		*/

		if( IsColorFormat( m_Format ) )
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		else
			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

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
		ImageViewCreateInfo.viewType = m_ArrayLevels > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
		ImageViewCreateInfo.format = VulkanFormat( m_Format );

		if( IsColorFormat( m_Format ) )
			ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		else
			ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = m_ArrayLevels;

		//if( m_Format == ImageFormat::DEPTH24STENCIL8 )
		//	ImageViewCreateInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		VK_CHECK( vkCreateImageView( VulkanContext::Get()->GetDevice(), &ImageViewCreateInfo, nullptr, &m_ImageView) );
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
		SamplerCreateInfo.maxLod = 1.0f;
		SamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		VK_CHECK( vkCreateSampler( VulkanContext::Get()->GetDevice(), &SamplerCreateInfo, nullptr, &m_Sampler ) );
		SetDebugUtilsObjectName( "Base image sampler", ( uint64_t ) m_Sampler, VK_OBJECT_TYPE_SAMPLER );

		m_DescriptorImageInfo.sampler = m_Sampler;
		m_DescriptorImageInfo.imageView = m_ImageView;

		m_ImageViews.resize( m_ArrayLevels );
		for( size_t i = 0; i < m_ArrayLevels; i++ )
		{
			// Create image view.
			VkImageViewCreateInfo ImageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			ImageViewCreateInfo.image = m_Image;
			ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ImageViewCreateInfo.format = VulkanFormat( m_Format );

			if( IsColorFormat( m_Format ) )
				ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			else 
				ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			//if( m_Format == ImageFormat::DEPTH24STENCIL8 )
			//	ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;

			ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			ImageViewCreateInfo.subresourceRange.levelCount = 1;
			ImageViewCreateInfo.subresourceRange.baseArrayLayer = ( uint32_t ) i;
			ImageViewCreateInfo.subresourceRange.layerCount = 1;

			VK_CHECK( vkCreateImageView( VulkanContext::Get()->GetDevice(), &ImageViewCreateInfo, nullptr, &m_ImageViews[ i ] ) );
			SetDebugUtilsObjectName( std::format( "Image view layer {}", i ), ( uint64_t ) m_ImageViews[ i ], VK_OBJECT_TYPE_IMAGE_VIEW );
		}
	}

	void Image2D::CopyBufferToImage( VkBuffer Buffer )
	{
		VkCommandBuffer CommandBuffer = VulkanContext::Get()->BeginSingleTimeCommands();

		VkBufferImageCopy Region = {};
		Region.bufferOffset = 0;
		Region.bufferRowLength = 0;
		Region.bufferImageHeight = 0;

		Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		Region.imageSubresource.mipLevel = 0;
		Region.imageSubresource.baseArrayLayer = 0;
		Region.imageSubresource.layerCount = 1;

		Region.imageOffset = { 0, 0, 0 };
		Region.imageExtent = { m_Width, m_Height, 1 };

		vkCmdCopyBufferToImage( CommandBuffer, Buffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region );

		VulkanContext::Get()->EndSingleTimeCommands( CommandBuffer );
	}

	void Image2D::TransitionImageLayout( VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout )
	{
		VkCommandBuffer CommandBuffer = VulkanContext::Get()->BeginSingleTimeCommands();

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

		VulkanContext::Get()->EndSingleTimeCommands( CommandBuffer );
	}

	void Image2D::TransitionImageLayout( 
		VkCommandBuffer CommandBuffer,
		VkImageLayout OldLayout, VkImageLayout NewLayout, 
		VkPipelineStageFlags DstStage, VkPipelineStageFlags SrcStage )
	{
		VkFormat vulkanFormat = VulkanFormat( m_Format );

		VkImageMemoryBarrier ImageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.oldLayout           = OldLayout;
		ImageBarrier.newLayout           = NewLayout;
		ImageBarrier.image               = m_Image;

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

}