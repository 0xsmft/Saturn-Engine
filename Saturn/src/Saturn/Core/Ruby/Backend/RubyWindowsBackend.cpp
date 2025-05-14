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

#include "sppch.h"
#include "RubyWindowsBackend.h"

#include "Saturn/Core/Ruby/RubyWindow.h"
#include "Saturn/Core/StringAuxiliary.h"

#if defined( SAT_RBY_INCLUDE_VULKAN )
#include <vulkan_win32.h>
#endif

#include <windowsx.h>

//////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK RubyWindowProc( HWND Handle, UINT Msg, WPARAM WParam, LPARAM LParam );

constexpr const wchar_t* DEFAULT_WINDOW_CLASS_NAME = L"RUBY_WINDOW";

struct RubyWindowRegister
{
	RubyWindowRegister() 
	{
		WNDCLASSW wc = { .style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC, .lpfnWndProc = RubyWindowProc, .hInstance = GetModuleHandle( nullptr ), .hCursor = LoadCursor( nullptr, IDC_ARROW ), .lpszClassName = DEFAULT_WINDOW_CLASS_NAME };

		::RegisterClassW( &wc );
	}

	~RubyWindowRegister()
	{
		::UnregisterClassW( DEFAULT_WINDOW_CLASS_NAME, GetModuleHandle( nullptr ) );
	}
} static s_RubyWindowRegister;

//////////////////////////////////////////////////////////////////////////

static int HandleKeyMods() 
{
	int Modifiers = Saturn::RubyKey::UnknownKey;

	if( GetKeyState( VK_LSHIFT ) & 0x8000 )
	{
		Modifiers |= Saturn::RubyKey::Shift;
	}

	if( GetKeyState( VK_RSHIFT ) & 0x8000 )
	{
		Modifiers |= Saturn::RubyKey::RightShift;
	}

	if( GetKeyState( VK_LMENU ) & 0x8000 )
	{
		Modifiers |= Saturn::RubyKey::Alt;
	}

	if( GetKeyState( VK_RMENU ) & 0x8000 )
	{
		Modifiers |= Saturn::RubyKey::RightAlt;
	}

	if( GetKeyState( VK_LCONTROL ) & 0x8000 )
	{
		Modifiers |= Saturn::RubyKey::Ctrl;
	}

	if( GetKeyState( VK_RCONTROL ) & 0x8000 )
	{
		Modifiers |= Saturn::RubyKey::RightCtrl;
	}

	return Modifiers;
}

