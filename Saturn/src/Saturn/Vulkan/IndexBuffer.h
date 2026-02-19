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

#include "VulkanAllocator.h"

#include <vulkan.h>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Saturn {

	// A face in a mesh
	// Defines three vertices which form a triangle.
	struct Index
	{
		uint32_t V1, V2, V3;
	};

	class IndexBuffer : public RefTarget
	{
	public:
		IndexBuffer() { }
		IndexBuffer( void* pData, size_t Size );
		~IndexBuffer();

		size_t GetSize() const { return m_Size; }

		void Bind( VkCommandBuffer CommandBuffer );
		void Draw( VkCommandBuffer CommandBuffer );

		void Destroy();
		
	private:
		void CreateBuffer();
		
	private:

		void* m_pData = nullptr;
		size_t m_Size = 0;

		VkBuffer m_Buffer = nullptr ;
		VmaAllocation m_Allocation = nullptr;
	};
}