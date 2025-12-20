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
#include "AluraRenderer.h"

#include "Renderer.h"
#include "VulkanDebug.h"

#include "Saturn/Core/Ruby/RubyWindow.h"

namespace Saturn {

	static constexpr uint32_t s_MaxQuads = 20000u;
	static constexpr uint32_t s_MaxVertices = s_MaxQuads * 4u;
	static constexpr uint32_t s_MaxIndices = s_MaxQuads * 6u;

	AluraRenderer::AluraRenderer()
	{
		m_Width = Application::Get().GetWindow()->GetWidth();
		m_Height = Application::Get().GetWindow()->GetHeight();
	}

	AluraRenderer::~AluraRenderer()
	{
		Terminate();
	}

	void AluraRenderer::Init( Ref<Pass> targetPass, Ref<Framebuffer> targetFramebuffer )
	{
		m_TargetRenderPass = targetPass;
		m_TargetFramebuffer = targetFramebuffer;

		InitBuffers();
		InitPhase2();
	}

	void AluraRenderer::InitBuffers()
	{
		m_VertexBuffers.resize( MAX_FRAMES_IN_FLIGHT );
		m_VertexBase.resize( MAX_FRAMES_IN_FLIGHT );

		for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
		{
			m_VertexBuffers[ i ] = Ref<VertexBuffer>::Create( s_MaxVertices * sizeof( AluraVertex ) );
			m_VertexBase[ i ] = new AluraVertex[ s_MaxVertices ];
		}

		uint32_t* pQuadBuffer = new uint32_t[ s_MaxIndices ];
		uint32_t offset = 0;
		for( size_t i = 0; i < s_MaxIndices; i += 6 )
		{
			pQuadBuffer[ i + 0 ] = offset + 0;
			pQuadBuffer[ i + 1 ] = offset + 1;
			pQuadBuffer[ i + 2 ] = offset + 2;

			pQuadBuffer[ i + 3 ] = offset + 2;
			pQuadBuffer[ i + 4 ] = offset + 3;
			pQuadBuffer[ i + 5 ] = offset + 0;

			offset += 4;
		}

		m_IndexBuffer = Ref<IndexBuffer>::Create( pQuadBuffer, s_MaxIndices );
		delete[] pQuadBuffer;
	}

	void AluraRenderer::InitPhase2()
	{
		if( !m_Shader )
		{
			m_Shader = ShaderLibrary::Get().FindOrLoad( "Primitives2D", "content/shaders/Primitives2D.glsl" );
			m_Material = Ref<Material>::Create( m_Shader, "Primitives2DMaterial" );
		}

		PipelineSpecification PipelineSpec{};
		PipelineSpec.Width = m_Width;
		PipelineSpec.Height = m_Height;
		PipelineSpec.Name = "Alura";
		PipelineSpec.Shader = m_Shader;
		PipelineSpec.RenderPass = m_TargetRenderPass;
		PipelineSpec.CullMode = CullMode::None;
		PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
		PipelineSpec.UseDepthTest = false;
		PipelineSpec.VertexLayout = {
			{ ShaderDataType::Float2, "a_Position" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float4, "a_Color" },
		};

		m_Pipeline = Ref<Pipeline>::Create( PipelineSpec );
	}

	void AluraRenderer::Terminate()
	{
		m_VertexBuffers.clear();
		for( auto buffer : m_VertexBase )
		{
			delete[] buffer;
		}
	}

	void AluraRenderer::SetViewportSize( uint32_t w, uint32_t h )
	{
		if( m_Width != w || m_Height != h )
		{
			m_Width = w;
			m_Height = h;
			m_Resized = true;
		}
	}

	void AluraRenderer::PreRender()
	{
		const uint32_t frame = Renderer::Get().GetCurrentFrame();
		m_VertexCount = 0;
		m_pVertexPtr = m_VertexBase[ frame ];

		m_IndexCount = 0;
	}

