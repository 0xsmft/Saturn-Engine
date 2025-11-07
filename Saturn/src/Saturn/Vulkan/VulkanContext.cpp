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
#include "VulkanContext.h"

#include "Saturn/Core/Ruby/RubyWindow.h"

#include "VulkanDebugMessenger.h"

#include "VulkanDebug.h"
#include "VulkanAllocator.h"

#include "Saturn/Core/Timer.h"
#include "SceneRenderer.h"
#include "Renderer2D.h"
#include "Helpers.h"

// ImGui
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

#include <glm/gtx/matrix_decompose.hpp>

#include <vulkan.h>
#include <cassert>
#include <set>
#include <iostream>
#include <string>

// Define this to force enable validation layers even in dist builds.
//#define SAT_FORCE_ENABLE_VALIDATION_LAYERS

namespace Saturn {
	
	VulkanContext::VulkanContext()
	{
		SingletonStorage::AddSingleton( this );
	}

	void VulkanContext::Init()
	{
		CreateInstance();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateCommandPool();

		m_pAllocator = new VulkanAllocator();
	
		// Create default pass.
		PassSpecification Specification = {};
		Specification.Name = "Swapchain render pass";
		Specification.IsSwapchainTarget = true;
		Specification.MSAASamples = GetMaxUsableMSAASamples();

		// BGRA8 will be VK_IMAGE_LAYOUT_PRESENT_SRC_KHR as this is the swapchain target.
		// No Depth should be created.
		Specification.Attachments = { ImageFormat::BGRA8 };

		m_DefaultPass = Ref<Pass>::Create( Specification );
		m_SwapChain.CreateFramebuffers();
		
		Renderer* pRenderer = new Renderer();
		pRenderer->Init();
	}

	void VulkanContext::Terminate()
	{		
		if( m_Terminated )
			return;

		// Wait for the device to be idle, then we delete all of our vulkan items.
		VK_CHECK( vkDeviceWaitIdle( m_LogicalDevice ) );

		vkDestroyCommandPool( m_LogicalDevice, m_CommandPool, nullptr );
		vkDestroyCommandPool( m_LogicalDevice, m_ComputeCommandPool, nullptr );
		
		m_DefaultPass->Terminate();
		m_DefaultPass = nullptr;

		m_SwapChain.Terminate();
		
		for( auto& rFunc : m_TerminateResourceFuncs )
			rFunc();

		Renderer::Get().Terminate();

		ShaderLibrary::Get().Shutdown();
		
		delete m_pAllocator;

		vkDestroyDevice( m_LogicalDevice, nullptr );

#if !defined(SAT_DIST) || defined( SAT_FORCE_ENABLE_VALIDATION_LAYERS )
		delete m_pDebugMessenger;
		m_pDebugMessenger = nullptr;
#endif

		vkDestroySurfaceKHR( m_Instance, m_Surface, nullptr );
		vkDestroyInstance( m_Instance, nullptr );

		SingletonStorage::RemoveSingleton( this );

		m_Terminated = true;
	}

