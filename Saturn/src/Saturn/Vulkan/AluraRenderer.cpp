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

#include "sppch.h"
#include "AluraRenderer.h"

#include "Renderer.h"
#include "VulkanDebug.h"

#include "Saturn/Core/Ruby/RubyWindow.h"
#include "Saturn/Core/App.h"

#include "Saturn/Alura/AluraMSDFGenerationData.h"
#include "Saturn/Alura/AluraFont.h"
#include "Saturn/Alura/AluraRect.h"

#include "Saturn/Core/Profiler.h"

namespace Saturn {

	static constexpr uint32_t s_MaxQuads = 20000u;
	static constexpr uint32_t s_MaxVertices = s_MaxQuads * 4u;
	static constexpr uint32_t s_MaxIndices = s_MaxQuads * 6u;

	AluraRenderer::AluraRenderer()
	{
		m_Width = Application::Get()->GetWindow()->GetWidth();
		m_Height = Application::Get()->GetWindow()->GetHeight();
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
		m_QuadVertexBase.resize( MAX_FRAMES_IN_FLIGHT );
		m_TextVertexBase.resize( MAX_FRAMES_IN_FLIGHT );

		m_VertexBuffers.resize( MAX_FRAMES_IN_FLIGHT );
		m_TextVertexBuffers.resize( MAX_FRAMES_IN_FLIGHT );

		for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
		{
			m_VertexBuffers[ i ] = Ref<VertexBuffer>::Create( s_MaxVertices * sizeof( AluraRectVertex ) );
			m_QuadVertexBase[ i ] = new AluraRectVertex[ s_MaxVertices ];

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

		m_Textures[ 0 ] = Renderer::Get()->GetPinkTexture();
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
			{ ShaderDataType::Float3, "a_Position" },
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
		for( auto buffer : m_QuadVertexBase )
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
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();
		
		m_QuadVertexCount = 0;
		m_QuadIndexCount = 0;
		m_pQuadVertexPtr = m_QuadVertexBase[ frame ];

		m_TextIndexCount = 0;
		m_pTextVertexPtr = m_TextVertexBase[ frame ];

		m_CurrentTextureSlot = 1;
		
		// Zero because its local to its 16 texture array
		m_CurrentTextureAtlasSlot = 0;
	}

	void AluraRenderer::Render()
	{
		SAT_PF_EVENT();

		if( m_Resized )
		{
			OnResize();
			m_Resized = false;
		}

		m_CommandBuffer = Renderer::Get()->ActiveCommandBuffer();
		
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

		const uint32_t frame = Renderer::Get()->GetCurrentFrame();

		m_IndexBuffer->Bind( m_CommandBuffer );

		const uint32_t dataSize = ( uint32_t ) ( ( uint8_t* ) m_pQuadVertexPtr - ( uint8_t* ) m_QuadVertexBase[ frame ] );
		if( dataSize >= 1 )
		{
			m_VertexBuffers[ frame ]->SetData( m_QuadVertexBase[ frame ], dataSize );
			m_VertexBuffers[ frame ]->Bind( m_CommandBuffer );

			for( uint32_t i = 0; i < 16; ++i )
			{
				if( m_Textures[ i ] )
					m_Material->SetResource( "u_InputTexture", m_Textures[ i ], i );
				else
					m_Material->SetResource( "u_InputTexture", Renderer::Get()->GetPinkTexture(), i );
			}

			m_Pipeline->Bind( m_CommandBuffer );

			m_Material->SetPC( "u_Transform.Projection", m_Projection );
			m_Material->SetResource( "u_InputTexture", Renderer::Get()->GetPinkTexture() );

			m_Material->Bind( m_CommandBuffer, m_Pipeline->GetPipelineLayout(), {} );

			vkCmdPushConstants( m_CommandBuffer, m_Pipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, ( uint32_t ) m_Material->GetPushConstantData().Size, m_Material->GetPushConstantData().Data );

			vkCmdDrawIndexed( m_CommandBuffer, m_QuadIndexCount, 1, 0, 0, 0 );
		}

		const uint32_t textDataSize = ( uint32_t ) ( ( uint8_t* ) m_pTextVertexPtr - ( uint8_t* ) m_TextVertexBase[ frame ] );
		if( textDataSize >= 1 )
		{
			m_TextVertexBuffers[ frame ]->SetData( m_TextVertexBase[ frame ], textDataSize );
			m_TextVertexBuffers[ frame ]->Bind( m_CommandBuffer );

			for( uint32_t textureIndex = 17, i = 0; textureIndex < m_Textures.size(); ++i, ++textureIndex )
			{
				if( m_Textures[ textureIndex ] )
					m_TextMaterial->SetResource( "u_FontAtlases", m_Textures[ textureIndex ], i );
				else
					m_TextMaterial->SetResource( "u_FontAtlases", Renderer::Get()->GetPinkTexture(), i );
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
		m_pQuadVertexPtr->Position = { rMin.x, rMin.y };
		m_pQuadVertexPtr->Color = rColor;
		m_pQuadVertexPtr->TexCoord = glm::vec2{ 0.0f, 0.0f };		
		m_pQuadVertexPtr->TextureIndex = 0.0f;
		++m_pQuadVertexPtr;

		m_pQuadVertexPtr->Position = { rMax.x, rMin.y };
		m_pQuadVertexPtr->Color = rColor;
		m_pQuadVertexPtr->TexCoord = glm::vec2{ 1.0f, 0.0f };
		m_pQuadVertexPtr->TextureIndex = 0.0f;
		++m_pQuadVertexPtr;

		m_pQuadVertexPtr->Position = { rMax.x, rMax.y };
		m_pQuadVertexPtr->Color = rColor;
		m_pQuadVertexPtr->TexCoord = glm::vec2{ 1.0f, 1.0f };
		m_pQuadVertexPtr->TextureIndex = 0.0f;
		++m_pQuadVertexPtr;

		m_pQuadVertexPtr->Position = { rMin.x, rMax.y };
		m_pQuadVertexPtr->Color = rColor;
		m_pQuadVertexPtr->TexCoord = glm::vec2{ 0.0f, 1.0f };
		m_pQuadVertexPtr->TextureIndex = 0.0f;
		++m_pQuadVertexPtr;

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

		m_pQuadVertexPtr->Position = { rMin.x, rMin.y };
		m_pQuadVertexPtr->Color = rColor;
		m_pQuadVertexPtr->TexCoord = glm::vec2{ rUV1.x, rUV1.y };
		m_pQuadVertexPtr->TextureIndex = ( float ) textureID;
		++m_pQuadVertexPtr;

		m_pQuadVertexPtr->Position = { rMax.x, rMin.y };
		m_pQuadVertexPtr->Color = rColor;
		m_pQuadVertexPtr->TexCoord = glm::vec2{ rUV2.x, rUV1.y };
		m_pQuadVertexPtr->TextureIndex = ( float ) textureID;
		++m_pQuadVertexPtr;

		m_pQuadVertexPtr->Position = { rMax.x, rMax.y };
		m_pQuadVertexPtr->Color = rColor;
		m_pQuadVertexPtr->TexCoord = glm::vec2{ rUV2.x, rUV2.y };
		m_pQuadVertexPtr->TextureIndex = ( float ) textureID;
		++m_pQuadVertexPtr;

		m_pQuadVertexPtr->Position = { rMin.x, rMax.y };
		m_pQuadVertexPtr->Color = rColor;
		m_pQuadVertexPtr->TexCoord = glm::vec2{ rUV1.x, rUV2.y };
		m_pQuadVertexPtr->TextureIndex = ( float ) textureID;
		++m_pQuadVertexPtr;

		m_QuadVertexCount += 4;
		m_QuadIndexCount += 6;
	}

	void AluraRenderer::SubmitRectFrame( const glm::vec2& rMin, const glm::vec2& rMax, float thickness, const glm::vec4& rColor )
	{
		// Top
		SubmitRect( rMin, { rMax.x, rMin.y + thickness }, rColor );

		// Bottom
		SubmitRect( { rMin.x, rMax.y - thickness }, rMax, rColor );

		// Left
		SubmitRect( { rMin.x, rMin.y + thickness }, { rMin.x + thickness, rMax.y - thickness }, rColor );
		
		// Right
		SubmitRect( { rMax.x - thickness, rMin.y + thickness }, { rMax.x, rMax.y - thickness }, rColor );
	}

	void AluraRenderer::SubmitString( 
		const std::string& rText, 
		const Ref<AluraFont> font,
		const float fontSizePx,
		const glm::vec2& rStartingPosition, 
		const glm::vec4& rColor )
	{
		glm::mat4 scale = glm::scale( glm::mat4( 1.0f ), glm::vec3( fontSizePx ) );
		glm::mat4 ts = glm::translate( glm::mat4( 1.0f ), glm::vec3( rStartingPosition, 0.0f ) ) * scale;
		SubmitString( rText, font, ts, rColor );
	}

	void AluraRenderer::SubmitString( 
		const std::string& rText, 
		const Ref<AluraFont> font,
		const glm::mat4& rTransform, 
		const glm::vec4& rColor )
	{
		auto& rFontGeo = font->GetFontData();
		const auto& rMetrics = font->GetFontData().GetMetrics();

		const double fsScale = 1 / ( rMetrics.AscenderY - rMetrics.DescenderY );

		double x = 0.0;
		double y = 0.0;
		for( size_t i = 0; i < rText.size(); ++i )
		{
			const char character = rText[ i ];
			if( character == '\r' ) continue;

			if( character == '\n' )
			{
				x = 0;
				y += fsScale * rMetrics.LineHeight;
				continue;
			}

			auto* pGlyph = rFontGeo.GetGlyph( character );
			if( character == ' ' )
			{
				double advance = pGlyph->GetAdvance();
				x += fsScale * advance;
				continue;
			}
			// TOOD: Add a font setting or a style setting to determinate how many spaces a tab should be
			// right now we'll do 4 spaces.
			else if( character == '\t' )
			{
				pGlyph = rFontGeo.GetGlyph( ' ' );
				double advance = pGlyph->GetAdvance() * 4.0 /* NUMBER_OF_SPACES_PER_TAB */;
				x += fsScale * advance;
				continue;
			}

			if( !pGlyph ) pGlyph = rFontGeo.GetGlyph( '?' );

			float atlasLeft, atlasBottom, atlasRight, atlasTop;
			pGlyph->GetQuadAtlasBounds( atlasLeft, atlasBottom, atlasRight, atlasTop );

			glm::vec2 texCoordMin( atlasLeft, atlasTop );
			glm::vec2 texCoordMax( atlasRight, atlasBottom );

			float pl, pb, pr, pt;
			pGlyph->GetQuadPlaneBounds( pl, pb, pr, pt );

			glm::vec2 quadMin( x + pl * fsScale, y - pt * fsScale );
			glm::vec2 quadMax( x + pr * fsScale, y - pb * fsScale );

			const float texelWidth = 1.0f / font->GetTexture()->Width();
			const float texelHeight = 1.0f / font->GetTexture()->Height();

			texCoordMin *= glm::vec2( texelWidth, texelHeight );
			texCoordMax *= glm::vec2( texelWidth, texelHeight );

			SubmitTextGlyph( quadMin, quadMax, texCoordMin, texCoordMax, rColor, font->GetTexture(), rTransform );

			// Next character spacing
			if( i < rText.size() - 1 )
			{
				double advance = pGlyph->GetAdvance();
				char next = rText[ i + 1 ];
				rFontGeo.GetAdvance( advance, character, next );

				x += fsScale * advance + 0.0f /* <-- kerning */;
			}
		}
	}

	void AluraRenderer::SubmitTextGlyph( 
		const glm::vec2& rMin, 
		const glm::vec2& rMax, 
		const glm::vec2& rTexCoordMin, 
		const glm::vec2& rTexCoordMax, 
		const glm::vec4& rColor, 
		const Ref<Texture2D> atlasTexture, 
		const glm::mat4& rTransform )
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

		m_pTextVertexPtr->Position = rTransform * glm::vec4( rMin, 0.0f, 1.0f );
		m_pTextVertexPtr->Color = rColor;
		m_pTextVertexPtr->TexCoord = rTexCoordMin;
		m_pTextVertexPtr->TextureIndex = ( float ) textureID;
		++m_pTextVertexPtr;
		
		m_pTextVertexPtr->Position = rTransform * glm::vec4( rMin.x, rMax.y, 0.0f, 1.0f );
		m_pTextVertexPtr->Color = rColor;
		m_pTextVertexPtr->TexCoord = { rTexCoordMin.x, rTexCoordMax.y };
		m_pTextVertexPtr->TextureIndex = ( float ) textureID;
		++m_pTextVertexPtr;

		m_pTextVertexPtr->Position = rTransform *  glm::vec4( rMax, 0.0f, 1.0f );
		m_pTextVertexPtr->Color = rColor;
		m_pTextVertexPtr->TexCoord = rTexCoordMax;
		m_pTextVertexPtr->TextureIndex = ( float ) textureID;
		++m_pTextVertexPtr;

		m_pTextVertexPtr->Position = rTransform * glm::vec4( rMax.x, rMin.y, 0.0f, 1.0f );
		m_pTextVertexPtr->Color = rColor;
		m_pTextVertexPtr->TexCoord = { rTexCoordMax.x, rTexCoordMin.y };
		m_pTextVertexPtr->TextureIndex = ( float ) textureID;
		++m_pTextVertexPtr;

		m_TextIndexCount += 6;
	}

	void AluraRenderer::SubmitCircleFilled( const glm::vec2& rPosition, float size, float thickness, const glm::vec4& rColor )
	{
	}

	void AluraRenderer::SubmitCircle( const glm::vec2& rCentre, float radius, float thickness, const glm::vec4& rColor )
	{
		constexpr int segments = 64;
		constexpr float step = 2.0f * glm::pi<float>() / segments;

		for( int i = 0; i < segments; ++i )
		{
			const float a0 = i * step;
			const float a1 = ( i + 1 ) * step;

			const glm::vec2 p0 = rCentre + glm::vec2( glm::cos( a0 ), glm::sin( a0 ) ) * radius;
			const glm::vec2 p1 = rCentre + glm::vec2( glm::cos( a1 ), glm::sin( a1 ) ) * radius;
			const glm::vec2 p2 = rCentre + glm::vec2( glm::cos( a1 ), glm::sin( a1 ) ) * ( radius + thickness );
			const glm::vec2 p3 = rCentre + glm::vec2( glm::cos( a0 ), glm::sin( a0 ) ) * ( radius + thickness );

			m_pQuadVertexPtr->Position = p0;
			m_pQuadVertexPtr->Color = rColor; 
			m_pQuadVertexPtr->TexCoord = { 0,0 };
			m_pQuadVertexPtr->TextureIndex = 0.0f; 
			++m_pQuadVertexPtr;
			
			m_pQuadVertexPtr->Position = p1;
			m_pQuadVertexPtr->Color = rColor; 
			m_pQuadVertexPtr->TexCoord = { 1,0 };
			m_pQuadVertexPtr->TextureIndex = 0.0f; 
			++m_pQuadVertexPtr;
			
			m_pQuadVertexPtr->Position = p2;
			m_pQuadVertexPtr->Color = rColor; 
			m_pQuadVertexPtr->TexCoord = { 1,1 }; 
			m_pQuadVertexPtr->TextureIndex = 0.0f; 
			++m_pQuadVertexPtr;
			
			m_pQuadVertexPtr->Position = p3;
			m_pQuadVertexPtr->Color = rColor; 
			m_pQuadVertexPtr->TexCoord = { 0,1 }; 
			m_pQuadVertexPtr->TextureIndex = 0.0f; 
			++m_pQuadVertexPtr;

			m_QuadVertexCount += 4;
			m_QuadIndexCount += 6;
		}
	}

#if !defined(SAT_DIST)
	void AluraRenderer::EdClearCommands()
	{
		PreRender();
	}
#endif

}
