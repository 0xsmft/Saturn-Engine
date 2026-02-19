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

#include "VulkanContext.h"

namespace Saturn {

	inline void SetDebugUtilsObjectName( VkDebugUtilsObjectNameInfoEXT* pInfo )
	{
		PFN_vkSetDebugUtilsObjectNameEXT Function = ( PFN_vkSetDebugUtilsObjectNameEXT )vkGetInstanceProcAddr( VulkanContext::Get()->GetInstance(), "vkSetDebugUtilsObjectNameEXT" );

		if ( Function )
		{
			Function( VulkanContext::Get()->GetDevice(), pInfo );
		}
	}

	inline void SetDebugUtilsObjectName( const std::string& Name, uint64_t Handle, VkObjectType ObjectType )
	{
		PFN_vkSetDebugUtilsObjectNameEXT Function = ( PFN_vkSetDebugUtilsObjectNameEXT )vkGetInstanceProcAddr( VulkanContext::Get()->GetInstance(), "vkSetDebugUtilsObjectNameEXT" );

		VkDebugUtilsObjectNameInfoEXT Info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		Info.objectHandle = ( uint64_t )Handle;
		Info.objectType = ObjectType;
		Info.pObjectName = Name.c_str();

		if( Function )
		{
			Function( VulkanContext::Get()->GetDevice(), &Info );
		}
	}

	inline void SetDebugUtilsObjectName( const char* pName, uint64_t Handle, VkObjectType ObjectType )
	{
		PFN_vkSetDebugUtilsObjectNameEXT Function = ( PFN_vkSetDebugUtilsObjectNameEXT )vkGetInstanceProcAddr( VulkanContext::Get()->GetInstance(), "vkSetDebugUtilsObjectNameEXT" );

		VkDebugUtilsObjectNameInfoEXT Info ={ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		Info.objectHandle = ( uint64_t )Handle;
		Info.objectType = ObjectType;
		Info.pObjectName = pName;

		if( Function )
		{
			Function( VulkanContext::Get()->GetDevice(), &Info );
		}
	}

	inline void SetDebugUtilsObjectTag( VkDebugUtilsObjectTagInfoEXT* pInfo )
	{
		PFN_vkSetDebugUtilsObjectTagEXT Function = ( PFN_vkSetDebugUtilsObjectTagEXT )vkGetInstanceProcAddr( VulkanContext::Get()->GetInstance(), "vkSetDebugUtilsObjectTagEXT" );

		if( Function )
		{
			Function( VulkanContext::Get()->GetDevice(), pInfo );
		}
	}

	inline void SetDebugUtilsObjectTag( uint64_t Tag, uint64_t Handle, VkObjectType ObjectType )
	{
		PFN_vkSetDebugUtilsObjectTagEXT Function = ( PFN_vkSetDebugUtilsObjectTagEXT )vkGetInstanceProcAddr( VulkanContext::Get()->GetInstance(), "vkSetDebugUtilsObjectTagEXT" );

		VkDebugUtilsObjectTagInfoEXT Info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT };
		Info.objectHandle = ( uint64_t )Handle;
		Info.objectType = ObjectType;
		Info.tagName = Tag;

		if( Function )
		{
			Function( VulkanContext::Get()->GetDevice(), &Info );
		}
	}

	inline void CmdBeginDebugLabel( VkCommandBuffer ComamndBuffer, const std::string& Name )
	{
		PFN_vkCmdBeginDebugUtilsLabelEXT Function = ( PFN_vkCmdBeginDebugUtilsLabelEXT ) vkGetDeviceProcAddr( VulkanContext::Get()->GetDevice(), "vkCmdBeginDebugUtilsLabelEXT" );

		VkDebugUtilsLabelEXT Info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
		Info.pLabelName = Name.c_str();
		
		if ( Function )
		{
			Function( ComamndBuffer, &Info );
		}
	}

	inline void CmdEndDebugLabel( VkCommandBuffer ComamndBuffer )
	{
		PFN_vkCmdEndDebugUtilsLabelEXT Function = ( PFN_vkCmdEndDebugUtilsLabelEXT ) vkGetDeviceProcAddr( VulkanContext::Get()->GetDevice(), "vkCmdEndDebugUtilsLabelEXT" );
		
		if( Function )
		{
			Function( ComamndBuffer );
		}
	}

	inline void CmdDebugMarkerBegin( VkCommandBuffer CommandBuffer, VkDebugMarkerMarkerInfoEXT* pMarkerInfo )
	{
		PFN_vkCmdDebugMarkerBeginEXT Function = ( PFN_vkCmdDebugMarkerBeginEXT )vkGetDeviceProcAddr( VulkanContext::Get()->GetDevice(), "vkCmdDebugMarkerBeginEXT" );

		if( Function )
		{
			Function( CommandBuffer, pMarkerInfo );
		}
	}

	inline void CmdDebugMarkerEnd( VkCommandBuffer CommandBuffer )
	{
		PFN_vkCmdDebugMarkerEndEXT Function = ( PFN_vkCmdDebugMarkerEndEXT )vkGetDeviceProcAddr( VulkanContext::Get()->GetDevice(), "vkCmdDebugMarkerEndEXT" );

		if( Function )
		{
			Function( CommandBuffer );
		}
	}
}