	void VulkanContext::CreateInstance()
	{
#if !defined( SAT_DIST ) || defined( SAT_FORCE_ENABLE_VALIDATION_LAYERS )
		SAT_CORE_ASSERT( CheckValidationLayerSupport(), "Unable to find validation layer." );
#endif

		VkApplicationInfo AppInfo  ={ VK_STRUCTURE_TYPE_APPLICATION_INFO };
		AppInfo.pApplicationName   = "Saturn Engine";
		AppInfo.pEngineName        = "Saturn Engine";
		AppInfo.applicationVersion = VK_MAKE_VERSION( 0, 0, 1 );
		AppInfo.engineVersion      = VK_MAKE_VERSION( 0, 0, 1 );
		AppInfo.apiVersion         = VK_API_VERSION_1_2;
		
		auto Extensions = Application::Get().GetWindow()->GetVulkanRequiredExtensions();
		Extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

		VkInstanceCreateInfo InstanceInfo ={ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		InstanceInfo.pApplicationInfo = &AppInfo;

#if !defined( SAT_DIST ) || defined( SAT_FORCE_ENABLE_VALIDATION_LAYERS )
		Extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );

		{
			// Include validation layer names and count.
			InstanceInfo.enabledLayerCount = static_cast< uint32_t >( ValidationLayers.size() );
			InstanceInfo.ppEnabledLayerNames = ValidationLayers.data();

			VkValidationFeatureEnableEXT  Enabled[]  = { VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };
			VkValidationFeatureDisableEXT Disabled[] = { VK_VALIDATION_FEATURE_DISABLE_UNIQUE_HANDLES_EXT };

			VkValidationFeaturesEXT      Features{ VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
			Features.disabledValidationFeatureCount = 0;
			Features.enabledValidationFeatureCount = 0;
			Features.pDisabledValidationFeatures = nullptr;
			Features.pEnabledValidationFeatures = nullptr;

			InstanceInfo.pNext = &Features;
		}
#endif

		InstanceInfo.enabledExtensionCount = ( uint32_t ) Extensions.size();
		InstanceInfo.ppEnabledExtensionNames = Extensions.data();

		VK_CHECK( vkCreateInstance( &InstanceInfo, nullptr, &m_Instance ) );

		CreateSurface();

#if !defined(SAT_DIST) || defined( SAT_FORCE_ENABLE_VALIDATION_LAYERS )
		m_pDebugMessenger = new VulkanDebugMessenger( m_Instance );
#endif
	}

	void VulkanContext::CreateSurface()
	{
		VK_CHECK( Application::Get().GetWindow()->CreateVulkanWindowSurface( m_Instance, &m_Surface ) );
	}

	void VulkanContext::PickPhysicalDevice()
	{
		uint32_t DeviceCount = 0U;
		VK_CHECK( vkEnumeratePhysicalDevices( m_Instance, &DeviceCount, nullptr ) );

		SAT_CORE_ASSERT( DeviceCount != 0U, "No device found that supports Vulkan." ); 

		// Create a list of the physical devices.
		std::vector< VkPhysicalDevice > PhysicalDevices( DeviceCount );

		// Enumerate again but fill PhysicalDevices' data.
		VK_CHECK( vkEnumeratePhysicalDevices( m_Instance, &DeviceCount, PhysicalDevices.data() ) );

		// Query queue families and check if we can use render queue.
		for( const auto& rDevice : PhysicalDevices )
		{
			uint32_t FamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties( rDevice, &FamilyCount, 0 );

			std::vector< VkQueueFamilyProperties > QueueProps( FamilyCount );

			// Get the queue family properties again but fill QueueProps' data.
			vkGetPhysicalDeviceQueueFamilyProperties( rDevice, &FamilyCount, QueueProps.data() );

			for( uint32_t i = 0; i < FamilyCount; i++ )
			{
				if( QueueProps[ i ].queueFlags & VK_QUEUE_GRAPHICS_BIT )
				{
					// Save this bit as we need this to do graphics operations on.
					m_Indices.GraphicsFamily = i;
				}

				if( QueueProps[ i ].queueFlags & VK_QUEUE_COMPUTE_BIT )
				{
					// Save this bit as we need this to do compute operations on.
					m_Indices.ComputeFamily = i;
				}

				// Check if we can present images to the surface.
				VkBool32 PresentSupport = false;
				VK_CHECK( vkGetPhysicalDeviceSurfaceSupportKHR( rDevice, i, m_Surface, &PresentSupport ) );

				// Again save the bit as we will need it for presenting.
				if( PresentSupport )
					m_Indices.PresentFamily = i;

				if( m_Indices.Complete() )
				{
					m_PhysicalDevice = rDevice;

					break;
				}
			}
		}
		
		for ( int i = 0; i < PhysicalDevices.size(); i++ )
		{
			m_DeviceProps.push_back( {} );
			vkGetPhysicalDeviceProperties( m_PhysicalDevice, &m_DeviceProps[ i ].DeviceProps );

			SAT_CORE_INFO( "===== Vulkan Device {0} Properties ===== ", i );
			SAT_CORE_INFO( " Device Name: {0}", m_DeviceProps[ i ].DeviceProps.deviceName );
			SAT_CORE_INFO( " Driver Version: {0}", m_DeviceProps[ i ].DeviceProps.driverVersion );
			SAT_CORE_INFO( " API Version: {0}", m_DeviceProps[ i ].DeviceProps.apiVersion );

			{
				uint32_t Count;
				vkEnumerateDeviceExtensionProperties( m_PhysicalDevice, nullptr, &Count, nullptr );
				std::vector<VkExtensionProperties> Extensions( Count );
				vkEnumerateDeviceExtensionProperties( m_PhysicalDevice, nullptr, &Count, Extensions.data() );

				SAT_CORE_INFO( " Physical Device {0} has {1} extensions ", i, Count );
				SAT_CORE_INFO( "  Available extensions:" );

				for( const auto& rExtension : Extensions )
				{
					SAT_CORE_INFO( "   {0}", rExtension.extensionName );
				}
			}

		}

		SAT_CORE_INFO( "======================================== " );
	}

	void VulkanContext::CreateLogicalDevice()
	{
		const float QueuePriority = 1.0f;

		std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;

		// We only want the unique values so therefore we need to create a std::set as a set only stores unqiue values.
		// And Vulkan only wants the unique vales anyway.
		std::set<uint32_t> UniqueQueueFamilies = { 
			m_Indices.GraphicsFamily.value(), m_Indices.PresentFamily.value(), m_Indices.ComputeFamily.value() };

		for( uint32_t QueueFamily : UniqueQueueFamilies )
		{
			VkDeviceQueueCreateInfo QueueCreateInfo ={ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			QueueCreateInfo.queueFamilyIndex = QueueFamily;
			QueueCreateInfo.queueCount = 1;
			QueueCreateInfo.pQueuePriorities = &QueuePriority;
			QueueCreateInfos.push_back( QueueCreateInfo );
		}

		// Enable the device features.
		// It's very unlikely for a modern GPU to not support 'samplerAnisotropy' but just in case we check.
		VkPhysicalDeviceFeatures Features;
		vkGetPhysicalDeviceFeatures( m_PhysicalDevice, &Features );

		SAT_CORE_VERIFY( Features.samplerAnisotropy, "The GPU does not support anisotropic filtering." );

		Features.samplerAnisotropy = VK_TRUE;

#if !defined( SAT_DIST )
		DeviceExtensions.push_back( VK_EXT_DEBUG_MARKER_EXTENSION_NAME );
#endif

		VkDeviceCreateInfo DeviceInfo      ={ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		DeviceInfo.enabledExtensionCount   = ( uint32_t ) DeviceExtensions.size();
		DeviceInfo.ppEnabledExtensionNames = DeviceExtensions.data();
		DeviceInfo.pQueueCreateInfos       = QueueCreateInfos.data();
		DeviceInfo.queueCreateInfoCount    = ( uint32_t ) QueueCreateInfos.size();
		DeviceInfo.pEnabledFeatures = &Features;
		DeviceInfo.pNext = nullptr;

		VK_CHECK( vkCreateDevice( m_PhysicalDevice, &DeviceInfo, nullptr, &m_LogicalDevice ) );
		SetDebugUtilsObjectName( "Physical Device", ( uint64_t )m_LogicalDevice, VK_OBJECT_TYPE_DEVICE );

		// Assign a queue to each family.
		vkGetDeviceQueue( m_LogicalDevice, m_Indices.GraphicsFamily.value(), 0, &m_GraphicsQueue );
		vkGetDeviceQueue( m_LogicalDevice, m_Indices.PresentFamily.value(), 0, &m_PresentQueue );
		vkGetDeviceQueue( m_LogicalDevice, m_Indices.ComputeFamily.value(), 0, &m_ComputeQueue );
	}

	// Get memory type.
	uint32_t VulkanContext::GetMemoryType( uint32_t TypeFilter, VkMemoryPropertyFlags Properties ) const
	{
		VkPhysicalDeviceMemoryProperties MemProperties;
		vkGetPhysicalDeviceMemoryProperties( m_PhysicalDevice, &MemProperties );

		for( uint32_t i = 0; i < MemProperties.memoryTypeCount; i++ )
		{
			if( ( TypeFilter & ( 1 << i ) ) &&
				( MemProperties.memoryTypes[ i ].propertyFlags & Properties ) == Properties )
			{
				return i;
			}
		}

		return 0;
	}

	// Data required for the swapchain to be created
	SwapchainCreationData VulkanContext::GetSwapchainCreationData()
	{
		SwapchainCreationData Data;

		// Get surface formats.

		VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( m_PhysicalDevice, m_Surface, &Data.FormatCount, 0 ) );

		Data.SurfaceFormats = std::vector<VkSurfaceFormatKHR>( Data.FormatCount );

		VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( m_PhysicalDevice, m_Surface, &Data.FormatCount, Data.SurfaceFormats.data() ) );

		for( VkSurfaceFormatKHR& rFormat : Data.SurfaceFormats )
		{
			// Use most common format type.
			if( rFormat.format == VK_FORMAT_B8G8R8A8_UNORM && rFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
			{
				// Set context's format.
				if( m_SurfaceFormat.format != rFormat.format )
					m_SurfaceFormat = rFormat;

				Data.CurrentFormat = m_SurfaceFormat;
				break;
			}
		}

		// Query Surface Capabilities
		VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_PhysicalDevice, m_Surface, &Data.SurfaceCaps ) );

		Data.ImageCount = Data.SurfaceCaps.minImageCount + 1;

		if( Data.SurfaceCaps.maxImageCount > 0 && Data.ImageCount > Data.SurfaceCaps.maxImageCount )
		{
			Data.ImageCount = Data.SurfaceCaps.maxImageCount;
		}

		return Data;
	}

