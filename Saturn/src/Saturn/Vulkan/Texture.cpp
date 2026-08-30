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
#include "Texture.h"

#include "Saturn/Core/JobSystem.h"
#include "Saturn/Core/Renderer/RenderThread.h"

#include "Renderer.h"
#include "VulkanAllocator.h"
#include "VulkanContext.h"
#include "VulkanDebug.h"
#include "VulkanImageAux.h"

#include <stb_image.h>

#if !defined(SAT_DIST)
#include <backends/imgui_impl_vulkan.h>
#endif

namespace Saturn {

	static VkSamplerAddressMode SaturnToVulkanAdressingMode( AddressingMode mode )
	{
		switch( mode )
		{
			case Saturn::AddressingMode::Repeat:
				return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case Saturn::AddressingMode::MirroredRepeat:
				return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			case Saturn::AddressingMode::ClampToEdge:
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case Saturn::AddressingMode::ClampToBorder:
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			default:
				break;
		}

		SAT_CORE_ASSERT( false );
		return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
	}

	static bool VulkanIsDepth( VkFormat format )
	{
		return ( format == VK_FORMAT_D32_SFLOAT ) || ( format == VK_FORMAT_D32_SFLOAT_S8_UINT );
	}

	static void ImagePipelineBarrier(
		VkCommandBuffer CommandBuffer, VkImage Image, VkAccessFlags SrcMask, VkAccessFlags DstMask, VkImageLayout OldImageLayout, VkImageLayout NewImageLayout, VkPipelineStageFlags SrcStage, VkPipelineStageFlags DstStage, VkImageSubresourceRange Range )
	{
		VkImageMemoryBarrier ImageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		ImageMemoryBarrier.image = Image;
		ImageMemoryBarrier.subresourceRange = Range;
		ImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageMemoryBarrier.srcAccessMask = SrcMask;
		ImageMemoryBarrier.dstAccessMask = DstMask;
		ImageMemoryBarrier.oldLayout = OldImageLayout;
		ImageMemoryBarrier.newLayout = NewImageLayout;

		vkCmdPipelineBarrier( CommandBuffer, SrcStage, DstStage, 0, 0, nullptr, 0, nullptr, 1, &ImageMemoryBarrier );
	}

	static VkFilter SaturnFilterToVulkan( TextureFilteringFlags flags )
	{
		switch( flags )
		{
			case TextureFilteringFlags::Linear:
				return VK_FILTER_LINEAR;

			case TextureFilteringFlags::Nearest:
				return VK_FILTER_NEAREST;

			default:
				break;
		}

		SAT_CORE_ASSERT( false );
		return VK_FILTER_MAX_ENUM;
	}

	static VkSamplerMipmapMode SaturnFilterToVulkanMipMap( TextureFilteringFlags flags )
	{
		switch( flags )
		{
			case TextureFilteringFlags::Linear:
				return VK_SAMPLER_MIPMAP_MODE_LINEAR;

			case TextureFilteringFlags::Nearest:
				return VK_SAMPLER_MIPMAP_MODE_NEAREST;

			default:
				break;
		}

		SAT_CORE_ASSERT( false );
		return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
	}

	//////////////////////////////////////////////////////////////////////////
	// TEXTURE

	Texture::Texture( uint32_t width, uint32_t height, VkFormat Format, const void* pData, AddressingMode Mode, TextureLoadFlags flags )
		: m_ImageFormat( Format )
	{
		m_Specification.CreateFromMemory = true;
		m_Specification.AddressingMode = Mode;
		m_Specification.LoadFlags = flags;
		m_Specification.pData = pData;
		m_Specification.Width = width;
		m_Specification.Height = height;
		m_Specification.Format = SaturnFormat( Format );
	}

	Texture::Texture( const TextureSpecification& rSpecification )
		: m_Specification( rSpecification )
	{
	}

	Texture::Texture( std::filesystem::path Path, AddressingMode Mode, TextureLoadFlags flags )
	{
		m_Specification.CreateFromMemory = false;
		m_Specification.TexturePath = Path;
		m_Specification.AddressingMode = Mode;
		m_Specification.LoadFlags = flags;
	}

	void Texture::Terminate()
	{
		if( m_Image )
			vkDestroyImage( VulkanContext::Get()->GetDevice(), m_Image, nullptr );

		if( m_ImageMemory )
			vkFreeMemory( VulkanContext::Get()->GetDevice(), m_ImageMemory, nullptr );

		if( m_ImageView )
			vkDestroyImageView( VulkanContext::Get()->GetDevice(), m_ImageView, nullptr );

		if( m_Sampler )
			vkDestroySampler( VulkanContext::Get()->GetDevice(), m_Sampler, nullptr );

		m_Image = nullptr;
		m_ImageMemory = nullptr;
		m_ImageView = nullptr;
		m_Sampler = nullptr;

		for( auto&& [mip, view] : m_MipToImageViewMap )
			vkDestroyImageView( VulkanContext::Get()->GetDevice(), view, nullptr );

		m_MipToImageViewMap.clear();
	}

