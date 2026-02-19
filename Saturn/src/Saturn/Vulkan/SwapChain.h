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

#include "VulkanError.h"
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <vulkan.h>

namespace Saturn {

	class Image2D;

	class Swapchain : public RefTarget
	{
	public:
		Swapchain();
		~Swapchain();

		void Create();
		void CreateFramebuffers();
		void Recreate();

		void Terminate();

		bool AcquireNextImage( uint32_t Timeout, VkSemaphore Semaphore, VkFence Fence, uint32_t* pImageIndex );

		uint32_t GetImageIndex() const { return m_ImageIndex; }

		VkSwapchainKHR& GetSwapchain() { return m_Swapchain; }
		std::vector< VkFramebuffer >& GetFramebuffers() { return m_Framebuffers; }

	private:
		void CreateImageViews();

		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		VkSemaphore m_PresentSemaphore = VK_NULL_HANDLE;
		
		uint32_t m_ImageIndex = 0;

		std::vector< VkFence > m_Fences;
		std::vector< VkImage > m_Images;
		std::vector< VkImageView > m_ImageViews;
		std::vector< VkFramebuffer > m_Framebuffers;
	};
}