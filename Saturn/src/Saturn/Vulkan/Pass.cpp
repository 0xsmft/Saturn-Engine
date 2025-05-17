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
#include "Pass.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"

namespace Saturn {

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

			case Saturn::ImageFormat::RED8:
				return VK_FORMAT_R8_UNORM;

			case Saturn::ImageFormat::DEPTH24STENCIL8:
				return VK_FORMAT_D32_SFLOAT_S8_UINT;
			case Saturn::ImageFormat::DEPTH32F:
				return VK_FORMAT_D32_SFLOAT;
		}

		return VK_FORMAT_UNDEFINED;
	}

	static bool IsColorFormat( ImageFormat format )
	{
		switch( format )
		{
			case Saturn::ImageFormat::RGBA8:
			case Saturn::ImageFormat::RGBA16F:
			case Saturn::ImageFormat::RGBA32F:
			case Saturn::ImageFormat::RGB32F:
			case Saturn::ImageFormat::BGRA8:
			case Saturn::ImageFormat::RED8:
				return true;
		}

		return false;
	}

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

	Pass::Pass( const PassSpecification& rPassSpec )
	{
		Create( rPassSpec );
	}

	Pass::~Pass()
	{
	}

	void Pass::Create( const PassSpecification& rPassSpec )
	{
		m_PassSpec = rPassSpec;
		VkSubpassDescription DefaultSubpass = {};

		m_ClearValues.resize( m_PassSpec.Attachments.size() );

		int i = 0;
		for ( auto attachment : m_PassSpec.Attachments )
		{
			if( IsColorFormat( attachment ) ) 
			{
				if( m_PassSpec.IsSwapchainTarget && m_PassSpec.MSAASamples > VK_SAMPLE_COUNT_1_BIT )
				{
					// This is a hack.
					// We only do this because the swapchain pass only has two attachments at the start, but when we add the MSAA pass we add it before the color attachment.
					m_ColorAttachments.emplace_back( ( uint32_t ) i + 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
				}
				else
				{
					m_ColorAttachments.emplace_back( ( uint32_t ) i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
				}

				m_ClearValues[ i ].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			}
			else 
			{
				if( m_PassSpec.IsSwapchainTarget && m_PassSpec.MSAASamples > VK_SAMPLE_COUNT_1_BIT )
				{
					// This is a hack. 
					// We only do this because the swapchain pass only has two attachments at the start, but when we add the MSAA pass we add it before the depth attachment.
					m_DepthAttachment = { .attachment = ( uint32_t ) i + 1, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
				}
				else
				{
					m_DepthAttachment = { .attachment = ( uint32_t ) i, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
				}

				m_ClearValues[ i ].depthStencil = { 1.0f, 0 };
			}

			i++;
		}

		if ( m_DepthAttachment.layout == VK_IMAGE_LAYOUT_UNDEFINED || m_DepthAttachment.layout == VK_IMAGE_LAYOUT_MAX_ENUM )
		{
			DefaultSubpass.pDepthStencilAttachment = nullptr;
		}
		else
		{
			DefaultSubpass.pDepthStencilAttachment = &m_DepthAttachment;
		}

		if( m_ColorAttachments.size() ) 
		{
			DefaultSubpass.pColorAttachments = m_ColorAttachments.data();
			DefaultSubpass.colorAttachmentCount = (uint32_t)m_ColorAttachments.size();
		}

		DefaultSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		// Dependencies
		std::vector< VkSubpassDependency > Dependencies;

		Dependencies.push_back( {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		} );

		Dependencies.push_back( {
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		} );

		// Attachment Descriptions
		std::vector< VkAttachmentDescription > AttachmentDescriptions;

		for ( auto attachment : m_PassSpec.Attachments )
		{
			VkAttachmentLoadOp clrOp = m_PassSpec.LoadColor ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
			VkAttachmentLoadOp dptOp = m_PassSpec.LoadDepth ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;

			if( m_PassSpec.IsSwapchainTarget )
			{
				AttachmentDescriptions.push_back(
				{
					.flags = 0,
					.format = VulkanFormat( attachment ),
					.samples = IsDepthFormat( attachment ) ? m_PassSpec.MSAASamples : VK_SAMPLE_COUNT_1_BIT,
					.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.finalLayout = IsColorFormat( attachment ) ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR  : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
				} );
			}
			else
			{
				AttachmentDescriptions.push_back(
				{
					.flags = 0,
					.format = VulkanFormat( attachment ),
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.loadOp = IsColorFormat( attachment ) ? clrOp : dptOp,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = IsColorFormat( attachment ) ? m_PassSpec.LoadColor ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :  VK_IMAGE_LAYOUT_UNDEFINED : m_PassSpec.LoadDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED ,
					.finalLayout = IsColorFormat( attachment ) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
				} );
			}
		}

		VkRenderPassCreateInfo RenderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		RenderPassCreateInfo.pAttachments = AttachmentDescriptions.data();
		RenderPassCreateInfo.attachmentCount = ( uint32_t ) AttachmentDescriptions.size();
		RenderPassCreateInfo.pSubpasses = &DefaultSubpass;
		RenderPassCreateInfo.subpassCount = 1;
		RenderPassCreateInfo.pDependencies = Dependencies.data();
		RenderPassCreateInfo.dependencyCount = ( uint32_t ) Dependencies.size();

		VK_CHECK( vkCreateRenderPass( VulkanContext::Get().GetDevice(), &RenderPassCreateInfo, nullptr, &m_Pass ) );
		SetDebugUtilsObjectName( m_PassSpec.Name, ( uint64_t ) m_Pass, VK_OBJECT_TYPE_RENDER_PASS );
	}

	void Pass::Terminate()
	{
		if( m_Pass )
			vkDestroyRenderPass( VulkanContext::Get().GetDevice(), m_Pass, nullptr );

		m_ColorAttachments.clear();

		m_Pass = nullptr;
	}
	
	void Pass::Recreate()
	{
		Terminate();
		
		Create( m_PassSpec );
	}

	void Pass::BeginPass( VkCommandBuffer CommandBuffer, VkFramebuffer Framebuffer, VkExtent2D Extent )
	{
		m_CommandBuffer = CommandBuffer;
		
		VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		RenderPassBeginInfo.renderPass = m_Pass;
		RenderPassBeginInfo.framebuffer = Framebuffer;
		RenderPassBeginInfo.renderArea.extent = Extent;
		RenderPassBeginInfo.pClearValues = m_ClearValues.data();
		RenderPassBeginInfo.clearValueCount = ( uint32_t )m_ClearValues.size();
		
		vkCmdBeginRenderPass( m_CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
	}

	void Pass::EndPass() const
	{
		vkCmdEndRenderPass( m_CommandBuffer );
	}
}