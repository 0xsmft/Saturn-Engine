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
#include "Renderer2D.h"

#include "Renderer.h"
#include "VulkanDebug.h"

#include "Saturn/Core/Ruby/RubyWindow.h"

// This 2D Renderer is mostly based from Hazel's 2D renderer (https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Renderer/Renderer2D.cpp)

namespace Saturn {

	static constexpr uint32_t s_MaxQuads = 5000u;
	static constexpr uint32_t s_MaxVertices = s_MaxQuads * 4u;
	static constexpr uint32_t s_MaxIndices = s_MaxQuads * 6u;
	static constexpr uint32_t s_MaxTextureSlots = 32;

	static constexpr uint32_t s_MaxLines = 1000u;
	static constexpr uint32_t s_MaxLineVertices = s_MaxLines * 2u;
	static constexpr uint32_t s_MaxLineIndices = s_MaxLines * 6u;

	static constexpr uint32_t s_MaxSolidLines = 1000u;
	static constexpr uint32_t s_MaxSolidLineVertices = s_MaxSolidLines * 2u;
	static constexpr uint32_t s_MaxSolidLineIndices = s_MaxSolidLines * 6u;

	Renderer2D::Renderer2D()
	{
	}

	Renderer2D::~Renderer2D()
	{
		Terminate();
	}

	void Renderer2D::Init( Ref<Pass> targetPass /*= nullptr */, Ref<Framebuffer> targetFramebuffer /*= nullptr*/ )
	{
		if( Application::Get()->HasFlag( ApplicationFlag_UIOnly ) )
			return;

		m_Width = Application::Get()->GetWindow()->GetWidth();
		m_Height = Application::Get()->GetWindow()->GetHeight();

		// Setup Quads
		m_QuadVertexPositions.reserve( 4 );

		m_QuadVertexPositions.emplace_back( -0.5f, -0.5f, 0.0f, 1.0f );
		m_QuadVertexPositions.emplace_back( -0.5f, 0.5f, 0.0f, 1.0f );
		m_QuadVertexPositions.emplace_back( 0.5f, 0.5f, 0.0f, 1.0f );
		m_QuadVertexPositions.emplace_back( 0.5f, -0.5f, 0.0f, 1.0f );

		// Setup vertex buffer
		m_QuadVertexBuffers.resize( 1 );
		m_QuadVertexBuffers[ 0 ].resize( MAX_FRAMES_IN_FLIGHT );

		m_CurrentQuadBases.resize( 1 );
		m_CurrentQuadBases[ 0 ].resize( MAX_FRAMES_IN_FLIGHT );

		m_pCurrentQuadPtr.resize( 1 );

		// Lines
		m_LineVertexBuffers.resize( 1 );
		m_LineVertexBuffers[ 0 ].resize( MAX_FRAMES_IN_FLIGHT );

		m_CurrentLineBases.resize( 1 );
		m_CurrentLineBases[ 0 ].resize( MAX_FRAMES_IN_FLIGHT );

		m_CurrentLinePtr.resize( 1 );
		
		m_TriangleVertexBuffers.resize( 1 );
		m_TriangleVertexBuffers[ 0 ].resize( MAX_FRAMES_IN_FLIGHT );

		m_CurrentTriangleBases.resize( 1 );
		m_CurrentTriangleBases[ 0 ].resize( MAX_FRAMES_IN_FLIGHT );
		m_CurrentTrianglePtr.resize( 1 );

		for( int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			m_QuadVertexBuffers[ 0 ][ i ] = Ref<VertexBuffer>::Create( s_MaxVertices * sizeof( QuadVertex ) );
			m_CurrentQuadBases[ 0 ][ i ] = new QuadVertex[ s_MaxVertices ];

			m_LineVertexBuffers[ 0 ][ i ] = Ref<VertexBuffer>::Create( s_MaxLineVertices * sizeof( LineDrawCommand ) );
			m_CurrentLineBases[ 0 ][ i ] = new LineDrawCommand[ s_MaxLineVertices ];

			m_TriangleVertexBuffers[ 0 ][ i ] = Ref<VertexBuffer>::Create( s_MaxSolidLineVertices * sizeof( LineDrawCommand ) );
			m_CurrentTriangleBases[ 0 ][ i ] = new LineDrawCommand[ s_MaxSolidLineVertices ];
		}

		// Setup Index Buffer
		uint32_t* quadBuffer = new uint32_t[ s_MaxIndices ];

		uint32_t offset = 0;

		for( uint32_t i = 0; i < s_MaxIndices; i += 6 )
		{
			quadBuffer[ i + 0 ] = offset + 0;
			quadBuffer[ i + 1 ] = offset + 1;
			quadBuffer[ i + 2 ] = offset + 2;

			quadBuffer[ i + 3 ] = offset + 2;
			quadBuffer[ i + 4 ] = offset + 3;
			quadBuffer[ i + 5 ] = offset + 0;

			offset += 4;
		}

		m_QuadIndexBuffer = Ref<IndexBuffer>::Create( quadBuffer, s_MaxIndices * sizeof( uint32_t ) );
		delete[] quadBuffer;

		uint32_t* pLineBuffer = new uint32_t[ s_MaxLineIndices ];
		for( uint32_t i = 0; i < s_MaxLineIndices; ++i )
			pLineBuffer[ i ] = i;

		m_LineIndexBuffer = Ref<IndexBuffer>::Create( pLineBuffer, s_MaxLineIndices * sizeof( uint32_t ) );
		delete[] pLineBuffer;

		pLineBuffer = new uint32_t[ s_MaxSolidLineIndices ];
		for( uint32_t i = 0; i < s_MaxSolidLineIndices; ++i )
			pLineBuffer[ i ] = i;

		m_TriangleIndexBuffer = Ref<IndexBuffer>::Create( pLineBuffer, s_MaxSolidLineIndices * sizeof( uint32_t ) );

		delete[] pLineBuffer;

		// Setup Textures
		m_Textures[ 0 ] = Renderer::Get()->GetPinkTexture();

		m_TargetRenderPass = targetPass;
		m_TargetFramebuffer = targetFramebuffer;
		LateInit( targetPass, targetFramebuffer );
	}