LRESULT CALLBACK RubyWindowProc( HWND Handle, UINT Msg, WPARAM WParam, LPARAM LParam ) 
{
	using namespace Saturn;

	RubyWindowsBackend* pThis = ( RubyWindowsBackend* ) ::GetPropW( Handle, L"RubyData" );

	if( !pThis )
		return ::DefWindowProc( Handle, Msg, WParam, LParam );

	switch( Msg )
	{
		case WM_QUIT: 
		case WM_CLOSE:
		{
			// Send a last minute event to tell the client that the window is about to close.
			pThis->GetParent()->DispatchEvent<RubyEvent>( RubyEventType::Close );

			pThis->CloseWindow();
			
			// Don't tell the OS about this message because if we did then the OS will destroy the window, that can be good but this will cause the engine to crash because the surface will be lost as the window no longer exists.
			// So instead we just return and then the Engine will take care of the rest.
		} return TRUE;

		case WM_DESTROY:
		{
			::PostQuitMessage( 0 );
		} break;

		//////////////////////////////////////////////////////////////////////////
		// Resize

		case WM_SIZE: 
		{
			const UINT width = LOWORD( LParam );
			const UINT height = HIWORD( LParam );

			if( pThis->GetParent()->GetCursorMode() == RubyCursorMode::Locked ) 
			{
				pThis->ConfigureClipRect();
				pThis->RecenterMousePos();
			}

			if( WParam == SIZE_MAXIMIZED )
			{
				pThis->GetParent()->DispatchEvent<RubyMaximizeEvent>( RubyEventType::WindowMaximized, true );
			}
			else if( WParam == SIZE_MINIMIZED ) 
			{
				pThis->GetParent()->DispatchEvent<RubyMinimizeEvent>( RubyEventType::WindowMinimized, true );
			}

			pThis->GetParent()->DispatchEvent<RubyWindowResizeEvent>( RubyEventType::Resize, static_cast< uint32_t >( width ), static_cast< uint32_t >( height ) );
		} return false;

		//////////////////////////////////////////////////////////////////////////
		// Window Position & Focus

		case WM_MOVE: 
		{
			pThis->GetParent()->DispatchEvent<RubyEvent>( RubyEventType::WindowMoved );

			if( pThis->GetParent()->GetCursorMode() == RubyCursorMode::Locked )
			{
				pThis->ConfigureClipRect();
				pThis->RecenterMousePos();
			}
		} return 0;

		//////////////////////////////////////////////////////////////////////////
		
		case WM_DISPLAYCHANGE: 
		{
			pThis->GetParent()->DispatchEvent<RubyEvent>( RubyEventType::DisplayChanged );
		} break;

		case WM_KILLFOCUS: 
		{
			if( pThis->GetParent()->GetCursorMode() == RubyCursorMode::Locked )
			{
				pThis->SetMouseCursor( RubyCursorType::Arrow );
			}

			pThis->GetParent()->DispatchEvent<RubyFocusEvent>( RubyEventType::WindowFocus, false );
		} return 0;

		case WM_SETFOCUS: 
		{
			if( pThis->GetParent()->GetCursorMode() == RubyCursorMode::Locked )
			{
				pThis->SetMouseCursor( RubyCursorType::None );
			}

			pThis->GetParent()->DispatchEvent<RubyFocusEvent>( RubyEventType::WindowFocus, true );
		} return 0;

		//////////////////////////////////////////////////////////////////////////
		// BEGIN: Mouse Events
		// Mouse Move

		case WM_MOUSEMOVE:
		{
			const int x = GET_X_LPARAM( LParam );
			const int y = GET_Y_LPARAM( LParam );

			if( !pThis->IsMouseTracked() )
			{
				TRACKMOUSEEVENT TrackMouseEvent{ sizeof( TrackMouseEvent ), TME_LEAVE, Handle };
				pThis->SetTrackMouse( ::TrackMouseEvent( &TrackMouseEvent ) );

				pThis->GetParent()->DispatchEvent<RubyEvent>( RubyEventType::MouseEnterWindow );
			}

			if( pThis->GetParent()->GetCursorMode() == RubyCursorMode::Locked )
			{
				RubyIVec2 lastPos = pThis->GetParent()->GetLastMousePos();
				
				RubyIVec2 deltaPos = { x - lastPos.x, y - lastPos.y };
				RubyIVec2 lockedDelta = pThis->GetParent()->GetVirtualMousePos();
				lockedDelta += deltaPos;
				
				pThis->GetParent()->DispatchEvent<RubyMouseMoveEvent>( 
					RubyEventType::MouseMoved, ( float ) lockedDelta.x, ( float ) lockedDelta.y );

				pThis->GetParent()->IntrnlSetLockedMousePos( lockedDelta );
			}
			else
			{
				pThis->GetParent()->DispatchEvent<RubyMouseMoveEvent>( RubyEventType::MouseMoved, ( float ) x, ( float ) y );
			}
			
			pThis->GetParent()->IntrnlSetLastMousePos( { x, y } );
		} return 0;

		case WM_SETCURSOR: 
		{
			if( LOWORD( LParam ) == HTCLIENT ) 
			{
				pThis->UpdateCursorIcon();
				return TRUE;
			}
		} break;

		//////////////////////////////////////////////////////////////////////////
		// Mouse Button Pressed & Released

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		{
			RubyMouseButton btn = ( Msg == WM_LBUTTONDOWN ? RubyMouseButton::Left : Msg == WM_RBUTTONDOWN ? RubyMouseButton::Right : RubyMouseButton::Middle );

			if( ::GetCapture() == nullptr && pThis->GetParent()->GetCurrentMouseButtons().size() == 0 )
				::SetCapture( Handle );

			pThis->GetParent()->IntrnlSetMouseState( btn );
			pThis->GetParent()->DispatchEvent<RubyMouseEvent>( RubyEventType::MousePressed, ( int ) btn );
		} return 0;

		case WM_XBUTTONDOWN:
		{
			RubyMouseButton xbtn = GET_XBUTTON_WPARAM( WParam ) == XBUTTON1 ? RubyMouseButton::Extra1 : RubyMouseButton::Extra2;

			if( ::GetCapture() == nullptr && pThis->GetParent()->GetCurrentMouseButtons().size() == 0 )
				::SetCapture( Handle );

			pThis->GetParent()->IntrnlSetMouseState( xbtn );
			pThis->GetParent()->DispatchEvent<RubyMouseEvent>( RubyEventType::MousePressed, ( int ) xbtn );
		} return 0;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			RubyMouseButton btn = ( Msg == WM_LBUTTONUP ? RubyMouseButton::Left : Msg == WM_RBUTTONUP ? RubyMouseButton::Right : RubyMouseButton::Middle );

			pThis->GetParent()->IntrnlSetMouseState( btn, false );
			pThis->GetParent()->DispatchEvent<RubyMouseEvent>( RubyEventType::MouseReleased, ( int )btn );
			
			if( ::GetCapture() == Handle && pThis->GetParent()->GetCurrentMouseButtons().size() == 0 )
				::ReleaseCapture();
		} return 0;

		case WM_XBUTTONUP:
		{
			RubyMouseButton xbtn = GET_XBUTTON_WPARAM( WParam ) == XBUTTON1 ? RubyMouseButton::Extra1 : RubyMouseButton::Extra2;

			pThis->GetParent()->IntrnlSetMouseState( xbtn, false );
			pThis->GetParent()->DispatchEvent<RubyMouseEvent>( RubyEventType::MouseReleased, ( int ) xbtn );

			if( ::GetCapture() == Handle && pThis->GetParent()->GetCurrentMouseButtons().size() == 0 )
				::ReleaseCapture();
		} return 0;

		case WM_MOUSELEAVE:
		{
			if( pThis->IsMouseTracked() )
			{
				pThis->SetTrackMouse( false );
			}

			pThis->GetParent()->DispatchEvent<RubyEvent>( RubyEventType::MouseLeaveWindow );
		} break;
		
		// Vertical Scroll
		case WM_MOUSEWHEEL:
		{
			int yOffset = GET_WHEEL_DELTA_WPARAM( WParam );
			yOffset /= WHEEL_DELTA;

			pThis->GetParent()->DispatchEvent<RubyMouseScrollEvent>( RubyEventType::MouseScroll, 0, yOffset );
		} return false;

		// Horizontal Scroll
		case WM_MOUSEHWHEEL: 
		{
			int xOffset = GET_WHEEL_DELTA_WPARAM( WParam );
			xOffset /= WHEEL_DELTA;

			pThis->GetParent()->DispatchEvent<RubyMouseScrollEvent>( RubyEventType::MouseScroll, xOffset, 0 );
		} return false;

		// END: Mouse Events
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// Key Events

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			// In Ruby our key codes match with the Win32 ones.
			int nativeCode = ( int ) WParam;
			int Modifiers = HandleKeyMods();

			pThis->GetParent()->IntrnlSetKeyDown( ( RubyKey ) nativeCode, true );
			pThis->GetParent()->DispatchEvent<RubyKeyEvent>( RubyEventType::KeyPressed, nativeCode, Modifiers );
		} return false;

		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			// In Ruby our key codes match with the Win32 ones.
			int nativeCode = ( int ) WParam;
			int Modifiers = HandleKeyMods();

			pThis->GetParent()->IntrnlSetKeyDown( ( RubyKey ) nativeCode, false );
			pThis->GetParent()->DispatchEvent<RubyKeyEvent>( RubyEventType::KeyReleased, nativeCode, Modifiers );
		} return false;

		// The WM_CHAR message is sent when a printable character key is pressed.
		// Handle Ansi (Ascii) characters and UTF-8
		case WM_CHAR: 
		{
			wchar_t wc = static_cast< wchar_t >( WParam );
			pThis->GetParent()->DispatchEvent<RubyCharacterEvent>( RubyEventType::InputCharacter, wc );
		} return false;

		case WM_UNICHAR: 
		{
			if( WParam == UNICODE_NOCHAR )
			{
				// Tell any other applications that we support UTF-16
				return TRUE;
			}
		} return false;

		//////////////////////////////////////////////////////////////////////////
		// Borderless Resizing support.
		// Thank You: https://github.com/Geno-IDE/Geno/blob/master/src/Geno/C%2B%2B/GUI/MainWindow.cpp#L520-L586

		case WM_NCHITTEST: 
		{
			if( pThis->GetParent()->GetStyle() != RubyStyle::Borderless || pThis->GetParent()->GetCursorMode() == RubyCursorMode::Locked )
				break;

			POINT MousePos;
			RECT WindowRect;

			::GetCursorPos( &MousePos );
			::GetWindowRect( Handle, &WindowRect );

			if( ::PtInRect( &WindowRect, MousePos ) )
			{
				const int BorderX = ::GetSystemMetrics( SM_CXFRAME ) + ::GetSystemMetrics( SM_CXPADDEDBORDER );
				const int BorderY = ::GetSystemMetrics( SM_CYFRAME ) + ::GetSystemMetrics( SM_CXPADDEDBORDER );

				// Top Section of the window
				if( MousePos.y < ( WindowRect.top + BorderY ) )
				{
					if( MousePos.x < ( WindowRect.left + BorderX ) ) { pThis->SetResizeCursor( RubyCursorType::ResizeNWSE ); return HTTOPLEFT; }
					else if( MousePos.x >= ( WindowRect.right - BorderX ) ) { pThis->SetResizeCursor( RubyCursorType::ResizeNESW ); return HTTOPRIGHT; }
					else { pThis->SetResizeCursor( RubyCursorType::ResizeNS ); return HTTOP; }
				}
				else if( MousePos.y >= ( WindowRect.bottom - BorderY ) ) // Bottom section of the window.
				{
					if( MousePos.x < ( WindowRect.left + BorderX ) ) { pThis->SetResizeCursor( RubyCursorType::ResizeNESW ); return HTBOTTOMLEFT; }
					else if( MousePos.x >= ( WindowRect.right - BorderX ) ) { pThis->SetResizeCursor( RubyCursorType::ResizeNWSE ); return HTBOTTOMRIGHT; }
					else { pThis->SetResizeCursor( RubyCursorType::ResizeNS ); return HTBOTTOM; }
				}
				else if( MousePos.x < ( WindowRect.left + BorderX ) ) // Left section of the window.
				{
					pThis->SetResizeCursor( RubyCursorType::ResizeEW );
					return HTLEFT;
				}
				else if( MousePos.x >= ( WindowRect.right - BorderX ) ) // Right section of the window.
				{
					pThis->SetResizeCursor( RubyCursorType::ResizeEW );
					return HTRIGHT;
				}
				else
				{
					if( MousePos.y < WindowRect.top + pThis->GetParent()->GetTitlebarHeight() && !::IsZoomed( Handle ) && !pThis->GetParent()->GetTitlebarCond() )
						return HTCAPTION;
				}

				pThis->ResetResizeCursor();
			}
		} break;

		case WM_NCCALCSIZE:
		{
			if( pThis->GetParent()->GetStyle() != RubyStyle::Borderless )
				break;

			if( WParam == TRUE )
			{
				WINDOWPLACEMENT WindowPlacement{ .length = sizeof( WINDOWPLACEMENT ) };

				if( ::GetWindowPlacement( Handle, &WindowPlacement ) && WindowPlacement.showCmd == SW_MAXIMIZE )
				{
					NCCALCSIZE_PARAMS& rParams = *reinterpret_cast< LPNCCALCSIZE_PARAMS >( LParam );
					const int BorderX = ::GetSystemMetrics( SM_CXFRAME ) + ::GetSystemMetrics( SM_CXPADDEDBORDER );
					const int BorderY = ::GetSystemMetrics( SM_CYFRAME ) + ::GetSystemMetrics( SM_CXPADDEDBORDER );

					rParams.rgrc[ 0 ].left += BorderX;
					rParams.rgrc[ 0 ].top += BorderY;
					rParams.rgrc[ 0 ].right -= BorderX;
					rParams.rgrc[ 0 ].bottom -= BorderY;

					return WVR_VALIDRECTS;
				}
			}

			return 0;

		} break;
	}

	return ::DefWindowProc( Handle, Msg, WParam, LParam );
}

