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

#include "AluraStyle.h"
#include "AluraFont.h"
#include "AluraDrawer.h"
#include "AluraRect.h"

#include "Saturn/Core/Base.h"
#include "Saturn/Core/UUID.h"
#include "Saturn/Core/Ruby/RubyEventType.h"

#include <glm/glm.hpp>

namespace Saturn {

	class AluraRenderer;

	//
	// AluraLayout
	//
	// This struct holds the current layout information which is then used to calculate where to place elements.
	// This struct is volatile meaning that at the beginning of every frame the information in the layout is reset.
	//
	class AluraLayout
	{
	public:
		// Starting point of the Cursor
		glm::vec2 CursorStartingPos{ 0.0f };

		// Current emitting position.
		glm::vec2 CursorPos{ 0.0f };
		glm::vec2 CursorPosPrevLine{ 0.0f };

		glm::vec2 CurrLineSize{ 0.0f };
		glm::vec2 PrevLineSize{ 0.0f };
		// Baseline offset (0.0f by default on a new line, generally == style.FramePadding.y when a framed item has been added).
		float     CurrLineTextBaseOffset{ 0.0f };
		float     PrevLineTextBaseOffset{ 0.0f };
		float     CurrentIndent{ 0.0f };

		bool IsSameLine = false;

	public:
		void Reset();
	};

	// Backup data when PushStyle is called
	struct AluraColorTemp
	{
		glm::vec4 OldValue{};
		std::underlying_type_t<AluraColor> Index;
	};
	
	struct AluraRegionData
	{
		glm::vec2 StartingPosition;
		glm::vec2 Size;
		uint64_t ID = 0llu;
		uint64_t ParentID = 0llu;
	};

	struct AluraCanvasSpecification
	{
		glm::vec2 Size{};
	
		// Position should be relative to the main window's position.
		glm::vec2 Position{};
		
		// TODO: Be a bit nicer to the user and have an embedded default font.
		// Specify the main font for this canvas to use, you *must* have a font in order for the canvas to be created.
		AssetID   MasterFontAssetID = 0;

		// Specify the main styling profile to be used, you don't need to have a styling profile asset as Alura will automatically default the style if no profile is specified.
		AssetID  StylingProfile = 0;
	};

	enum class AluraInputState : uint8_t
	{
		NoState  = 0,
		Pressed  = 1,
		Held	 = 2,
		Released = 3,
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
	// Y+ == down
	// 
	// Only one AluraCanvas can exist and at a time, an attempt to create a new canvas while there is already an
	// existing one will result in an assert.
	//
	class AluraCanvas : public RefTarget
	{
	public:
		AluraCanvas( const AluraCanvasSpecification& rSpecification );
		virtual ~AluraCanvas();

		// Init
		void NewFrame();
		void DrawAllDrawers( Timestep ts );
		void HandleDrawerEvents( Event& rEvent );
		void Destroy();
		void OnSceneChange();
		void EndFrame();

		void AddDrawer( Ref<AluraDrawer> drawer );
		void SetContext( Ref<AluraRenderer> context );

		void PushFontAndSetActive( Ref<AluraFont> font );
		// Pops the newest font in the fonts list.
		// NOTE: There must always be an active font, so if you pop the last remaining font, Saturn will assert.
		void PopFont();

	public:
		// Drawing and widgets
		void AddRect( const glm::vec2& rSize, const glm::vec4& rColor = glm::one<glm::vec4>() );

#if !defined(SAT_DIST)
		void AddImage( const glm::vec2& rSize, Ref<Texture2D> image, const glm::vec4& rColor = glm::one<glm::vec4>(), const glm::vec2& rUV1 = { 0.0F, 1.0F }, const glm::vec2& rUV2 = { 1.0F, 0.0F } );
		
		[[nodiscard]] bool AddImageButton( const glm::vec2& rSize, Ref<Texture2D> image, const glm::vec4& rColor = glm::one<glm::vec4>(), const glm::vec2& rUV1 = { 0.0F, 1.0F }, const glm::vec2& rUV2 = { 1.0F, 0.0F } );
#else
		void AddImage( const glm::vec2& rSize, Ref<Texture2D> image, const glm::vec4& rColor = glm::one<glm::vec4>(), const glm::vec2& rUV1 = { 1.0F, 0.0F }, const glm::vec2& rUV2 = { 0.0F, 1.0F } );
		[[nodiscard]] bool AddImageButton( const glm::vec2& rSize, Ref<Texture2D> image, const glm::vec4& rColor = glm::one<glm::vec4>(), const glm::vec2& rUV1 = { 1.0F, 0.0F }, const glm::vec2& rUV2 = { 0.0F, 1.0F } );
#endif

		// NOTE: fraction is a normalised value between 0.0 - 1.0, because we are working with the percent in decimal from.
		void AddProgressBar( float fraction, const glm::vec2& rSize );

		void AddText( const std::string& rText, const glm::vec4& rColor = glm::one<glm::vec4>() );
		
		template<typename... Args>
		void TextFormatted( std::format_string<Args...> fmt, Args&&... rrArgs ) 
		{
			std::string text;
			std::format_to( std::back_inserter( text ), fmt, std::forward<Args>( rrArgs )... );

			AddText( text );
		}

		[[nodiscard]] bool AddButton( const glm::vec2& rSize, const glm::vec4& rColor = glm::one<glm::vec4>() );
		
		// Add a button with text.
		// If no size is specified then Alura will calculate the spacing needed.
		[[nodiscard]] bool AddButton( const std::string& rText, const glm::vec2& rSize = glm::zero<glm::vec2>() );

