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

#pragma once

#include "Image2DDefines.h"

#include <vulkan.h>

namespace Saturn {

	class Image3D : public RefTarget
	{
	public:
		Image3D( ImageFormat Format, 
			uint32_t Width, 
			uint32_t Height, 
			uint32_t Depth, 
			uint32_t MipCount = 1u,
			ImageTiling Tiling = ImageTiling::Optimal, 
			bool storage = false );
		
		virtual ~Image3D();

		void SetDebugName( const std::string& rName ) const;
		void Resize( uint32_t w, uint32_t h, uint32_t d );

	public:
		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }
		uint32_t GetDepth() const { return m_Depth; }

		VkImage GetImage() const { return m_Image; }
		VkImageView GetImageView() const { return m_ImageView; }
		VkSampler GetSampler() const { return m_Sampler; }
		VkDeviceMemory GetMemory() const { return m_Memory; }

		ImageFormat GetImageFormat() const { return m_Format; }
		ImageTiling GetTiling() const { return m_Tiling; }

		VkDescriptorImageInfo& GetDescriptorInfo() { return m_DescriptorImageInfo; }
		const VkDescriptorImageInfo& GetDescriptorInfo() const { return m_DescriptorImageInfo; }

		void TransitionImageLayout( VkCommandBuffer cmdBuffer, VkImageLayout NewLayout );
	
	private:
		void Create();
		void CopyBufferToImage( VkBuffer buffer );

		void TransitionImageLayout( VkCommandBuffer CommandBuffer, VkImageLayout OldLayout, VkImageLayout NewLayout, VkPipelineStageFlags DstStage, VkPipelineStageFlags SrcStage );
	
		void TransitionImageLayout( VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout, VkCommandBuffer cmdBuffer = nullptr );
	
	private:
		uint32_t m_Width = 0u;
		uint32_t m_Height = 0u;
		uint32_t m_Depth = 0u;
		uint32_t m_MipCount = 0u;

		ImageFormat m_Format;
		ImageTiling m_Tiling;

		VkImage m_Image = VK_NULL_HANDLE;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
		VkDeviceMemory m_Memory = VK_NULL_HANDLE;

		VkDescriptorImageInfo m_DescriptorImageInfo;
		bool m_Storage = false;
	};
	
}
