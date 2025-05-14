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

#include "sppch.h"
#include "RubyWindow.h"

#include "Saturn/Core/StringAuxiliary.h"

#if defined(_WIN32)
#include "Backend/RubyWindowsBackend.h"
#endif

namespace Saturn {

	RubyWindow::RubyWindow( const RubyWindowSpecification& rSpec )
		: m_WindowTitle( rSpec.Name ), m_GraphicsAPI( rSpec.GraphicsAPI ), m_Style( rSpec.Style )
	{
#if defined(_WIN32)
		m_pDefaultBackend = new RubyWindowsBackend( rSpec, this );
#endif

		m_pDefaultBackend->Create();

		if( rSpec.ShowNow )
			m_pDefaultBackend->PresentWindow();
	}

	RubyWindow::~RubyWindow()
	{
		m_pEventTarget = nullptr;

		m_pDefaultBackend->DestroyWindow();
		delete m_pDefaultBackend;
	}

	void RubyWindow::PollEvents()
	{
		m_pDefaultBackend->PollEvents();

		if( !m_pDefaultBackend->Focused() && m_CursorMode >= RubyCursorMode::Hidden )
		{
			SetMouseCursorMode( RubyCursorMode::Normal );
			SetMouseCursor( RubyCursorType::Arrow );
		}
	}

	bool RubyWindow::ShouldClose()
	{
		return !m_pDefaultBackend->PendingClose();
	}

	void RubyWindow::Maximize()
	{
		// Win32 will handle if we should maximize or restore the window natively.
		// However if we aren't using a border then we'll need to do this our self.
		switch( m_Style )
		{
			case RubyStyle::Default:
			{
				m_pDefaultBackend->Maximize();
			} break;

			case RubyStyle::BorderlessNoResize:
			case RubyStyle::Borderless:
			{
				if( Maximized() )
				{
					Restore();
				}
				else
				{
					m_pDefaultBackend->Maximize();
				}
			}	break;

			default:
				break;
		}
	}

	void RubyWindow::Minimize()
	{
		// Win32 will handle if we should maximize or restore the window natively.
		// However if we aren't using a border then we'll need to do this our self.
		switch( m_Style )
		{
			case RubyStyle::Default:
			{
				m_pDefaultBackend->Minimize();
			} break;

			case RubyStyle::BorderlessNoResize:
			case RubyStyle::Borderless:
			{
				if( Minimized() )
				{
					Restore();
				}
				else
				{
					m_pDefaultBackend->Minimize();
				}
			}	break;

			default:
				break;
		}
	}

	void RubyWindow::Restore()
	{
		m_pDefaultBackend->Restore();
	}

	void RubyWindow::Resize( uint32_t Width, uint32_t Height )
	{
		m_pDefaultBackend->ResizeWindow( Width, Height );
	}

	void RubyWindow::Show( RubyWindowShowCmd Command /*= RubyWindowShowCmd::Default */ )
	{
		m_ShowCommand = Command;

		m_pDefaultBackend->PresentWindow( Command );
	}

	void RubyWindow::SetPosition( int x, int y )
	{
		m_pDefaultBackend->MoveWindow( x, y );
	}

	void RubyWindow::SetMousePos( double x, double y )
	{
		if( m_CursorMode == RubyCursorMode::Normal || m_CursorMode == RubyCursorMode::Hidden )
		{
			m_pDefaultBackend->SetMousePos( x, y );
		}
		else
		{
			m_LockedMousePosition = { ( int ) x, ( int ) y };
		}
	}

	void RubyWindow::GetMousePos( double* x, double* y )
	{
		if( m_CursorMode == RubyCursorMode::Normal || m_CursorMode == RubyCursorMode::Hidden )
		{
			m_pDefaultBackend->GetMousePos( x, y );
		}
		else
		{
			*x = m_LockedMousePosition.x;
			*y = m_LockedMousePosition.y;
		}
	}

	void RubyWindow::SetMouseCursor( RubyCursorType Cursor )
	{
		m_pDefaultBackend->SetMouseCursor( Cursor );
	}

	void RubyWindow::SetMouseCursorMode( RubyCursorMode mode )
	{
		if( m_CursorMode == mode )
			return;

		m_LastCursorMode = m_CursorMode;
		m_CursorMode = mode;

		m_pDefaultBackend->SetMouseCursorMode( mode );
	}

	RubyIVec2 RubyWindow::GetSize() const
	{
		return m_pDefaultBackend->GetSize();
	}

	uint32_t RubyWindow::GetWidth() const
	{
		return m_pDefaultBackend->GetSize().x;
	}

	uint32_t RubyWindow::GetHeight() const
	{
		return m_pDefaultBackend->GetSize().y;
	}

	const char* RubyWindow::GetClipboardText()
	{
		return m_pDefaultBackend->GetClipboardText();
	}

	const wchar_t* RubyWindow::GetClipboardTextW()
	{
		return m_pDefaultBackend->GetClipboardTextW();
	}

	bool RubyWindow::IsFocused()
	{
		return m_pDefaultBackend->Focused();
	}

	bool RubyWindow::Minimized()
	{
		return m_pDefaultBackend->Minimized();
	}

	bool RubyWindow::Maximized()
	{
		return m_pDefaultBackend->Maximized();
	}

	void RubyWindow::ChangeTitle( const std::string& rTitle )
	{
		m_WindowTitle = Auxiliary::ConvertString( rTitle );
		m_pDefaultBackend->SetTitle( m_WindowTitle );
	}

	void RubyWindow::ChangeTitle( const std::wstring& rTitle )
	{
		m_WindowTitle = rTitle;
		m_pDefaultBackend->SetTitle( rTitle );
	}

	void RubyWindow::SetClipboardText( const std::string& rTextData )
	{
		m_pDefaultBackend->SetClipboardText( rTextData );
	}

	void RubyWindow::SetClipboardText( const std::wstring& rTextData )
	{
		m_pDefaultBackend->SetClipboardText( rTextData );
	}

	void RubyWindow::Focus()
	{
		m_pDefaultBackend->Focus();
	}

	void RubyWindow::FlashAttention()
	{
		m_pDefaultBackend->FlashAttention();
	}

	bool RubyWindow::IsKeyDown( RubyKey key )
	{
		return RubyLibrary::Get().IsKeyDown( key );
	}

	bool RubyWindow::IsMouseButtonDown( RubyMouseButton button )
	{
		return RubyLibrary::Get().IsMouseButtonDown( button );
	}

	bool RubyWindow::MouseInWindow()
	{
		return m_pDefaultBackend->MouseInRect();
	}

	void RubyWindow::SetTiltebarHeight( uint32_t height )
	{
		m_TitlebarHeight = height;
	}

	void RubyWindow::SetTitlebarCondition( bool condition )
	{
		m_TitlebarCondition = condition;
	}

	WindowType RubyWindow::GetNativeHandle()
	{
		return m_pDefaultBackend->GetNativeHandle();
	}

	std::vector<const char*> RubyWindow::GetVulkanRequiredExtensions()
	{
		std::vector<const char*> Extensions;
		Extensions.push_back( "VK_KHR_surface" );

#if defined(_WIN32)
		Extensions.push_back( "VK_KHR_win32_surface" );
#endif

		return Extensions;
	}

	VkResult RubyWindow::CreateVulkanWindowSurface( VkInstance Instance, VkSurfaceKHR* pOutSurface )
	{
		return m_pDefaultBackend->CreateVulkanWindowSurface( Instance, pOutSurface );
	}

}