		void AddCircle( float radius, float thinkness = 1.0f, bool filled = false, const glm::vec4& rColor = glm::one<glm::vec4>() );

		[[nodiscard]] bool AddCheckbox( const std::string& rLabel, bool* pValue );
		[[nodiscard]] bool AddCheckboxRight( const std::string& rLabel, bool* pValue );


		void BeginRegion( const std::string& rID, const glm::vec2& rBounds );
		void EndRegion();

		void AddDummy( const glm::vec2& rSize );

		void DrawDemo();

	public:
		// Widget control, style control and hit testing
		void SetNextItemPosition( const glm::vec2& rPosition );

		void Indent( float width = 0.0f );
		void Unindent( float width = 0.0f );

		void AlignNextItemCenterXY( const glm::vec2& rSize );
		void AlignNextItemCenterX( const glm::vec2& rSize );

		// Add rOffset to the next item position.
		void NudgeNextItemPosition( const glm::vec2& rOffset, bool addItemSpacing = true );

		void SameLine( float offset = 0.0f, float spacing = -1.0f );

		void PushStyle( std::underlying_type_t<AluraColor> index, const glm::vec4& rNewValue );
		void PopStyle();

		void PushFontSize( float newSize );
		void PopFontSize();

		float GetFrameHeight() const;

		[[nodiscard]] glm::vec2 CalcTextSize( const std::string& rText );

		[[nodiscard]] bool IsAnyItemHot() const      { return m_Hot != 0llu; }
		[[nodiscard]] bool IsAnyItemActive() const   { return m_Active != 0llu; }
		[[nodiscard]] bool IsAnyItemFocused() const  { return m_Focused != 0llu; }
		[[nodiscard]] bool IsAnyItemSelected() const { return m_Selected != 0llu; }

	public:
#if !defined(SAT_DIST)
		// Editor only function, clears the users drawing commands to allow us to draw on top of it.
		void EdClearCanvas();
#endif

	public:
		glm::vec2 GetPosition() const { return m_Position; }
		glm::vec2 GetSize() const { return m_CanvasSize; }
		
		float GetWidth() const { return m_CanvasSize.x; }
		float GetHeight() const { return m_CanvasSize.y; }

		glm::vec2 GetCursorPosition() const { return m_Layout.CursorPos; }

		const AluraStyle& GetStyle() const { return m_Style; }
		Ref<AluraFont> GetActiveFont() const { return m_ActiveFont; }
		
		Ref<AluraFont> GetEditorFont() const { return m_EditorFont; }

	public:
		void UpdateMouseInputState( const RubyMouseButton btn, const AluraInputState state );
		void UpdateKeyInputState( const RubyKey btn, const AluraInputState state );

	public:
		inline void SetPosition( const glm::vec2& rPosition ) 
		{
			m_Position = rPosition;
		}

		inline void SetSize( const glm::vec2& rSize )
		{
			m_CanvasSize = rSize;
		}

	private:
		void ItemSize( const glm::vec2& rSize, float textBaselineY = -1.0f );
		bool IsMouseHoveringRect( const glm::vec2& rMin, const glm::vec2& rMax ) const;
		bool IsMouseHoveringRect( const AluraRect& rRect ) const;
		
		glm::vec2 CalcItemSize( glm::vec2 usrSize, float w, float h );

		bool ButtonBehaviour( const AluraRect& rRect, uint64_t id, bool* pOutHovered, bool* pOutHeld );

		void ResetInputStates();

		[[nodiscard]] bool MouseButtonPressed( RubyMouseButton btn );
		[[nodiscard]] bool MouseButtonReleased( RubyMouseButton btn );
		[[nodiscard]] bool KeyPressed( RubyMouseButton btn );
		[[nodiscard]] bool KeyReleased( RubyMouseButton btn );
		[[nodiscard]] bool KeyHeld( RubyMouseButton btn );

	private:
		UUID m_ID;
		glm::vec2 m_CanvasSize;
		glm::vec2 m_Position;

		glm::vec2 m_PendingNextItemPosition{};
		float m_PushedFontSize = 0.0f;
		bool m_WantToSetItemPosition = false;
		bool m_FirstFrameEver = true;

		// The mouse position relative to this canvas' positions.
		glm::vec2 m_MousePosition{};

		Ref<AluraRenderer> m_Renderer;

		std::vector<Ref<AluraDrawer>> m_Drawers;
		std::vector<Ref<AluraFont>> m_Fonts;
		std::stack<AluraColorTemp> m_ColorStack;
		std::stack<AluraRegionData> m_RegionStack;

		std::array<AluraInputState, RubyMouseButton_EnumSize> m_MouseInputStates{ AluraInputState::NoState };
		std::array<AluraInputState, RubyKey_EnumSize> m_KeyInputStates{ AluraInputState::NoState };

		Ref<AluraFont> m_ActiveFont = nullptr;
		
		// Fallback font if active font has an issue.
		// Also used by the editor as well if we need to draw some text.
		Ref<AluraFont> m_EditorFont = nullptr;

		// Hovered item.
		UUID m_Hot = 0llu;

		// Active item, may be come active if clicked, or if typing.
		UUID m_Active = 0llu;

		// Keyboard focus or gamepad.
		UUID m_Focused = 0llu;

		// Persistent selection.
		UUID m_Selected = 0llu;

		AluraStyle m_Style{};
		AluraLayout m_Layout{};
	};
	
}
