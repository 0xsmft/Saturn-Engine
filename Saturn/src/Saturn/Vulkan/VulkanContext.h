#pragma once

#include "VulkanError.h"
#include "SwapChain.h"
#include "Pass.h"

#include "VulkanAllocator.h"
#include "VulkanDebugMessenger.h"
#include "SingletonStorage.h"

#include <vulkan.h>
#include <vector>
#include <optional>

namespace Saturn {
	
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> GraphicsFamily;
		std::optional<uint32_t> PresentFamily;
		std::optional<uint32_t> ComputeFamily;

		bool Complete() const { return GraphicsFamily.has_value() && PresentFamily.has_value() && ComputeFamily.has_value(); }
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

	class Renderer;

	class VulkanContext
	{
	public:
		static inline VulkanContext* Get() { return SingletonStorage::GetSingleton<VulkanContext>(); }

	public:
		VulkanContext();
		~VulkanContext();

		void Init();
		void ResizeEvent();

		uint32_t GetMemoryType( uint32_t TypeFilter, VkMemoryPropertyFlags Properties ) const;
		
	public:
		VkFormat FindSupportedFormat( const std::vector<VkFormat>& Formats, VkImageTiling Tiling, VkFormatFeatureFlags Features ) const;

		VkFormat FindDepthFormat() const;
		bool HasStencilComponent( VkFormat Format ) const;

		bool FormatLinearBlitSupported( VkFormat Format, bool source = false ) const;
		bool FormatOptimalBlitSupported( VkFormat Format, bool source = false ) const;

		VkCommandBuffer BeginSingleTimeCommands() const;
		void EndSingleTimeCommands( VkCommandBuffer CommandBuffer );
		
		VkCommandBuffer BeginNewCommandBuffer() const;
		VkCommandBuffer CreateComputeCommandBuffer() const;
		VkCommandBuffer CreateSubCommandBuffer() const;

		Ref<Pass> GetDefaultPass() const { return m_DefaultPass; }
		VkRenderPass GetDefaultVulkanPass() const { return m_DefaultPass->GetVulkanPass(); }

		void LockQueue( bool compute = false );
		void UnlockQueue( bool compute = false );

	public:
		VkInstance GetInstance() const { return m_Instance; }

		VkDevice GetDevice() const { return m_LogicalDevice; }

		VkSurfaceKHR GetSurface() const { return m_Surface; }
		VkSurfaceFormatKHR& GetSurfaceFormat() { return m_SurfaceFormat; }

		SwapchainCreationData GetSwapchainCreationData();

		QueueFamilyIndices& GetQueueFamilyIndices() { return m_Indices; };

		VkCommandPool GetCommandPool() const { return m_CommandPool; }

		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		VkQueue GetPresentQueue() const { return m_PresentQueue; }

		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }

		VkQueue GetComputeQueue() const { return m_ComputeQueue; }

		Swapchain& GetSwapchain() { return m_SwapChain; }

		VulkanAllocator* GetVulkanAllocator() { return m_Allocator.get(); }

		// "rrFunction" will be called just before the device is destroyed.
		void SubmitTerminateResource( std::function<void()>&& rrFunction ) { m_TerminateResourceFuncs.push_back( std::move( rrFunction ) ); }

		std::vector< PhysicalDeviceProperties > GetPhysicalDeviceProperties() { return m_DeviceProps; }
		std::vector< PhysicalDeviceProperties > const GetPhysicalDeviceProperties() const { return m_DeviceProps; }

		VkSampleCountFlagBits GetMaxUsableMSAASamples();

	private:
		void Terminate();

		void CreateInstance();
		void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSwapChain();
		void CreateCommandPool();

		bool CheckValidationLayerSupport();

	private:
		VkInstance m_Instance = nullptr;
		VkSurfaceKHR m_Surface = nullptr;
		VkPhysicalDevice m_PhysicalDevice = nullptr;
		VkDevice m_LogicalDevice = nullptr;
		Swapchain m_SwapChain ={};
		VkExtent2D m_SwapChainExtent = {};
		VkCommandPool m_CommandPool = nullptr;
		VkCommandPool m_ComputeCommandPool = nullptr;
		VkCommandBuffer m_CommandBuffer = nullptr;

		std::unique_ptr<VulkanDebugMessenger> m_DebugMessenger;
		std::unique_ptr<VulkanAllocator> m_Allocator;
		std::unique_ptr<class Renderer> m_Renderer;

		VkQueue m_GraphicsQueue = nullptr, m_PresentQueue = nullptr, m_ComputeQueue = nullptr;

		VkSurfaceFormatKHR m_SurfaceFormat{ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_MAX_ENUM_KHR };

		QueueFamilyIndices m_Indices;

		// Default.
		Ref<Pass> m_DefaultPass;

		bool m_Terminated = false;

		std::vector<PhysicalDeviceProperties> m_DeviceProps;
		
		std::vector<std::function<void()>> m_TerminateResourceFuncs;

		std::vector<const char*> m_DeviceExtensions  ={ VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME };

		std::array<const char*, 1> m_ValidationLayers ={ "VK_LAYER_KHRONOS_validation" };

		std::mutex m_GraphicsQueueMutex, m_ComputeQueueMutex;

	private:
		friend class Swapchain;
		friend class VulkanDebug;
		friend class Application;
	};
}
