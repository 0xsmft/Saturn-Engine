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

#include "Saturn/Core/Ruby/RubyCore.h"

#include <string>
#include <stdint.h>

#if defined( SAT_RBY_INCLUDE_VULKAN )
#include <vulkan.h>
#endif

namespace Saturn {

	class RubyWindow;

	enum class RubyMouseCursorSetReason
	{
		User,
		Resize,
		Backend
	};

	class RubyBackendBase
	{
	public:
		RubyBackendBase() = default;

		RubyBackendBase( const RubyWindowSpecification& rSpec, RubyWindow* pWindow )
			: m_WindowSpecification( rSpec ), m_pWindow( pWindow )
		{
		}
		virtual ~RubyBackendBase() = default;

	public:
		virtual void Create() = 0;
		virtual void DestroyWindow() = 0;

		virtual void CloseWindow() = 0;
		virtual void PresentWindow( RubyWindowShowCmd Command = RubyWindowShowCmd::Default ) = 0;

		virtual void ResizeWindow( uint32_t Width, uint32_t Height ) = 0;
		virtual RubyIVec2 GetSize() = 0;

		virtual void SetTitle( const std::string& rTitle ) = 0;
		virtual void SetTitle( const std::wstring& rTitle ) = 0;

		virtual void Maximize() = 0;
		virtual void Minimize() = 0;
		virtual void Restore() = 0;

		virtual void MoveWindow( int x, int y ) = 0;

		virtual void SetMousePos( double x, double y ) = 0;
		virtual RubyVec2 GetMousePos() = 0;

		virtual void SetClipboardText( const std::string& rTextData ) = 0;
		virtual void SetClipboardText( const std::wstring& rTextData ) = 0;

		virtual const char* GetClipboardText() = 0;
		virtual const wchar_t* GetClipboardTextW() = 0;

		virtual WindowType GetNativeHandle() = 0;

		virtual VkResult CreateVulkanWindowSurface( VkInstance Instance, VkSurfaceKHR* pOutSurface ) = 0;

		virtual bool Minimized() = 0;
		virtual bool Maximized() = 0;
		virtual bool Focused() = 0;

		virtual void SetMouseCursor( RubyCursorType Cursor, RubyMouseCursorSetReason Reason = RubyMouseCursorSetReason::User ) = 0;
		virtual void SetMouseCursorMode( RubyCursorMode mode ) = 0;
		virtual void Focus() = 0;

		virtual RubyIVec2 GetWindowPos() = 0;
		virtual bool MouseInRect() = 0;

		virtual void FlashAttention() = 0;

	public:
		virtual bool PendingClose() = 0;

	protected:
		RubyWindowSpecification m_WindowSpecification{};
		RubyWindow* m_pWindow = nullptr;

		bool m_ShouldClose = false;
		bool m_BlockMouseCursor = false;

	private:
		friend class RubyWindow;
	};
}
