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

#include "Saturn/Core/Base.h"
#include "Saturn/Core/Buffer.h"

#include <vulkan.h>

namespace Saturn {

	enum class ImageFormat
	{
		None = 0,

		// Color
		RGBA8 = 1,
		RGBA16F = 2,
		RGBA32F = 3,
		RGB32F = 4,
		BGRA8 = 5,
		RED8 = 6,

		// Depth
		DEPTH32F = 7,
		DEPTH24STENCIL8 = 8,

		Depth = DEPTH32F
	};

	// Linear tiled images:
	// These are stored as is and can be copied directly to. But due to the linear nature they're not a good match for GPUs and format and feature support is very limited.
	// It's not advised to use linear tiled images for anything else than copying from host to GPU if buffer copies are not an option.
	//
	// Optimal tiled images:
	// These are stored in an implementation specific layout matching the capability of the hardware. They usually support more formats and features and are much faster.
	// Optimal tiled images are stored on the device and not accessible by the host. So they can't be written directly to (like liner tiled images) and always require some sort of data copy, either from a buffer or a linear tiled image.
	//
	// In Short: Always use optimal tiled images for rendering.
	enum class ImageTiling
	{
		Optimal = 0,
		Linear = 1,
		MaxEnum = 0x7FFFFFFF
	};

	// Represents a Vulkan Image, ImageView and Sampler
	// This is different from the "Texture, Texture2D and TextureCube" classes because an Image2D and not be created from a file path.
	// This should only be used as a memory only Image.
	// For images that require a file use Texture2D or TextureCube
	// Image2D's can not be storage images
	class Image2D : public RefTarget
	{
	public:
		Image2D( ImageFormat Format, uint32_t Width, uint32_t Height, uint32_t ArrayLevels = 1, uint32_t MipCount = 1, uint32_t MSAASamples = 1, ImageTiling Tiling = ImageTiling::Optimal, bool storage = false );
		~Image2D();

		void SetDebugName( const std::string& rName ) const;

		void Resize( uint32_t Width, uint32_t Height );

		Buffer CopyToBuffer();
		Buffer CopyToBufferPixel( int x, int y );

		VkDescriptorImageInfo& GetDescriptorInfo() { return m_DescriptorImageInfo; }

		VkImage GetImage() const { return m_Image; }
		VkImageView GetImageView( size_t index = 0 ) { return m_ImageViews[ index ]; }
		VkSampler GetSampler() const { return m_Sampler; }
		VkDeviceMemory GetMemory() const { return m_Memory; }

		ImageFormat GetImageFormat() const { return m_Format; }
		ImageTiling GetTiling() const { return m_Tiling; }

		void TransitionImageLayout( VkCommandBuffer CommandBuffer, VkImageLayout OldLayout, VkImageLayout NewLayout, VkPipelineStageFlags DstStage, VkPipelineStageFlags SrcStage );

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

	private:
		void Create();
		void CopyBufferToImage( VkBuffer Buffer );
		
		// Internal TransitionImageLayout
		void TransitionImageLayout( VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout );

	private:
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		bool m_Storage = false;

		VkSampleCountFlagBits m_MSAASamples;

		ImageFormat m_Format;
		ImageTiling m_Tiling;

		std::vector<VkImageView> m_ImageViews;

		VkImage m_Image = VK_NULL_HANDLE;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
		VkDeviceMemory m_Memory = VK_NULL_HANDLE;

		uint32_t m_ArrayLevels = 0;
		uint32_t m_MipCount = 0;

		Buffer m_ImageBuffer;

		VkDescriptorImageInfo m_DescriptorImageInfo;
	};
}