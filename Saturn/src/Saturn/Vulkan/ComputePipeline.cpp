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
#include "ComputePipeline.h"

#include "VulkanContext.h"
#include "VulkanDebug.h"

namespace Saturn {

	static VkFence s_ComputeFence;

	ComputePipeline::ComputePipeline( Ref<Shader> ComputeShader )
		: m_ComputeShader( ComputeShader )
	{
		// TODO: Check if shader is really a compute shader.
		Create();
	}

	ComputePipeline::~ComputePipeline()
	{
		vkDestroyPipeline( VulkanContext::Get().GetDevice(), m_Pipeline, nullptr );
		vkDestroyPipelineLayout( VulkanContext::Get().GetDevice(), m_PipelineLayout, nullptr );
	}

	void ComputePipeline::Bind()
	{
		m_CommandBuffer = VulkanContext::Get().CreateComputeCommandBuffer();
		m_UseGraphicsQueue = false;

		vkCmdBindPipeline( m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline );
	}

	void ComputePipeline::BindWithCommandBuffer( VkCommandBuffer CommandBuffer )
	{
		if( CommandBuffer )
		{
			m_CommandBuffer = CommandBuffer;
			m_UseGraphicsQueue = true;
		}

		vkCmdBindPipeline( m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline );
	}

	void ComputePipeline::Execute( Ref<Material> material, uint32_t X, uint32_t Y, uint32_t Z )
	{
		material->Bind( m_CommandBuffer, m_PipelineLayout, {}, VK_PIPELINE_BIND_POINT_COMPUTE );

		Buffer pcBuffer = material->GetPushConstantData();
		if( pcBuffer.Size )
		{
			vkCmdPushConstants( m_CommandBuffer, m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, (uint32_t) pcBuffer.Size, pcBuffer.Data );
		}
		
		vkCmdDispatch( m_CommandBuffer, X, Y, Z );
	}

	void ComputePipeline::Unbind()
	{
		auto LogicalDevice = VulkanContext::Get().GetDevice();

		auto ComputeQueue = VulkanContext::Get().GetComputeQueue();

		if( m_UseGraphicsQueue )
			return;

		vkEndCommandBuffer( m_CommandBuffer );

		if( !s_ComputeFence )
		{
			VkFenceCreateInfo FenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
			FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			VK_CHECK( vkCreateFence( LogicalDevice, &FenceCreateInfo, nullptr, &s_ComputeFence ) );
		}

		vkWaitForFences( LogicalDevice, 1, &s_ComputeFence, VK_TRUE, UINT64_MAX );
		vkResetFences( LogicalDevice, 1, &s_ComputeFence );

		VkSubmitInfo Info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		Info.commandBufferCount = 1;
		Info.pCommandBuffers = &m_CommandBuffer;

		VK_CHECK( vkQueueSubmit( ComputeQueue, 1, &Info, s_ComputeFence ) );

		// Wait for shader execution.
		vkWaitForFences( LogicalDevice, 1, &s_ComputeFence, VK_TRUE, UINT64_MAX );

		m_CommandBuffer = nullptr;

		vkDestroyFence( VulkanContext::Get().GetDevice(), s_ComputeFence, nullptr );
		s_ComputeFence = nullptr;
	}

	void ComputePipeline::Create()
	{
		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		if( m_ComputeShader->GetPushConstantRanges().size() > 0 ) 
		{
			PipelineLayoutCreateInfo.pushConstantRangeCount = (uint32_t)m_ComputeShader->GetPushConstantRanges().size();
			PipelineLayoutCreateInfo.pPushConstantRanges = m_ComputeShader->GetPushConstantRanges().data();
		}

		const auto layouts = m_ComputeShader->GetSetLayouts();

		PipelineLayoutCreateInfo.setLayoutCount = (uint32_t) layouts.size();
		PipelineLayoutCreateInfo.pSetLayouts = layouts.data();

		VK_CHECK( vkCreatePipelineLayout( VulkanContext::Get().GetDevice(), &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout ) );

		// Create the shader module.
		auto& SpvSrc = ShaderLibrary::Get().Find( m_ComputeShader->GetName() )->GetSpvCode();
		std::vector<uint32_t> Code = SpvSrc.at( { ShaderType::Compute, 0 } );

		VkShaderModuleCreateInfo ShaderModuleCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		ShaderModuleCreateInfo.codeSize = 4 * Code.size();
		ShaderModuleCreateInfo.pCode = ( uint32_t* ) Code.data();

		VkShaderModule ShaderModule = VK_NULL_HANDLE;

		VK_CHECK( vkCreateShaderModule( VulkanContext::Get().GetDevice(), &ShaderModuleCreateInfo, nullptr, &ShaderModule ) );

		std::string debugName = m_ComputeShader->GetName() + "/Compute/X0";
		SetDebugUtilsObjectName( debugName.c_str(), ( uint64_t ) ShaderModule, VK_OBJECT_TYPE_SHADER_MODULE );

		// Shader stage
		VkPipelineShaderStageCreateInfo ShaderStage = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = ShaderModule,
			.pName = "main" 
		};

		// Pipeline info
		VkComputePipelineCreateInfo ComputePipelineCreateInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
		ComputePipelineCreateInfo.layout = m_PipelineLayout;
		ComputePipelineCreateInfo.stage = ShaderStage;

		VK_CHECK( vkCreateComputePipelines( VulkanContext::Get().GetDevice(), nullptr, 1, &ComputePipelineCreateInfo, nullptr, &m_Pipeline ) );

		vkDestroyShaderModule( VulkanContext::Get().GetDevice(), ShaderModule, nullptr );

		std::string pipelineName = m_ComputeShader->GetName() + "/ComputePipeline";
		SetDebugUtilsObjectName( pipelineName.c_str(), ( uint64_t ) m_Pipeline, VK_OBJECT_TYPE_PIPELINE );
	}

}