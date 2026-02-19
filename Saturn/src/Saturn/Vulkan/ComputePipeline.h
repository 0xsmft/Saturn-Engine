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

#include "Shader.h"
#include "Material.h"

#include <vulkan.h>
#include <vector>

namespace Saturn {

	class ComputePipeline : public RefTarget
	{
	public:
		ComputePipeline( Ref<Shader> ComputeShader );
		~ComputePipeline();

		void Bind();
		void Unbind();

		// Bind the pipeline to a command buffer using the graphics queue.
		void BindWithCommandBuffer( VkCommandBuffer CommandBuffer );

		// Bind using the compute queue.
		void Execute( Ref<Material> material, uint32_t X, uint32_t Y, uint32_t Z );

		VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; }
		VkPipelineLayout GetLayout() const { return m_PipelineLayout; }

	private:
		void Create();

	private:
		VkPipeline m_Pipeline = nullptr;
		VkPipelineLayout m_PipelineLayout = nullptr;

		Ref<Shader> m_ComputeShader;

		VkCommandBuffer m_CommandBuffer = nullptr;

		bool m_UseGraphicsQueue = false;
	};
}