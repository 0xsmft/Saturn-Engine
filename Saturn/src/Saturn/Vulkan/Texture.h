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

#pragma once

#include "Base.h"
#include "Image2D.h"
#include "Saturn/Core/Memory/Buffer.h"

#include <filesystem>

namespace Saturn {
	
	extern void CreateImage( 
						uint32_t Width, 
						uint32_t Height,
						VkFormat Format, 
						VkImageType ImageType,
						VkImageTiling Tiling,
						VkImageUsageFlags Usage, 
						VkMemoryPropertyFlags MemProps,
						VkImage& rImage, VkDeviceMemory& rDeviceMemory, uint32_t MipLevels = 1, uint32_t ArrayLevels = 1, VkImageCreateFlags Flags = 0 );

	extern VkImageView CreateImageView( 
						VkImage Image, 
						VkFormat Format );
	
	extern VkImageView CreateImageView(
					VkImage Image,
					VkFormat Format,
					VkImageAspectFlags AspectFlags );

	extern void TransitionImageLayout( VkImage Image, VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout );

	enum class AddressingMode
	{
		Repeat,
		MirroredRepeat,
		ClampToEdge,
		ClampToBorder
	};

	// Represents a Vulkan Image, ImageView and Sampler
	// Texture holds an Image2D internally and is used to create a texture from a file path
	// This is different from the Image2D class as the Image2D as this has to be created by a filepath
	class Texture : public RefTarget
	{
	public:
		Texture() = default;
		Texture( uint32_t width, uint32_t height, VkFormat Format, const void* pData );
		Texture( std::filesystem::path Path, AddressingMode Mode ) : m_Path( Path ), m_AddressingMode( Mode ) {}

		virtual ~Texture() = default;

		void TransitionImageLayout( VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout );
		void TransitionImageLayout( VkImageSubresourceRange& rCommand, VkImageLayout OldLayout, VkImageLayout NewLayout );

		uint32_t GetMipMapLevels();

		std::pair<uint32_t, uint32_t> GetMipSize( uint32_t mip ) const;

		void CopyBufferToImage( VkBuffer Buffer );

		void SetPath( const std::filesystem::path& rPath ) { m_Path = rPath; };

	public:
		VkSampler GetSampler()					   const { return m_Sampler; }
		VkImageView GetImageView()                 const { return m_ImageView; }
		VkImage GetImage()                         const { return m_Image; }
		VkDescriptorSet GetDescriptorSet()         const { return m_DescriptorSet; }
		VkDescriptorImageInfo& GetDescriptorInfo()       { return m_DescriptorImageInfo; }

		const VkDescriptorImageInfo& GetDescriptorInfo() const { return m_DescriptorImageInfo; }

		std::filesystem::path GetPath() { return m_Path; }
		const std::filesystem::path& GetPath() const { return m_Path; }

		int Width() const { return m_Width; }
		int Height() const { return m_Height; }

	public:
		virtual void CreateTextureImage( bool flip ) = 0;
		virtual void SetData( const void* pData ) = 0;
		void SetIsRendererTexture( bool RendererTexture );
		virtual void SetForceTerminate( bool ForceTerminate ) { m_ForceTerminate = ForceTerminate; }

		virtual bool IsRendererTexture() { return m_IsRendererTexture; }

		virtual void CreateMips() = 0;

		virtual VkImageView GetOrCreateMipImageView( uint32_t mip ) = 0;

	protected:
		void Terminate();

	protected:
		std::filesystem::path m_Path = "";
		
		VkImage m_Image = VK_NULL_HANDLE;
		VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
		VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
		VkDescriptorImageInfo m_DescriptorImageInfo = {};
		VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;

		bool m_HDR = false;
		bool m_IsRendererTexture = false;
		bool m_ForceTerminate = false;
		bool m_MipsCreated = false;
		
		void* m_pData = nullptr;

		AddressingMode m_AddressingMode = AddressingMode::Repeat;

		std::unordered_map<uint32_t, VkImageView> m_MipToImageViewMap;

		int m_Width = 0;
		int m_Height = 0;

		bool m_Storage = false;
	};
	
	class Texture2D : public Texture
	{
	public:
		Texture2D() = default;
		Texture2D( const std::filesystem::path& rPath, AddressingMode Mode = AddressingMode::Repeat, bool flip = true );
		Texture2D( ImageFormat format, uint32_t width, uint32_t height, const void* pData, bool storage = false );
		
		~Texture2D();

		void Copy( Ref<Texture2D> rOther );

		VkImageView GetOrCreateMipImageView( uint32_t mip ) override;

		void SetDebugName( const std::string& rName );

	private:
		void CreateTextureImage( bool flip ) override;
		void SetData( const void* pData ) override;
		void CreateMips() override;
	};

	class TextureCube : public Texture
	{
	public:
		TextureCube() : Texture() {}
		TextureCube( const std::filesystem::path& rPath, AddressingMode Mode );
		TextureCube( ImageFormat Format, uint32_t width, uint32_t height, const void* pData = nullptr );

		~TextureCube();

		void CreateMips() override;

		VkImageView GetOrCreateMipImageView( uint32_t mip ) override;
	private:

		void CreateTextureImage(bool flip) override;
		void SetData( const void* pData ) override;
	};
}