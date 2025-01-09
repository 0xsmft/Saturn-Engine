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
#include "VertexBuffer.h"

#include "DescriptorSet.h"

#include <vulkan.h>

namespace Saturn {
	
	class Pass;

	enum class CullMode 
	{
		None,
		Front,
		Back,
		FrontAndBack
	};

	struct RequestDescriptorSetInfo
	{
		ShaderType Stage = ShaderType::None;
		uint32_t SetIndex;
	};

	struct PipelineSpecification
	{
		PipelineSpecification() {}
		~PipelineSpecification() {}

		//////

		Ref<Shader> Shader;
		Ref<Pass> RenderPass;
		std::string Name = "Pipeline";
		VertexBufferLayout VertexLayout;
		VertexBufferLayout InstanceLayout;
		uint32_t Width = 0, Height = 0;
		bool UseDepthTest = false;
		bool UseStencilTest = false;
		bool HasColorAttachment = true;
		CullMode CullMode = CullMode::Back;
		VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		VkPolygonMode PolygonMode = VK_POLYGON_MODE_FILL;
		VkCompareOp DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		// SpecializationInfo
		VkSpecializationInfo SpecializationInfo = {};
		bool UseSpecializationInfo = false;
		ShaderType SpecializationStage = ShaderType::None;

		RequestDescriptorSetInfo RequestDescriptorSets = {};
	};

	class Pipeline : public RefTarget
	{
	public:
		Pipeline() { }
		Pipeline( const PipelineSpecification& Spec );
		~Pipeline() { Terminate(); }
		
		void Bind( VkCommandBuffer CommandBuffer );
		void Recreate();
		void Terminate();

		VkPipeline GetPipeline() { return m_Pipeline; }
		VkPipelineLayout GetPipelineLayout() { return m_PipelineLayout; }
		
		operator VkPipeline() const { return m_Pipeline; }

		Ref<Shader>& GetShader() { return m_Specification.Shader; }

		[[ nodiscard ]] Ref<DescriptorSet>& GetDescriptorSet( ShaderType Stage, uint32_t SetIndex ) { return m_DescriptorSets[Stage][SetIndex]; }
		
		VkDescriptorSet GetVulkanSet( ShaderType Stage, uint32_t SetIndex ) { return m_DescriptorSets[Stage][SetIndex]->GetVulkanSet(); }

	private:
		void Create();

		PipelineSpecification m_Specification = {};
		
		// STAGE -> SET INDEX -> DESCRIPTOR SET
		std::unordered_map< ShaderType, std::unordered_map< uint32_t, Ref< DescriptorSet > > > m_DescriptorSets;

		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
	};
}