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
#include "SwapChain.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"

#include "VulkanImageAux.h"

namespace Saturn {

	Swapchain::Swapchain()
	{
	}

	Swapchain::~Swapchain()
	{
		Terminate();
	}

	void Swapchain::Create()
	{
		SwapchainCreationData SwapchainData = VulkanContext::Get().GetSwapchainCreationData();

		VkSwapchainCreateInfoKHR SwapchainCreateInfo ={ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		SwapchainCreateInfo.surface          = VulkanContext::Get().GetSurface();
		SwapchainCreateInfo.minImageCount    = SwapchainData.ImageCount;
		SwapchainCreateInfo.imageFormat      = SwapchainData.CurrentFormat.format;
		SwapchainCreateInfo.imageColorSpace  = SwapchainData.CurrentFormat.colorSpace;
		SwapchainCreateInfo.imageExtent      = SwapchainData.SurfaceCaps.currentExtent;
		SwapchainCreateInfo.imageArrayLayers = 1;
		SwapchainCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		auto& rQueueFamilyIndices = VulkanContext::Get().GetQueueFamilyIndices();
		uint32_t _QueueFamilyIndices[] ={ rQueueFamilyIndices.GraphicsFamily.value(), rQueueFamilyIndices.PresentFamily.value() };

		if( rQueueFamilyIndices.GraphicsFamily != rQueueFamilyIndices.PresentFamily )
		{
			SwapchainCreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
			SwapchainCreateInfo.queueFamilyIndexCount = 2;
			SwapchainCreateInfo.pQueueFamilyIndices   = _QueueFamilyIndices;
		}
		else
		{
			SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			SwapchainCreateInfo.queueFamilyIndexCount = 0;
			SwapchainCreateInfo.pQueueFamilyIndices   = nullptr;
		}

		SwapchainCreateInfo.preTransform   = SwapchainData.SurfaceCaps.currentTransform;
		SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // No alpha.
		SwapchainCreateInfo.presentMode    = VK_PRESENT_MODE_IMMEDIATE_KHR;
		SwapchainCreateInfo.clipped        = true;

		if( m_Swapchain )
			SwapchainCreateInfo.oldSwapchain = m_Swapchain;
		else
			SwapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
			
		VK_CHECK( vkCreateSwapchainKHR( VulkanContext::Get().GetDevice(), &SwapchainCreateInfo, nullptr, &m_Swapchain ) );
		
		SetDebugUtilsObjectName( "Swap chain", ( uint64_t )m_Swapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR );

		CreateImageViews();
	}

	void Swapchain::CreateFramebuffers()
	{
		SwapchainCreationData SwapchainData = VulkanContext::Get().GetSwapchainCreationData();

		m_Framebuffers.resize( m_ImageViews.size() );
		
		if( VulkanContext::Get().GetMaxUsableMSAASamples() > VK_SAMPLE_COUNT_1_BIT )
		{
			// As we are the swapchain we know that we are going to have an MSAA sample so, we will create the image here.
			auto format = SaturnFormat( SwapchainData.CurrentFormat.format );
			m_MSAAImage = Ref<Image2D>::Create( format, SwapchainData.SurfaceCaps.currentExtent.width, SwapchainData.SurfaceCaps.currentExtent.height, 1, VulkanContext::Get().GetMaxUsableMSAASamples() );
		}

		for( int i = 0; i < m_ImageViews.size(); i++ )
		{
			std::vector< VkImageView > Attachments;
			Attachments.push_back( m_ImageViews[ i ] );
			if( m_MSAAImage )
				Attachments.push_back( m_MSAAImage->GetImageView() );
			//Attachments.push_back( VulkanContext::Get().GetDepthImageView() );
			
			VkFramebufferCreateInfo FramebufferCreateInfo ={ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
			FramebufferCreateInfo.width = SwapchainData.SurfaceCaps.currentExtent.width;
			FramebufferCreateInfo.height = SwapchainData.SurfaceCaps.currentExtent.height;

			FramebufferCreateInfo.renderPass = VulkanContext::Get().GetDefaultVulkanPass(); // Swap chain render pass
			FramebufferCreateInfo.layers = 1;
			FramebufferCreateInfo.pAttachments = Attachments.data();
			FramebufferCreateInfo.attachmentCount = ( uint32_t ) Attachments.size();

			VK_CHECK( vkCreateFramebuffer( VulkanContext::Get().GetDevice(), &FramebufferCreateInfo, nullptr, &m_Framebuffers[ i ] ) );

			SetDebugUtilsObjectName( "Swapchain framebuffer", ( uint64_t )m_Framebuffers[ i ], VK_OBJECT_TYPE_FRAMEBUFFER );
		}
	}

	void Swapchain::Recreate()
	{
		// Destroy old framebuffers and image views that are going to be linked to old swapchain.

		for( auto& rFramebuffer : m_Framebuffers )
		{
			vkDestroyFramebuffer( VulkanContext::Get().GetDevice(), rFramebuffer, nullptr );
		}

		for( auto& rImageView : m_ImageViews )
		{
			vkDestroyImageView( VulkanContext::Get().GetDevice(), rImageView, nullptr );
		}

		// TODO: Destroy Swapchain?

		m_MSAAImage = nullptr;

		m_Framebuffers.clear();
		m_ImageViews.clear();
		m_Images.clear();

		Create();
		CreateFramebuffers();
	}

	void Swapchain::Terminate()
	{
		for( auto& rFramebuffer : m_Framebuffers )
		{
			vkDestroyFramebuffer( VulkanContext::Get().GetDevice(), rFramebuffer, nullptr );
		}

		for( auto& rImageView : m_ImageViews )
		{
			vkDestroyImageView( VulkanContext::Get().GetDevice(), rImageView, nullptr );
		}

		m_MSAAImage = nullptr;

		m_Framebuffers.clear();
		m_ImageViews.clear();

		if( m_Swapchain )
		{
			vkDestroySwapchainKHR( VulkanContext::Get().GetDevice(), m_Swapchain, nullptr );
		}
		
		m_Swapchain = nullptr;
	}

	bool Swapchain::AcquireNextImage( uint32_t Timeout, VkSemaphore Semaphore, VkFence Fence, uint32_t* pImageIndex )
	{
		VkResult Result;

		Result = vkAcquireNextImageKHR( VulkanContext::Get().GetDevice(), m_Swapchain, Timeout, Semaphore, Fence, pImageIndex );

		if( Result == VK_ERROR_OUT_OF_DATE_KHR )
		{
			SAT_CORE_WARN( "Swap chain was out of date! recreating..." );

			Recreate();
			Result = vkAcquireNextImageKHR( VulkanContext::Get().GetDevice(), m_Swapchain, Timeout, Semaphore, Fence, pImageIndex );
			if( Result == VK_ERROR_OUT_OF_DATE_KHR )
				return false;
		}
		else if( Result != VK_SUCCESS )
			return false;

		return true;
	}

	void Swapchain::CreateImageViews()
	{
		SwapchainCreationData SwapchainData = VulkanContext::Get().GetSwapchainCreationData();

		VK_CHECK( vkGetSwapchainImagesKHR( VulkanContext::Get().GetDevice(), m_Swapchain, &SwapchainData.ImageCount, nullptr ) );

		m_Images.clear();
		m_Images.resize( SwapchainData.ImageCount );

		VK_CHECK( vkGetSwapchainImagesKHR( VulkanContext::Get().GetDevice(), m_Swapchain, &SwapchainData.ImageCount, m_Images.data() ) );

		// self note: We can't use a VkImage, we need something called a VkImageView. A VkImageView is a view into a VkImage, as a VkImage is just memory in the physical device (GPU).

		VkImageViewCreateInfo ImageViewCreateInfo ={ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ImageViewCreateInfo.format   = SwapchainData.CurrentFormat.format;
		ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;

		m_ImageViews.resize( SwapchainData.ImageCount );

		for( size_t i = 0; i < SwapchainData.ImageCount; i++ )
		{
			ImageViewCreateInfo.image = m_Images[ i ];

			VK_CHECK( vkCreateImageView( VulkanContext::Get().GetDevice(), &ImageViewCreateInfo, nullptr, &m_ImageViews[ i ] ) );
			SetDebugUtilsObjectName( "Image view", ( uint64_t )m_ImageViews[ i ], VK_OBJECT_TYPE_IMAGE_VIEW );
		}
	}
}