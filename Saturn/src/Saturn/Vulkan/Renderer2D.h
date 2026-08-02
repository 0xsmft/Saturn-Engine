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

#include "Saturn/Core/Ref.h"

#include "Saturn/Alura/AluraFont.h"

#include "Pass.h"
#include "Texture.h"
#include "Framebuffer.h"
#include "VertexBuffer.h"

namespace Saturn {

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		
		// TexCoord is calculated before we render but after we submit.
		glm::vec2 TexCoord;
		float TextureIndex;
	};

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
	};

	struct TextVertex
	{
		glm::vec3 Position;
		glm::vec2 TexCoord;
		glm::vec4 Color;
		float TextureIndex;
	};

	class Renderer2D : public RefTarget
	{
	public:
		Renderer2D();
		~Renderer2D();

		void SetInitialRenderPass( Ref<Pass> pass, Ref<Framebuffer> targetFramebuffer );
		Ref<Pass> GetTargetRenderPass() { return m_TargetRenderPass; }

		void Render();

		void SubmitQuad( const glm::mat4& transform, const glm::vec4& color );
		void SubmitQuad( const glm::vec3& position, const glm::vec4& color, const glm::vec2& size );
		void SubmitQuadTextured( const glm::mat4& transform, const glm::vec4& color, const Ref<Texture2D>& rTexture );
		
		void SubmitBillboard( const glm::vec3& position, const glm::vec4& color, const glm::vec2& rSize );
		void SubmitBillboardTextured( const glm::vec3& position, const glm::vec4& color, const Ref<Texture2D>& rTexture, const glm::vec2& rSize );
		void SubmitBillboardTexturedFlipped( const glm::vec3& position, const glm::vec4& color, const Ref<Texture2D>& rTexture, const glm::vec2& rSize );

		void SubmitLine( const glm::vec3& rStart, const glm::vec3& rEnd, const glm::vec4& rColor, bool onTop = false );
		void SubmitLine( const glm::vec3& rStart, const glm::vec3& rEnd, const glm::vec4& rColor, float Thinkness );

		void SubmitArrow( const glm::vec3& rStart, const glm::vec3& rEnd, const glm::vec4& rColor, float headLength = 10.0f, float headAngle = 0.5f );

		void SubmitDiamond( const glm::vec3& rCenter, float size, const glm::vec4& rColor );

		void SubmitSingleLine( const glm::vec3& rStart, const glm::vec4& rColor, bool onTop = false );

		void SubmitAABB( const AABB& rAABB, const glm::mat4& rTransform, const glm::vec4& rColor );
		void SubmitAABB( const AABB& rAABB, const glm::vec4& rColor );

		void SubmitTriangle( const glm::vec3& rV0, const glm::vec3& rV1, const glm::vec3& rV2, const glm::vec4& rColor );
		void SubmitVertex( const glm::vec3& rV0, const glm::vec4& rColor );

		void SubmitString( const std::string& rText, Ref<AluraFont> font, const glm::mat4& rTransform, const glm::vec4& rColor );

		void SubmitTextGlyph( const glm::vec2& rMin, const glm::vec2& rMax, const glm::vec2& rTexCoordMin, const glm::vec2& rTexCoordMax, const glm::vec4& rColor, Ref<Texture2D> atlasTexture, const glm::mat4& rTransform );

		void SetCamera( const RendererCamera& rRendererCamera );

		void PreRender();

		void Init( Ref<Pass> targetPass, Ref<Framebuffer> targetFramebuffer );
		void Terminate();
		void SetViewportSize( uint32_t w, uint32_t h );

		void ReplaceTexture( Ref<Texture2D> old, Ref<Texture2D> newTexture );

	private:
		void LateInit( Ref<Pass> targetPass = nullptr, Ref<Framebuffer> framebuffer = nullptr );

		void RenderAll();
		void RenderAllQuads();
		void RenderAllLines();
		void RenderAllText();

		void AddQuadBuffer();
		void AddLineBuffer();
		void AddLineOnTopBuffer();
		void AddTriangleLineBuffer();
		void AddTextBuffer();

		QuadVertex*& GetQuadBuffer();
		LineVertex*& GetLineBuffer();
		LineVertex*& GetLineOnTopBuffer();
		LineVertex*& GetTriangleLineBuffer();
		TextVertex*& GetTextBuffer();

	private:
		Ref<Pass> m_TargetRenderPass = nullptr;
		Ref<Pass> m_TempRenderPass = nullptr;

		using VertexBufferPerFrame = std::vector< Ref<VertexBuffer> >;

		//////////////////////////////////////////////////////////////////////////
		// QUADS
		std::vector<glm::vec4> m_QuadVertexPositions;
		// Per frame vertex buffer
		std::vector< VertexBufferPerFrame > m_QuadVertexBuffers;
		std::vector< std::vector< QuadVertex* > > m_CurrentQuadBases;
		
		std::vector< QuadVertex* > m_pCurrentQuadPtr;

		size_t m_QuadBufferIndex = 0llu;

		//////////////////////////////////////////////////////////////////////////
		// LINES
		std::vector< VertexBufferPerFrame > m_LineVertexBuffers;
		std::vector< std::vector< LineVertex* > > m_CurrentLineBases;
		std::vector<LineVertex*> m_CurrentLineVertexBufferPtr;

		size_t m_LineBufferIndex = 0llu;

		// On top lines
		std::vector< VertexBufferPerFrame > m_OnTopLineVertexBuffers;
		std::vector< std::vector< LineVertex* > > m_CurrentLineOnTopBases;
		std::vector<LineVertex*> m_CurrentLineOnTopVertexBufferPtr;

		size_t m_OnTopLineBufferIndex = 0llu;

		// Triangle (part of the lines)
		std::vector< VertexBufferPerFrame > m_TriangleVertexBuffers;
		std::vector< std::vector<LineVertex*> > m_CurrentTriangleBases;
		std::vector<LineVertex*> m_CurrentTrianglePtr;

		size_t m_LineTriangleBufferIndex = 0llu;

		//////////////////////////////////////////////////////////////////////////
		// TEXT

		std::vector< VertexBufferPerFrame > m_TextVertexBuffers;
		std::vector< std::vector<TextVertex*> > m_CurrentTextBases;
		std::vector<TextVertex*> m_CurrentTextPtr;

		size_t m_TextBufferIndex = 0llu;

		//////////////////////////////////////////////////////////////////////////
		// Counts
		uint32_t m_QuadIndexCount = 0;
		uint32_t m_LineIndexCount = 0;
		uint32_t m_LineOnTopIndexCount = 0;
		uint32_t m_TriangleIndexCount = 0;
		uint32_t m_TextIndexCount = 0;

		//////////////////////////////////////////////////////////////////////////
		
		std::array<Ref<Texture2D>, 33> m_Textures;
		uint32_t m_DefaultTextureSlot = 0;
		uint32_t m_CurrentTextureSlot = 1;

		glm::mat4 m_CameraView = glm::mat4( 1.0f );
		glm::mat4 m_CameraProjection = glm::mat4( 1.0f );
		glm::mat4 m_CameraViewProjection = glm::mat4( 1.0f );

		uint32_t m_Width = 0;
		uint32_t m_Height = 0;

		VkCommandBuffer m_CommandBuffer = nullptr;
		// Default line width specified in Pipeline.cpp
		float m_LineWidth = 2.0f;
		bool m_Resized = false;

		//////////////////////////////////////////////////////////////////////////
		// VULKAN RESOURCES
		Ref<Framebuffer> m_TargetFramebuffer = nullptr;

		// Quad
		Ref<Pipeline> m_QuadPipeline = nullptr;
		Ref<IndexBuffer> m_QuadIndexBuffer = nullptr;
		Ref<Shader> m_QuadShader = nullptr;
		Ref<Material> m_QuadMaterial = nullptr;

		// Lines
		Ref<Pipeline> m_LinePipeline = nullptr;
		Ref<IndexBuffer> m_LineIndexBuffer = nullptr;
		Ref<Shader> m_LineShader = nullptr;
		Ref<Material> m_LineMaterial = nullptr;
		
		// Line fill (triangle)
		Ref<Pipeline> m_TrianglePipeline = nullptr;
		Ref<IndexBuffer> m_TriangleIndexBuffer = nullptr;

		// Line on top
		Ref<Pipeline> m_LineOnTopPipeline = nullptr;
		Ref<IndexBuffer> m_LineOnTopIndexBuffer = nullptr;

		// Text
		Ref<Pipeline> m_TextPipeline = nullptr;
		Ref<IndexBuffer> m_TextIndexBuffer = nullptr;
		Ref<Shader> m_TextShader = nullptr;
		Ref<Material> m_TextMaterial = nullptr;
	};
}