//////////////////////////////////////////////////////////////////////////

namespace Saturn {

	RubyWindowsBackend::RubyWindowsBackend( const RubyWindowSpecification& rSpec, RubyWindow* pWindow )
	{
		m_pWindow = pWindow;
		m_WindowSpecification = rSpec;
	}

	RubyWindowsBackend::~RubyWindowsBackend()
	{
		DestroyWindow();
	}

	void RubyWindowsBackend::Create()
	{
		DWORD WindowStyle = ChooseStyle();

		HWND ParentHWND = nullptr;
		if( m_WindowSpecification.pParentWindow != nullptr )
			ParentHWND = m_WindowSpecification.pParentWindow->GetNativeHandle();

		m_Handle = ::CreateWindowExW( 
			0, 
			DEFAULT_WINDOW_CLASS_NAME, 
			m_pWindow->m_WindowTitle.data(), 
			WindowStyle, 
			CW_USEDEFAULT, CW_USEDEFAULT, 
			( int ) m_WindowSpecification.Width, ( int ) m_WindowSpecification.Height,
			ParentHWND, nullptr, GetModuleHandle( nullptr ), nullptr );

		::SetPropW( m_Handle, L"RubyData", this );

		if( m_WindowSpecification.Style == RubyStyle::Borderless )
			::SetWindowLong( m_Handle, GWL_STYLE, GetWindowLong( m_Handle, GWL_STYLE ) | WS_CAPTION );
	}