	VkSampleCountFlagBits VulkanContext::GetMaxUsableMSAASamples()
	{
		return VK_SAMPLE_COUNT_1_BIT;

		std::unreachable();

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties( m_PhysicalDevice, &props );

		VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;

		if( counts & VK_SAMPLE_COUNT_64_BIT ) { return VK_SAMPLE_COUNT_64_BIT; }
		if( counts & VK_SAMPLE_COUNT_32_BIT ) { return VK_SAMPLE_COUNT_32_BIT; }
		if( counts & VK_SAMPLE_COUNT_16_BIT ) { return VK_SAMPLE_COUNT_16_BIT; }
		if( counts & VK_SAMPLE_COUNT_8_BIT ) { return VK_SAMPLE_COUNT_8_BIT; }
		if( counts & VK_SAMPLE_COUNT_4_BIT ) { return VK_SAMPLE_COUNT_4_BIT; }
		if( counts & VK_SAMPLE_COUNT_2_BIT ) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}

	void VulkanContext::CreateSwapChain()
	{
		m_SwapChain.Create();
	}

	void VulkanContext::ResizeEvent()
	{
		GetSwapchainCreationData();
		m_SwapChain.Recreate();
		m_DefaultPass->Recreate();
	}

	bool VulkanContext::CheckValidationLayerSupport()
	{
		uint32_t LayerCount;

		vkEnumerateInstanceLayerProperties( &LayerCount, nullptr );

		std::vector<VkLayerProperties> AvailableLayers( LayerCount );

		vkEnumerateInstanceLayerProperties( &LayerCount, AvailableLayers.data() );

		// Check if all layers in VailationLayers exists in the AvailableLayers list.
		for( const char* pLayerName : ValidationLayers )
		{
			bool LayerFound = false;

			for( const auto& rLayerProps : AvailableLayers )
			{
				if( strcmp( pLayerName, rLayerProps.layerName ) == 0 )
				{
					LayerFound = true;
					break;
				}
			}

			if( !LayerFound )
			{
				return false;
			}
		}

		return true;
	}
	
