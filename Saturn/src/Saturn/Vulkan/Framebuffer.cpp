/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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
#include "Framebuffer.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "VulkanImageAux.h"
#include "Saturn/Serialisation/ImageFileAux.h"

#include <backends/imgui_impl_vulkan.h>
#include <stb_image_resize.h>

namespace Saturn {

	namespace FramebufferUtills {

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

		static VkFormat VulkanFormat( ImageFormat format )
		{
			switch( format )
			{
				case Saturn::ImageFormat::RGBA8:
					return VK_FORMAT_R8G8B8A8_UNORM;

				case Saturn::ImageFormat::RGBA16F:
					return VK_FORMAT_R16G16B16A16_UNORM;

				case Saturn::ImageFormat::RGBA32F:
					return VK_FORMAT_R32G32B32A32_SFLOAT;

				case ImageFormat::BGRA8:
					return VK_FORMAT_B8G8R8A8_UNORM;

				case ImageFormat::RED8:
					return VK_FORMAT_R8_UNORM;

				case Saturn::ImageFormat::DEPTH24STENCIL8:
					return VK_FORMAT_D32_SFLOAT_S8_UINT;
				case Saturn::ImageFormat::DEPTH32F:
					return VK_FORMAT_D32_SFLOAT;
			}

			return VK_FORMAT_UNDEFINED;
		}
	}

	Framebuffer::Framebuffer( const FramebufferSpecification& Specification )
		: m_Specification( Specification )
	{

		for( auto format : m_Specification.Attachments.Attachments )
		{
			if( FramebufferUtills::IsDepthFormat( format.TextureFormat ) )
				m_DepthFormat = format.TextureFormat;
			else
				m_ColorAttachmentsFormats.push_back( format.TextureFormat );
		}

		Create();
	}

	Framebuffer::~Framebuffer()
	{
		Terminate();
	}

	void Framebuffer::Terminate()
	{
		if( m_Framebuffer )
			vkDestroyFramebuffer( VulkanContext::Get().GetDevice(), m_Framebuffer, nullptr );

		for( auto& resource : m_ColorAttachmentsResources )
		{
			resource = nullptr;
		}

		for( auto& [index, resource] : m_Specification.ExistingImages )
		{
			resource = nullptr;
		}

		m_DepthAttachmentResource = nullptr;

		m_ColorAttachmentsResources.clear();
		m_ColorAttachmentsFormats.clear();
		m_AttachmentImageViews.clear();
	}

	void Framebuffer::Recreate( uint32_t Width, uint32_t Height, const FramebufferSpecification& newSpec /*= {}*/ )
	{
		if( newSpec.ExistingImages.size() )
		{
			for( auto& [index, resource] : m_Specification.ExistingImages )
			{
				resource = nullptr;
			}
		}

		Terminate();
		
		m_Specification.ExistingImages = newSpec.ExistingImages;

		////

		m_Specification.Width = Width;
		m_Specification.Height = Height;

		for( auto format : m_Specification.Attachments.Attachments )
		{
			if( FramebufferUtills::IsDepthFormat( format.TextureFormat ) )
				m_DepthFormat = format.TextureFormat;
			else
				m_ColorAttachmentsFormats.push_back( format.TextureFormat );
		}

		Create();
	}

