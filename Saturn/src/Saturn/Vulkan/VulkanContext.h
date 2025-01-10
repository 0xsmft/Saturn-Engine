#pragma once

#include "Base.h"
#include "Saturn/Core/App.h"
#include "Saturn/Core/Renderer/EditorCamera.h"
#include "Shader.h"

#include "IndexBuffer.h"
#include "VertexBuffer.h"

#include "SwapChain.h"
#include "Mesh.h"
#include "Pass.h"
#include "Texture.h"
#include "Pipeline.h"

#include "SingletonStorage.h"

#include <glm/glm.hpp>

#include <vulkan.h>
#include <vector>
#include <optional>

namespace Saturn {

	class VulkanDebugMessenger;
	class VulkanAllocator;
	
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> GraphicsFamily;
		std::optional<uint32_t> PresentFamily;
		std::optional<uint32_t> ComputeFamily;

		bool Complete() { return GraphicsFamily.has_value() && PresentFamily.has_value() && ComputeFamily.has_value(); }
	};

	struct SwapchainCreationData
	{
		uint32_t FormatCount = 0;
		uint32_t ImageCount = 0;

		std::vector<VkSurfaceFormatKHR> SurfaceFormats;

		VkSurfaceFormatKHR CurrentFormat;

		VkSurfaceCapabilitiesKHR SurfaceCaps ={};
	};

	struct PhysicalDeviceProperties
	{
		VkPhysicalDeviceProperties DeviceProps ={};
	};

	class VulkanContext
	{
	public:
		static inline VulkanContext& Get() { return *SingletonStorage::GetSingleton<VulkanContext>(); }

	public:
		VulkanContext();
		~VulkanContext() { Terminate(); }

		void Init();
		void ResizeEvent();

		uint32_t GetMemoryType( uint32_t TypeFilter, VkMemoryPropertyFlags Properties );
		
	public:

		VkFormat FindSupportedFormat( const std::vector<VkFormat>& Formats, VkImageTiling Tiling, VkFormatFeatureFlags Features );
		VkFormat FindDepthFormat();
		bool HasStencilComponent( VkFormat Format );

		bool FormatLinearBlitSupported( VkFormat Format, bool source = false );
		bool FormatOptimalBlitSupported( VkFormat Format, bool source = false );

		VkCommandBuffer BeginSingleTimeCommands();
		void EndSingleTimeCommands( VkCommandBuffer CommandBuffer );
		
		VkCommandBuffer BeginNewCommandBuffer();
		VkCommandBuffer CreateComputeCommandBuffer();

		Ref<Pass> GetDefaultPass() { return m_DefaultPass; }
		VkRenderPass GetDefaultVulkanPass() { return m_DefaultPass->GetVulkanPass(); }

	public:
		
		VkInstance GetInstance() { return m_Instance; }

		VkDevice GetDevice() { return m_LogicalDevice; }

		VkSurfaceKHR GetSurface() { return m_Surface; }
		VkSurfaceFormatKHR& GetSurfaceFormat() { return m_SurfaceFormat; }

		SwapchainCreationData GetSwapchainCreationData();

		QueueFamilyIndices& GetQueueFamilyIndices() { return m_Indices; };

		VkCommandPool GetCommandPool() { return m_CommandPool; }

		VkQueue GetGraphicsQueue() { return m_GraphicsQueue; }
		VkQueue GetPresentQueue() { return m_PresentQueue; }

		VkPhysicalDevice GetPhysicalDevice() { return m_PhysicalDevice; }

		VkQueue GetComputeQueue() { return m_ComputeQueue; }

		Swapchain& GetSwapchain() { return m_SwapChain; }

		VulkanAllocator* GetVulkanAllocator() { return m_pAllocator; }

		// "rrFunction" will be called just before the device is destroyed.
		void SubmitTerminateResource( std::function<void()>&& rrFunction ) { m_TerminateResourceFuncs.push_back( std::move( rrFunction ) ); }

		std::vector< PhysicalDeviceProperties > GetPhysicalDeviceProperties() { return m_DeviceProps; }
		std::vector< PhysicalDeviceProperties > const GetPhysicalDeviceProperties() const { return m_DeviceProps; }

		VkSampleCountFlagBits GetMaxUsableMSAASamples();

		void OnEvent( RubyEvent& e );

		VkImageView GetDepthImageView() { return m_DepthImage->GetImageView(); }
		VkImage GetDepthImage() { return m_DepthImage->GetImage(); }

	private:
		void Terminate();

		void CreateInstance();
		void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSwapChain();
		void CreateCommandPool();
		void CreateDepthResources();

		bool CheckValidationLayerSupport();

		VkInstance m_Instance = nullptr;
		VkSurfaceKHR m_Surface = nullptr;
		VkPhysicalDevice m_PhysicalDevice = nullptr;
		VkDevice m_LogicalDevice = nullptr;
		Swapchain m_SwapChain ={};
		VkDebugUtilsMessengerEXT m_DebugMessenger = nullptr;
		VkExtent2D m_SwapChainExtent = {};
		VkCommandPool m_CommandPool = nullptr;
		VkCommandPool m_ComputeCommandPool = nullptr;
		VkCommandBuffer m_CommandBuffer = nullptr;
	
		// Depth resources.
		Ref<Image2D> m_DepthImage = nullptr;

		VulkanDebugMessenger* m_pDebugMessenger;
		VulkanAllocator* m_pAllocator;

		VkQueue m_GraphicsQueue, m_PresentQueue, m_ComputeQueue;

		VkSurfaceFormatKHR m_SurfaceFormat;

		QueueFamilyIndices m_Indices;

		// Default.
		Ref<Pass> m_DefaultPass;

		bool m_Terminated = false;

		std::vector<PhysicalDeviceProperties> m_DeviceProps;
		
		std::vector<std::function<void()>> m_TerminateResourceFuncs;

		std::vector<const char*> DeviceExtensions  ={ VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME };

		std::vector<const char*> ValidationLayers ={ "VK_LAYER_KHRONOS_validation" };

	private:
		friend class Swapchain;
		friend class VulkanDebug;
		friend class Application;
	};
}