	void Texture::TransitionImageLayout( VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout )
	{
		VkCommandBuffer CommandBuffer = VulkanContext::Get()->BeginSingleTimeCommands();

		VkPipelineStageFlags SrcStage = 0;
		VkPipelineStageFlags DstStage = 0;

		VkImageMemoryBarrier ImageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		ImageBarrier.oldLayout = OldLayout;
		ImageBarrier.newLayout = NewLayout;
		ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.image = m_Image;
		ImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageBarrier.subresourceRange.baseMipLevel = 0;
		ImageBarrier.subresourceRange.levelCount = 1;
		ImageBarrier.subresourceRange.baseArrayLayer = 0;
		ImageBarrier.subresourceRange.layerCount = 1;

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
		else if( OldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_GENERAL )
		{
			ImageBarrier.srcAccessMask = 0;
			ImageBarrier.dstAccessMask = 0;

			SrcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			DstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		}
		else if( OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_GENERAL )
		{
			ImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			ImageBarrier.dstAccessMask = 0;

			SrcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			DstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		}
		else
		{
			ImageBarrier.srcAccessMask = 0;
			ImageBarrier.dstAccessMask = 0;

			SrcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			DstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}

		vkCmdPipelineBarrier( CommandBuffer, SrcStage, DstStage, 0, 0, nullptr, 0, nullptr, 1, &ImageBarrier );

		VulkanContext::Get()->EndSingleTimeCommands( CommandBuffer );
	}

	void Texture::TransitionImageLayout( VkImageSubresourceRange& rCommand, VkImageLayout OldLayout, VkImageLayout NewLayout )
	{
		VkCommandBuffer CommandBuffer = VulkanContext::Get()->BeginSingleTimeCommands();

		VkPipelineStageFlags SrcStage = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
		VkPipelineStageFlags DstStage = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;

		VkImageMemoryBarrier ImageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		ImageBarrier.oldLayout = OldLayout;
		ImageBarrier.newLayout = NewLayout;
		ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.image = m_Image;
		ImageBarrier.subresourceRange = rCommand;

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
		else if( OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_GENERAL )
		{
			ImageBarrier.srcAccessMask = 0;

			SrcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			DstStage = SrcStage;
		}

		vkCmdPipelineBarrier( CommandBuffer, SrcStage, DstStage, 0, 0, nullptr, 0, nullptr, 1, &ImageBarrier );

		VulkanContext::Get()->EndSingleTimeCommands( CommandBuffer );
	}

	uint32_t Texture::GetMipMapLevels() const
	{
		if( ( m_Specification.LoadFlags & TextureLoadFlags_NoMips ) != 0 )
			return 1;

		// Based from https://www.oreilly.com/library/view/opengl-programming-guide/9780132748445/ch06lev2sec20.html
		return static_cast< uint32_t >( std::floor( std::log2( glm::min( Width(), Height() ) ) ) + 1 );
	}

	std::pair<uint32_t, uint32_t> Texture::GetMipSize( uint32_t mip ) const
	{
		uint32_t width = Width();
		uint32_t height = Height();
		while( mip != 0 )
		{
			width /= 2;
			height /= 2;
			--mip;
		}

		return { width, height };
	}

	void Texture::CopyBufferToImage( VkBuffer Buffer )
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
		Region.imageExtent = { Width(), Height(), 1 };

		vkCmdCopyBufferToImage( CommandBuffer, Buffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region );

		VulkanContext::Get()->EndSingleTimeCommands( CommandBuffer );
	}

	void Texture::SetIsRendererTexture( bool RendererTexture )
	{
		m_IsRendererTexture = RendererTexture;
		m_Specification.TexturePath = "RPT";

		SetDebugUtilsObjectName( "RPT-Sampler", ( uint64_t ) m_Sampler, VK_OBJECT_TYPE_SAMPLER );
		SetDebugUtilsObjectName( "RPT-Img", ( uint64_t ) m_Image, VK_OBJECT_TYPE_IMAGE );
		SetDebugUtilsObjectName( "RPT-View", ( uint64_t ) m_ImageView, VK_OBJECT_TYPE_IMAGE_VIEW );
	}

	//////////////////////////////////////////////////////////////////////////
	// GLOBAL HELPERS														//
	//////////////////////////////////////////////////////////////////////////

	void CreateImage(
		uint32_t Width,
		uint32_t Height,
		VkFormat Format,
		VkImageType ImageType,
		VkImageTiling Tiling,
		VkImageUsageFlags Usage,
		VkMemoryPropertyFlags MemProps,
		VkImage& rImage, VkDeviceMemory& rDeviceMemory, uint32_t MipLevels, uint32_t ArrayLevels, VkImageCreateFlags Flags )
	{
		VkImageCreateInfo ImageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		ImageCreateInfo.imageType = ImageType;
		ImageCreateInfo.extent.width = Width;
		ImageCreateInfo.extent.height = Height;
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.mipLevels = MipLevels;
		ImageCreateInfo.arrayLayers = ArrayLevels;
		ImageCreateInfo.format = Format;
		ImageCreateInfo.tiling = Tiling;
		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ImageCreateInfo.usage = Usage;
		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		ImageCreateInfo.flags = Flags;

		VK_CHECK( vkCreateImage( VulkanContext::Get()->GetDevice(), &ImageCreateInfo, nullptr, &rImage ) );
#if defined(SAT_DEBUG)
		SetDebugUtilsObjectName( "Texture Image (FN/CreateImage)", ( uint64_t ) rImage, VK_OBJECT_TYPE_IMAGE );
#else
		SetDebugUtilsObjectName( "Texture Image", ( uint64_t ) rImage, VK_OBJECT_TYPE_IMAGE );
#endif

		VkMemoryRequirements MemReq;
		vkGetImageMemoryRequirements( VulkanContext::Get()->GetDevice(), rImage, &MemReq );

		VkMemoryAllocateInfo AllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		AllocInfo.allocationSize = MemReq.size;
		AllocInfo.memoryTypeIndex = VulkanContext::Get()->GetMemoryType( MemReq.memoryTypeBits, MemProps );

		VK_CHECK( vkAllocateMemory( VulkanContext::Get()->GetDevice(), &AllocInfo, nullptr, &rDeviceMemory ) );

		VK_CHECK( vkBindImageMemory( VulkanContext::Get()->GetDevice(), rImage, rDeviceMemory, 0 ) );
	}

	VkImageView CreateImageView( VkImage Image, VkFormat Format )
	{
		VkImageViewCreateInfo ImageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		ImageViewCreateInfo.image = Image;
		ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ImageViewCreateInfo.format = Format;
		ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		ImageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };

		VkImageView ImageView;
		VK_CHECK( vkCreateImageView( VulkanContext::Get()->GetDevice(), &ImageViewCreateInfo, nullptr, &ImageView ) );