	DWORD RubyWindowsBackend::ChooseStyle()
	{
		switch( m_WindowSpecification.Style )
		{
			case RubyStyle::Default:
				return WS_OVERLAPPEDWINDOW;

			case RubyStyle::BorderlessNoResize:
				return WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP | WS_MINIMIZEBOX;

			case RubyStyle::Borderless:
				// Create the borderless window as a normal window however we will then set the required styles.
				// TODO: For some reason adding WS_CAPTION does not work, so we'll add it when set the style long.
				return WS_POPUP | WS_EX_TOPMOST | WS_MAXIMIZEBOX;

			default:
				return 0;
		}
	}

	LPTSTR RubyWindowsBackend::ChooseCursor( RubyCursorType Cursor )
	{
		switch( Cursor )
		{
			case RubyCursorType::Arrow:
				return IDC_ARROW;

			case RubyCursorType::Hand:
				return IDC_HAND;

			case RubyCursorType::IBeam:
				return IDC_IBEAM;

			case RubyCursorType::NotAllowed:
				return IDC_NO;

			case RubyCursorType::ResizeEW:
				return IDC_SIZEWE;

			case RubyCursorType::ResizeNS:
				return IDC_SIZENS;

			case RubyCursorType::ResizeNESW:
				return IDC_SIZENESW;

			case RubyCursorType::ResizeNWSE:
				return IDC_SIZENWSE;
		}

		return nullptr;
	}

