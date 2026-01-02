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

#include "Saturn/Alura/AluraMSDFData.h"
#include "Saturn/Alura/AluraFont.h"

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
			{ ShaderDataType::Float, "a_TextureIndex" },
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

		m_Projection = glm::ortho( 0.0f, ( float ) m_Width, ( float ) m_Height, 0.0f );
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
		
		// Zero because its local to its 16 texture array
		m_CurrentTextureAtlasSlot = 0;
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

		m_IndexBuffer->Bind( m_CommandBuffer );

		const uint32_t dataSize = ( uint32_t ) ( ( uint8_t* ) m_pVertexPtr - ( uint8_t* ) m_VertexBase[ frame ] );
		if( dataSize >= 1 )
		{
			m_VertexBuffers[ frame ]->Reallocate( m_VertexBase[ frame ], dataSize );
			m_VertexBuffers[ frame ]->Bind( m_CommandBuffer );

			for( uint32_t i = 0; i < 16; i++ )
			{
				if( m_Textures[ i ] )
					m_Material->SetResource( "u_InputTexture", m_Textures[ i ], i );
				else
					m_Material->SetResource( "u_InputTexture", Renderer::Get().GetPinkTexture(), i );
			}

			m_Pipeline->Bind( m_CommandBuffer );

			m_Material->SetPC( "u_Transform.Projection", m_Projection );
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

			for( uint32_t textureIndex = 17, i = 0; textureIndex < m_Textures.size(); i++, textureIndex++ )
			{
				if( m_Textures[ textureIndex ] )
					m_TextMaterial->SetResource( "u_FontAtlases", m_Textures[ textureIndex ], i );
				else
					m_TextMaterial->SetResource( "u_FontAtlases", Renderer::Get().GetPinkTexture(), i );
			}
			
			m_TextMaterial->SetPC( "u_Transform.Projection", m_Projection );

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
		m_pVertexPtr->TextureIndex = 0.0f;
		++m_pVertexPtr;

		m_pVertexPtr->Position = { rMax.x, rMin.y };
		m_pVertexPtr->Color = rColor;
		m_pVertexPtr->TexCoord = glm::vec2{ 1.0f, 0.0f };
		m_pVertexPtr->TextureIndex = 0.0f;
		++m_pVertexPtr;

		m_pVertexPtr->Position = { rMax.x, rMax.y };
		m_pVertexPtr->Color = rColor;
		m_pVertexPtr->TexCoord = glm::vec2{ 1.0f, 1.0f };
		m_pVertexPtr->TextureIndex = 0.0f;
		++m_pVertexPtr;

		m_pVertexPtr->Position = { rMin.x, rMax.y };
		m_pVertexPtr->Color = rColor;
		m_pVertexPtr->TexCoord = glm::vec2{ 0.0f, 1.0f };
		m_pVertexPtr->TextureIndex = 0.0f;
		++m_pVertexPtr;

		m_QuadVertexCount += 4;
		m_QuadIndexCount += 6;
	}

	void AluraRenderer::SubmitRect( const glm::vec2& rMin, const glm::vec2& rMax, Ref<Texture2D> texture, const glm::vec4& rColor, const glm::vec2& rUV1, const glm::vec2& rUV2 )
	{
		int textureID = 0;
		for( uint32_t i = 1; i < m_CurrentTextureSlot; ++i )
		{
			if( m_Textures[ i ] == texture )
			{
				textureID = i;
				break;
			}
		}

		if( textureID == 0 )
		{
			textureID = m_CurrentTextureSlot;
			m_Textures[ textureID ] = texture;
			++m_CurrentTextureSlot;
		}

		m_pVertexPtr->Position = { rMin.x, rMin.y };
		m_pVertexPtr->Color = rColor;
		m_pVertexPtr->TexCoord = glm::vec2{ rUV1.x, rUV1.y };
		m_pVertexPtr->TextureIndex = ( float ) textureID;
		++m_pVertexPtr;

		m_pVertexPtr->Position = { rMax.x, rMin.y };
		m_pVertexPtr->Color = rColor;
		m_pVertexPtr->TexCoord = glm::vec2{ rUV2.x, rUV1.y };
		m_pVertexPtr->TextureIndex = ( float ) textureID;
		++m_pVertexPtr;

		m_pVertexPtr->Position = { rMax.x, rMax.y };
		m_pVertexPtr->Color = rColor;
		m_pVertexPtr->TexCoord = glm::vec2{ rUV2.x, rUV2.y };
		m_pVertexPtr->TextureIndex = ( float ) textureID;
		++m_pVertexPtr;

		m_pVertexPtr->Position = { rMin.x, rMax.y };
		m_pVertexPtr->Color = rColor;
		m_pVertexPtr->TexCoord = glm::vec2{ rUV1.x, rUV2.y };
		m_pVertexPtr->TextureIndex = ( float ) textureID;
		++m_pVertexPtr;

		m_QuadVertexCount += 4;
		m_QuadIndexCount += 6;
	}

	void AluraRenderer::SubmitRectFrame( const glm::vec2& rMin, const glm::vec2& rMax, float thinkness, const glm::vec4& rColor )
	{
		// Top
		SubmitRect( rMin, { rMax.x, rMin.y + thinkness }, rColor );

		// Bottom
		SubmitRect( { rMin.x, rMax.y - thinkness }, rMax, rColor );

		// Left
		SubmitRect( { rMin.x, rMin.y + thinkness }, { rMin.x + thinkness, rMax.y - thinkness }, rColor );
		
		// Right
		SubmitRect( { rMax.x - thinkness, rMin.y + thinkness }, { rMax.x, rMax.y - thinkness }, rColor );
	}

	void AluraRenderer::SubmitString( const std::string& rText, Ref<AluraFont> font, float fontScale, const glm::vec2& rCursorPos, const glm::vec4& rColor )
	{
		const auto& rFontGeo = font->GetMSDFData()->FontGeometry;
		const auto& rMetrics = rFontGeo.getMetrics();

		double x = 0.0;
		const double fsScale = fontScale / ( rMetrics.ascenderY - rMetrics.descenderY );

#if !defined(SAT_DIST)
		double y = fsScale * rMetrics.ascenderY;
#else
		double y = -fsScale * rMetrics.ascenderY;
#endif

		for( size_t i = 0; i < rText.size(); i++ )
		{
			const char character = rText[ i ];
			if( character == '\r' ) continue;

			if( character == '\n' )
			{
				x = 0;
#if !defined(SAT_DIST)
				y += fsScale * rMetrics.lineHeight;
#else
				y -= fsScale * rMetrics.lineHeight;
#endif
				continue;
			}

			auto pGlyph = rFontGeo.getGlyph( character );
			if( character == ' ' )
			{
				double advance = pGlyph->getAdvance();
				x += fsScale * advance;
				continue;
			}
			// TOOD: Add a font setting or a style setting to determinate how many spaces a tab should be
			// right now we'll do 4 spaces.
			// AluraCanvas::PushStyleVar( AluraStyleVar_FontSize, 18.0f )? or passing it into every AddText call?
			else if( character == '\t' )
			{
				pGlyph = rFontGeo.getGlyph( ' ' );
				double advance = pGlyph->getAdvance() * 4 /* NUMBER_OF_SPACES_PER_TAB */;
				x += fsScale * advance;
				continue;
			}

			if( !pGlyph ) pGlyph = rFontGeo.getGlyph( '?' );

			double atlasLeft, atlasBottom, atlasRight, atlasTop;
			pGlyph->getQuadAtlasBounds( atlasLeft, atlasBottom, atlasRight, atlasTop );

			// NOTE: Vulkan: We have to flip the atlasTop and atlasBottom because in the Editor the UI origin is the bottom-left
			// the reason why it's the bottom-left is because when this image gets flipped in the viewport, the elements at the bottom-left
			// will be at the top-left, which is correct as the real origin is actually at the top-left.
#if !defined(SAT_DIST)
			glm::vec2 texCoordMin( atlasLeft, atlasTop );
			glm::vec2 texCoordMax( atlasRight, atlasBottom );
#else
			glm::vec2 texCoordMin( atlasLeft, atlasBottom );
			glm::vec2 texCoordMax( atlasRight, atlasTop );
#endif

			double planeLeft, planeBottom, planeRight, planeTop;
			pGlyph->getQuadPlaneBounds( planeLeft, planeBottom, planeRight, planeTop );

			// NOTE: Vulkan: Same as above.
#if !defined(SAT_DIST)
			glm::vec2 quadMin( x + planeLeft * fsScale, y - planeTop * fsScale );
			glm::vec2 quadMax( x + planeRight * fsScale, y - planeBottom * fsScale );
#else
			glm::vec2 quadMin( x + planeLeft * fsScale, y + planeBottom * fsScale );
			glm::vec2 quadMax( x + planeRight * fsScale, y + planeTop * fsScale );
#endif

			const float texelWidth = 1.0f / font->GetTexture()->Width();
			const float texelHeight = 1.0f / font->GetTexture()->Height();

			texCoordMin *= glm::vec2( texelWidth, texelHeight );
			texCoordMax *= glm::vec2( texelWidth, texelHeight );

			SubmitTextGlyph( quadMin, quadMax, texCoordMin, texCoordMax, rColor, font->GetTexture(), rCursorPos );

			// Next character spacing
			if( i < rText.size() - 1 )
			{
				double advance = pGlyph->getAdvance();
				char next = rText[ i + 1 ];
				rFontGeo.getAdvance( advance, character, next );

				x += fsScale * advance + 0.0f;
			}
		}
	}

	void AluraRenderer::SubmitTextGlyph( const glm::vec2& rMin, const glm::vec2& rMax, const glm::vec2& rTexCoordMin, const glm::vec2& rTexCoordMax, const glm::vec4& rColor, Ref<Texture2D> atlasTexture, const glm::vec2& rCursorPos )
	{
		uint32_t textureID = 0u;
		uint32_t fullTextureIndex = 0u;

		for( uint32_t textureIndex = 17u, i = 0u; i < m_CurrentTextureAtlasSlot; ++i, ++textureIndex )
		{
			if( m_Textures[ textureIndex ] == atlasTexture )
			{
				textureID = i;
				fullTextureIndex = textureIndex;
				break;
			}
		}

		if( fullTextureIndex == 0 )
		{
			textureID = m_CurrentTextureAtlasSlot;
			m_Textures[ textureID + 17u ] = atlasTexture;
			++m_CurrentTextureAtlasSlot;
		}

		m_pTextVertexPtr->Position = rCursorPos + rMin;
		m_pTextVertexPtr->Color = rColor;
		m_pTextVertexPtr->TexCoord = rTexCoordMin;
		m_pTextVertexPtr->TextureIndex = ( float ) textureID;
		++m_pTextVertexPtr;
		
		m_pTextVertexPtr->Position = rCursorPos + glm::vec2{ rMin.x, rMax.y };
		m_pTextVertexPtr->Color = rColor;
		m_pTextVertexPtr->TexCoord = { rTexCoordMin.x, rTexCoordMax.y };
		m_pTextVertexPtr->TextureIndex = ( float ) textureID;
		++m_pTextVertexPtr;

		m_pTextVertexPtr->Position = rCursorPos + rMax;
		m_pTextVertexPtr->Color = rColor;
		m_pTextVertexPtr->TexCoord = rTexCoordMax;
		m_pTextVertexPtr->TextureIndex = ( float ) textureID;
		++m_pTextVertexPtr;

		m_pTextVertexPtr->Position = rCursorPos + glm::vec2{ rMax.x, rMin.y };
		m_pTextVertexPtr->Color = rColor;
		m_pTextVertexPtr->TexCoord = { rTexCoordMax.x, rTexCoordMin.y };
		m_pTextVertexPtr->TextureIndex = ( float ) textureID;
		++m_pTextVertexPtr;

		m_TextIndexCount += 6;
	}

}
