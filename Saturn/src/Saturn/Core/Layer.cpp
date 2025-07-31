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
#include "Layer.h"

#include "Saturn/Core/Renderer/RenderThread.h"

#include "Saturn/ImGui/Styles.h"
#include "Saturn/Vulkan/VulkanContext.h"
#include "Saturn/Vulkan/Renderer.h"
#include "Saturn/Vulkan/VulkanDebug.h"

#if !defined(SAT_DIST)
#include "ImGuizmo/ImGuizmo.h"
#endif

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_ruby.h>

#include <Saturn/Core/Ruby/RubyWindow.h>

#include "Saturn/ImGui/EmbededFonts/NotoSansAll.embed"

namespace Saturn {
	
	//////////////////////////////////////////////////////////////////////////

	ImGuiLayer::ImGuiLayer()
	{
	}

	ImGuiLayer::~ImGuiLayer()
	{
	}

	void ImGuiLayer::OnAttach( void )
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& rIO = ImGui::GetIO();

		rIO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		rIO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		rIO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& rStyle = ImGui::GetStyle();
		{
			rStyle.WindowRounding = 0.0f;
			rStyle.Colors[ ImGuiCol_WindowBg ].w = 1.0f;
		}

		rIO.Fonts->AddFontFromMemoryTTF( ( void* ) GNotoSansRegularEmbeded, sizeof( GNotoSansRegularEmbeded ), 18.0f );
		rIO.FontDefault = rIO.Fonts->Fonts.back();
		
		rIO.Fonts->AddFontFromMemoryTTF( ( void* ) GNotoSansBoldEmbeded, sizeof( GNotoSansBoldEmbeded ), 18.0f );
		rIO.Fonts->AddFontFromMemoryTTF( ( void* ) GNotoSansItalicEmbeded, sizeof( GNotoSansItalicEmbeded ), 18.0f );

		Styles::Dark();

		ImGui_ImplRuby_InitForVulkan( Application::Get().GetWindow() );

		// Create ImGui Descriptor Pool
		{
			std::vector< VkDescriptorPoolSize > PoolSizes;

			PoolSizes.push_back( { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 } );
			PoolSizes.push_back( { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 } );
			PoolSizes.push_back( { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 } );
			PoolSizes.push_back( { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 } );
			PoolSizes.push_back( { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 } );
			PoolSizes.push_back( { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 } );
			PoolSizes.push_back( { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 } );
			PoolSizes.push_back( { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 } );
			PoolSizes.push_back( { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 } );
			PoolSizes.push_back( { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 } );
			PoolSizes.push_back( { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } );

			VkDescriptorPoolCreateInfo PoolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
			PoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			PoolCreateInfo.maxSets = 1000;
			PoolCreateInfo.poolSizeCount = ( uint32_t ) PoolSizes.size();
			PoolCreateInfo.pPoolSizes = PoolSizes.data();

			VK_CHECK( vkCreateDescriptorPool( VulkanContext::Get().GetDevice(), &PoolCreateInfo, nullptr, &m_DescriptorPool ) );
		}
		
		ImGui_ImplVulkan_InitInfo ImGuiInitInfo = {};
		ImGuiInitInfo.Instance = VulkanContext::Get().GetInstance();
		ImGuiInitInfo.PhysicalDevice = VulkanContext::Get().GetPhysicalDevice();
		ImGuiInitInfo.Device = VulkanContext::Get().GetDevice();
		ImGuiInitInfo.Queue = VulkanContext::Get().GetGraphicsQueue();
		ImGuiInitInfo.DescriptorPool = m_DescriptorPool;
		ImGuiInitInfo.MinImageCount = 2;
		ImGuiInitInfo.ImageCount = MAX_FRAMES_IN_FLIGHT;
		ImGuiInitInfo.MSAASamples = VulkanContext::Get().GetMaxUsableMSAASamples();

		ImGuiInitInfo.CheckVkResultFn = _VkCheckResult;

		ImGui_ImplVulkan_Init( &ImGuiInitInfo, VulkanContext::Get().GetDefaultVulkanPass() );
	}

	void ImGuiLayer::OnDetach( void )
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplRuby_Shutdown();

		vkDestroyDescriptorPool( VulkanContext::Get().GetDevice(), m_DescriptorPool, nullptr );
		vkDestroyDescriptorSetLayout( VulkanContext::Get().GetDevice(), m_DescriptorLayout, nullptr );

		m_DescriptorPool = nullptr;
		m_DescriptorLayout = nullptr;
	}

	void ImGuiLayer::Begin()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplRuby_NewFrame();

		ImGui::NewFrame();
#if !defined(SAT_DIST)
		ImGuizmo::BeginFrame();
#endif
	}

	void ImGuiLayer::End( VkCommandBuffer CommandBuffer )
	{
		Swapchain& rSwapchain = VulkanContext::Get().GetSwapchain();

		ImGui::Render();

		uint32_t WindowWidth = Application::Get().GetWindow()->GetWidth();
		uint32_t WindowHeight = Application::Get().GetWindow()->GetHeight();

		std::vector<VkClearValue> ClearColor;
		ClearColor.push_back( { .color = { { 0.1f, 0.1f, 0.1f, 1.0f } } } );
		if( VulkanContext::Get().GetMaxUsableMSAASamples() > VK_SAMPLE_COUNT_1_BIT )
			ClearColor.push_back( { .color = { { 0.1f, 0.1f, 0.1f, 1.0f } } } );
		ClearColor.push_back( { .depthStencil = { 1.0f, 0 } } );

		VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		RenderPassBeginInfo.renderPass = VulkanContext::Get().GetDefaultVulkanPass();
		RenderPassBeginInfo.framebuffer = rSwapchain.GetFramebuffers()[ Renderer::Get().GetImageIndex() ];
		RenderPassBeginInfo.renderArea.offset = { 0, 0 };
		RenderPassBeginInfo.renderArea.extent = { WindowWidth, WindowHeight };
		RenderPassBeginInfo.clearValueCount = (uint32_t)ClearColor.size();
		RenderPassBeginInfo.pClearValues = ClearColor.data();

		// Begin swap chain pass
		CmdBeginDebugLabel( CommandBuffer, "Swap chain pass" );
		vkCmdBeginRenderPass( CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = 0;
		Viewport.width = ( float ) WindowWidth;
		Viewport.height = ( float ) WindowHeight;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		VkRect2D Scissor = { { 0, 0 }, { WindowWidth, WindowHeight } };

		vkCmdSetViewport( CommandBuffer, 0, 1, &Viewport );
		vkCmdSetScissor( CommandBuffer, 0, 1, &Scissor );

		ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), CommandBuffer );

		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();

		vkCmdEndRenderPass( CommandBuffer );
		CmdEndDebugLabel( CommandBuffer );
	}
}