#if defined(SAT_DEBUG)
		SetDebugUtilsObjectName( "Texture Image View (FN/CreateImageView)", ( uint64_t ) ImageView, VK_OBJECT_TYPE_IMAGE_VIEW );
#else
		SetDebugUtilsObjectName( "Texture Image View", ( uint64_t ) ImageView, VK_OBJECT_TYPE_IMAGE_VIEW );
#endif

		return ImageView;
	}

	VkImageView CreateImageView( const VkImageSubresourceRange& rRange, VkImage Image, VkFormat Format )
	{
		VkImageViewCreateInfo ImageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		ImageViewCreateInfo.image = Image;
		ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ImageViewCreateInfo.format = Format;
		ImageViewCreateInfo.subresourceRange = rRange;
		ImageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };

		VkImageView ImageView;
		VK_CHECK( vkCreateImageView( VulkanContext::Get()->GetDevice(), &ImageViewCreateInfo, nullptr, &ImageView ) );
#if defined(SAT_DEBUG)
		SetDebugUtilsObjectName( "Texture Image View (FN/CreateImageView(VkImageSubresourceRange)", ( uint64_t ) ImageView, VK_OBJECT_TYPE_IMAGE_VIEW );
#else
		SetDebugUtilsObjectName( "Texture Image View", ( uint64_t ) ImageView, VK_OBJECT_TYPE_IMAGE_VIEW );
