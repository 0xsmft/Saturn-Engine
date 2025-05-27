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
#include "Renderer2D.h"

#include "Renderer.h"
#include "VulkanDebug.h"

#include "Saturn/Core/Ruby/RubyWindow.h"

// This 2D Renderer is mostly based from Hazel's 2D renderer (https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Renderer/Renderer2D.cpp)

namespace Saturn {

	static constexpr uint32_t s_MaxQuads = 20000;
	static constexpr uint32_t s_MaxVertices = s_MaxQuads * 4;
	static constexpr uint32_t s_MaxIndices = s_MaxQuads * 6;
	static constexpr uint32_t s_MaxTextureSlots = 32;

	static constexpr uint32_t s_MaxLines = 1024 + 1024;
	static constexpr uint32_t s_MaxLineVertices = s_MaxLines * 2;
	static constexpr uint32_t s_MaxLineIndices = s_MaxLines * 6;

	static constexpr uint32_t s_MaxSolidLines = 1024 + 1024 + 1024;
	static constexpr uint32_t s_MaxSolidLineVertices = s_MaxSolidLines * 2;
	static constexpr uint32_t s_MaxSolidLineIndices = s_MaxSolidLines * 6;

	void Renderer2D::Init()
	{
		if( Application::Get().HasFlag( ApplicationFlag_UIOnly ) )
			return;

		m_Width = Application::Get().GetWindow()->GetWidth();
		m_Height = Application::Get().GetWindow()->GetHeight();

		// Setup Quads
		m_QuadVertexPositions.reserve( 4 );

		m_QuadVertexPositions.emplace_back( -0.5f, -0.5f, 0.0f, 1.0f );
		m_QuadVertexPositions.emplace_back( -0.5f,  0.5f, 0.0f, 1.0f );
		m_QuadVertexPositions.emplace_back( 0.5f,  0.5f, 0.0f, 1.0f  );
		m_QuadVertexPositions.emplace_back( 0.5f, -0.5f, 0.0f, 1.0f  );

		// Setup vertex buffer
		m_QuadVertexBuffers.resize( MAX_FRAMES_IN_FLIGHT );
		m_LineVertexBuffers.resize( MAX_FRAMES_IN_FLIGHT );
		m_TriangleVertexBuffers.resize( MAX_FRAMES_IN_FLIGHT );

		m_CurrentQuadBase.resize( MAX_FRAMES_IN_FLIGHT );
		m_CurrentLineBase.resize( MAX_FRAMES_IN_FLIGHT );
		m_CurrentTriangleBase.resize( MAX_FRAMES_IN_FLIGHT );

		for( int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			m_QuadVertexBuffers[ i ] = Ref<VertexBuffer>::Create( s_MaxVertices * sizeof( QuadDrawCommand ) );
			m_CurrentQuadBase[ i ] = new QuadDrawCommand[ s_MaxVertices ];

			m_LineVertexBuffers[ i ] = Ref<VertexBuffer>::Create( s_MaxLineVertices * sizeof( LineDrawCommand ) );
			m_CurrentLineBase[ i ] = new LineDrawCommand[ s_MaxLineVertices ];

			m_TriangleVertexBuffers[ i ] = Ref<VertexBuffer>::Create( s_MaxSolidLineVertices * sizeof( LineDrawCommand ) );
			m_CurrentTriangleBase[ i ] = new LineDrawCommand[ s_MaxSolidLineVertices ];
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

		m_QuadIndexBuffer = Ref<IndexBuffer>::Create( quadBuffer, s_MaxIndices );
		delete[] quadBuffer;

		uint32_t* pLineBuffer = new uint32_t[ s_MaxLineIndices ];
		for( uint32_t i = 0; i < s_MaxLineIndices; i++ )
			pLineBuffer[ i ] = i;

		m_LineIndexBuffer = Ref<IndexBuffer>::Create( pLineBuffer, s_MaxLineIndices );
		delete[] pLineBuffer;

		pLineBuffer = new uint32_t[ s_MaxSolidLineIndices ];
		for( uint32_t i = 0; i < s_MaxSolidLineIndices; i++ )
			pLineBuffer[ i ] = i;

		m_TriangleIndexBuffer = Ref<IndexBuffer>::Create( pLineBuffer, s_MaxSolidLineIndices );

		delete[] pLineBuffer;

		// Setup Textures
		m_Textures[ 0 ] = Renderer::Get().GetPinkTexture();

		// Construct a temporary render pass this is to be changed when the scene renderer is ready.
		PassSpecification PassSpec;
		PassSpec.Name = "Renderer2D-TemporaryRP";
		PassSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };

		m_TempRenderPass = Ref<Pass>::Create( PassSpec );
		m_TargetRenderPass = m_TempRenderPass;

		LateInit();
	}

