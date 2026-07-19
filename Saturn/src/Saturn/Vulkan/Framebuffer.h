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

#include "Pass.h"
#include "Renderer.h"
#include "Image2D.h"

#include <vulkan.h>

namespace Saturn {

	struct FramebufferTextureSpecification
	{
		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification( ImageFormat format ) : TextureFormat( format ) { }

		ImageFormat TextureFormat = ImageFormat::None;
		bool StorageImage = false;
	};

	struct FramebufferAttachmentSpecification
	{
		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification( const std::initializer_list<FramebufferTextureSpecification>&attachments ) : Attachments( attachments ) {}

		std::vector<FramebufferTextureSpecification> Attachments;
	};

	struct FramebufferSpecification
	{
		uint32_t Width = 0;
		uint32_t Height = 0;
		uint32_t ArrayLevels = 1;
		uint32_t ExistingImageLayer = 0;

		VkSampleCountFlagBits MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		bool CreateDepth = true;

		//
		// Existing Images
		//
		// This allows framebuffers to use existing images that could of been created in a earlier stage.
		// You do not need to specify the Image Attachment for these existing images in the "Attachment" 
		// variable because this image already exists so there is no point to recreate it.
		//
		// However, make sure that if this Framebuffer gets reconstructed that you specify the new existing images 
		// because the framebuffer (by default) does not reset the existing images because it does not know 
		// what they could be. 
		//
		// This is partly because the "Attachments" variable are indirectly linked to an image.
		//
		// Existing Image Index (in the render pass) -> ExistingImage
		std::unordered_map< uint32_t, Ref< Image2D > > ExistingImages;

		// Target Render Pass
		Ref< Pass > RenderPass = nullptr;

		// The Framebuffer Image attachments to create.
		// If you have existing images then do not specify the image formats in the attachment.
		// This is to specify what attachment should be created!
		FramebufferAttachmentSpecification Attachments;
	};

	class Framebuffer : public RefTarget
	{
	public:
		Framebuffer( const FramebufferSpecification& Specification );
		virtual ~Framebuffer();
		
		// "newSpec" can be null, "newSpec" should only be used to reassign Existing images in the framebuffer.
		void Recreate( uint32_t Width, uint32_t Height, const FramebufferSpecification& newSpec = {} );

		operator VkFramebuffer() const { return m_Framebuffer; }

		VkFramebuffer GetVulkanFramebuffer() const { return m_Framebuffer; }

		std::vector< Ref<Image2D> >& GetColorAttachmentsResources() { return m_ColorAttachmentsResources; }
		const std::vector< Ref<Image2D> >& GetColorAttachmentsResources() const { return m_ColorAttachmentsResources; }

		std::vector< ImageFormat >& GetColorAttachmentsFormats() { return m_ColorAttachmentsFormats; }
		const std::vector< ImageFormat >& GetColorAttachmentsFormats() const { return m_ColorAttachmentsFormats; }

		Ref<Image2D> GetDepthAttachmentResource() { return m_DepthAttachmentResource; }
		const Ref<Image2D> GetDepthAttachmentResource() const { return m_DepthAttachmentResource; }

		void Capture( const std::filesystem::path& rPath, uint32_t ColorAttachmentIndex = 0, const glm::vec2& rResize = {} );

		uint32_t GetWidth() const { return m_Specification.Width; }
		uint32_t GetHeight() const { return m_Specification.Height; }

	private:
		void Create();
		void Terminate();

		VkFramebuffer m_Framebuffer = nullptr;

		std::vector< ImageFormat > m_ColorAttachmentsFormats;
		std::vector< Ref<Image2D> > m_ColorAttachmentsResources;

		ImageFormat m_DepthFormat = ImageFormat::Depth;
		Ref<Image2D> m_DepthAttachmentResource;

		std::vector< VkImageView > m_AttachmentImageViews;

		FramebufferSpecification m_Specification;
	};
}