	void Framebuffer::Create()
	{
		// Create Attachment resources
		// Color
		VkExtent3D Extent = { m_Specification.Width, m_Specification.Height, 1 };

		size_t totalImageViews = m_Specification.Attachments.Attachments.size() + m_Specification.ExistingImages.size();

		m_AttachmentImageViews.resize( totalImageViews );

		for( auto& [imageIndex, rImage] : m_Specification.ExistingImages )
		{
			if( FramebufferUtills::IsDepthFormat( rImage->GetImageFormat() ) )
			{
				m_DepthAttachmentResource = rImage;
				m_DepthFormat = rImage->GetImageFormat();
			}
			else
			{
				if( m_ColorAttachmentsResources.size() )
					m_ColorAttachmentsResources[ imageIndex ] = rImage;
				else
					m_ColorAttachmentsResources.push_back( rImage );
			}

			if( m_AttachmentImageViews.size() )
			{
				m_AttachmentImageViews[ imageIndex ] = rImage->GetImageView( m_Specification.ExistingImageLayer );
			}
			else 
			{
				m_AttachmentImageViews.push_back( rImage->GetImageView( m_Specification.ExistingImageLayer ) );
			}
		}

		int i = 0;
		for( auto format : m_ColorAttachmentsFormats )
		{
			Ref<Image2D> image = Ref<Image2D>::Create( format, m_Specification.Width, m_Specification.Height, m_Specification.ArrayLevels, m_Specification.MSAASamples );

			std::string imageDebugName = std::format( "Color Attachment for framebuffer {0} ({1})", m_Specification.RenderPass->GetName(), i );

			SetDebugUtilsObjectName( imageDebugName.c_str(), (uint64_t)image->GetImage(), VK_OBJECT_TYPE_IMAGE );

			if( imageDebugName == "Color Attachment for framebuffer Late Composite pass (0)" )
				Core::BreakDebug();

			m_ColorAttachmentsResources.push_back( image );

			if( m_AttachmentImageViews.size() )
				m_AttachmentImageViews[ i ] = image->GetImageView();
			else
				m_AttachmentImageViews.push_back( image->GetImageView() );

			i++;
		}
		
		if( !m_DepthAttachmentResource && m_Specification.CreateDepth ) 
		{
			m_DepthAttachmentResource = Ref<Image2D>::Create( m_DepthFormat, m_Specification.Width, m_Specification.Height, m_Specification.ArrayLevels, m_Specification.MSAASamples );

			std::string imageDebugName = std::format( "Depth Attachment for framebuffer {0}", m_Specification.RenderPass->GetName() );

			SetDebugUtilsObjectName( imageDebugName.c_str(), ( uint64_t ) m_DepthAttachmentResource->GetImage(), VK_OBJECT_TYPE_IMAGE );

			if( m_AttachmentImageViews.size() )
				m_AttachmentImageViews[ i ] = m_DepthAttachmentResource->GetImageView( m_Specification.ExistingImageLayer );
			else
				m_AttachmentImageViews.push_back( m_DepthAttachmentResource->GetImageView( m_Specification.ExistingImageLayer ) );
		}

		VkFramebufferCreateInfo FramebufferCreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		FramebufferCreateInfo.renderPass = m_Specification.RenderPass->GetVulkanPass();

		FramebufferCreateInfo.attachmentCount = ( uint32_t ) m_AttachmentImageViews.size();
		FramebufferCreateInfo.pAttachments = m_AttachmentImageViews.data();

		FramebufferCreateInfo.width = m_Specification.Width;
		FramebufferCreateInfo.height = m_Specification.Height;
		FramebufferCreateInfo.layers = 1;

		VK_CHECK( vkCreateFramebuffer( VulkanContext::Get().GetDevice(), &FramebufferCreateInfo, nullptr, &m_Framebuffer ) );
	}