	void RubyWindowsBackend::FindMouseRestorePoint()
	{
		if( m_pWindow->GetLastCursorMode() < RubyCursorMode::Locked )
		{
			POINT pos{};
			::GetCursorPos( &pos );
			::ScreenToClient( m_Handle, &pos );

			m_MouseRestorePoint.x = pos.x;
			m_MouseRestorePoint.y = pos.y;
		}
	}

	void RubyWindowsBackend::SetResizeCursor( RubyCursorType Type )
	{
		// Safety
		UnblockMouseCursor();

		SetMouseCursor( Type );

		BlockMouseCursor();
	}

	void RubyWindowsBackend::ResetResizeCursor()
	{
		if( m_BlockMouseCursor )
		{
			UnblockMouseCursor();

			// m_CurrentCursorType is not changed when we are setting the cursor for resizing
			SetMouseCursor( m_CurrentCursorType );
		}
	}

	void RubyWindowsBackend::ConfigureClipRect()
	{
		RECT WindowRect;
		::GetClientRect( m_Handle, &WindowRect );
		::ClientToScreen( m_Handle, ( POINT* ) &WindowRect.left );
		::ClientToScreen( m_Handle, ( POINT* ) &WindowRect.right );

		::ClipCursor( &WindowRect );
	}

	void RubyWindowsBackend::RecenterMousePos()
	{
		SetMousePos( m_pWindow->GetWidth() / 2.0, m_pWindow->GetHeight() / 2.0 );
	}

