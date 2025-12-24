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
		
		m_TextVertexBase.resize( MAX_FRAMES_IN_FLIGHT );
		m_TextVertexBuffers.resize( MAX_FRAMES_IN_FLIGHT );

		for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
		{
			m_VertexBuffers[ i ] = Ref<VertexBuffer>::Create( s_MaxVertices * sizeof( AluraVertex ) );
			m_VertexBase[ i ] = new AluraVertex[ s_MaxVertices ];

			m_TextVertexBuffers[ i ] = Ref<VertexBuffer>::Create( s_MaxVertices * sizeof( AluraTextVertex ) );
			m_TextVertexBase[ i ] = new AluraTextVertex[ s_MaxVertices ];

#if !defined(SAT_DIST)
			m_VertexBuffers[ i ]->SetDebugName( std::format( "AluraQuadVB/{0}", i ) );
			m_TextVertexBuffers[ i ]->SetDebugName( std::format( "AluraTxtVB/{0}", i ) );
#endif
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
		m_TextIndexBuffer = Ref<IndexBuffer>::Create( pQuadBuffer, s_MaxIndices );
		delete[] pQuadBuffer;

		m_Textures[ 0 ] = Renderer::Get().GetPinkTexture();
	}

	void AluraRenderer::InitPhase2()
	{
		if( !m_Shader )
		{
			m_Shader = ShaderLibrary::Get().FindOrLoad( "Primitives2D", "content/shaders/Primitives2D.glsl" );
			m_Material = Ref<Material>::Create( m_Shader, "Primitives2DMaterial" );
		}

		if( !m_TextShader )
		{
			m_TextShader = ShaderLibrary::Get().FindOrLoad( "MsdfText", "content/shaders/MsdfText.glsl" );
			m_TextMaterial = Ref<Material>::Create( m_TextShader, "MsdfTextMaterial" );
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

		// Text pipeline
		PipelineSpec.Name = "Alura/Text";
		PipelineSpec.Shader = m_TextShader;
		PipelineSpec.VertexLayout = {
			{ ShaderDataType::Float2, "a_Position" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float, "a_TexIndex" },
		};

		m_TextPipeline = Ref<Pipeline>::Create( PipelineSpec );
	}

	void AluraRenderer::Terminate()
	{
		m_VertexBuffers.clear();
		for( auto buffer : m_VertexBase )
		{
			delete[] buffer;
		}

		m_TextVertexBuffers.clear();
		for( auto buffer : m_TextVertexBase )
		{
			delete[] buffer;
		}

		for( auto& texture : m_Textures )
		{
			texture.Reset();
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
		
		m_QuadVertexCount = 0;
		m_QuadIndexCount = 0;
		m_pVertexPtr = m_VertexBase[ frame ];

		m_TextIndexCount = 0;
		m_pTextVertexPtr = m_TextVertexBase[ frame ];

		m_CurrentTextureSlot = 1;
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
		Viewport.y = 0;
		Viewport.width = ( float ) m_Width;
		Viewport.height = ( float ) m_Height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		const VkRect2D Scissor = { .offset = { 0,0 }, .extent = Extent };

		vkCmdSetScissor( m_CommandBuffer, 0, 1, &Scissor );
		vkCmdSetViewport( m_CommandBuffer, 0, 1, &Viewport );

		const uint32_t frame = Renderer::Get().GetCurrentFrame();

		const auto projection = glm::ortho( 0.0f, ( float ) m_Width, ( float ) m_Height, 0.0f );

		const uint32_t dataSize = ( uint32_t ) ( ( uint8_t* ) m_pVertexPtr - ( uint8_t* ) m_VertexBase[ frame ] );
		if( dataSize >= 1 )
		{
			m_VertexBuffers[ frame ]->Reallocate( m_VertexBase[ frame ], dataSize );
			m_VertexBuffers[ frame ]->Bind( m_CommandBuffer );
			m_IndexBuffer->Bind( m_CommandBuffer );

			m_Pipeline->Bind( m_CommandBuffer );

			m_Material->SetPC( "u_Transform.Projection", projection );
			m_Material->SetResource( "u_InputTexture", Renderer::Get().GetPinkTexture() );

			m_Material->Bind( m_CommandBuffer, m_Pipeline->GetPipelineLayout(), {} );

			vkCmdPushConstants( m_CommandBuffer, m_Pipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, ( uint32_t ) m_Material->GetPushConstantData().Size, m_Material->GetPushConstantData().Data );

			vkCmdDrawIndexed( m_CommandBuffer, m_QuadIndexCount, 1, 0, 0, 0 );
		}

		const uint32_t textDataSize = ( uint32_t ) ( ( uint8_t* ) m_pTextVertexPtr - ( uint8_t* ) m_TextVertexBase[ frame ] );
		if( textDataSize >= 1 )
		{
			m_TextVertexBuffers[ frame ]->Reallocate( m_TextVertexBase[ frame ], textDataSize );
			m_TextVertexBuffers[ frame ]->Bind( m_CommandBuffer );
			m_TextIndexBuffer->Bind( m_CommandBuffer );

			for( uint32_t i = 0; i < m_Textures.size(); i++ )
			{
				if( m_Textures[ i ] )
					m_TextMaterial->SetResource( "u_FontAtlases", m_Textures[ i ], i );
				else
					m_TextMaterial->SetResource( "u_FontAtlases", Renderer::Get().GetPinkTexture(), i );
			}
			
			m_TextMaterial->SetPC( "u_Transform.Projection", projection );

			m_TextMaterial->Bind( m_CommandBuffer, m_TextPipeline->GetPipelineLayout(), {} );
			m_TextPipeline->Bind( m_CommandBuffer );

			vkCmdPushConstants( m_CommandBuffer, m_TextPipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, ( uint32_t ) m_TextMaterial->GetPushConstantData().Size, m_TextMaterial->GetPushConstantData().Data );

			vkCmdDrawIndexed( m_CommandBuffer, m_TextIndexCount, 1, 0, 0, 0 );
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

		m_QuadVertexCount += 4;
		m_QuadIndexCount += 6;
	}

	void AluraRenderer::SubmitText( const glm::vec2& rMin, const glm::vec2& rMax, const glm::vec2& rTexCoordMin, const glm::vec2& rTexCoordMax, const glm::vec4& rColor, Ref<Texture2D> atlasTexture, const glm::vec2& rCursorPos )
	{
		int textureID = 0;
		for( uint32_t i = 1; i < m_CurrentTextureSlot; ++i )
		{
			if( m_Textures[ i ] == atlasTexture )
			{
				textureID = i;
				break;
			}
		}

		if( textureID == 0 )
		{
			textureID = m_CurrentTextureSlot;
			m_Textures[ textureID ] = atlasTexture;
			++m_CurrentTextureSlot;
		}

		m_pTextVertexPtr->Position = rMin;
		m_pTextVertexPtr->Color = rColor;
		m_pTextVertexPtr->TexCoord = rTexCoordMin;
		m_pTextVertexPtr->TextureIndex = textureID;
		++m_pTextVertexPtr;
		
		m_pTextVertexPtr->Position = glm::vec2{ rMin.x, rMax.y };
		m_pTextVertexPtr->Color = rColor;
		m_pTextVertexPtr->TexCoord = { rTexCoordMin.x, rTexCoordMax.y };
		m_pTextVertexPtr->TextureIndex = textureID;
		++m_pTextVertexPtr;

		m_pTextVertexPtr->Position = rMax;
		m_pTextVertexPtr->Color = rColor;
		m_pTextVertexPtr->TexCoord = rTexCoordMax;
		m_pTextVertexPtr->TextureIndex = textureID;
		++m_pTextVertexPtr;

		m_pTextVertexPtr->Position = glm::vec2{ rMax.x, rMin.y };
		m_pTextVertexPtr->Color = rColor;
		m_pTextVertexPtr->TexCoord = { rTexCoordMax.x, rTexCoordMin.y };
		m_pTextVertexPtr->TextureIndex = textureID;
		++m_pTextVertexPtr;

		m_TextIndexCount += 6;
	}

}
