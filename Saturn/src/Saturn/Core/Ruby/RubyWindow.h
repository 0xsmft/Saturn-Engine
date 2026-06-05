/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2023 BEAST                                                           		*
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

#include "Backend/RubyBackendBase.h"

#include "RubyEvent.h"
#include "RubyPerfTimer.h"
#include "RubyLibrary.h"

#include "Saturn/Core/Ref.h"

#include <string_view>
#include <unordered_set>

namespace Saturn {

	class RubyWindow : public RefTarget
	{
		SAT_RBY_DISABLE_COPY( RubyWindow );
	public:
		RubyWindow( const RubyWindowSpecification& rSpec );
		~RubyWindow();

		// FOR USE BY RUBYLIBRARY ONLY!
		void PollEvents();
		void Maximize();
		void Minimize();
		void Restore();
		void Resize( uint32_t Width, uint32_t Height );
		void Show( RubyWindowShowCmd Command = RubyWindowShowCmd::Default );
		void SetPosition( int x, int y );
		void SetMousePos( double x, double y );
		RubyVec2 GetMousePos();
		void SetMouseCursor( RubyCursorType Cursor );
		void SetMouseCursorMode( RubyCursorMode mode );
		void ChangeTitle( const std::string& rTitle );
		void ChangeTitle( const std::wstring& rTitle );
		void SetClipboardText( const std::string& rTextData );
		void SetClipboardText( const std::wstring& rTextData );
		void Focus();
		void FlashAttention();

		RubyIVec2 GetPosition() { return m_pDefaultBackend->GetWindowPos(); }
		RubyIVec2 GetLastMousePos()  const   { return m_LastMousePosition; }
		RubyIVec2 GetVirtualMousePos() const { return m_LockedMousePosition; }

		RubyCursorMode GetCursorMode()  const { return m_CursorMode; }
		RubyCursorMode GetLastCursorMode() const { return m_LastCursorMode; }

		RubyIVec2 GetSize()   const;
		uint32_t  GetWidth()  const;
		uint32_t  GetHeight() const;

		RubyGraphicsAPI GetGraphicsAPI() const { return m_GraphicsAPI; }
		RubyStyle GetStyle() const { return m_Style; }

		std::string  GetClipboardText();
		std::wstring GetClipboardTextW();

		[[nodiscard]] bool IsFocused();
		[[nodiscard]] bool Minimized();
		[[nodiscard]] bool Maximized();
		[[nodiscard]] bool ShouldClose();
		[[nodiscard]] bool IsKeyDown( RubyKey key );
		[[nodiscard]] bool IsMouseButtonDown( RubyMouseButton button );
		[[nodiscard]] bool MouseInWindow();

		double GetTime() const { return m_Timer.GetTicks(); }

		// Set title bar hit-test height
		void SetTiltebarHeight( uint32_t height );
		void SetTitlebarCondition( bool condition );

		uint32_t GetTitlebarHeight() const { return m_TitlebarHeight; }
		bool GetTitlebarCond() const       { return m_TitlebarCondition; }

		const std::unordered_set<RubyKey>& GetCurrentKeys() const { return RubyLibrary::Get().GetKeys(); }
		std::unordered_set<RubyKey>& GetCurrentKeys() { return RubyLibrary::Get().GetKeys(); }

		const std::unordered_set<RubyMouseButton>& GetCurrentMouseButtons() const { return RubyLibrary::Get().GetCurrentMouseButtons(); }
		std::unordered_set<RubyMouseButton>& GetCurrentMouseButtons() { return RubyLibrary::Get().GetCurrentMouseButtons(); }

	public:
		WindowType GetNativeHandle();

	public:
		//////////////////////////////////////////////////////////////////////////
		// Vulkan Functions
		std::vector<const char*> GetVulkanRequiredExtensions();
		VkResult CreateVulkanWindowSurface( VkInstance Instance, VkSurfaceKHR* pOutSurface );

		RubyWindowShowCmd GetCurrentShowCommand()  const { return m_ShowCommand; }

	public:
		template<typename Ty>
		void SetEventTarget( Ty* Target )
		{
			m_pEventTarget = Target;
		}

		RubyEventTarget* GetEventTarget() const { return m_pEventTarget; }

		// Dispatch an event immediately, this is an internal function called by a Window Backend, and should not be used!
		template<typename Ty, typename... Args>
		bool DispatchEvent( EventType Type, Args&&... args )
		{
			Ty event( Type, std::forward<Args>( args )... );

			if( m_pEventTarget )
				return m_pEventTarget->OnEvent( event );

			return false;
		}

	public:
		//////////////////////////////////////////////////////////////////////////
		// Internal Functions. Do not call.

		void IntrnlSetKeyDown( RubyKey key, bool value )
		{
			RubyLibrary::Get().SetKeyState( key, value );
		}

		void IntrnlSetMouseState( RubyMouseButton button, bool pressed = true )
		{
			RubyLibrary::Get().IntrnlSetMouseState( button, pressed );
		}

		void IntrnlSetLockedMousePos( const RubyIVec2& Position )
		{
			m_LockedMousePosition = Position;
		}

		void IntrnlSetLastMousePos( const RubyIVec2& Position )
		{
			m_LastMousePosition = Position;
		}

		void IntrnlClearKeysAndMouse()
		{
			RubyLibrary::Get().IntrnlClearKeysAndMouse();
		}

	protected:
		// Only used for borderless windows.
		uint32_t m_TitlebarHeight = 0;
		bool m_TitlebarCondition = false;

		RubyCursorMode m_CursorMode = RubyCursorMode::Normal;
		RubyCursorMode m_LastCursorMode = RubyCursorMode::Normal;

		RubyWindowShowCmd m_ShowCommand = RubyWindowShowCmd::Fullscreen;

		RubyIVec2 m_LockedMousePosition{};
		RubyIVec2 m_LastMousePosition{};

		RubyPerfTimer m_Timer;

	private:
		std::unique_ptr<RubyBackendBase> m_pDefaultBackend = nullptr;
		RubyEventTarget* m_pEventTarget = nullptr;

		std::wstring m_WindowTitle = L"";
		RubyGraphicsAPI m_GraphicsAPI = RubyGraphicsAPI::None;
		RubyStyle m_Style = RubyStyle::Default;

	private:
		friend class RubyBackendBase;

#if defined(_WIN32)
		friend class RubyWindowsBackend;
#endif
	};

}