	void RubyWindowsBackend::DisableCursor()
	{
		FindMouseRestorePoint();

		UpdateCursorIcon();

		// Keep the mouse in the center of the window so we don't move out of the window.
		RecenterMousePos();

		ConfigureClipRect();
	}

	void RubyWindowsBackend::SetTitle( const std::string& rTitle )
	{
		::SetWindowTextA( m_Handle, rTitle.data() );
	}

	void RubyWindowsBackend::SetTitle( const std::wstring& rTitle )
	{
		::SetWindowTextW( m_Handle, rTitle.data() );
	}

	void RubyWindowsBackend::Maximize()
	{
		::ShowWindow( m_Handle, SW_MAXIMIZE );
	}

	void RubyWindowsBackend::Minimize()
	{
		::ShowWindow( m_Handle, SW_MINIMIZE );
	}

	void RubyWindowsBackend::Restore()
	{
		::ShowWindow( m_Handle, SW_RESTORE );
	}

	WindowType RubyWindowsBackend::GetNativeHandle()
	{
		return m_Handle;
	}

	bool RubyWindowsBackend::Minimized()
	{
		return ::IsIconic( m_Handle );
	}

	bool RubyWindowsBackend::Maximized()
	{
		return ::IsZoomed( m_Handle );
	}

	bool RubyWindowsBackend::Focused()
	{
		return ::GetActiveWindow() == m_Handle;
	}

	VkResult RubyWindowsBackend::CreateVulkanWindowSurface( VkInstance Instance, VkSurfaceKHR* pOutSurface )
	{
		VkWin32SurfaceCreateInfoKHR CreateInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
		CreateInfo.hinstance = GetModuleHandle( nullptr );
		CreateInfo.hwnd = m_Handle;

		return vkCreateWin32SurfaceKHR( Instance, &CreateInfo, nullptr, pOutSurface );
	}

	void RubyWindowsBackend::SetMouseCursor( RubyCursorType Cursor, RubyMouseCursorSetReason Reason )
	{
		if( m_BlockMouseCursor || m_CurrentCursorType == Cursor )
			return;

		m_CurrentCursorType = Cursor;

		LPTSTR NativeCursorRes = ChooseCursor( Cursor );
		m_CurrentMouseCursorIcon = ::LoadCursor( nullptr, NativeCursorRes );

		UpdateCursorIcon();
	}

	void RubyWindowsBackend::UpdateCursorIcon()
	{
		if( m_pWindow->GetCursorMode() == RubyCursorMode::Locked )
		{
			::SetCursor( NULL );
			m_CurrentMouseCursorIcon = nullptr;
		}
		else if( m_CurrentMouseCursorIcon )
			::SetCursor( m_CurrentMouseCursorIcon );
		else
			m_CurrentMouseCursorIcon = ::SetCursor( ::LoadCursor( nullptr, IDC_ARROW ) );
	}

