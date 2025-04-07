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
#include "VulkanDebugMessenger.h"

// Vulkan Message callback
static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCB(
	VkDebugUtilsMessageSeverityFlagBitsEXT             MessageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT                    MessageType,
	const VkDebugUtilsMessengerCallbackDataEXT*        pCallbackData,
	void*                                              pUserData )
{
	if( ( MessageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ) != 0 )
		SAT_CORE_ERROR( "{0}", pCallbackData->pMessage );
	else if( ( MessageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ) != 0 )
		SAT_CORE_WARN( "{0}", pCallbackData->pMessage );
//	else if( ( MessageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT ) != 0 )
//		SAT_CORE_INFO( "Vulkan Information: {0}", pCallbackData->pMessage );

	return VK_FALSE;
}

namespace Saturn {

	VulkanDebugMessenger::VulkanDebugMessenger( VkInstance instance )
	{
		VkDebugUtilsMessengerCreateInfoEXT CreateInfo;
		CreateDebugMessengerInfo( &CreateInfo );

		m_pCreateFunc = ( PFN_vkCreateDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );

		m_pDestroyFunc = ( PFN_vkDestroyDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );

		X31_vkCreateDebugUtilsMessenger( instance, &CreateInfo, nullptr, &m_DebugMessenger );
	}

	VulkanDebugMessenger::~VulkanDebugMessenger()
	{
		X31_vkDestroyDebugUtilsMessenger( VulkanContext::Get().GetInstance(), m_DebugMessenger, nullptr );

		m_pCreateFunc = nullptr;
		m_pDestroyFunc = nullptr;
	}

	//////////////////////////////////////////////////////////////////////////
	// Internal functions

	void VulkanDebugMessenger::CreateDebugMessengerInfo( VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo )
	{
		VkDebugUtilsMessengerCreateInfoEXT TempCreateInfo ={ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

		TempCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		TempCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		TempCreateInfo.pfnUserCallback = vkDebugCB;
		TempCreateInfo.pUserData = nullptr;

		*pCreateInfo = TempCreateInfo;
	}

	VkResult VulkanDebugMessenger::X31_vkCreateDebugUtilsMessenger( VkInstance Instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger )
	{
		// Find and load the "vkCreateDebugUtilsMessengerEXT" function ptr.
		if( m_pCreateFunc != nullptr )
		{
			// Call vkCreateDebugUtilsMessengerEXT
			return ( m_pCreateFunc )( Instance, pCreateInfo, pAllocator, pDebugMessenger );
		}
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void VulkanDebugMessenger::X31_vkDestroyDebugUtilsMessenger( VkInstance Instance, VkDebugUtilsMessengerEXT DebugMessenger, const VkAllocationCallbacks* pAllocator )
	{
		if( m_pDestroyFunc )
		{
			( m_pDestroyFunc )( Instance, DebugMessenger, pAllocator );
		}
	}

}