	VkFormat VulkanContext::FindSupportedFormat( const std::vector<VkFormat>& Formats, VkImageTiling Tiling, VkFormatFeatureFlags Features ) const
	{
		for ( VkFormat Format : Formats )
		{
			VkFormatProperties FormatProps;
			vkGetPhysicalDeviceFormatProperties( m_PhysicalDevice, Format, &FormatProps );

			if ( ( Tiling == VK_IMAGE_TILING_LINEAR && ( FormatProps.linearTilingFeatures & Features ) == Features ) ||
				( Tiling == VK_IMAGE_TILING_OPTIMAL && ( FormatProps.optimalTilingFeatures & Features ) == Features ) )
			{
				return Format;
			}
		}

		return VK_FORMAT_UNDEFINED;
	}

	VkFormat VulkanContext::FindDepthFormat() const
	{
		return FindSupportedFormat( 
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, 
			VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
	}

	bool VulkanContext::HasStencilComponent( VkFormat Format ) const
	{
		return Format == VK_FORMAT_D32_SFLOAT_S8_UINT || Format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	bool VulkanContext::FormatLinearBlitSupported( VkFormat Format, bool source ) const
	{
		VkFormatProperties FormatProps{};
		vkGetPhysicalDeviceFormatProperties( m_PhysicalDevice, Format, &FormatProps );

		return FormatProps.linearTilingFeatures & ( source ? VK_FORMAT_FEATURE_BLIT_SRC_BIT : VK_FORMAT_FEATURE_BLIT_DST_BIT );
	}

	bool VulkanContext::FormatOptimalBlitSupported( VkFormat Format, bool source ) const
	{
		VkFormatProperties FormatProps{};
		vkGetPhysicalDeviceFormatProperties( m_PhysicalDevice, Format, &FormatProps );

		return FormatProps.optimalTilingFeatures & ( source ? VK_FORMAT_FEATURE_BLIT_SRC_BIT : VK_FORMAT_FEATURE_BLIT_DST_BIT );
	}

	VkCommandBuffer VulkanContext::BeginSingleTimeCommands() const
	{
		VkCommandBufferAllocateInfo AllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		AllocInfo.commandPool = m_CommandPool;
		AllocInfo.commandBufferCount = 1;
		
		VkCommandBuffer CommandBuffer;
		VK_CHECK( vkAllocateCommandBuffers( m_LogicalDevice, &AllocInfo, &CommandBuffer ) );
		
		// Begin the command buffer.
		VkCommandBufferBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		
		VK_CHECK( vkBeginCommandBuffer( CommandBuffer, &BeginInfo ) );
		
		return CommandBuffer;
	}

	void VulkanContext::EndSingleTimeCommands( VkCommandBuffer CommandBuffer ) const
	{
		constexpr uint64_t FENCE_TIMEOUT = 100000000000;

		VK_CHECK( vkEndCommandBuffer( CommandBuffer ) );

		// Submit the command buffer.
		VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &CommandBuffer;

		VkFenceCreateInfo FenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		FenceCreateInfo.flags = 0;

		VkFence Fence;
		VK_CHECK( vkCreateFence( m_LogicalDevice, &FenceCreateInfo, nullptr, &Fence ) );

		VK_CHECK( vkQueueSubmit( m_GraphicsQueue, 1, &SubmitInfo, Fence ) );

		VK_CHECK( vkWaitForFences( m_LogicalDevice, 1, &Fence, VK_TRUE, FENCE_TIMEOUT ) );

		// Free the command buffer.
		vkDestroyFence( m_LogicalDevice, Fence, nullptr );
		vkFreeCommandBuffers( m_LogicalDevice, m_CommandPool, 1, &CommandBuffer );
	}

	VkCommandBuffer VulkanContext::BeginNewCommandBuffer() const
	{
		VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.commandPool = m_CommandPool;
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocateInfo.commandBufferCount = 1;

		VkCommandBuffer CommandBuffer;
		VK_CHECK( vkAllocateCommandBuffers( m_LogicalDevice, &cmdBufAllocateInfo, &CommandBuffer ) );

		// If requested, also start the new command buffer
		VkCommandBufferBeginInfo cmdBufferBeginInfo{};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		VK_CHECK( vkBeginCommandBuffer( CommandBuffer, &cmdBufferBeginInfo ) );

		return CommandBuffer;
	}

	VkCommandBuffer VulkanContext::CreateComputeCommandBuffer() const
	{
		VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.commandPool = m_ComputeCommandPool;
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocateInfo.commandBufferCount = 1;

		VkCommandBuffer CommandBuffer;
		VK_CHECK( vkAllocateCommandBuffers( m_LogicalDevice, &cmdBufAllocateInfo, &CommandBuffer ) );

		// If requested, also start the new command buffer
		VkCommandBufferBeginInfo cmdBufferBeginInfo{};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		VK_CHECK( vkBeginCommandBuffer( CommandBuffer, &cmdBufferBeginInfo ) );

		return CommandBuffer;
	}

	void VulkanContext::CreateCommandPool()
	{
		VkCommandPoolCreateInfo PoolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		PoolInfo.queueFamilyIndex = m_Indices.GraphicsFamily.value();
		PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		
		VK_CHECK( vkCreateCommandPool( m_LogicalDevice, &PoolInfo, nullptr, &m_CommandPool ) );

		PoolInfo.queueFamilyIndex = m_Indices.ComputeFamily.value();
		VK_CHECK( vkCreateCommandPool( m_LogicalDevice, &PoolInfo, nullptr, &m_ComputeCommandPool) );

		SetDebugUtilsObjectName( "Context Command Pool", (uint64_t)m_CommandPool, VK_OBJECT_TYPE_COMMAND_POOL );
		SetDebugUtilsObjectName( "Context Compute Command Pool", (uint64_t)m_CommandPool, VK_OBJECT_TYPE_COMMAND_POOL );
	}

}