	void RubyWindowsBackend::SetMouseCursorMode( RubyCursorMode mode )
	{
		switch( mode )
		{
			case RubyCursorMode::Normal:
			{
				if( m_pWindow->GetLastCursorMode() == RubyCursorMode::Locked )
				{
					// Unclip the mouse
					::ClipCursor( nullptr );

					SetMousePos( m_MouseRestorePoint.x, m_MouseRestorePoint.y );

					// Rest the restore point and the locked mouse position.
					m_MouseRestorePoint = {};
					m_pWindow->m_LockedMousePosition = {};
				}

				::ShowCursor( TRUE );
				SetMouseCursor( RubyCursorType::Arrow );

			} break;

			case RubyCursorMode::Hidden:
			{
				::ClipCursor( nullptr );
				::ShowCursor( FALSE );
			} break;

			case RubyCursorMode::Locked:
			{
				if( !Focused() )
					break;

				DisableCursor();
			} break;

			default:
				break;
		}
	}

	void RubyWindowsBackend::SetMousePos( double x, double y )
	{
		m_pWindow->IntrnlSetLastMousePos( { ( int ) x, ( int ) y } );

		POINT newPos{ ( int ) x, ( int ) y };

		::ClientToScreen( m_Handle, &newPos );
		::SetCursorPos( newPos.x, newPos.y );
	}

	void RubyWindowsBackend::GetMousePos( double* x, double* y )
	{
		POINT pos{};

		if( x ) *x = 0;
		if( y ) *y = 0;

		if( ::GetCursorPos( &pos ) ) 
		{
			::ScreenToClient( m_Handle, &pos );

			*x = pos.x;
			*y = pos.y;
		}
	}

	void RubyWindowsBackend::DestroyWindow()
	{
		if( m_Handle )
		{
			ReleaseDC( m_Handle, m_DrawContext );

			::RemovePropW( m_Handle, L"RubyData" );

			::DestroyWindow( m_Handle );
		}
	}

	void RubyWindowsBackend::CloseWindow()
	{
		m_ShouldClose = true;
	}

	void RubyWindowsBackend::PresentWindow( RubyWindowShowCmd Command )
	{
		switch( Command )
		{
			case RubyWindowShowCmd::Default:
				::ShowWindow( m_Handle, SW_SHOW );
				break;

			case RubyWindowShowCmd::Fullscreen:
			{
				HMONITOR Monitor = ::MonitorFromWindow( m_Handle, MONITOR_DEFAULTTONEAREST );

				MONITORINFO Info = { .cbSize = sizeof( MONITORINFO ) };
				::GetMonitorInfo( Monitor, &Info );

				::MoveWindow( m_Handle, Info.rcMonitor.left, Info.rcMonitor.top,
					Info.rcMonitor.right - Info.rcMonitor.left, Info.rcMonitor.bottom - Info.rcMonitor.top, TRUE );
				::ShowWindow( m_Handle, SW_SHOW );
			} break;
		}
	}

	void RubyWindowsBackend::ResizeWindow( uint32_t Width, uint32_t Height )
	{
		RECT newWindowRect{ 0, 0, ( long ) Width, ( long ) Height };

		::AdjustWindowRect( &newWindowRect, ChooseStyle(), false );

		::SetWindowPos( m_Handle, HWND_TOP, 0, 0, newWindowRect.right - newWindowRect.left, newWindowRect.bottom - newWindowRect.top, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER );
	}

	RubyIVec2 RubyWindowsBackend::GetSize()
	{
		RECT size;
		::GetClientRect( m_Handle, &size );

		return { size.right, size.bottom };
	}

	void RubyWindowsBackend::MoveWindow( int x, int y )
	{
		RECT newWindowRect{ x, y, x, y };

		::AdjustWindowRect( &newWindowRect, ChooseStyle(), false );

		::SetWindowPos( m_Handle, nullptr, newWindowRect.left, newWindowRect.top, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER );
	}

