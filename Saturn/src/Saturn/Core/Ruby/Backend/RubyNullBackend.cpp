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

#include "sppch.h"
#include "RubyNullBackend.h"

#include "Saturn/Vulkan/Texture.h"

namespace Saturn {
	
	RubyNullBackend::RubyNullBackend( const RubyWindowSpecification& rSpec, RubyWindow* pWindow )
	{
		m_WindowSpecification = rSpec;
		m_pWindow = pWindow;
	}

	RubyNullBackend::~RubyNullBackend()
	{
	}

	void RubyNullBackend::PollEvents()
	{
	}

	void RubyNullBackend::Maximize()
	{
	}

	void RubyNullBackend::Minimize()
	{
	}

	void RubyNullBackend::Restore()
	{
	}

	bool RubyNullBackend::Minimized()
	{
		return false;
	}

	bool RubyNullBackend::Maximized()
	{
		return false;
	}

	bool RubyNullBackend::Focused()
	{
		return false;
	}

	WindowType RubyNullBackend::GetNativeHandle()
	{
		return ( WindowType ) NULL;
	}

	void RubyNullBackend::Create()
	{
	}

	void RubyNullBackend::DestroyWindow()
	{
	}

	void RubyNullBackend::CloseWindow()
	{
	}

	void RubyNullBackend::PresentWindow( RubyWindowShowCmd Command /*= RubyWindowShowCmd::Default */ )
	{
	}

	void RubyNullBackend::HideWindow()
	{
	}

	void RubyNullBackend::ResizeWindow( uint32_t Width, uint32_t Height )
	{
	}

	RubyIVec2 RubyNullBackend::GetSize()
	{
		return { 0, 0 };
	}

	void RubyNullBackend::MoveWindow( int x, int y )
	{
	}

	void RubyNullBackend::SetTitle( const std::string& rTitle )
	{
	}

	void RubyNullBackend::SetTitle( const std::wstring& rTitle )
	{
	}

	void RubyNullBackend::SetMousePos( double x, double y )
	{
	}

	RubyVec2 RubyNullBackend::GetMousePos()
	{
		return { 0, 0 };
	}

	VkResult RubyNullBackend::CreateVulkanWindowSurface( VkInstance Instance, VkSurfaceKHR* pOutSurface )
	{
		return VK_NOT_READY;
	}

	void RubyNullBackend::SetMouseCursor( RubyCursorType Cursor, RubyMouseCursorSetReason Reason /*= RubyMouseCursorSetReason::User */ )
	{
	}

	void RubyNullBackend::SetMouseCursorMode( RubyCursorMode mode )
	{
	}

	void RubyNullBackend::SetClipboardText( const std::string& rTextData )
	{
	}

	void RubyNullBackend::SetClipboardText( const std::wstring& rTextData )
	{
	}

	std::string RubyNullBackend::GetClipboardText()
	{
		return {};
	}

	std::wstring RubyNullBackend::GetClipboardTextW()
	{
		return {};
	}

	bool RubyNullBackend::PendingClose()
	{
		return false;
	}

	void RubyNullBackend::Focus()
	{
	}

	RubyIVec2 RubyNullBackend::GetWindowPos()
	{
		return { 0, 0 };
	}

	bool RubyNullBackend::MouseInRect()
	{
		return false;
	}

	void RubyNullBackend::FlashAttention()
	{
	}

	void RubyNullBackend::SetIcon( Ref<class Texture2D> icon )
	{
	}

}