#endif

		return ImageView;
	}

	VkImageView CreateImageView( VkImage Image, VkFormat Format, VkImageAspectFlags AspectMask )
	{
		VkImageViewCreateInfo ImageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		ImageViewCreateInfo.image = Image;
		ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ImageViewCreateInfo.format = Format;
		ImageViewCreateInfo.subresourceRange.aspectMask = AspectMask;
		ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;

		VkImageView ImageView;
		VK_CHECK( vkCreateImageView( VulkanContext::Get()->GetDevice(), &ImageViewCreateInfo, nullptr, &ImageView ) );
#if defined(SAT_DEBUG)
		SetDebugUtilsObjectName( "Texture Image View (FN/CreateImageView(VkImageAspectFlags)", ( uint64_t ) ImageView, VK_OBJECT_TYPE_IMAGE_VIEW );
#else
		SetDebugUtilsObjectName( "Texture Image View", ( uint64_t ) ImageView, VK_OBJECT_TYPE_IMAGE_VIEW );
#endif

		return ImageView;
	}

	void TransitionImageLayout( VkImage Image, VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout )
	{
		VkCommandBuffer CommandBuffer = VulkanContext::Get()->BeginSingleTimeCommands();

		VkPipelineStageFlags SrcStage = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
		VkPipelineStageFlags DstStage = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;

		VkImageMemoryBarrier ImageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		ImageBarrier.oldLayout = OldLayout;
		ImageBarrier.newLayout = NewLayout;
		ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.image = Image;
		ImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageBarrier.subresourceRange.baseMipLevel = 0;
		ImageBarrier.subresourceRange.levelCount = 1;
		ImageBarrier.subresourceRange.baseArrayLayer = 0;
		ImageBarrier.subresourceRange.layerCount = 1;

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

		vkCmdPipelineBarrier( CommandBuffer, SrcStage, DstStage, 0, 0, nullptr, 0, nullptr, 1, &ImageBarrier );

		VulkanContext::Get()->EndSingleTimeCommands( CommandBuffer );
	}

	static size_t GetImageMemorySize( ImageFormat format, uint32_t width, uint32_t height )
	{
		switch( format )
		{
			default:
			case Saturn::ImageFormat::DEPTH32F:
			case Saturn::ImageFormat::DEPTH24STENCIL8:
			case Saturn::ImageFormat::None: break;

			case Saturn::ImageFormat::BGRA8:
			case Saturn::ImageFormat::RGBA8:
				return width * height * 4;

			case Saturn::ImageFormat::RGBA16:
				return width * height * 4 * sizeof( uint16_t );

			case Saturn::ImageFormat::RGBA32F:
				return width * height * 4 * sizeof( float );

			case Saturn::ImageFormat::RGB32F:
				return width * height * 3 * sizeof( float );

			case Saturn::ImageFormat::RED8:
				return width * height;

			case Saturn::ImageFormat::RG8:
				return width * height * 2;

			case Saturn::ImageFormat::RED32F:
				return width * height * sizeof( float );

			case Saturn::ImageFormat::RGB8:
				return width * height * 3;
		}

		SAT_CORE_ASSERT( false );
		return 0;
	}

	static VkFormat GetVulkanFormatFromChannelCount( uint32_t channel )
	{
		switch( channel )
		{
			case 1:
				return VK_FORMAT_R8_UNORM;

			case 2:
				return VK_FORMAT_R8G8_UNORM;

				// Some vulkan GPUs do not support RGB8 images with the features that we need, 
				// so instead of doing more work to check, we are just going to take the easy, 
				// way out and covert it to a RGBA8 texture.
			case 3:
			case 4:
				return VK_FORMAT_R8G8B8A8_UNORM;

			default:
				return VK_FORMAT_UNDEFINED;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// TEXTURE 2D

	Texture2D::Texture2D( ImageFormat format, uint32_t width, uint32_t height, const void* pData, bool storage, AddressingMode Mode, TextureLoadFlags flags /*= TextureLoadFlags_None*/ )
		: Texture( width, height, VulkanFormat( format ), pData, Mode, flags )
	{
		m_Specification.Storage = storage;
		SetData( pData );
	}

	Texture2D::Texture2D( const std::filesystem::path& rPath, AddressingMode Mode /*= AddressingMode::Repeat*/, TextureLoadFlags loadFlags /*= TextureLoadFlags_FlipVertically*/ )
		: Texture( rPath, Mode, loadFlags )
	{
		CreateTextureImage();
	}

	Texture2D::Texture2D( const TextureSpecification& rSpecification )
		: Texture( rSpecification )
	{
		if( rSpecification.CreateFromMemory )
		{
			SetData( rSpecification.pData );
		}
		else
		{
			CreateTextureImage();
		}
	}

	Texture2D::~Texture2D()
	{
		Terminate();
	}

	void Texture2D::Copy( Ref<Texture2D> rOther )
	{
		m_Image = rOther->m_Image;
		m_ImageMemory = rOther->m_ImageMemory;
		m_ImageView = rOther->m_ImageView;
		m_Sampler = rOther->m_Sampler;
		m_DescriptorImageInfo = rOther->m_DescriptorImageInfo;

		m_HDR = rOther->m_HDR;
		m_MipsCreated = rOther->m_MipsCreated;

#if !defined(SAT_DIST)
		m_DescriptorSet = rOther->m_DescriptorSet;
#endif
	}

	void Texture2D::RT_Reset()
	{
		m_Image = nullptr;
		m_ImageMemory = nullptr;
		m_ImageView = nullptr;
		m_Sampler = nullptr;
		m_DescriptorImageInfo = {};

		m_HDR = false;
		m_MipsCreated = false;

#if !defined(SAT_DIST)
		m_DescriptorSet = nullptr;
#endif
	}

	void Texture2D::PrepareTextureForJobSystem()
	{
		int Width, Height, Channels;
		SAT_CORE_VERIFY( stbi_info( m_Specification.TexturePath.string().c_str(), &Width, &Height, &Channels ), "Failed to get information about texture file." );

		m_Specification.Width = Width;
		m_Specification.Height = Height;
		m_ImageFormat = GetVulkanFormatFromChannelCount( Channels );

		m_pWorkingData = new uint32_t[ Width * Height ];
		std::memset( m_pWorkingData, 0xFFFF00FF, sizeof( uint32_t ) * Width * Height );

		SetData( m_pWorkingData );

		JobSystem::Get().QueueJob( [ this ]()
		{
			int Width, Height, Channels;

			// Flip texture
			stbi_set_flip_vertically_on_load_thread( HasLoadFlag( TextureLoadFlags_FlipVertically ) );

			stbi_uc* pTextureData;

			const auto texturePathStr = m_Specification.TexturePath.string();

			if( stbi_is_hdr( texturePathStr.c_str() ) )
			{
				SAT_CORE_INFO( "Loading HDR texture {0}", texturePathStr );
				pTextureData = ( uint8_t* ) stbi_loadf( texturePathStr.c_str(), &Width, &Height, &Channels, 4 );

				m_HDR = true;
			}
			else
			{
				SAT_CORE_INFO( "Loading texture {0}", texturePathStr );

				pTextureData = stbi_load( texturePathStr.c_str(), &Width, &Height, &Channels, 4 );

				m_HDR = false;
			}

			if( m_pWorkingData )
				delete[] m_pWorkingData;

			m_pWorkingData = pTextureData;

			m_Specification.Width = Width;
			m_Specification.Height = Height;

			RenderThread::Get().Queue( [ this, pTextureData ]()
			{
				SetData( m_pWorkingData );
				stbi_image_free( pTextureData );
				m_pWorkingData = nullptr;
			} );
		} );
	}

	// Load and create a texture 2D for a file path.
	// Create a texture 2D
	void Texture2D::CreateTextureImage()
	{
		SAT_CORE_VERIFY( std::filesystem::exists( m_Specification.TexturePath ), "Fucking texture path does not exist!" );
		SAT_CORE_ASSERT( m_Specification.TexturePath.extension().string() != ".stx", "You cannot load a Saturn Texture Source Asset (.stx) you must use the Texture Source Asset to load it's raw image file, you've passed in the path of the Asset not the image file." );

		if( !HasLoadFlag( TextureLoadFlags_LoadOnMainThread ) )
		{
			PrepareTextureForJobSystem();
		}
		else
		{
			const auto texturePathStr = m_Specification.TexturePath.string();

			int Width = 0, Height = 0, Channels = 0;

			// Flip texture
			stbi_set_flip_vertically_on_load_thread( HasLoadFlag( TextureLoadFlags_FlipVertically ) );

			stbi_uc* pTextureData;

			if( stbi_is_hdr( texturePathStr.c_str() ) )
			{
				SAT_CORE_INFO( "Loading HDR texture {0}", texturePathStr );
				pTextureData = ( uint8_t* ) stbi_loadf( texturePathStr.c_str(), &Width, &Height, &Channels, 4 );

				m_HDR = true;
			}
			else
			{
				SAT_CORE_INFO( "Loading texture {0}", texturePathStr );

				pTextureData = stbi_load( texturePathStr.c_str(), &Width, &Height, &Channels, 4 );

				m_HDR = false;
			}

			m_Specification.Width = Width;
			m_Specification.Height = Height;
			m_ImageFormat = GetVulkanFormatFromChannelCount( Channels );

			SetData( pTextureData );

			stbi_image_free( pTextureData );
		}
	}

	void Texture2D::CreateMips()
	{
		VkCommandBuffer CommandBuffer = VulkanContext::Get()->BeginSingleTimeCommands();

		uint32_t mips = GetMipMapLevels();

		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.image = m_Image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		for( size_t i = 1; i < mips; ++i )
		{
			VkImageBlit imageBlit{};

			imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.srcSubresource.layerCount = 1;
			imageBlit.srcSubresource.mipLevel = ( uint32_t ) i - 1;
			imageBlit.srcOffsets[ 1 ].x = int32_t( Width() >> ( i - 1 ) );
			imageBlit.srcOffsets[ 1 ].y = int32_t( Height() >> ( i - 1 ) );
			imageBlit.srcOffsets[ 1 ].z = 1;

			imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.dstSubresource.layerCount = 1;
			imageBlit.dstSubresource.mipLevel = ( uint32_t ) i;
			imageBlit.dstOffsets[ 1 ].x = int32_t( Width() >> i );
			imageBlit.dstOffsets[ 1 ].y = int32_t( Height() >> i );
			imageBlit.dstOffsets[ 1 ].z = 1;

			VkImageSubresourceRange mipSubRange = {};
			mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipSubRange.baseMipLevel = ( uint32_t ) i;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = 1;

			ImagePipelineBarrier( CommandBuffer, m_Image, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, mipSubRange );

			vkCmdBlitImage( CommandBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR );

			ImagePipelineBarrier( CommandBuffer, m_Image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, mipSubRange );
		}

		VkImageSubresourceRange range =
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = mips,
			.baseArrayLayer = 0,
			.layerCount = 1
		};

		if( !m_Specification.Storage )
		{
			ImagePipelineBarrier( CommandBuffer, m_Image, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, range );

			m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		else
		{
			ImagePipelineBarrier( CommandBuffer, m_Image, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, range );
		}

		VulkanContext::Get()->EndSingleTimeCommands( CommandBuffer );

		m_MipsCreated = true;
	}

	VkImageView Texture2D::GetOrCreateMipImageView( uint32_t mip )
	{
		if( m_MipToImageViewMap.find( mip ) == m_MipToImageViewMap.end() )
		{
			VkImageView view;

			VkImageSubresourceRange range{};
			range.aspectMask = VulkanIsDepth( m_ImageFormat ) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = mip;
			range.layerCount = 1;
			range.baseArrayLayer = 0;
			range.levelCount = 1;

			view = CreateImageView( range, m_Image, m_ImageFormat );

			m_MipToImageViewMap[ mip ] = view;
		}

		return m_MipToImageViewMap.at( mip );
	}

	void Texture2D::SetDebugName( const std::string& rName )
	{
		SetDebugUtilsObjectName( rName.c_str(), ( uint64_t ) m_Image, VK_OBJECT_TYPE_IMAGE );
		SetDebugUtilsObjectName( rName.c_str(), ( uint64_t ) m_ImageView, VK_OBJECT_TYPE_IMAGE_VIEW );
		SetDebugUtilsObjectName( rName.c_str(), ( uint64_t ) m_Sampler, VK_OBJECT_TYPE_SAMPLER );
	}

	static void X31TransitionImageLayout(
		VkCommandBuffer CommandBuffer,
		VkImageLayout OldLayout, VkImageLayout NewLayout,
		VkPipelineStageFlags DstStage, VkPipelineStageFlags SrcStage, VkFormat Format, VkImage Image )
	{
		VkImageMemoryBarrier ImageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.oldLayout = OldLayout;
		ImageBarrier.newLayout = NewLayout;
		ImageBarrier.image = Image;

		ImageBarrier.subresourceRange.aspectMask = IsColorFormat( Format ) ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
		ImageBarrier.subresourceRange.baseMipLevel = 0;
		ImageBarrier.subresourceRange.levelCount = 1;
		ImageBarrier.subresourceRange.baseArrayLayer = 0;
		ImageBarrier.subresourceRange.layerCount = 1;

		if( Format == VK_FORMAT_D32_SFLOAT_S8_UINT )
			ImageBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		vkCmdPipelineBarrier( CommandBuffer,
			SrcStage,
			DstStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &ImageBarrier );
	}

	static void X31MipTransitionImageLayout(
		VkCommandBuffer CommandBuffer,
		VkImageLayout OldLayout, VkImageLayout NewLayout,
		VkPipelineStageFlags DstStage, VkPipelineStageFlags SrcStage, VkFormat Format, VkImage Image, uint32_t mip )
	{
		VkImageMemoryBarrier ImageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImageBarrier.oldLayout = OldLayout;
		ImageBarrier.newLayout = NewLayout;
		ImageBarrier.image = Image;

		ImageBarrier.subresourceRange.aspectMask = IsColorFormat( Format ) ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
		ImageBarrier.subresourceRange.baseMipLevel = mip;
		ImageBarrier.subresourceRange.levelCount = 1;
		ImageBarrier.subresourceRange.baseArrayLayer = 0;
		ImageBarrier.subresourceRange.layerCount = 1;

		if( Format == VK_FORMAT_D32_SFLOAT_S8_UINT )
			ImageBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		vkCmdPipelineBarrier( CommandBuffer,
			SrcStage,
			DstStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &ImageBarrier );
	}

	Buffer Texture2D::X31CopyToBuffer() const
	{
		VkDeviceSize ImageSize = GetImageMemorySize( SaturnFormat( m_ImageFormat ), Width(), Height() );

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
		X31TransitionImageLayout(
			CommandBuffer,
			m_DescriptorImageInfo.imageLayout,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, m_ImageFormat, m_Image );

		VkBufferImageCopy Region = {};
		Region.bufferOffset = 0;
		Region.bufferRowLength = 0;
		Region.bufferImageHeight = 0;
		Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		Region.imageSubresource.mipLevel = 0;
		Region.imageSubresource.baseArrayLayer = 0;
		Region.imageSubresource.layerCount = 1;
		Region.imageOffset = { 0, 0, 0 };
		Region.imageExtent = { Width(), Height(), 1 };

		vkCmdCopyImageToBuffer( CommandBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ImgBuffer, 1, &Region );

		// TRANSITION: VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL back to DescriptorImageInfo layout
		X31TransitionImageLayout(
			CommandBuffer,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_DescriptorImageInfo.imageLayout,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, m_ImageFormat, m_Image );

		VulkanContext::Get()->EndSingleTimeCommands( CommandBuffer );

		void* pMappedData = pAllocator->MapMemory<void*>( BufferAlloc );

		Buffer buf = Buffer::Copy( ( const void* ) pMappedData, ImageSize );

		pAllocator->UnmapMemory( BufferAlloc );
		pAllocator->DestroyBuffer( ImgBuffer );

		return buf;
	}

	Buffer Texture2D::GetMipTextureData( uint32_t w, uint32_t h, uint32_t mip ) const
	{
		const VkDeviceSize ImageSize = GetImageMemorySize( SaturnFormat( m_ImageFormat ), Width(), Height() );

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
		X31MipTransitionImageLayout(
			CommandBuffer,
			m_DescriptorImageInfo.imageLayout,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, m_ImageFormat, m_Image, mip );

		VkBufferImageCopy Region = {};
		Region.bufferOffset = 0;
		Region.bufferRowLength = 0;
		Region.bufferImageHeight = 0;
		Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		Region.imageSubresource.mipLevel = mip;
		Region.imageSubresource.baseArrayLayer = 0;
		Region.imageSubresource.layerCount = 1;
		Region.imageOffset = { 0, 0, 0 };
		Region.imageExtent = { w, h, 1 };

		vkCmdCopyImageToBuffer( CommandBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ImgBuffer, 1, &Region );

		// TRANSITION: VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL back to DescriptorImageInfo layout
		X31MipTransitionImageLayout(
			CommandBuffer,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_DescriptorImageInfo.imageLayout,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, m_ImageFormat, m_Image, mip );

		VulkanContext::Get()->EndSingleTimeCommands( CommandBuffer );

		void* pMappedData = pAllocator->MapMemory<void*>( BufferAlloc );

		Buffer buf = Buffer::Copy( ( const void* ) pMappedData, ImageSize );

		pAllocator->UnmapMemory( BufferAlloc );
		pAllocator->DestroyBuffer( ImgBuffer );

		return buf;
	}

	void Texture2D::CopyBufferToImageAndMips( void* pData )
	{
		auto pAllocator = VulkanContext::Get()->GetVulkanAllocator();

		// Staging Buffer.
		VkBuffer NewDataBuffer = nullptr;

		if( pData )
		{
			VkDeviceSize ImageSize = GetImageMemorySize( SaturnFormat( m_ImageFormat ), Width(), Height() );

			VkBufferCreateInfo BufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			BufferCreateInfo.size = ImageSize;
			BufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			auto rBufferAlloc = pAllocator->AllocateBuffer( BufferCreateInfo, VMA_MEMORY_USAGE_CPU_ONLY, &NewDataBuffer );

			void* pDstData = pAllocator->MapMemory< void* >( rBufferAlloc );

			std::memcpy( pDstData, pData, ImageSize );

			pAllocator->UnmapMemory( rBufferAlloc );
		}

		TransitionImageLayout( m_ImageFormat, m_DescriptorImageInfo.imageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

		CopyBufferToImage( NewDataBuffer );

		const auto MipCount = GetMipMapLevels();

		// TRANSITION (based on following conditions):
		// TRANSFER_DST_OPTIMAL to TRANSFER_SRC_OPTIMAL if we have mips
		// TRANSFER_DST_OPTIMAL to SHADER_READ_ONLY_OPTIMAL if we do not mips and we are not a storage image
		if( MipCount > 1 )
		{
			TransitionImageLayout( m_ImageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );

			CreateMips();

			//TransitionImageLayout( m_ImageFormat, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_DescriptorImageInfo.imageLayout );
		}
		else
			TransitionImageLayout( m_ImageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_DescriptorImageInfo.imageLayout );

		pAllocator->DestroyBuffer( NewDataBuffer );
	}

	void Texture2D::SetData( const void* pData )
	{
		auto pAllocator = VulkanContext::Get()->GetVulkanAllocator();

		const VkDeviceSize ImageSize = GetImageMemorySize( SaturnFormat( m_ImageFormat ), Width(), Height() );

		const auto MipCount = GetMipMapLevels();

		// Staging Buffer.
		VkBuffer StagingBuffer = nullptr;

		if( pData )
		{
			VkBufferCreateInfo BufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			BufferCreateInfo.size = ImageSize;
			BufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			auto rBufferAlloc = pAllocator->AllocateBuffer( BufferCreateInfo, VMA_MEMORY_USAGE_CPU_ONLY, &StagingBuffer );

			void* pDstData = pAllocator->MapMemory< void* >( rBufferAlloc );

			std::memcpy( pDstData, pData, ImageSize );

			pAllocator->UnmapMemory( rBufferAlloc );
		}

		// Create the image.
		if( m_ImageMemory )
			vkFreeMemory( VulkanContext::Get()->GetDevice(), m_ImageMemory, nullptr );

		if( m_Image )
			vkDestroyImage( VulkanContext::Get()->GetDevice(), m_Image, nullptr );

		VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		if( m_Specification.Storage )
			usage |= VK_IMAGE_USAGE_STORAGE_BIT;

		CreateImage( Width(), Height(), m_ImageFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_ImageMemory, MipCount, 1 );

		//if( m_Storage )
		//	TransitionImageLayout( m_ImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL );
		//else
		TransitionImageLayout( m_ImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

		if( pData )
		{
			CopyBufferToImage( StagingBuffer );
			pAllocator->DestroyBuffer( StagingBuffer );
		}

		// TRANSITION (based on following conditions):
		// TRANSFER_DST_OPTIMAL to TRANSFER_SRC_OPTIMAL if we have mips
		// TRANSFER_DST_OPTIMAL to SHADER_READ_ONLY_OPTIMAL if we do not mips and we are not a storage image
		// TRANSFER_DST_OPTIMAL to GENERAL if do not have mips and we are a storage image.
		if( MipCount > 1 )
			TransitionImageLayout( m_ImageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );
		else if( !m_Specification.Storage )
			TransitionImageLayout( m_ImageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		else
			TransitionImageLayout( m_ImageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL );

		// Create image views
		if( m_ImageView )
			vkDestroyImageView( VulkanContext::Get()->GetDevice(), m_ImageView, nullptr );

		VkImageSubresourceRange range = {};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.baseArrayLayer = 0;
		range.layerCount = 1;
		range.levelCount = MipCount;

		m_ImageView = CreateImageView( range, m_Image, m_ImageFormat );

		// Create sampler
		const auto magMinFilter = SaturnFilterToVulkan( m_Specification.FilteringFlags );
		const auto mipMapFilter = SaturnFilterToVulkanMipMap( m_Specification.FilteringFlags );

		VkSamplerCreateInfo SamplerCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		SamplerCreateInfo.magFilter = magMinFilter;
		SamplerCreateInfo.minFilter = magMinFilter;
		SamplerCreateInfo.mipmapMode = mipMapFilter;

		const auto addressingMode = SaturnToVulkanAdressingMode( m_Specification.AddressingMode );
		SamplerCreateInfo.addressModeU = addressingMode;
		SamplerCreateInfo.addressModeV = addressingMode;
		SamplerCreateInfo.addressModeW = addressingMode;

		SamplerCreateInfo.anisotropyEnable = VK_FALSE;

#if SAT_ANISOTROPIC_TEXTURES
		// We don't know the max anisotropy level, so we'll need to get it from the properties of the physical device.
		// We do this as this is the best way to get the max anisotropy level as it can be different on other devices.
		VkPhysicalDeviceProperties Properties = {};
		vkGetPhysicalDeviceProperties( VulkanContext::Get()->GetPhysicalDevice(), &Properties );
#endif

		SamplerCreateInfo.maxAnisotropy = 1;

		SamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		SamplerCreateInfo.compareEnable = VK_FALSE;
		SamplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		SamplerCreateInfo.mipLodBias = 0.0f;
		SamplerCreateInfo.minLod = 0.0f;
		SamplerCreateInfo.maxLod = ( float ) MipCount;

		if( m_Sampler )
			vkDestroySampler( VulkanContext::Get()->GetDevice(), m_Sampler, nullptr );

		VK_CHECK( vkCreateSampler( VulkanContext::Get()->GetDevice(), &SamplerCreateInfo, nullptr, &m_Sampler ) );
#if defined(SAT_DEBUG)
		SetDebugUtilsObjectName( std::format( "Texture Sampler (FN/SetData) PATH/{0} PINK/{1}", m_Specification.TexturePath.stem().string(), m_IsRendererTexture ), ( uint64_t ) m_Sampler, VK_OBJECT_TYPE_SAMPLER );
#else
		SetDebugUtilsObjectName( "Texture Sampler", ( uint64_t ) m_Sampler, VK_OBJECT_TYPE_SAMPLER );
#endif

		m_DescriptorImageInfo = {};
		m_DescriptorImageInfo.imageLayout = m_Specification.Storage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_DescriptorImageInfo.imageView = m_ImageView;
		m_DescriptorImageInfo.sampler = m_Sampler;

#if !defined(SAT_DIST)
		m_DescriptorSet = ( VkDescriptorSet ) ImGui_ImplVulkan_AddTexture( m_Sampler, m_ImageView, m_Specification.Storage ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
#endif

		if( MipCount > 1 )
			CreateMips();
	}

	//////////////////////////////////////////////////////////////////////////
	// Texture Cube
	//////////////////////////////////////////////////////////////////////////

	TextureCube::TextureCube( ImageFormat Format, uint32_t width, uint32_t height, const void* pData /*= nullptr */ )
		: Texture( width, height, VulkanFormat( Format ), pData )
	{
		m_Specification.pData = pData;
		CreateTextureImage();
	}

	TextureCube::TextureCube( const std::filesystem::path& rPath, AddressingMode Mode )
		: Texture( rPath, Mode, TextureLoadFlags_FlipVertically )
	{
	}

	TextureCube::~TextureCube()
	{
		Terminate();
	}

	void TextureCube::CreateTextureImage()
	{
		// Create the image.
		//CreateImage( m_Width, m_Height, m_ImageFormat,
		//	VK_IMAGE_TYPE_2D, 
		//	VK_IMAGE_TILING_OPTIMAL, 
		//	VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | //VK_IMAGE_USAGE_STORAGE_BIT, 
		//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		//	m_Image, m_ImageMemory, GetMipMapLevels(), 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT );

		VkImageCreateInfo ImageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		ImageCreateInfo.extent.width = Width();
		ImageCreateInfo.extent.height = Height();
		ImageCreateInfo.extent.depth = 1;
		ImageCreateInfo.mipLevels = GetMipMapLevels();
		ImageCreateInfo.arrayLayers = 6;
		ImageCreateInfo.format = m_ImageFormat;
		ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		ImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		ImageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VK_CHECK( vkCreateImage( VulkanContext::Get()->GetDevice(), &ImageCreateInfo, nullptr, &m_Image ) );

		VkMemoryRequirements MemReq;
		vkGetImageMemoryRequirements( VulkanContext::Get()->GetDevice(), m_Image, &MemReq );

		VkMemoryAllocateInfo AllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		AllocInfo.allocationSize = MemReq.size;
		AllocInfo.memoryTypeIndex = VulkanContext::Get()->GetMemoryType( MemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

		VK_CHECK( vkAllocateMemory( VulkanContext::Get()->GetDevice(), &AllocInfo, nullptr, &m_ImageMemory ) );

		VK_CHECK( vkBindImageMemory( VulkanContext::Get()->GetDevice(), m_Image, m_ImageMemory, 0 ) );

		//////////////////////////////////////////////////////////////////////////

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0u;
		subresourceRange.levelCount = GetMipMapLevels();
		subresourceRange.layerCount = 6u;

		// No need to transition image layout as the image layout will become VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while creating mips.
		// TRANSITION: VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_GENERAL (temporary until mips are generated).
		TransitionImageLayout( subresourceRange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL );

		// Create image sampler.
		VkSamplerCreateInfo SamplerCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		SamplerCreateInfo.maxAnisotropy = 1.0f;
		SamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		SamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		SamplerCreateInfo.mipLodBias = 0.0f;
		SamplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		SamplerCreateInfo.minLod = 0.0f;
		SamplerCreateInfo.maxLod = ( float ) GetMipMapLevels();
		SamplerCreateInfo.anisotropyEnable = VK_FALSE;
		SamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		VK_CHECK( vkCreateSampler( VulkanContext::Get()->GetDevice(), &SamplerCreateInfo, nullptr, &m_Sampler ) );

		// Create image view

		VkImageSubresourceRange range = {};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.baseArrayLayer = 0;
		range.layerCount = 6;
		range.levelCount = GetMipMapLevels();

		VkImageViewCreateInfo ImageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		ImageViewCreateInfo.image = m_Image;
		ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		ImageViewCreateInfo.format = m_ImageFormat;
		ImageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		ImageViewCreateInfo.subresourceRange = range;

		VK_CHECK( vkCreateImageView( VulkanContext::Get()->GetDevice(), &ImageViewCreateInfo, nullptr, &m_ImageView ) );

		m_DescriptorImageInfo.sampler = m_Sampler;
		m_DescriptorImageInfo.imageView = m_ImageView;
	}

	void TextureCube::CreateMips()
	{
		if( m_MipsCreated )
			return;

		VkCommandBuffer CommandBuffer = VulkanContext::Get()->BeginNewCommandBuffer();

		const uint32_t mipLevels = GetMipMapLevels();
		for( uint32_t face = 0; face < 6; ++face )
		{
			VkImageSubresourceRange mipSubRange = {};
			mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipSubRange.baseMipLevel = 0;
			mipSubRange.baseArrayLayer = face;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = 1;

			// TRANSITION: Prepare current mip level as image blit destination.
			ImagePipelineBarrier( CommandBuffer, m_Image,
				0, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				mipSubRange );
		}

		for( uint32_t i = 1; i < mipLevels; ++i )
		{
			for( uint32_t face = 0; face < 6; ++face )
			{
				VkImageBlit imageBlit{};

				// Source
				imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.srcSubresource.layerCount = 1;
				imageBlit.srcSubresource.mipLevel = i - 1;
				imageBlit.srcSubresource.baseArrayLayer = face;
				imageBlit.srcOffsets[ 1 ].x = int32_t( Width() >> ( i - 1 ) );
				imageBlit.srcOffsets[ 1 ].y = int32_t( Height() >> ( i - 1 ) );
				imageBlit.srcOffsets[ 1 ].z = 1;

				// Destination
				imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.dstSubresource.layerCount = 1;
				imageBlit.dstSubresource.mipLevel = i;
				imageBlit.dstSubresource.baseArrayLayer = face;
				imageBlit.dstOffsets[ 1 ].x = int32_t( Width() >> i );
				imageBlit.dstOffsets[ 1 ].y = int32_t( Height() >> i );
				imageBlit.dstOffsets[ 1 ].z = 1;

				VkImageSubresourceRange mipSubRange = {};
				mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				mipSubRange.baseMipLevel = i;
				mipSubRange.baseArrayLayer = face;
				mipSubRange.levelCount = 1;
				mipSubRange.layerCount = 1;

				// TRANSITION: Prepare current mip level as image blit destination.
				ImagePipelineBarrier( CommandBuffer, m_Image,
					0, VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					mipSubRange );

				// Blit from previous level.
				vkCmdBlitImage(
					CommandBuffer,
					m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &imageBlit,
					VK_FILTER_LINEAR );

				// TRANSITION: Prepare current mip level as image blit source for next level.
				ImagePipelineBarrier( CommandBuffer, m_Image,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					mipSubRange );
			}
		}

		// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to GENERAL.
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.layerCount = 6;
		subresourceRange.levelCount = mipLevels;

		// TRANSITION: Revert back to general image layout.
		ImagePipelineBarrier( CommandBuffer, m_Image,
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			subresourceRange );

		m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VulkanContext::Get()->EndSingleTimeCommands( CommandBuffer );

		m_MipsCreated = true;
	}

	VkImageView TextureCube::GetOrCreateMipImageView( uint32_t mip )
	{
		return nullptr;
	}

	void TextureCube::SetData( const void* pData )
	{
	}

}