	void RubyWindowsBackend::PollEvents()
	{
		// TODO: I don't want to just update our window. 
		// We should update all windows all at once.

		MSG Message = {};
		while( ::PeekMessage( &Message, nullptr, 0, 0, PM_REMOVE ) > 0 )
		{
			::TranslateMessage( &Message );
			::DispatchMessage( &Message );
		}

		// Lock the mouse back to the center if it has moved.
		if( m_pWindow->GetCursorMode() == RubyCursorMode::Locked )
		{
			RubyIVec2 lastPos = m_pWindow->GetLastMousePos();

			uint32_t w = m_pWindow->GetWidth() / 2;
			uint32_t h = m_pWindow->GetHeight() / 2;

			if( lastPos.x != w || lastPos.y != h )
			{
				RecenterMousePos();
			}
		}
	}

	bool RubyWindowsBackend::PendingClose()
	{
		return m_ShouldClose;
	}

	void RubyWindowsBackend::Focus()
	{
		::BringWindowToTop( m_Handle );
		::SetForegroundWindow( m_Handle );
		::SetFocus( m_Handle );
	}

	RubyIVec2 RubyWindowsBackend::GetWindowPos()
	{
		POINT WindowPosition{ .x = 0, .y = 0 };
		::ClientToScreen( m_Handle, &WindowPosition );

		return { ( int ) WindowPosition.x, ( int ) WindowPosition.y };
	}

	bool RubyWindowsBackend::MouseInRect()
	{
		RECT WindowRect;
		POINT MousePos;

		::GetClientRect( m_Handle, &WindowRect );
		::GetCursorPos( &MousePos );

		::ClientToScreen( m_Handle, ( POINT* ) &WindowRect.left );
		::ClientToScreen( m_Handle, ( POINT* ) &WindowRect.right );

		return ::PtInRect( &WindowRect, MousePos );
	}

	void RubyWindowsBackend::FlashAttention()
	{
		::FlashWindow( m_Handle, FALSE );
	}

	void RubyWindowsBackend::SetClipboardText( const std::string& rTextData )
	{
		std::wstring textDataW = Auxiliary::ConvertString( rTextData );
		SetClipboardText( textDataW );
	}

	void RubyWindowsBackend::SetClipboardText( const std::wstring& rTextData )
	{
		// Try open the clipboard.
		if( ::OpenClipboard( m_Handle ) )
		{
			::EmptyClipboard();

			size_t DataSize = ( rTextData.size() + 1 ) * sizeof( wchar_t );
			HGLOBAL ClipboardData = ::GlobalAlloc( GMEM_MOVEABLE, DataSize );

			if( ClipboardData )
			{
				// Lock and copy data.
				void* pData = ::GlobalLock( ClipboardData );

				if( pData )
				{
					// Copy data.
					wcscpy_s( ( wchar_t* ) pData, rTextData.size() + 1, rTextData.c_str() );

					::GlobalUnlock( ClipboardData );

					// Set the data on the clipboard.
					::SetClipboardData( CF_UNICODETEXT, ClipboardData );
				}
			}

			::CloseClipboard();
		}
	}

	const char* RubyWindowsBackend::GetClipboardText()
	{
		// TODO: Who deletes this buffer?

		const wchar_t* pResult = nullptr;
		pResult = GetClipboardTextW();

		int size = ::WideCharToMultiByte( CP_UTF8, 0, pResult, -1, NULL, 0, NULL, NULL );
		char* pBuffer = new char[ size ];
		::WideCharToMultiByte( CP_UTF8, 0, pResult, -1, pBuffer, size, NULL, NULL );

		return pBuffer;
	}

	const wchar_t* RubyWindowsBackend::GetClipboardTextW()
	{
		wchar_t* result = nullptr;

		// Try open the clipboard.
		if( ::OpenClipboard( m_Handle ) )
		{
			HANDLE ClipboardData = ::GetClipboardData( CF_UNICODETEXT );

			if( ClipboardData )
			{
				// Lock and copy data.
				wchar_t* pData = static_cast< wchar_t* >( ::GlobalLock( ClipboardData ) );

				if( pData )
				{
					result = pData;

					::GlobalUnlock( ClipboardData );
				}
			}

			::CloseClipboard();
		}

		return result;
	}

}