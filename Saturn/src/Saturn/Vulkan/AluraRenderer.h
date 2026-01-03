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

#include "Saturn/Core/Ref.h"

#include "Pass.h"
#include "Texture.h"
#include "Framebuffer.h"
#include "VertexBuffer.h"

namespace Saturn {

	struct AluraVertex
	{
		glm::vec2 Position;
		glm::vec2 TexCoord;
		glm::vec4 Color;
		float TextureIndex;
	};

	struct AluraTextVertex
	{
		glm::vec2 Position;
		glm::vec2 TexCoord;
		glm::vec4 Color;
		float TextureIndex;
	};

	class AluraFont;

	class AluraRenderer : public RefTarget
	{
	public:
		AluraRenderer();
		~AluraRenderer();

		void Init( Ref<Pass> targetPass, Ref<Framebuffer> targetFramebuffer );
		void Terminate();
		void SetViewportSize( uint32_t w, uint32_t h );
		void SetCamera( const RendererCamera& rRendererCamera );
		void PreRender();
		void Render();
		void EndFrame();

		void SubmitRect( const glm::vec2& rMin, const glm::vec2& rMax, const glm::vec4& rColor );
		void SubmitRectFrame( const glm::vec2& rMin, const glm::vec2& rMax, float thinkness, const glm::vec4& rColor );
		void SubmitRect( const glm::vec2& rMin, const glm::vec2& rMax, Ref<Texture2D> texture, const glm::vec4& rColor, const glm::vec2& rUV1 = { 0.0f, 1.0f }, const glm::vec2& rUV2 = { 1.0f, 1.0f } );
	
		void SubmitString( const std::string& rText, Ref<AluraFont> font, float fontScale, const glm::vec2& rCursorPos, const glm::vec4& rColor );

		void SubmitTextGlyph( const glm::vec2& rMin, const glm::vec2& rMax, const glm::vec2& rTexCoordMin, const glm::vec2& rTexCoordMax, const glm::vec4& rColor, Ref<Texture2D> atlasTexture, const glm::vec2& rCursorPos );

	public:
		[[nodiscard]] uint32_t Width() const { return m_Width; }
		[[nodiscard]] uint32_t Height() const { return m_Height; }

	private:
		void OnResize();
		void RenderProper();
		void InitBuffers();
		void InitPhase2();

	private:
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		glm::mat4 m_Projection{};

		bool m_Resized = false;

		VkCommandBuffer m_CommandBuffer = nullptr;

		std::vector< AluraVertex* > m_VertexBase;
		AluraVertex* m_pVertexPtr = nullptr;

		uint32_t m_QuadVertexCount = 0;
		uint32_t m_QuadIndexCount = 0;
		
		uint32_t m_TextIndexCount = 0;

		uint32_t m_FallbackTextureSlot = 1;
		uint32_t m_CurrentTextureSlot = 0;
		uint32_t m_CurrentTextureAtlasSlot = 0;

		std::vector< AluraTextVertex* > m_TextVertexBase;
		AluraTextVertex* m_pTextVertexPtr = nullptr;

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
