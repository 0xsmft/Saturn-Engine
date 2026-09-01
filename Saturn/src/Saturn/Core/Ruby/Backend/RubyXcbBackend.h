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

#include "RubyBackendBase.h"

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

namespace Saturn {

	class RubyXcbBackend : public RubyBackendBase
	{
	public:
		RubyXcbBackend( const RubyWindowSpecification& rSpec, RubyWindow* pWindow );
		~RubyXcbBackend();

		RubyWindow* GetParent() { return m_pWindow; }

		void Maximize() override;
		void Minimize() override;
		void Restore() override;

		bool Minimized() override;
		bool Maximized() override;
		bool Focused() override;

		WindowType GetNativeHandle() override;

	public:
		void Create() override;
		void DestroyWindow() override;

		void CloseWindow() override;
		void PresentWindow( RubyWindowShowCmd Command = RubyWindowShowCmd::Default ) override;

		virtual void HideWindow() override;
		virtual void SetIcon( Ref<class Texture2D> icon ) override;

		void ResizeWindow( uint32_t Width, uint32_t Height ) override;
		RubyIVec2 GetSize() override;

		void MoveWindow( int x, int y ) override;

		void SetTitle( const std::string& rTitle ) override;
		void SetTitle( const std::wstring& rTitle ) override;

		void SetMousePos( double x, double y ) override;
		RubyVec2 GetMousePos() override;

		VkResult CreateVulkanWindowSurface( VkInstance Instance, VkSurfaceKHR* pOutSurface ) override;

		void SetMouseCursor( RubyCursorType Cursor, RubyMouseCursorSetReason Reason = RubyMouseCursorSetReason::User ) override;
		void SetMouseCursorMode( RubyCursorMode mode ) override;

		void SetClipboardText( const std::string& rTextData ) override;
		void SetClipboardText( const std::wstring& rTextData ) override;

		std::string GetClipboardText() override;
		std::wstring GetClipboardTextW() override;

		static void PollEvents();
		bool PendingClose() override;

		void Focus() override;
		RubyIVec2 GetWindowPos() override;

		bool MouseInRect() override;

		void FlashAttention() override;

	public:
		void BlockMouseCursor() { m_BlockMouseCursor = true; }
		void UnblockMouseCursor() { m_BlockMouseCursor = false; }

	public:
		void ConfigureClipRect();
		void RecenterMousePos();
		void UpdateCursorIcon();
		void SetResizeCursor( RubyCursorType Type );
		void ResetResizeCursor();
		bool IsMouseTracked() const { return m_MouseTracked; }
		void SetTrackMouse( bool value ) { m_MouseTracked = value; }

		xcb_key_symbols_t* GetSymbols() { return m_Symbols; }

	private:
		void DisableCursor();
		void FindMouseRestorePoint();
		static void HandleXcbEvents();

	private:
		xcb_window_t m_Handle{};
		xcb_connection_t* m_pConnection = NULL;
		xcb_key_symbols_t* m_Symbols = NULL;

		// For disabled mouse mode.
		RubyIVec2 m_MouseRestorePoint{};
		RubyCursorType m_CurrentCursorType = RubyCursorType::None;

		bool m_MouseTracked = false;
	};
}
