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
#include "Framebuffer.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "VulkanImageAux.h"
#include "Helpers.h"

#include "Saturn/Serialisation/Raw/ImageFileAux.h"

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

				case Saturn::ImageFormat::RGBA16:
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

	//////////////////////////////////////////////////////////////////////////

	Framebuffer::Framebuffer( const FramebufferSpecification& Specification )
		: m_Specification( Specification )
	{
		for( auto& rFormat : m_Specification.Attachments.Attachments )
		{
			if( FramebufferUtills::IsDepthFormat( rFormat.TextureFormat ) )
				m_DepthFormat = rFormat.TextureFormat;
			else
				m_ColorAttachmentsFormats.push_back( rFormat.TextureFormat );
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
			vkDestroyFramebuffer( VulkanContext::Get()->GetDevice(), m_Framebuffer, nullptr );

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

		for( auto& rFormat : m_Specification.Attachments.Attachments )
		{
			if( FramebufferUtills::IsDepthFormat( rFormat.TextureFormat ) )
				m_DepthFormat = rFormat.TextureFormat;
			else
				m_ColorAttachmentsFormats.push_back( rFormat.TextureFormat );
		}

		Create();
	}

	void Framebuffer::Create()
	{
		// Resize to accommodate all attachments
		size_t totalImageViews = m_Specification.Attachments.Attachments.size() + m_Specification.ExistingImages.size();
		m_AttachmentImageViews.resize( totalImageViews );

		//////////////////////////////////////////////////////////////////////////
		// Existing attachments

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

		//////////////////////////////////////////////////////////////////////////
		// New attachments

		// Color
		size_t i = 0llu;
		for( auto format : m_ColorAttachmentsFormats )
		{
			const auto& rTextureSpec = m_Specification.Attachments.Attachments[ i ];

			Ref<Image2D> image = Ref<Image2D>::Create( 
				format, 
				m_Specification.Width, m_Specification.Height, 
				m_Specification.ArrayLevels, 
				1, 
				m_Specification.MSAASamples, 
				ImageTiling::Optimal, 
				rTextureSpec.StorageImage 
			);

			const std::string imageDebugName = std::format( "Color Attachment for FB/{0}@({1})", m_Specification.RenderPass->GetName(), i );
			SetDebugUtilsObjectName( imageDebugName.c_str(), ( uint64_t ) image->GetImage(), VK_OBJECT_TYPE_IMAGE );

			if( imageDebugName == "Color Attachment for FB/Late Composite pass@(0)" )
				Core::BreakDebug();

			m_ColorAttachmentsResources.push_back( image );

			if( m_AttachmentImageViews.size() )
				m_AttachmentImageViews[ i ] = image->GetImageView();
			else
				m_AttachmentImageViews.push_back( image->GetImageView() );

			++i;
		}
		
		// Depth
		if( !m_DepthAttachmentResource && m_Specification.CreateDepth ) 
		{
			m_DepthAttachmentResource = Ref<Image2D>::Create(
				m_DepthFormat,
				m_Specification.Width, m_Specification.Height,
				m_Specification.ArrayLevels,
				1,
				m_Specification.MSAASamples,
				ImageTiling::Optimal,
				false
			);

			const std::string imageDebugName = std::format( "Depth Attachment for FB/{0}", m_Specification.RenderPass->GetName() );
			SetDebugUtilsObjectName( imageDebugName.c_str(), ( uint64_t ) m_DepthAttachmentResource->GetImage(), VK_OBJECT_TYPE_IMAGE );

			if( m_AttachmentImageViews.size() )
				m_AttachmentImageViews[ i ] = m_DepthAttachmentResource->GetImageView( m_Specification.ExistingImageLayer );
			else
				m_AttachmentImageViews.push_back( m_DepthAttachmentResource->GetImageView( m_Specification.ExistingImageLayer ) );
		}

		SAT_CORE_ASSERT( m_Specification.RenderPass, "RenderPass cannot ever be null when creating a Framebuffer!" );

		// Create Framebuffer
		VkFramebufferCreateInfo FramebufferCreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		FramebufferCreateInfo.renderPass = m_Specification.RenderPass->GetVulkanPass();

		FramebufferCreateInfo.attachmentCount = ( uint32_t ) m_AttachmentImageViews.size();
		FramebufferCreateInfo.pAttachments = m_AttachmentImageViews.data();

		FramebufferCreateInfo.width = m_Specification.Width;
		FramebufferCreateInfo.height = m_Specification.Height;
		FramebufferCreateInfo.layers = 1;

		VK_CHECK( vkCreateFramebuffer( VulkanContext::Get()->GetDevice(), &FramebufferCreateInfo, nullptr, &m_Framebuffer ) );
	}

	void Framebuffer::Capture( const std::filesystem::path& rPath, uint32_t ColorAttachmentIndex /*= 0 */, const glm::vec2& rResize )
	{
		Ref<Image2D> SrcImage = m_ColorAttachmentsResources[ ColorAttachmentIndex ];
		VkFormat VulkanFormatSrc = VulkanFormat( SrcImage->GetImageFormat() );

		bool SrcBlitSuppored = false;
		if( SrcImage->GetTiling() == ImageTiling::Linear )
		{
			SrcBlitSuppored = VulkanContext::Get()->FormatLinearBlitSupported( VulkanFormatSrc, true );
		}
		else
		{
			SrcBlitSuppored = VulkanContext::Get()->FormatOptimalBlitSupported( VulkanFormatSrc, true );
		}

		// Check if dist supports Blit
		bool BlitSupported = VulkanContext::Get()->FormatLinearBlitSupported( VK_FORMAT_BC1_RGBA_UNORM_BLOCK, false ) && SrcBlitSuppored;

		VkFormat textureFormat = VulkanFormatSrc;
		if( !BlitSupported )
		{
			// If not, check if the Source Image's format supports dst blitting and use that format
			if( SrcImage->GetTiling() == ImageTiling::Linear )
			{
				BlitSupported = VulkanContext::Get()->FormatLinearBlitSupported( VulkanFormatSrc, false );
			}
			else
			{
				BlitSupported = VulkanContext::Get()->FormatOptimalBlitSupported( VulkanFormatSrc, false );
			}
		}

		VkImage DstImage;
		VkDeviceMemory ImageMemory;
		Auxiliary::VulkanCreateImage( 
			VK_IMAGE_TYPE_2D, 
			textureFormat, 
			{ m_Specification.Width, m_Specification.Height, 1 },
			1, /*array levels*/ 
			VK_IMAGE_TILING_LINEAR, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			&DstImage, &ImageMemory );

		//////////////////////////////////////////////////////////////////////////

		VkCommandBuffer CommandBuffer = VulkanContext::Get()->BeginSingleTimeCommands();
		
		//////////////////////////////////////////////////////////////////////////

		VkImageSubresourceRange SubresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 };

		// TRANSITION: Destination image to transfer destination layout (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL).
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

		// Copy or blit image
		if( BlitSupported )
		{
			// Blit image.
			VkOffset3D SrcOffset = { .x = static_cast< int32_t >( m_Specification.Width ), .y = static_cast< int32_t >( m_Specification.Height ), .z = 1 };

			VkImageBlit BlitRegion{};
			BlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			BlitRegion.srcSubresource.layerCount = 1;
			BlitRegion.srcOffsets[ 1 ] = SrcOffset;

			VkOffset3D DstOffset = SrcOffset;
			if( rResize.x != 0.0f && rResize.y != 0.0f ) 
				DstOffset = { .x = static_cast< int32_t >( rResize.x ), .y = static_cast< int32_t >( rResize.y ), .z = 1 };

			BlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			BlitRegion.dstSubresource.layerCount = 1;
			BlitRegion.dstOffsets[ 1 ] = DstOffset;

			vkCmdBlitImage( CommandBuffer,
				SrcImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				DstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &BlitRegion, VK_FILTER_LINEAR );
		}
		else
		{
			VkImageCopy CopyRegion{};
			CopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			CopyRegion.srcSubresource.layerCount = 1;

			CopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			CopyRegion.dstSubresource.layerCount = 1;

			CopyRegion.extent.width = m_Specification.Width;
			CopyRegion.extent.height = m_Specification.Height;
			CopyRegion.extent.depth = 1;

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
		VulkanContext::Get()->EndSingleTimeCommands( CommandBuffer );

		//////////////////////////////////////////////////////////////////////////
		VkImageSubresource Subresource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
		VkSubresourceLayout SubresourceLayout;
		vkGetImageSubresourceLayout( VulkanContext::Get()->GetDevice(), DstImage, &Subresource, &SubresourceLayout );

		const char* pData = nullptr;
		VK_CHECK( vkMapMemory( VulkanContext::Get()->GetDevice(), ImageMemory, 0, VK_WHOLE_SIZE, 0, ( void** ) &pData ) );
		pData += SubresourceLayout.offset;

		VkExtent3D extent = { m_Specification.Width, m_Specification.Height, 1 };
		if( rResize.x != 0.0f && rResize.y != 0.0f )
			extent = { ( uint32_t ) rResize.x, ( uint32_t ) rResize.y, 1 };

		/*
		ktxTextureCreateInfo textureCreateInfo{};
		textureCreateInfo.vkFormat = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		textureCreateInfo.baseWidth = extent.width;
		textureCreateInfo.baseHeight = extent.height;
		textureCreateInfo.baseDepth = 1;
		textureCreateInfo.numFaces = 1;
		textureCreateInfo.numLayers = 1;
		textureCreateInfo.numLevels = 1;
		textureCreateInfo.numDimensions = 2;
		textureCreateInfo.isArray = false;

		ktxTexture2* pTexture = nullptr;
		VK_KTX_CHECK( ktxTexture2_Create( &textureCreateInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &pTexture ) );

		ktxTexture_SetImageFromMemory( reinterpret_cast<ktxTexture*>( pTexture ), 0, 0, 0, (unsigned char*)&pData, ( extent.width * extent.height * 4 ) );

		ktxTexture2_CompressBasis( pTexture, 128 );

		ktxTexture_WriteToNamedFile( reinterpret_cast< ktxTexture* >( pTexture ), rPath.string().c_str() );

		ktxTexture_Destroy( reinterpret_cast< ktxTexture* >( pTexture ) );
		*/

		Auxiliary::WriteImageFile( rPath, Auxiliary::ImageFileType::PNG, extent.width, extent.height, 4, pData, ( int ) SubresourceLayout.rowPitch );

		vkUnmapMemory( VulkanContext::Get()->GetDevice(), ImageMemory );
		vkFreeMemory( VulkanContext::Get()->GetDevice(), ImageMemory, nullptr );
		vkDestroyImage( VulkanContext::Get()->GetDevice(), DstImage, nullptr );
	}

}