	void Renderer2D::LateInit( Ref<Pass> targetPass /*= nullptr */, Ref<Framebuffer> targetFramebuffer /*= nullptr*/ )
	{
		if( !targetFramebuffer )
		{
			FramebufferSpecification FBSpec = {};
			FBSpec.Width = m_Width;
			FBSpec.Height = m_Height;

			FBSpec.RenderPass = m_TempRenderPass;
			FBSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };

			m_TargetFramebuffer = Ref<Framebuffer>::Create( FBSpec );
		}

		if( !m_QuadShader )
		{
			m_QuadShader = ShaderLibrary::Get().FindOrLoad( "Renderer2D", "content/shaders/Renderer2D.glsl" );
			m_QuadMaterial = Ref<Material>::Create( m_QuadShader, "QuadMaterial" );
		}

		if( !m_LineShader )
		{
			m_LineShader = ShaderLibrary::Get().FindOrLoad( "DebugLine", "content/shaders/DebugLine.glsl" );
			m_LineMaterial = Ref<Material>::Create( m_LineShader, "DebugLineMaterial" );
		}

		PipelineSpecification PipelineSpec{};
		PipelineSpec.Width = m_Width;
		PipelineSpec.Height = m_Height;
		PipelineSpec.Name = "Renderer2D(Quads)";
		PipelineSpec.Shader = m_QuadShader;
		PipelineSpec.RenderPass = targetPass == nullptr ? m_TempRenderPass : targetPass;
		PipelineSpec.CullMode = CullMode::None;
		PipelineSpec.FrontFace = VK_FRONT_FACE_CLOCKWISE;
		PipelineSpec.UseDepthTest = true;
		PipelineSpec.VertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" },
			{ ShaderDataType::Float2, "a_TexCoord" },
			{ ShaderDataType::Float, "a_TextureIndex" },
		};

		m_QuadPipeline = Ref<Pipeline>::Create( PipelineSpec );

		PipelineSpec.Name = "Renderer2D(Lines)";
		PipelineSpec.Shader = m_LineShader;
		PipelineSpec.PolygonMode = VK_POLYGON_MODE_LINE;
		PipelineSpec.Topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		PipelineSpec.CullMode = CullMode::Back;
		PipelineSpec.VertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" }
		};

		m_LinePipeline = Ref<Pipeline>::Create( PipelineSpec );

		PipelineSpec.Name = "Renderer2D(Lines|Solid)";
		PipelineSpec.PolygonMode = VK_POLYGON_MODE_FILL;
		PipelineSpec.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		m_TrianglePipeline = Ref<Pipeline>::Create( PipelineSpec );
	}

	void Renderer2D::Terminate()
	{
		if( m_TempRenderPass )
			m_TempRenderPass = nullptr;

		m_TargetFramebuffer = nullptr;
		m_TargetRenderPass = nullptr;

		m_QuadPipeline = nullptr;
		m_QuadIndexBuffer = nullptr;
		m_QuadShader = nullptr;
		m_QuadMaterial = nullptr;

		m_LinePipeline = nullptr;
		m_LineShader = nullptr;
		m_LineMaterial = nullptr;
		m_LineIndexBuffer = nullptr;

		m_TriangleIndexBuffer = nullptr;
		m_TrianglePipeline = nullptr;

		m_QuadVertexBuffers.clear();
		m_LineVertexBuffers.clear();
		m_TriangleVertexBuffers.clear();

		for( auto& texture : m_Textures )
			texture = nullptr;

		for( auto& rBuffers : m_CurrentQuadBases )
			for( auto buffer : rBuffers )
				delete[] buffer;

		for( auto& rBuffers : m_CurrentLineBases )
			for( auto buffer : rBuffers )
				delete[] buffer;

		for( auto& rBuffers : m_CurrentTriangleBases )
			for( auto buffer : rBuffers )
				delete[] buffer;
	}

	void Renderer2D::SetViewportSize( uint32_t w, uint32_t h )
	{
		if( m_Width != w || m_Height != h )
		{
			m_Width = w;
			m_Height = h;
			m_Resized = true;
		}
	}

	void Renderer2D::ReplaceTexture( Ref<Texture2D> old, Ref<Texture2D> newTexture )
	{
		for( size_t i = 0; i < m_Textures.size(); i++ )
		{
			if( m_Textures[ i ] == old )
				m_Textures[ i ] = newTexture;
		}
	}

	void Renderer2D::SetInitialRenderPass( Ref<Pass> pass, Ref<Framebuffer> targetFramebuffer )
	{
		if( m_TargetRenderPass != pass )
		{
			m_TargetRenderPass = pass;
			m_TargetFramebuffer = targetFramebuffer;

			m_QuadPipeline = nullptr;
			m_LinePipeline = nullptr;

			LateInit( m_TargetRenderPass, targetFramebuffer );

			m_TempRenderPass = nullptr;
		}
	}

	void Renderer2D::AddQuadBuffer()
	{
		std::vector< Ref<VertexBuffer> >& rNewVB = m_QuadVertexBuffers.emplace_back();
		std::vector< QuadVertex* >& rNewBase = m_CurrentQuadBases.emplace_back();

		rNewVB.resize( MAX_FRAMES_IN_FLIGHT );
		rNewBase.resize( MAX_FRAMES_IN_FLIGHT );
	
		for( auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			const uint64_t allocSize = s_MaxVertices * sizeof( QuadVertex );
			rNewVB[ i ] = Ref<VertexBuffer>::Create( allocSize );
			rNewBase[ i ] = new QuadVertex[ s_MaxVertices ];
		}
	}

	void Renderer2D::AddLineBuffer()
	{
		SAT_CORE_INFO( "AddLineBuffer, VBs:{0}, Bases:{1}", m_LineVertexBuffers.size(), m_CurrentLineBases.size() );

		std::vector< Ref<VertexBuffer> >& rNewVB = m_LineVertexBuffers.emplace_back();
		std::vector< LineDrawCommand* >& rNewBase = m_CurrentLineBases.emplace_back();

		rNewVB.resize( MAX_FRAMES_IN_FLIGHT );
		rNewBase.resize( MAX_FRAMES_IN_FLIGHT );

		for( auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			const uint64_t allocSize = s_MaxLineVertices * sizeof( LineDrawCommand );
			rNewVB[ i ] = Ref<VertexBuffer>::Create( allocSize );
			rNewBase[ i ] = new LineDrawCommand[ s_MaxLineVertices ];
		}
	}

	void Renderer2D::AddTriangleLineBuffer()
	{
		SAT_CORE_INFO( "AddTriangleLineBuffer, VBs:{0}, Bases:{1}", m_TriangleVertexBuffers.size(), m_CurrentTriangleBases.size() );

		std::vector< Ref<VertexBuffer> >& rNewVB = m_TriangleVertexBuffers.emplace_back();
		std::vector< LineDrawCommand* >& rNewBase = m_CurrentTriangleBases.emplace_back();

		rNewVB.resize( MAX_FRAMES_IN_FLIGHT );
		rNewBase.resize( MAX_FRAMES_IN_FLIGHT );

		for( auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			const uint64_t allocSize = s_MaxSolidLineVertices * sizeof( LineDrawCommand );
			rNewVB[ i ] = Ref<VertexBuffer>::Create( allocSize );
			rNewBase[ i ] = new LineDrawCommand[ s_MaxSolidLineVertices ];
		}
	}

	QuadVertex*& Renderer2D::GetQuadBuffer()
	{
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();

		m_QuadBufferIndex = m_QuadIndexCount / s_MaxIndices;
		if( m_QuadBufferIndex >= m_QuadVertexBuffers.size() )
		{
			AddQuadBuffer();
			m_pCurrentQuadPtr.emplace_back();
			m_pCurrentQuadPtr[ m_QuadBufferIndex ] = m_CurrentQuadBases[ m_QuadBufferIndex ][ frame ];
		}

		return m_pCurrentQuadPtr[ m_QuadBufferIndex ];
	}

	LineDrawCommand*& Renderer2D::GetLineBuffer()
	{
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();
		
		const uint32_t indicesPerBuffer = s_MaxLineIndices;
		const uint32_t linesPerBuffer = indicesPerBuffer / 2u;

		m_LineBufferIndex = m_LineIndexCount / s_MaxLineIndices;

		if( m_LineBufferIndex >= m_LineVertexBuffers.size() )
		{
			AddLineBuffer();
			m_CurrentLinePtr.emplace_back();
			m_CurrentLinePtr[ m_LineBufferIndex ] = m_CurrentLineBases[ m_LineBufferIndex ][ frame ];
		}

		return m_CurrentLinePtr[ m_LineBufferIndex ];
	}

	LineDrawCommand*& Renderer2D::GetTriangleLineBuffer()
	{
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();

		const uint32_t indicesPerBuffer = s_MaxLineIndices;
		const uint32_t linesPerBuffer = indicesPerBuffer / 2u;

		m_LineTriangleBufferIndex = m_TriangleIndexCount / s_MaxLineIndices;
		
		if( m_LineTriangleBufferIndex >= m_TriangleVertexBuffers.size() )
		{
			AddTriangleLineBuffer();
			m_CurrentTrianglePtr.emplace_back();
			m_CurrentTrianglePtr[ m_LineTriangleBufferIndex ] = m_CurrentTriangleBases[ m_LineTriangleBufferIndex ][ frame ];
		}

		return m_CurrentTrianglePtr[ m_LineTriangleBufferIndex ];
	}

	void Renderer2D::SubmitQuad( const glm::mat4& transform, const glm::vec4& color )
	{
		// One quad has 4 vertices so we need to submit them one by one.
		const glm::vec2 TexCoord[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		auto& prCurrentQuad = GetQuadBuffer();
		for( size_t i = 0; i < 4; ++i )
		{
			prCurrentQuad->Position = transform * m_QuadVertexPositions[ i ];
			prCurrentQuad->Color = color;
			prCurrentQuad->TexCoord = TexCoord[ i ];
			prCurrentQuad->TextureIndex = 0;

			++prCurrentQuad;
		}

		m_QuadIndexCount += 6;
	}

	void Renderer2D::SubmitQuad( const glm::vec3& position, const glm::vec4& color, const glm::vec2& size )
	{
		const glm::vec2 TexCoord[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		const glm::mat4 transform = glm::translate( glm::mat4( 1.0f ), position )
			* glm::scale( glm::mat4( 1.0f ), { size.x, size.y, 1.0f } );

		auto& prCurrentQuad = GetQuadBuffer();
		for( size_t i = 0; i < 4; ++i )
		{
			prCurrentQuad->Position = transform * m_QuadVertexPositions[ i ];
			prCurrentQuad->Color = color;
			prCurrentQuad->TexCoord = TexCoord[ i ];
			prCurrentQuad->TextureIndex = 0;

			++prCurrentQuad;
		}

		m_QuadIndexCount += 6;
	}

	void Renderer2D::SubmitQuadTextured( const glm::mat4& transform, const glm::vec4& color, const Ref<Texture2D>& rTexture )
	{
		// One quad has 4 vertexes so we need to submit them one by one.
		const glm::vec2 TexCoord[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		int textureID = 0;
		for( uint32_t i = 1; i < m_CurrentTextureSlot; ++i )
		{
			if( m_Textures[ i ] == rTexture ) 
			{
				textureID = i;
				break;
			}
		}

		if( textureID == 0 )
		{
			if( m_CurrentTextureSlot >= s_MaxTextureSlots ) 
			{
				SAT_CORE_ASSERT( false, "Remind me to implement this again..." );
			}

			textureID = m_CurrentTextureSlot;
			m_Textures[ textureID ] = rTexture;
			++m_CurrentTextureSlot;
		}

		auto& prCurrentQuad = GetQuadBuffer();
		for( size_t i = 0; i < 4; ++i )
		{
			prCurrentQuad->Position = transform * m_QuadVertexPositions[ i ];
			prCurrentQuad->Color = color;
			prCurrentQuad->TexCoord = TexCoord[ i ];
			prCurrentQuad->TextureIndex = (float)textureID;

			++prCurrentQuad;
		}

		m_QuadIndexCount += 6;
	}

	void Renderer2D::SubmitBillboard( const glm::vec3& position, const glm::vec4& color, const glm::vec2& rSize )
	{
		const glm::vec2 TexCoord[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		const glm::vec3 CamRight = { m_CameraView[ 0 ][ 0 ], m_CameraView[ 1 ][ 0 ], m_CameraView[ 2 ][ 0 ] };
		const glm::vec3 CamUp = { m_CameraView[ 0 ][ 1 ], m_CameraView[ 1 ][ 1 ], m_CameraView[ 2 ][ 1 ] };

		auto& prCurrentQuad = GetQuadBuffer();
		for( size_t i = 0; i < 4; ++i )
		{
			prCurrentQuad->Position = position + CamRight * ( m_QuadVertexPositions[ i ].x ) * rSize.x + CamUp * m_QuadVertexPositions[ i ].y * rSize.y;
			prCurrentQuad->Color = color;
			prCurrentQuad->TexCoord = TexCoord[ i ];
			prCurrentQuad->TextureIndex = 1;

			++prCurrentQuad;
		}

		m_QuadIndexCount += 6;
	}

	void Renderer2D::SubmitBillboardTextured( const glm::vec3& position, const glm::vec4& color, const Ref<Texture2D>& rTexture, const glm::vec2& rSize )
	{
		constexpr glm::vec2 TexCoord[] = { { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f } };

		const glm::vec3 CamRight = { m_CameraView[ 0 ][ 0 ], m_CameraView[ 1 ][ 0 ], m_CameraView[ 2 ][ 0 ] };
		const glm::vec3 CamUp = { m_CameraView[ 0 ][ 1 ], m_CameraView[ 1 ][ 1 ], m_CameraView[ 2 ][ 1 ] };

		int textureID = 0;
		for( uint32_t i = 1; i < m_CurrentTextureSlot; ++i )
		{
			if( m_Textures[ i ] == rTexture )
			{
				textureID = i;
				break;
			}
		}

		if( textureID == 0 )
		{
			if( m_CurrentTextureSlot >= s_MaxTextureSlots ) 
			{
				SAT_CORE_ASSERT( false, "Remind me to implement this again..." );
			}

			textureID = m_CurrentTextureSlot;
			m_Textures[ textureID ] = rTexture;
			++m_CurrentTextureSlot;
		}

		auto& prCurrentQuad = GetQuadBuffer();
		for( size_t i = 0; i < 4; ++i )
		{
			prCurrentQuad->Position = position + CamRight * ( m_QuadVertexPositions[ i ].x ) * rSize.x + CamUp * m_QuadVertexPositions[ i ].y * rSize.y;
			prCurrentQuad->Color = color;
			prCurrentQuad->TexCoord = TexCoord[ i ];
			prCurrentQuad->TextureIndex = (float)textureID;

			++prCurrentQuad;
		}

		m_QuadIndexCount += 6;
	}

	void Renderer2D::SubmitBillboardTexturedFlipped( const glm::vec3& position, const glm::vec4& color, const Ref<Texture2D>& rTexture, const glm::vec2& rSize )
	{
		constexpr glm::vec2 TexCoord[] = { { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f } };

		const glm::vec3 CamRight = { m_CameraView[ 0 ][ 0 ], m_CameraView[ 1 ][ 0 ], m_CameraView[ 2 ][ 0 ] };
		const glm::vec3 CamUp = { m_CameraView[ 0 ][ 1 ], m_CameraView[ 1 ][ 1 ], m_CameraView[ 2 ][ 1 ] };

		int textureID = 0;
		for( uint32_t i = 1; i < m_CurrentTextureSlot; ++i )
		{
			if( m_Textures[ i ] == rTexture )
			{
				textureID = i;
				break;
			}
		}

		if( textureID == 0 )
		{
			if( m_CurrentTextureSlot >= s_MaxTextureSlots )
			{
				SAT_CORE_ASSERT( false, "Remind me to implement this again..." );
			}

			textureID = m_CurrentTextureSlot;
			m_Textures[ textureID ] = rTexture;
			++m_CurrentTextureSlot;
		}

		auto& prCurrentQuad = GetQuadBuffer();
		for( size_t i = 0; i < 4; ++i )
		{
			prCurrentQuad->Position = position + CamRight * ( m_QuadVertexPositions[ i ].x ) * rSize.x + CamUp * m_QuadVertexPositions[ i ].y * rSize.y;
			prCurrentQuad->Color = color;
			prCurrentQuad->TexCoord = TexCoord[ i ];
			prCurrentQuad->TextureIndex = ( float ) textureID;

			++prCurrentQuad;
		}

		m_QuadIndexCount += 6;
	}

	void Renderer2D::SubmitLine( const glm::vec3& rStart, const glm::vec3& rEnd, const glm::vec4& rColor )
	{
		auto& prCurrentLinePtr = GetLineBuffer();

		prCurrentLinePtr->Position = rStart;
		prCurrentLinePtr->Color = rColor;
	
		++prCurrentLinePtr;
	
		prCurrentLinePtr->Position = rEnd;
		prCurrentLinePtr->Color = rColor;

		++prCurrentLinePtr;

		m_LineIndexCount += 2;
	}

	void Renderer2D::SubmitLine( const glm::vec3& rStart, const glm::vec3& rEnd, const glm::vec4& rColor, float Thinkness )
	{
		auto& prCurrentLinePtr = GetLineBuffer();

		prCurrentLinePtr->Position = rStart;
		prCurrentLinePtr->Color = rColor;

		++prCurrentLinePtr;

		prCurrentLinePtr->Position = rEnd;
		prCurrentLinePtr->Color = rColor;

		++prCurrentLinePtr;

		m_LineIndexCount += 2;
	}

	void Renderer2D::SubmitSingleLine( const glm::vec3& rStart, const glm::vec4& rColor )
	{
		auto& prCurrentLinePtr = GetLineBuffer();
		prCurrentLinePtr->Position = rStart;
		prCurrentLinePtr->Color = rColor;

		++prCurrentLinePtr;
		++m_LineIndexCount;
	}

	void Renderer2D::SubmitArrow( const glm::vec3& rStart, const glm::vec3& rEnd, const glm::vec4& rColor, float headLength /*= 10.0f*/, float headAngle /*= 0.5f */ )
	{
		SubmitLine( rStart, rEnd, rColor );

		glm::vec3 dir = glm::normalize( rEnd - rStart );

		// Build arrowhead
		glm::vec3 up( 0.0f, 1.0f, 0.0f );
		if( glm::abs( glm::dot( up, dir ) ) > 0.99f )
			up = glm::vec3( 1.0f, 0.0f, 0.0f ); // pick another axis if colinear

		glm::vec3 right = glm::normalize( glm::cross( dir, up ) );
		glm::vec3 upVec = glm::normalize( glm::cross( right, dir ) );

		const float angleRad = glm::radians( headAngle );
		const float cosA = std::cos( angleRad );
		const float sinA = std::sin( angleRad );

		glm::vec3 headDir1 = glm::normalize( cosA * ( -dir ) + sinA * right ) * headLength;
		glm::vec3 headDir2 = glm::normalize( cosA * ( -dir ) - sinA * right ) * headLength;
		glm::vec3 headDir3 = glm::normalize( cosA * ( -dir ) + sinA * upVec ) * headLength;
		glm::vec3 headDir4 = glm::normalize( cosA * ( -dir ) - sinA * upVec ) * headLength;

		SubmitLine( rEnd, rEnd + headDir1, rColor );
		SubmitLine( rEnd, rEnd + headDir2, rColor );
		SubmitLine( rEnd, rEnd + headDir3, rColor );
		SubmitLine( rEnd, rEnd + headDir4, rColor );
	}

	void Renderer2D::SubmitDiamond( const glm::vec3& rCenter, float size, const glm::vec4& rColor )
	{
		const float halfSize = size * 0.5f;

		const glm::vec3 top     = rCenter + glm::vec3( 0.0f, halfSize, 0.0f );
		const glm::vec3 bottom  = rCenter + glm::vec3( 0.0f, -halfSize, 0.0f );

		// Square in the middle
		const glm::vec3 front   = rCenter + glm::vec3( 0.0f, 0.0f, halfSize );
		const glm::vec3 back    = rCenter + glm::vec3( 0.0f, 0.0f, -halfSize );
		const glm::vec3 right   = rCenter + glm::vec3( halfSize, 0.0f, 0.0f );
		const glm::vec3 left    = rCenter + glm::vec3( -halfSize, 0.0f, 0.0f );

		// A diamond is a square on the XZ plane (when looking right down at it)
		/*
		*
		*
		*		 top
		*		  *
		*		 /|\
		*	    / | \
		* left *--+--* right
		*	    \ | /
		*		 \|/
		*		  *
		*		bottom
		*
		*/

		// Top 4, draw line from top to every corner on the square
		SubmitLine( top, front, rColor );
		SubmitLine( top, back, rColor );
		SubmitLine( top, left, rColor );
		SubmitLine( top, right, rColor );

		// Bottom 4, draw line from bottom to every corner on the square
		SubmitLine( bottom, front, rColor );
		SubmitLine( bottom, back, rColor );
		SubmitLine( bottom, left, rColor );
		SubmitLine( bottom, right, rColor );
	}

	void Renderer2D::SubmitAABB( const AABB& rAABB, const glm::mat4& rTransform, const glm::vec4& rColor )
	{
		glm::vec4 corners[ 8 ] =
		{
			rTransform * glm::vec4 { rAABB.Min.x, rAABB.Min.y, rAABB.Max.z, 1.0f },
			rTransform * glm::vec4 { rAABB.Min.x, rAABB.Max.y, rAABB.Max.z, 1.0f },
			rTransform * glm::vec4 { rAABB.Max.x, rAABB.Max.y, rAABB.Max.z, 1.0f },
			rTransform * glm::vec4 { rAABB.Max.x, rAABB.Min.y, rAABB.Max.z, 1.0f },

			rTransform * glm::vec4 { rAABB.Min.x, rAABB.Min.y, rAABB.Min.z, 1.0f },
			rTransform * glm::vec4 { rAABB.Min.x, rAABB.Max.y, rAABB.Min.z, 1.0f },
			rTransform * glm::vec4 { rAABB.Max.x, rAABB.Max.y, rAABB.Min.z, 1.0f },
			rTransform * glm::vec4 { rAABB.Max.x, rAABB.Min.y, rAABB.Min.z, 1.0f }
		};


		/*
		* (Z = Max.z (Top face))
		* Y+
		   1--------2
		  /|       /|
		 / |      / |
		0--|-----3  |
		|  5-----|--6    (Z = Min.z [Bottom Face])
		| /      | /     /
		|/       |/     /
		4--------7     ---> X+
		*/

		// Top (0 -> 1, 1 -> 2, 2 -> 3, 3 -> 0)
		for( uint32_t i = 0; i < 4; i++ )
			SubmitLine( corners[ i ], corners[ ( i + 1 ) % 4 ], rColor );

		// Bottom ( 4 -> 5, 5 -> 6, 6 -> 7, 7 -> 4 )
		for( uint32_t i = 0; i < 4; i++ )
			SubmitLine( corners[ i + 4 ], corners[ ( ( i + 1 ) % 4 ) + 4 ], rColor );

		// Vertical ( 0 -> 4, 1 -> 5, 2 -> 6, 3 -> 7 )
		for( uint32_t i = 0; i < 4; i++ )
			SubmitLine( corners[ i ], corners[ i + 4 ], rColor );
	}

	void Renderer2D::SubmitAABB( const AABB& rAABB, const glm::vec4& rColor )
	{
		glm::vec4 corners[ 8 ] =
		{
			glm::vec4 { rAABB.Min.x, rAABB.Min.y, rAABB.Max.z, 1.0f },
			glm::vec4 { rAABB.Min.x, rAABB.Max.y, rAABB.Max.z, 1.0f },
			glm::vec4 { rAABB.Max.x, rAABB.Max.y, rAABB.Max.z, 1.0f },
			glm::vec4 { rAABB.Max.x, rAABB.Min.y, rAABB.Max.z, 1.0f },

			glm::vec4 { rAABB.Min.x, rAABB.Min.y, rAABB.Min.z, 1.0f },
			glm::vec4 { rAABB.Min.x, rAABB.Max.y, rAABB.Min.z, 1.0f },
			glm::vec4 { rAABB.Max.x, rAABB.Max.y, rAABB.Min.z, 1.0f },
			glm::vec4 { rAABB.Max.x, rAABB.Min.y, rAABB.Min.z, 1.0f }
		};

		/*
		* (Z = Max.z (Top face))
		* Y+
		   1--------2
		  /|       /|
		 / |      / |
		0--|-----3  |
		|  5-----|--6    (Z = Min.z [Bottom Face])
		| /      | /     /
		|/       |/     /
		4--------7     ---> X+
		*/

		// Top (0 -> 1, 1 -> 2, 2 -> 3, 3 -> 0)
		for( uint32_t i = 0; i < 4; i++ )
			SubmitLine( corners[ i ], corners[ ( i + 1 ) % 4 ], rColor );

		// Bottom ( 4 -> 5, 5 -> 6, 6 -> 7, 7 -> 4 )
		for( uint32_t i = 0; i < 4; i++ )
			SubmitLine( corners[ i + 4 ], corners[ ( ( i + 1 ) % 4 ) + 4 ], rColor );

		// Vertical ( 0 -> 4, 1 -> 5, 2 -> 6, 3 -> 7 )
		for( uint32_t i = 0; i < 4; i++ )
			SubmitLine( corners[ i ], corners[ i + 4 ], rColor );
	}

	void Renderer2D::SubmitTriangle( const glm::vec3& rV0, const glm::vec3& rV1, const glm::vec3& rV2, const glm::vec4& rColor )
	{
		SubmitVertex( rV0, rColor );
		SubmitVertex( rV1, rColor );
		SubmitVertex( rV2, rColor );
	}

	void Renderer2D::SubmitVertex( const glm::vec3& rV0, const glm::vec4& rColor )
	{
		SAT_CORE_ASSERT( m_TriangleIndexCount < s_MaxLineIndices );

		auto& prCurrentTrianglePtr = GetTriangleLineBuffer();

		prCurrentTrianglePtr->Position = rV0;
		prCurrentTrianglePtr->Color = rColor;

		++prCurrentTrianglePtr;
		++m_TriangleIndexCount;
	}

	void Renderer2D::SetCamera( const RendererCamera& rRendererCamera )
	{
		m_CameraView = rRendererCamera.ViewMatrix;
		m_CameraViewProjection = rRendererCamera.pCamera->ProjectionMatrix() * rRendererCamera.ViewMatrix;
	}

	void Renderer2D::PreRender()
	{
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();
		
		m_QuadIndexCount = 0;
		for( size_t i = 0; i < m_pCurrentQuadPtr.size(); i++ )
			m_pCurrentQuadPtr[ i ] = m_CurrentQuadBases[ i ][ frame ];

		m_LineIndexCount = 0;
		for( size_t i = 0; i < m_CurrentLinePtr.size(); i++ )
			m_CurrentLinePtr[ i ] = m_CurrentLineBases[ i ][ frame ];

		m_TriangleIndexCount = 0;
		for( size_t i = 0; i < m_CurrentTrianglePtr.size(); i++ )
			m_CurrentTrianglePtr[ i ] = m_CurrentTriangleBases[ i ][ frame ];
	
		// Not great... but, we want to clear the textures and reset the slot.
		// So works for now.
		// TODO: Fix (later)
		m_Textures.fill( nullptr );
		m_Textures[ 0 ] = Renderer::Get()->GetPinkTexture();
		m_CurrentTextureSlot = 1;
	}

	void Renderer2D::RenderAll()
	{
		VkExtent2D Extent = { m_Width, m_Height };

		m_TargetRenderPass->BeginPass( m_CommandBuffer, m_TargetFramebuffer->GetVulkanFramebuffer(), Extent );

		VkViewport Viewport = {};
		Viewport.x = 0;
		Viewport.y = 0;
		Viewport.width = ( float ) m_Width;
		Viewport.height = ( float ) m_Height;
		Viewport.minDepth = 0.0f;
		Viewport.maxDepth = 1.0f;

		VkRect2D Scissor = { .offset = { 0,0 }, .extent = Extent };

		vkCmdSetScissor( m_CommandBuffer, 0, 1, &Scissor );
		vkCmdSetViewport( m_CommandBuffer, 0, 1, &Viewport );

		RenderAllQuads();
		RenderAllLines();

		m_TargetRenderPass->EndPass();
	}

	void Renderer2D::RenderAllQuads()
	{
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();

		struct QuadMatricesObject
		{
			glm::mat4 ViewProjection = glm::mat4( 1.0f );
		} u_Matrices;

		u_Matrices.ViewProjection = m_CameraViewProjection;

		m_QuadMaterial->UploadDataToUB( 0, &u_Matrices, sizeof( u_Matrices ) );

		for( size_t i = 0; i <= m_QuadBufferIndex; i++ )
		{
			const uint32_t dataSize = ( uint32_t ) ( ( uint8_t* ) m_pCurrentQuadPtr[ i ] - ( uint8_t* ) m_CurrentQuadBases[ i ][ frame ] );

			if( dataSize )
			{
				m_QuadVertexBuffers[ i ][ frame ]->Reallocate( m_CurrentQuadBases[ i ][ frame ], dataSize );

				for( uint32_t j = 0; j < m_Textures.size(); j++ )
				{
					if( m_Textures[ j ] )
						m_QuadMaterial->SetResource( "u_InputTexture", m_Textures[ j ], j );
					else
						m_QuadMaterial->SetResource( "u_InputTexture", Renderer::Get()->GetPinkTexture(), j );
				}

				m_QuadMaterial->Bind( m_CommandBuffer, m_QuadPipeline->GetPipelineLayout(), {} );

				m_QuadPipeline->Bind( m_CommandBuffer );

				m_QuadIndexBuffer->Bind( m_CommandBuffer );

				m_QuadVertexBuffers[ i ][ frame ]->Bind( m_CommandBuffer );

				const glm::mat4 transform = glm::mat4( 1.0f );
				vkCmdPushConstants( m_CommandBuffer, m_QuadPipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( glm::mat4 ), &transform );

				const uint32_t indexCount = i == m_QuadBufferIndex ? m_QuadIndexCount - ( s_MaxIndices * ( uint32_t ) i ) : s_MaxIndices;
				vkCmdDrawIndexed( m_CommandBuffer, indexCount, 1, 0, 0, 0 );
			}
		}
	}

	void Renderer2D::RenderAllLines()
	{
		const uint32_t frame = Renderer::Get()->GetCurrentFrame();

		struct QuadMatricesObject
		{
			glm::mat4 ViewProjection = glm::mat4( 1.0f );
		} u_Matrices;

		u_Matrices.ViewProjection = m_CameraViewProjection;

		m_LineMaterial->UploadDataToUB( 0, &u_Matrices, sizeof( u_Matrices ) );

		for( size_t i = 0; i <= m_LineBufferIndex; i++ )
		{
			const uint32_t dataSize = ( uint32_t ) ( ( uint8_t* ) m_CurrentLinePtr[ i ] - ( uint8_t* ) m_CurrentLineBases[ i ][ frame ] );
			if( dataSize )
			{
				m_LineVertexBuffers[ i ][ frame ]->Reallocate( m_CurrentLineBases[ i ][ frame ], dataSize );

				m_LineMaterial->Bind( m_CommandBuffer, m_LinePipeline->GetPipelineLayout(), {} );

				m_LinePipeline->Bind( m_CommandBuffer );

				m_LineIndexBuffer->Bind( m_CommandBuffer );

				m_LineVertexBuffers[ i ][ frame ]->Bind( m_CommandBuffer );

				uint32_t indexCount = 0;
				if( i == m_LineBufferIndex )
				{
					uint32_t base = s_MaxLineIndices * ( uint32_t ) i;

					SAT_CORE_ASSERT( m_LineIndexCount >= base );

					indexCount = m_LineIndexCount - base;
				}
				else
				{
					indexCount = s_MaxLineIndices;
				}
				
				vkCmdDrawIndexed( m_CommandBuffer, indexCount, 1, 0, 0, 0 );
			}
		}

		for( size_t i = 0; i <= m_LineTriangleBufferIndex; i++ )
		{
			// solid
			const uint32_t dataSize = ( uint32_t ) ( ( uint8_t* ) m_CurrentTrianglePtr[ i ] - ( uint8_t* ) m_CurrentTriangleBases[ i ][ frame ] );
			if( dataSize )
			{
				m_TriangleVertexBuffers[ i ][ frame ]->Reallocate( m_CurrentTriangleBases[ i ][ frame ], dataSize );

				m_LineMaterial->Bind( m_CommandBuffer, m_TrianglePipeline->GetPipelineLayout(), {} );

				m_TrianglePipeline->Bind( m_CommandBuffer );

				m_TriangleIndexBuffer->Bind( m_CommandBuffer );

				m_TriangleVertexBuffers[ i ][ frame ]->Bind( m_CommandBuffer );

				const uint32_t indexCount = i == m_LineTriangleBufferIndex ? m_TriangleIndexCount - ( s_MaxLineIndices * ( uint32_t ) i ) : s_MaxLineIndices;
				vkCmdDrawIndexed( m_CommandBuffer, indexCount, 1, 0, 0, 0 );
			}
		}
	}

	void Renderer2D::Render()
	{
		m_CommandBuffer = Renderer::Get()->ActiveCommandBuffer();

		// First, check if we have a render pass.
		if( !m_TargetRenderPass )
		{
			return;
		}

		CmdBeginDebugLabel( m_CommandBuffer, "Late Composite/Renderer2D" );

		RenderAll();

		CmdEndDebugLabel( m_CommandBuffer );
	}

}
