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

#if defined(SAT_PLATFORM_WINDOWS)
#include "RubyBackendBase.h"

#include <Windows.h>

namespace Saturn {

	//
	// Win32 window backend.
	//
	class RubyWindowsBackend : public RubyBackendBase
	{
	public:
		RubyWindowsBackend( const RubyWindowSpecification& rSpec, RubyWindow* pWindow );
		virtual ~RubyWindowsBackend();

		RubyWindow* GetParent() { return m_pWindow; }

	public:
		static void PollEvents();

	public:
		virtual void Maximize() override;
		virtual void Minimize() override;
		virtual void Restore() override;
		virtual bool Minimized() override;
		virtual bool Maximized() override;
		virtual bool Focused() override;
		virtual WindowType GetNativeHandle() override;
		virtual void Create() override;
		virtual void DestroyWindow() override;
		virtual void CloseWindow() override;
		virtual void PresentWindow( RubyWindowShowCmd Command = RubyWindowShowCmd::Default ) override;
		virtual void HideWindow() override;
		virtual void ResizeWindow( uint32_t Width, uint32_t Height ) override;
		virtual RubyIVec2 GetSize() override;
		virtual void MoveWindow( int x, int y ) override;
		virtual void SetTitle( const std::string& rTitle ) override;
		virtual void SetTitle( const std::wstring& rTitle ) override;
		virtual void SetMousePos( double x, double y ) override;
		virtual RubyVec2 GetMousePos() override;
		virtual VkResult CreateVulkanWindowSurface( VkInstance Instance, VkSurfaceKHR* pOutSurface ) override;
		virtual void SetMouseCursor( RubyCursorType Cursor, RubyMouseCursorSetReason Reason = RubyMouseCursorSetReason::User ) override;
		virtual void SetMouseCursorMode( RubyCursorMode mode ) override;
		virtual void SetClipboardText( const std::string& rTextData ) override;
		virtual void SetClipboardText( const std::wstring& rTextData ) override;
		virtual std::string GetClipboardText() override;
		virtual std::wstring GetClipboardTextW() override;
		virtual bool PendingClose() override;
		virtual void Focus() override;
		virtual RubyIVec2 GetWindowPos() override;
		virtual bool MouseInRect() override;
		virtual void FlashAttention() override;
		virtual void SetIcon( Ref<class Texture2D> icon ) override;

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

	private:
		DWORD ChooseStyle();
		LPTSTR ChooseCursor( RubyCursorType Cursor );

		void DisableCursor();
		void FindMouseRestorePoint();

	private:
		HWND m_Handle = nullptr;

		// For disabled mouse mode.
		RubyIVec2 m_MouseRestorePoint{};

		// The current cursor image.
		// For example: Arrow, Hand or IBeam.
		HCURSOR m_CurrentMouseCursorIcon = nullptr;
		RubyCursorType m_CurrentCursorType = RubyCursorType::None;

		bool m_MouseTracked = false;
	};
}

#endif