	void Framebuffer::Screenshot( uint32_t ColorAttachmentIndex, const std::filesystem::path& rPath, glm::vec2 resize /*= {}*/ )
	{
		Ref<Image2D> SrcImage = m_ColorAttachmentsResources[ ColorAttachmentIndex ];
		
		// First, check if the device supports blitting to linear images, and check if the device supports blitting from to optimal images.
		bool BlitSuppored = false;
		BlitSuppored = VulkanContext::Get().FormatOptimalBlitSupported();
		BlitSuppored = VulkanContext::Get().FormatLinearBlitSupported( VulkanFormat( SrcImage->GetImageFormat() ) );

		// Create the custom destination image
		VkImageCreateInfo ImageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ImageCreateInfo.format = VulkanFormat( SrcImage->GetImageFormat() ); // Use the framebuffer's format
		ImageCreateInfo.extent.width = m_Specification.Width;
		ImageCreateInfo.extent.height = m_Specification.Height;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.arrayLayers = 1;
		ImageCreateInfo.mipLevels = 1;
		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		ImageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
		ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		VkImage DstImage;
		VK_CHECK( vkCreateImage( VulkanContext::Get().GetDevice(), &ImageCreateInfo, nullptr, &DstImage ) );

		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements( VulkanContext::Get().GetDevice(), DstImage, &MemoryRequirements );

		VkMemoryAllocateInfo MemoryAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
		MemoryAllocateInfo.memoryTypeIndex = VulkanContext::Get().GetMemoryType( MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

		VkDeviceMemory ImageMemory;
		VK_CHECK( vkAllocateMemory( VulkanContext::Get().GetDevice(), &MemoryAllocateInfo, nullptr, &ImageMemory ) );
		VK_CHECK( vkBindImageMemory( VulkanContext::Get().GetDevice(), DstImage, ImageMemory, 0 ) );

		///////////////////////////////////////

		VkCommandBuffer CommandBuffer = VulkanContext::Get().BeginSingleTimeCommands();

		///////////////////////////////////////
		VkImageSubresourceRange SubresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 };


		// TRANSITION: Destination image to transfer destination layout.
		TransitionImageLayout( 
			CommandBuffer,
			DstImage,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			SubresourceRange, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT );

		// TRANSITION: Framebuffer image to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL.
		SrcImage->TransitionImageLayout( 
			CommandBuffer,
			SrcImage->GetDescriptorInfo().imageLayout, 
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT );

		if( BlitSuppored )
		{
			// Blit image.
			VkOffset3D Offset = { .x = static_cast< int32_t >( m_Specification.Width ), .y = static_cast< int32_t >( m_Specification.Height ), .z = 1 };
			VkImageBlit BlitRegion{};
			BlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			BlitRegion.srcSubresource.layerCount = 1;
			BlitRegion.srcOffsets[ 1 ] = Offset;

			BlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			BlitRegion.dstSubresource.layerCount = 1;
			BlitRegion.dstOffsets[ 1 ] = Offset;

			vkCmdBlitImage( CommandBuffer,
				SrcImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				DstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &BlitRegion, VK_FILTER_NEAREST );
		}
		else
		{
			VkImageCopy CopyRegion{};
			CopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			CopyRegion.srcSubresource.layerCount = 1;
		
			CopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			CopyRegion.dstSubresource.layerCount = 1;

			CopyRegion.extent.width  = m_Specification.Width;
			CopyRegion.extent.height = m_Specification.Height;
			CopyRegion.extent.depth  = 1;

			vkCmdCopyImage( CommandBuffer, 
				SrcImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				DstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &CopyRegion );
		}

		// TRANSITION: Destination image to general layout for copying.
		TransitionImageLayout(
			CommandBuffer,
			DstImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_GENERAL, 
			SubresourceRange, 
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT );

		// TRANSITION: Framebuffer image format back to previous specified format in descriptor layout.
		//             Descriptor layout does not update because the image does not know we are changing it's layout (not a bug it's a feature).
		SrcImage->TransitionImageLayout( 
			CommandBuffer,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
			SrcImage->GetDescriptorInfo().imageLayout, 
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT );

		// Execute command buffer.
		VulkanContext::Get().EndSingleTimeCommands( CommandBuffer );

		/////////////////////////////////

		// Save the image to a file.
		VkImageSubresource Subresource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
		VkSubresourceLayout SubresourceLayout;
		vkGetImageSubresourceLayout( VulkanContext::Get().GetDevice(), DstImage, &Subresource, &SubresourceLayout );

		const char* pData = nullptr;
		VK_CHECK( vkMapMemory( VulkanContext::Get().GetDevice(), ImageMemory, 0, VK_WHOLE_SIZE, 0, ( void** ) &pData ) );
		pData += SubresourceLayout.offset;

		/*
		bool ColorSwizzle = false;
		// TODO: Add more colors.
		if( !BlitSuppored )
		{
			std::vector<VkFormat> FormatsRGB{ VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };

			ColorSwizzle = std::find( FormatsRGB.begin(), FormatsRGB.end(), VulkanContext::Get().GetSurfaceFormat().format ) != FormatsRGB.end();
		}
		*/

		// Resize image if needed
		if( resize.x > 0 && resize.y > 0 || resize.x != m_Specification.Width && resize.y != m_Specification.Height )
		{
			Buffer newData;
			newData.Allocate( resize.x * resize.y * 4 );
			newData.Zero_Memory();

			stbir_resize( pData,
				( int ) m_Specification.Width,
				( int ) m_Specification.Height,
				( int ) SubresourceLayout.rowPitch,

				newData.Data,
				( int ) resize.x,
				( int ) resize.y,
				0,
				STBIR_RGBA, STBIR_TYPE_UINT8, STBIR_EDGE_CLAMP, STBIR_FILTER_MITCHELL );

			Auxiliary::WriteImageFile( rPath, Auxiliary::ImageFileType::PNG, resize.x, resize.y, 4, newData.Data, 0 );

			newData.Free();
		}
		else
		{
			Auxiliary::WriteImageFile( rPath, Auxiliary::ImageFileType::PNG, m_Specification.Width, m_Specification.Height, 4, pData, (int)SubresourceLayout.rowPitch );
		}
		
		vkUnmapMemory( VulkanContext::Get().GetDevice(), ImageMemory );
		vkFreeMemory( VulkanContext::Get().GetDevice(), ImageMemory, nullptr );
		vkDestroyImage( VulkanContext::Get().GetDevice(), DstImage, nullptr );
	}

}