	void AluraRenderer::Render()
	{
		if( m_Resized )
		{
			OnResize();
			m_Resized = false;
		}

		m_CommandBuffer = Renderer::Get().ActiveCommandBuffer();
		
		CmdBeginDebugLabel( m_CommandBuffer, "Late Composite/Alura Pass" );
		{
			RenderProper();
		}
		CmdEndDebugLabel( m_CommandBuffer );
	}

	void AluraRenderer::EndFrame()
	{
	}

	void AluraRenderer::OnResize()
	{
		InitPhase2();
	}

	void AluraRenderer::RenderProper()
	{
		const VkExtent2D Extent = { m_Width, m_Height };
		
		m_TargetRenderPass->BeginPass( m_CommandBuffer, m_TargetFramebuffer->GetVulkanFramebuffer(), Extent );

		// Flip vulkan viewport so that we render at top-left origin.
		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = ( float ) m_Height;
		Viewport.width = ( float ) m_Width;
		Viewport.height = -( float ) m_Height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		const VkRect2D Scissor = { .offset = { 0,0 }, .extent = Extent };

		vkCmdSetScissor( m_CommandBuffer, 0, 1, &Scissor );
		vkCmdSetViewport( m_CommandBuffer, 0, 1, &Viewport );

		const uint32_t frame = Renderer::Get().GetCurrentFrame();

		const glm::vec2 scale{ 2.0f / m_Width, 2.0f / m_Height };
		const glm::vec2 translation{ -1.0f - m_Position.x * scale.x, -1.0f - m_Position.y * scale.y };

		const uint32_t dataSize = ( uint32_t ) ( ( uint8_t* ) m_pVertexPtr - ( uint8_t* ) m_VertexBase[ frame ] );
		if( dataSize >= 1 )
		{
			m_VertexBuffers[ frame ]->Reallocate( m_VertexBase[ frame ], dataSize );
			m_VertexBuffers[ frame ]->Bind( m_CommandBuffer );
			m_IndexBuffer->Bind( m_CommandBuffer );

			m_Pipeline->Bind( m_CommandBuffer );

			m_Material->SetPC( "u_Transform.Scale", scale );
			m_Material->SetPC( "u_Transform.Translate", translation );
			m_Material->SetResource( "u_InputTexture", Renderer::Get().GetPinkTexture() );

			m_Material->Bind( m_CommandBuffer, m_Pipeline->GetPipelineLayout(), {} );

			vkCmdPushConstants( m_CommandBuffer, m_Pipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, ( uint32_t ) m_Material->GetPushConstantData().Size, m_Material->GetPushConstantData().Data );

			vkCmdDrawIndexed( m_CommandBuffer, m_IndexCount, 1, 0, 0, 0 );
		}

		m_TargetRenderPass->EndPass();
	}

	void AluraRenderer::SubmitRect( const glm::vec2& rMin, const glm::vec2& rMax, const glm::vec4& rColor )
	{
		m_pVertexPtr->Position = { rMin.x, rMin.y };
		m_pVertexPtr->Color = rColor;
		m_pVertexPtr->TexCoord = glm::vec2{ 0.0f, 0.0f };		
		++m_pVertexPtr;

		m_pVertexPtr->Position = { rMax.x, rMin.y };
		m_pVertexPtr->Color = rColor;
		m_pVertexPtr->TexCoord = glm::vec2{ 1.0f, 0.0f };
		++m_pVertexPtr;

		m_pVertexPtr->Position = { rMax.x, rMax.y };
		m_pVertexPtr->Color = rColor;
		m_pVertexPtr->TexCoord = glm::vec2{ 1.0f, 1.0f };
		++m_pVertexPtr;

		m_pVertexPtr->Position = { rMin.x, rMax.y };
		m_pVertexPtr->Color = rColor;
		m_pVertexPtr->TexCoord = glm::vec2{ 0.0f, 1.0f };
		++m_pVertexPtr;

		m_VertexCount += 4;
		m_IndexCount += 6;
	}

}