	void Renderer2D::LateInit( Ref<Pass> targetPass /*= nullptr */, Ref<Framebuffer> targetFramebuffer /*= nullptr*/ )
	{
		if( Application::Get().HasFlag( ApplicationFlag_UIOnly ) )
			return;

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

	void Renderer2D::Reset()
	{
		uint32_t frame = Renderer::Get().GetCurrentFrame();

		m_pCurrentQuad = m_CurrentQuadBase[ frame ];
		m_QuadIndexCount = 0;

		m_pCurrentLine = m_CurrentLineBase[ frame ];
		m_LineVertexCount = 0;

		m_pCurrentTriangle = m_CurrentTriangleBase[ frame ];
		m_TriangleVextexCount = 0;

		m_CurrentTextureSlot = 1;
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

		for( auto buffer : m_CurrentQuadBase )
			delete[] buffer;

		for( auto buffer : m_CurrentLineBase )
			delete[] buffer;

		for( auto buffer : m_CurrentTriangleBase )
			delete[] buffer;
	}

	void Renderer2D::SetViewportSize( uint32_t w, uint32_t h )
	{
		if( m_Width != w && m_Height != h )
		{
			m_Width = w;
			m_Height = h;
			m_Resized = true;
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
		uint32_t frame = Renderer::Get().GetCurrentFrame();

		struct QuadMatricesObject
		{
			glm::mat4 ViewProjection = glm::mat4( 1.0f );
		} u_Matrices;

		u_Matrices.ViewProjection = m_CameraViewProjection;

		m_QuadMaterial->UploadDataToUB( 0, &u_Matrices, sizeof( u_Matrices ) );

		uint32_t dataSize = ( uint32_t ) ( ( uint8_t* ) m_pCurrentQuad - ( uint8_t* ) m_CurrentQuadBase[ frame ] );
		if( dataSize )
		{
			m_QuadVertexBuffers[ frame ]->Reallocate( m_CurrentQuadBase[ frame ], dataSize );

			for( uint32_t i = 0; i < m_Textures.size(); i++ )
			{
				if( m_Textures[ i ] )
					m_QuadMaterial->SetResource( "u_InputTexture", m_Textures[ i ], i );
				else
					m_QuadMaterial->SetResource( "u_InputTexture", Renderer::Get().GetPinkTexture(), i );
			}

			m_QuadMaterial->Bind( m_CommandBuffer, m_QuadPipeline->GetPipelineLayout(), {} );

			m_QuadPipeline->Bind( m_CommandBuffer );

			m_QuadIndexBuffer->Bind( m_CommandBuffer );

			m_QuadVertexBuffers[ frame ]->Bind( m_CommandBuffer );

			glm::mat4 transform = glm::mat4( 1.0f );
			vkCmdPushConstants( m_CommandBuffer, m_QuadPipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( glm::mat4 ), &transform );

			vkCmdDrawIndexed( m_CommandBuffer, m_QuadIndexCount, 1, 0, 0, 0 );
		}
	}

	void Renderer2D::RenderAllLines()
	{
		uint32_t frame = Renderer::Get().GetCurrentFrame();

		struct QuadMatricesObject
		{
			glm::mat4 ViewProjection = glm::mat4( 1.0f );
		} u_Matrices;

		u_Matrices.ViewProjection = m_CameraViewProjection;

		m_LineMaterial->UploadDataToUB( 0, &u_Matrices, sizeof( u_Matrices ) );

		uint32_t dataSize = ( uint32_t ) ( ( uint8_t* ) m_pCurrentLine - ( uint8_t* ) m_CurrentLineBase[ frame ] );
		if( dataSize )
		{
			m_LineVertexBuffers[ frame ]->Reallocate( m_CurrentLineBase[ frame ], dataSize );

			m_LineMaterial->Bind( m_CommandBuffer, m_LinePipeline->GetPipelineLayout(), {} );

			m_LinePipeline->Bind( m_CommandBuffer );

			m_LineIndexBuffer->Bind( m_CommandBuffer );

			m_LineVertexBuffers[ frame ]->Bind( m_CommandBuffer );

			vkCmdDrawIndexed( m_CommandBuffer, m_LineVertexCount, 1, 0, 0, 0 );
		}

		// solid
		dataSize = ( uint32_t ) ( ( uint8_t* ) m_pCurrentTriangle - ( uint8_t* ) m_CurrentTriangleBase[ frame ] );
		if( dataSize )
		{
			m_TriangleVertexBuffers[ frame ]->Reallocate( m_CurrentTriangleBase[ frame ], dataSize );

			m_LineMaterial->Bind( m_CommandBuffer, m_TrianglePipeline->GetPipelineLayout(), {} );

			m_TrianglePipeline->Bind( m_CommandBuffer );

			m_TriangleIndexBuffer->Bind( m_CommandBuffer );

			m_TriangleVertexBuffers[ frame ]->Bind( m_CommandBuffer );

			vkCmdDrawIndexed( m_CommandBuffer, m_TriangleVextexCount, 1, 0, 0, 0 );
		}
	}

	void Renderer2D::SubmitQuad( const glm::mat4& transform, const glm::vec4& color )
	{
		// One quad has 4 vertices so we need to submit them one by one.
		glm::vec2 TexCoord[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		for( size_t i = 0; i < 4; i++ )
		{
			m_pCurrentQuad->Position = transform * m_QuadVertexPositions[ i ];
			m_pCurrentQuad->Color = color;
			m_pCurrentQuad->TexCoord = TexCoord[ i ];

			m_pCurrentQuad++;
		}

		m_QuadIndexCount += 6;
	}

	void Renderer2D::SubmitQuad( const glm::vec3& position, const glm::vec4& color, const glm::vec2& size )
	{
		glm::vec2 TexCoord[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		glm::mat4 transform = glm::translate( glm::mat4( 1.0f ), position )
			* glm::scale( glm::mat4( 1.0f ), { size.x, size.y, 1.0f } );

		for( size_t i = 0; i < 4; i++ )
		{
			m_pCurrentQuad->Position = transform * m_QuadVertexPositions[ i ];
			m_pCurrentQuad->Color = color;
			m_pCurrentQuad->TexCoord = TexCoord[ i ];
			m_pCurrentQuad->TextureIndex = 0;

			m_pCurrentQuad++;
		}

		m_QuadIndexCount += 6;
	}

	void Renderer2D::SubmitQuadTextured( const glm::mat4& transform, const glm::vec4& color, const Ref<Texture2D>& rTexture )
	{
		// One quad has 4 vertexes so we need to submit them one by one.
		glm::vec2 TexCoord[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		int textureID = 0;
		for( uint32_t i = 1; i < m_CurrentTextureSlot; i++ )
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
				Reset();

			textureID = m_CurrentTextureSlot;
			m_Textures[ textureID ] = rTexture;
			m_CurrentTextureSlot++;
		}

		for( size_t i = 0; i < 4; i++ )
		{
			m_pCurrentQuad->Position = transform * m_QuadVertexPositions[ i ];
			m_pCurrentQuad->Color = color;
			m_pCurrentQuad->TexCoord = TexCoord[ i ];
			m_pCurrentQuad->TextureIndex = (float)textureID;

			m_pCurrentQuad++;
		}

		m_QuadIndexCount += 6;
	}

	void Renderer2D::SubmitBillboard( const glm::vec3& position, const glm::vec4& color, const glm::vec2& rSize )
	{
		glm::vec2 TexCoord[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		glm::vec3 CamRight = { m_CameraView[ 0 ][ 0 ], m_CameraView[ 1 ][ 0 ], m_CameraView[ 2 ][ 0 ] };
		glm::vec3 CamUp = { m_CameraView[ 0 ][ 1 ], m_CameraView[ 1 ][ 1 ], m_CameraView[ 2 ][ 1 ] };

		for( size_t i = 0; i < 4; i++ )
		{
			m_pCurrentQuad->Position = position + CamRight * ( m_QuadVertexPositions[ i ].x ) * rSize.x + CamUp * m_QuadVertexPositions[ i ].y * rSize.y;
			m_pCurrentQuad->Color = color;
			m_pCurrentQuad->TexCoord = TexCoord[ i ];
			m_pCurrentQuad->TextureIndex = 1;

			m_pCurrentQuad++;
		}

		m_QuadIndexCount += 6;
	}

	void Renderer2D::SubmitBillboardTextured( const glm::vec3& position, const glm::vec4& color, const Ref<Texture2D>& rTexture, const glm::vec2& rSize )
	{
		constexpr glm::vec2 TexCoord[] = { { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f } };

		glm::vec3 CamRight = { m_CameraView[ 0 ][ 0 ], m_CameraView[ 1 ][ 0 ], m_CameraView[ 2 ][ 0 ] };
		glm::vec3 CamUp = { m_CameraView[ 0 ][ 1 ], m_CameraView[ 1 ][ 1 ], m_CameraView[ 2 ][ 1 ] };

		int textureID = 0;
		for( uint32_t i = 1; i < m_CurrentTextureSlot; i++ )
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
				Reset();

			textureID = m_CurrentTextureSlot;
			m_Textures[ textureID ] = rTexture;
			m_CurrentTextureSlot++;
		}

		for( size_t i = 0; i < 4; i++ )
		{
			m_pCurrentQuad->Position = position + CamRight * ( m_QuadVertexPositions[ i ].x ) * rSize.x + CamUp * m_QuadVertexPositions[ i ].y * rSize.y;
			m_pCurrentQuad->Color = color;
			m_pCurrentQuad->TexCoord = TexCoord[ i ];
			m_pCurrentQuad->TextureIndex = (float)textureID;

			m_pCurrentQuad++;
		}

		m_QuadIndexCount += 6;
	}

	void Renderer2D::SubmitLine( const glm::vec3& rStart, const glm::vec3& rEnd, const glm::vec4& rColor )
	{
		if( m_LineVertexCount >= s_MaxLineVertices )
			Reset();

		m_pCurrentLine->Position = rStart;
		m_pCurrentLine->Color = rColor;
	
		m_pCurrentLine++;
	
		m_pCurrentLine->Position = rEnd;
		m_pCurrentLine->Color = rColor;

		m_pCurrentLine++;

		m_LineVertexCount += 2;
	}

	void Renderer2D::SubmitLine( const glm::vec3& rStart, const glm::vec3& rEnd, const glm::vec4& rColor, float Thinkness )
	{
		if( m_LineVertexCount >= s_MaxLineVertices )
			Reset();

		m_pCurrentLine->Position = rStart;
		m_pCurrentLine->Color = rColor;

		m_pCurrentLine++;

		m_pCurrentLine->Position = rEnd;
		m_pCurrentLine->Color = rColor;

		m_pCurrentLine++;

		m_LineVertexCount += 2;
	}

	void Renderer2D::SubmitSingleLine( const glm::vec3& rStart, const glm::vec4& rColor )
	{
		if( m_LineVertexCount >= s_MaxLineVertices )
			Reset();

		m_pCurrentLine->Position = rStart;
		m_pCurrentLine->Color = rColor;

		m_pCurrentLine++;
		m_LineVertexCount++;
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

	void Renderer2D::SubmitTriangle3( const glm::vec3& rV0, const glm::vec3& rV1, const glm::vec3& rV2, const glm::vec4& rColor )
	{
		SubmitTriangle1( rV0, rColor );
		SubmitTriangle1( rV1, rColor );
		SubmitTriangle1( rV2, rColor );
	}

	void Renderer2D::SubmitTriangle1( const glm::vec3& rV0, const glm::vec4& rColor )
	{
		if( m_TriangleVextexCount >= s_MaxSolidLineVertices )
			Reset();

		m_pCurrentTriangle->Position = rV0;
		m_pCurrentTriangle->Color = rColor;

		m_pCurrentTriangle++;
		m_TriangleVextexCount++;
	}

	void Renderer2D::SetCamera( const RendererCamera& rRendererCamera )
	{
		m_CameraView = rRendererCamera.ViewMatrix;
		m_CameraViewProjection = rRendererCamera.pCamera->ProjectionMatrix() * rRendererCamera.ViewMatrix;
	}

	void Renderer2D::PreRender()
	{
		uint32_t frame = Renderer::Get().GetCurrentFrame();
		
		m_QuadIndexCount = 0;
		m_pCurrentQuad = m_CurrentQuadBase[ frame ];

		m_LineVertexCount = 0;
		m_pCurrentLine = m_CurrentLineBase[ frame ];

		m_TriangleVextexCount = 0;
		m_pCurrentTriangle = m_CurrentTriangleBase[ frame ];
	}

	void Renderer2D::Render()
	{
		m_CommandBuffer = Renderer::Get().ActiveCommandBuffer();

		// First, check if we have a render pass.
		if( !m_TargetRenderPass || !m_pCurrentQuad || !m_pCurrentLine )
		{
			return;
		}

		CmdBeginDebugLabel( m_CommandBuffer, "Late Composite/Renderer2D" );

		RenderAll();

		CmdEndDebugLabel( m_CommandBuffer );
	}

}