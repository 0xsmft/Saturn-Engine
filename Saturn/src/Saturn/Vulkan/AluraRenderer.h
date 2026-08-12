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

#include "Saturn/Alura/AluraRect.h"

#include "Pass.h"
#include "Texture.h"
#include "Framebuffer.h"
#include "VertexBuffer.h"

namespace Saturn {

	struct AluraQuadVertex
	{
		glm::vec2 Position{};
		glm::vec2 TexCoord{};
		glm::vec4 Color{};
		float TextureIndex = 0.0f;
	};

	struct AluraTextVertex
	{
		glm::vec3 Position{};
		glm::vec2 TexCoord{};
		glm::vec4 Color{};
		float TextureIndex = 0.0f;
	};

	enum class AluraDrawPipelineType
	{
		NotSet,

		// Use the quad pipeline
		Quad,

		// Use the text pipeline
		Text
	};

	struct AluraDrawCommand
	{
		AluraDrawPipelineType PipelineType = AluraDrawPipelineType::NotSet;
		uint32_t IndexCount = 0u;
		uint32_t IndexOffset = 0u;
		VkRect2D Scissor{};
	};

	class AluraFont;

	class AluraRenderer : public RefTarget
	{
	public:
		AluraRenderer();
		virtual ~AluraRenderer();

		void Init( Ref<Pass> targetPass, Ref<Framebuffer> targetFramebuffer );
		void Terminate();
		void SetViewportSize( uint32_t w, uint32_t h );
		void SetCamera( const RendererCamera& rRendererCamera );
		void PreRender();
		void Render();
		void EndFrame();

		void SubmitRect( const AluraRect& rRect, const glm::vec4& rColor );
		void SubmitRect( const AluraRect& rRect, 
			Ref<Texture2D> texture, 
			const glm::vec4& rColor,
			const glm::vec2& rUV1 = { 0.0f, 1.0f }, const glm::vec2& rUV2 = { 1.0f, 1.0f } );

		void SubmitRect( const glm::vec2& rMin, const glm::vec2& rMax, const glm::vec4& rColor );
		void SubmitRect( 
			const glm::vec2& rMin, const glm::vec2& rMax, 
			Ref<Texture2D> texture, 
			const glm::vec4& rColor, 
			const glm::vec2& rUV1 = { 0.0f, 1.0f }, const glm::vec2& rUV2 = { 1.0f, 1.0f } );

		void SubmitRectFrame( const glm::vec2& rMin, const glm::vec2& rMax, float thickness, const glm::vec4& rColor );
	
		void SubmitString( 
			const std::string& rText, 
			const Ref<AluraFont> font,
			const glm::mat4& rTransform, 
			const glm::vec4& rColor );

		void SubmitString( 
			const std::string& rText, 
			const Ref<AluraFont> font, 
			const float fontSizePx, 
			const glm::vec2& rStartingPosition,
			const glm::vec4& rColor );

		void SubmitCircleFilled( const glm::vec2& rPosition, float size, float thickness, const glm::vec4& rColor );
		void SubmitCircle( const glm::vec2& rCentre, float radius, float thickness, const glm::vec4& rColor );

		void SubmitCheckMark( const glm::vec2& rPosition, const glm::vec4& rColor, float size );

		void SubmitLine( const glm::vec2& rA, const glm::vec2& rB, float thickness, const glm::vec4& rColor );
		void SubmitClipRect( const AluraRect& rRect );

#if !defined(SAT_DIST)
		// Editor only function, clears the users drawing commands to allow us to draw on top of it.
		void EdClearCommands();
#endif

	public:
		[[nodiscard]] uint32_t Width() const { return m_Width; }
		[[nodiscard]] uint32_t Height() const { return m_Height; }

	private:
		void OnResize();
		void RenderProper();
		void InitBuffers();
		void InitPhase2();

		void SubmitTextGlyph( 
			const glm::vec2& rMin, 
			const glm::vec2& rMax, 
			const glm::vec2& rTexCoordMin, 
			const glm::vec2& rTexCoordMax, 
			const glm::vec4& rColor, 
			const Ref<Texture2D> atlasTexture, 
			const glm::mat4& rTransform );

		AluraDrawCommand& GetOrCreateDrawCommand( AluraDrawPipelineType pipelineType, uint32_t indexOffset );

	private:
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		glm::mat4 m_Projection{};

		bool m_Resized = false;

		VkCommandBuffer m_CommandBuffer = nullptr;
		VkRect2D m_CurrentScissor{};

		std::vector< AluraQuadVertex* > m_QuadVertexBase;
		AluraQuadVertex* m_pQuadVertexPtr = nullptr;

		std::vector< AluraTextVertex* > m_TextVertexBase;
		AluraTextVertex* m_pTextVertexPtr = nullptr;

		std::vector< AluraDrawCommand > m_DrawCommands;

		uint32_t m_QuadVertexCount = 0;
		uint32_t m_QuadIndexCount = 0;
		
		uint32_t m_TextIndexCount = 0;

		uint32_t m_FallbackTextureSlot = 1;
		uint32_t m_CurrentTextureSlot = 0;
		uint32_t m_CurrentTextureAtlasSlot = 0;

		// Per flight in frame
		std::vector< Ref<VertexBuffer> > m_VertexBuffers;
		std::vector< Ref<VertexBuffer> > m_TextVertexBuffers;

		// Textures, 32 writable textures in total, 1 for the default fall back
		// 16 for quads
		// 16 for texture atlases
		// 1 for fallback texture
		std::array<Ref<Texture2D>, 16 + 16 + 1> m_Textures;
		
		//////////////////////////////////////////////////////////////////////////
		// VULKAN RESOURCES
		Ref<Pass> m_TargetRenderPass = nullptr;
		Ref<Framebuffer> m_TargetFramebuffer = nullptr;

		Ref<IndexBuffer> m_IndexBuffer = nullptr;

		// Quad
		Ref<Shader> m_Shader = nullptr;
		Ref<Material> m_Material = nullptr;
		Ref<Pipeline> m_Pipeline = nullptr;

		// Text
		Ref<Shader> m_TextShader = nullptr;
		Ref<Material> m_TextMaterial = nullptr;
		Ref<Pipeline> m_TextPipeline = nullptr;
	};

}
