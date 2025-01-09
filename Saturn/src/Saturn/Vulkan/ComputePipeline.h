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

#pragma once

#include "Shader.h"

#include <vulkan.h>
#include <vector>

namespace Saturn {

	class ComputePipeline : public RefTarget
	{
	public:
		ComputePipeline( Ref<Shader> ComputeShader );
		~ComputePipeline();

		void Bind();
		void BindWithCommandBuffer( VkCommandBuffer CommandBuffer );

		void Execute( VkDescriptorSet DescriptorSet, uint32_t X, uint32_t Y, uint32_t Z );

		void AddPushConstant( const void* pData, uint32_t Offset = 0, size_t Size = 1 );

		void Unbind();

		VkCommandBuffer GetCommandBuffer() { return m_CommandBuffer; }

	private:
		void Create();

	private:
		VkPipeline m_Pipeline;
		VkPipelineLayout m_PipelineLayout;

		Ref<Shader> m_ComputeShader;

		bool m_UseGraphicsQueue = false;

		VkCommandBuffer m_CommandBuffer;
	};
}