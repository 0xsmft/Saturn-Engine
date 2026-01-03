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

#include "AluraElement.h"
#include "AluraStyle.h"
#include "AluraFont.h"

#include "Saturn/Core/Base.h"
#include "Saturn/Core/UUID.h"
#include "Saturn/Core/Ruby/RubyEventType.h"

#include <glm/glm.hpp>
#include <stack>

namespace Saturn {

	class AluraRenderer;

	//
	// AluraLayout
	//
	// This struct holds the current layout information which is then used to calculate where to place elements.
	// This struct is volatile meaning that at the beginning of every frame the information in the layout is reset.
	//
	struct AluraLayout
	{
		// Current emitting position.
		glm::vec2 CursorPos{ 0.0f };
		glm::vec2 CursorPosPrevLine{ 0.0f };

		glm::vec2 CurrLineSize{ 0.0f };
		glm::vec2 PrevLineSize{ 0.0f };
		float     CurrLineTextBaseOffset{ 0.0f };
		float     PrevLineTextBaseOffset{ 0.0f };
		float     CurrentIndent{ 0.0f };

		bool IsSameLine = false;
	};

	struct AluraColorTemp
	{
		glm::vec4 OldValue{};
		std::underlying_type_t<AluraColor> Index;
	};
	
	struct AluraCanvasSpecification
	{
		glm::vec2 Size{};
	
		// Position should be relative to the main window's position.
		glm::vec2 Position{};
		
		// TODO: Be a bit nicer to the user and have an embedded default font.
		// Specify the main font for this canvas to use, you *must* have a font in order for the canvas to be created.
		AssetID   MasterFontAssetID = 0;
	};

	//
	// AluraCanvas
	//
	// AluraCanvas acts as the current "viewport" per se, it acts as a bridge between the lower level AluraRenderer
	// Use this class to draw elements directly on to the screen.
	//
	// NOTE: All position parameters in drawing functions are relative to the canvas, meaning that an element placed at 
	// 0,0 will be the top-left of the canvas even if the the canvas is moved.
	//
	class AluraCanvas : public RefTarget
	{
	public:
		// NOTE: The position must be relative to the window's position!
		AluraCanvas( const AluraCanvasSpecification& rSpecification );
		~AluraCanvas();

		// Init
		void Begin();
		void Destory();

		void SetContext( Ref<AluraRenderer> context );

		void PushFontAndSetActive( Ref<AluraFont> font );
		
		// Pops the newest font in the fonts list.
		// NOTE: There must always be an active font, so if you pop the last remaining font, Saturn will assert.
		void PopFont();

	public:
		[[nodiscard]] AluraElement* GetLastElement();

		// Drawing and widgets
		AluraElement& AddRect( const glm::vec2& rSize, const glm::vec4& rColor = glm::one<glm::vec4>() );

#if !defined(SAT_DIST)
		AluraElement& AddImage( const glm::vec2& rSize, Ref<Texture2D> image, const glm::vec4& rColor = glm::one<glm::vec4>(), const glm::vec2& rUV1 = { 0.0F, 1.0F }, const glm::vec2& rUV2 = { 1.0F, 0.0F } );
		
		[[nodiscard]] bool AddImageButton( const glm::vec2& rSize, Ref<Texture2D> image, const glm::vec4& rColor = glm::one<glm::vec4>(), const glm::vec2& rUV1 = { 0.0F, 1.0F }, const glm::vec2& rUV2 = { 1.0F, 0.0F } );
#else
		AluraElement& AddImage( const glm::vec2& rSize, Ref<Texture2D> image, const glm::vec4& rColor = glm::one<glm::vec4>(), const glm::vec2& rUV1 = { 1.0F, 0.0F }, const glm::vec2& rUV2 = { 0.0F, 1.0F } );
		[[nodiscard]] bool AddImageButton( const glm::vec2& rSize, Ref<Texture2D> image, const glm::vec4& rColor = glm::one<glm::vec4>(), const glm::vec2& rUV1 = { 1.0F, 0.0F }, const glm::vec2& rUV2 = { 0.0F, 1.0F } );
#endif

		// NOTE: fraction is a normalised value between 0.0 - 1.0, because we are working with the percent in decimal from.
		AluraElement& AddProgressBar( float fraction, const glm::vec2& rSize  );

		AluraElement& AddText( const std::string& rText, const glm::vec4& rColor = glm::one<glm::vec4>() );
		
		[[nodiscard]] bool AddButton( const glm::vec2& rSize, const glm::vec4& rColor = glm::one<glm::vec4>() );
		
		[[nodiscard]] bool AddButton( const std::string& rText );

	public:
		// Widget control and hit testing
		void SetNextItemPosition( const glm::vec2& rPosition );

		void Indent( float width = 0.0f );
		void Unindent( float width = 0.0f );

		[[nodiscard]] bool IsItemHovered();
		[[nodiscard]] bool IsItemClicked( RubyMouseButton mouseBtn );
		void AlignNextItemCenterXY( const glm::vec2& rSize );

		void PushStyle( std::underlying_type_t<AluraColor> index, const glm::vec4& rNewValue );
		void PopStyle();

	public:
		glm::vec2 GetPosition() const { return m_Position; }
		glm::vec2 GetSize() const { return m_Size; }
		
		float GetWidth() const { return m_Size.x; }
		float GetHeight() const { return m_Size.y; }

		const AluraStyle& GetStyle() const { return m_Style; }

	public:
		inline void SetPosition( const glm::vec2& rPosition ) 
		{
			m_Position = rPosition;
		}

		inline void SetSize( const glm::vec2& rSize )
		{
			m_Size = rSize;
		}

	private:
		void AdvanceCursor( const glm::vec2& rSize );
		bool IsMouseHoveringRect( const glm::vec2& rMin, const glm::vec2& rMax ) const;


	private:
		void InitStyle();

	private:
		UUID m_ID;
		glm::vec2 m_Size;
		glm::vec2 m_Position;

		glm::vec2 m_PendingNextItemPosition{};
		bool m_WantToSetItemPosition = false;

		// The mouse position relative to this canvas' positions.
		glm::vec2 m_MousePosition{};

		Ref<AluraRenderer> m_Renderer;

		std::vector<AluraElement> m_Elements;
		std::vector<Ref<AluraFont>> m_Fonts;
		std::stack<AluraColorTemp> m_ColorStack;

		Ref<AluraFont> m_ActiveFont = nullptr;

		AluraStyle m_Style{};
		AluraLayout m_Layout{};
	};
	
}
