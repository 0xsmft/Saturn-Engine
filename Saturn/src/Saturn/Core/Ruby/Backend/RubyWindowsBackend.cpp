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
	int Modifiers = Saturn::RubyKey_UnknownKey;

	if( ::GetKeyState( VK_LSHIFT ) & 0x8000 )
	{
		Modifiers |= Saturn::RubyKey_LeftShift;
	}

	if( ::GetKeyState( VK_RSHIFT ) & 0x8000 )
	{
		Modifiers |= Saturn::RubyKey_RightShift;
	}

	if( ::GetKeyState( VK_LMENU ) & 0x8000 )
	{
		Modifiers |= Saturn::RubyKey_LeftAlt;
	}

	if( ::GetKeyState( VK_RMENU ) & 0x8000 )
	{
		Modifiers |= Saturn::RubyKey_RightAlt;
	}

	if( ::GetKeyState( VK_LCONTROL ) & 0x8000 )
	{
		Modifiers |= Saturn::RubyKey_LeftCtrl;
	}

	if( ::GetKeyState( VK_RCONTROL ) & 0x8000 )
	{
		Modifiers |= Saturn::RubyKey_RightCtrl;
	}

	return Modifiers;
}

static Saturn::RubyKey ConvertWinScancodeToRuby( uint32_t scanCode ) 
{
	using namespace Saturn;

	switch( scanCode )
	{
		case 0x01: return RubyKey_Esc;
		case 0x02: return RubyKey_Num1;
		case 0x03: return RubyKey_Num2;
		case 0x04: return RubyKey_Num3;
		case 0x05: return RubyKey_Num4;
		case 0x06: return RubyKey_Num5;
		case 0x07: return RubyKey_Num6;
		case 0x08: return RubyKey_Num7;
		case 0x09: return RubyKey_Num8;
		case 0x0A: return RubyKey_Num9;
		case 0x0B: return RubyKey_Num0;
		case 0x0C: return RubyKey_Minus;
		case 0x0D: return RubyKey_Equal;
		case 0x0E: return RubyKey_Backspace;
		case 0x0F: return RubyKey_Tab;
		case 0x10: return RubyKey_Q;
		case 0x11: return RubyKey_W;
		case 0x12: return RubyKey_E;
		case 0x13: return RubyKey_R;
		case 0x14: return RubyKey_T;
		case 0x15: return RubyKey_Y;
		case 0x16: return RubyKey_U;
		case 0x17: return RubyKey_I;
		case 0x18: return RubyKey_O;
		case 0x19: return RubyKey_P;
		case 0x1A: return RubyKey_LeftBracket;     // [ {
		case 0x1B: return RubyKey_RightBracket;    // ] }
		case 0x1C: return RubyKey_Enter;
		case 0x1D: return RubyKey_LeftCtrl;
		case 0x1E: return RubyKey_A;
		case 0x1F: return RubyKey_S;
		case 0x20: return RubyKey_D;
		case 0x21: return RubyKey_F;
		case 0x22: return RubyKey_G;
		case 0x23: return RubyKey_H;
		case 0x24: return RubyKey_J;
		case 0x25: return RubyKey_K;
		case 0x26: return RubyKey_L;
		case 0x27: return RubyKey_Semicolon;       // ; :
		case 0x28: return RubyKey_Apostrophe;      // ' "
		case 0x29: return RubyKey_Grave;           // ` ~
		case 0x2A: return RubyKey_LeftShift;
		case 0x2B: return RubyKey_Backslash;       // \ |
		case 0x2C: return RubyKey_Z;
		case 0x2D: return RubyKey_X;
		case 0x2E: return RubyKey_C;
		case 0x2F: return RubyKey_V;
		case 0x30: return RubyKey_B;
		case 0x31: return RubyKey_N;
		case 0x32: return RubyKey_M;
		case 0x33: return RubyKey_Comma;           // , <
		case 0x34: return RubyKey_Period;          // . >
		case 0x35: return RubyKey_Slash;           // / ?
		case 0x36: return RubyKey_RightShift;
		case 0x37: return RubyKey_NumpadMultiply;
		case 0x38: return RubyKey_LeftAlt;
		case 0x39: return RubyKey_Space;
		case 0x3A: return RubyKey_CapsLock;
		case 0x3B: return RubyKey_F1;
		case 0x3C: return RubyKey_F2;
		case 0x3D: return RubyKey_F3;
		case 0x3E: return RubyKey_F4;
		case 0x3F: return RubyKey_F5;
		case 0x40: return RubyKey_F6;
		case 0x41: return RubyKey_F7;
		case 0x42: return RubyKey_F8;
		case 0x43: return RubyKey_F9;
		case 0x44: return RubyKey_F10;
		case 0x45: return RubyKey_Pause;
		case 0x46: return RubyKey_ScrollLock;

		// Numpad (non-extended)
		case 0x47: return RubyKey_Numpad7;
		case 0x48: return RubyKey_Numpad8;
		case 0x49: return RubyKey_Numpad9;
		case 0x4A: return RubyKey_NumpadSubtract;
		case 0x4B: return RubyKey_Numpad4;
		case 0x4C: return RubyKey_Numpad5;
		case 0x4D: return RubyKey_Numpad6;
		case 0x4E: return RubyKey_NumpadAdd;
		case 0x4F: return RubyKey_Numpad1;
		case 0x50: return RubyKey_Numpad2;
		case 0x51: return RubyKey_Numpad3;
		case 0x52: return RubyKey_Numpad0;
		case 0x53: return RubyKey_NumpadDecimal;
		case 0x56: return RubyKey_Backslash;
		case 0x57: return RubyKey_F11;
		case 0x58: return RubyKey_F12;

		//////////////////////////////////////////////////////////////////////////
		// ^^[END OF SINGLE BYTE SCANCODES]

		case 0x11C: return RubyKey_NumpadEnter;
		case 0x11D: return RubyKey_RightCtrl;
		case 0x135: return RubyKey_NumpadDivide;
		case 0x145: return RubyKey_NumLock;
		case 0x147: return RubyKey_Home;
		case 0x148: return RubyKey_UpArrow;
		case 0x149: return RubyKey_PageUp;
		case 0x14B: return RubyKey_LeftArrow;
		case 0x14D: return RubyKey_RightArrow;
		case 0x14F: return RubyKey_End;
		case 0x150: return RubyKey_DownArrow;
		case 0x151: return RubyKey_PageUp;
		case 0x152: return RubyKey_Insert;
		case 0x15B: return RubyKey_OSKey;
		case 0x15D: return RubyKey_Menu;

		// NOTE: Clicking the "Alt Gr" key is the same as doing Ctrl+Alt
		// So, Windows fires two key events one for the left Ctrl and then one for the right alt
		case 0x138: return RubyKey_RightAlt;
		
		case 0x146: return RubyKey_Pause;
		case 0x153: return RubyKey_Delete;

		// Extended keys (0xE0, 57344)
		case 0xE01C: return RubyKey_NumpadEnter;
		case 0xE01D: return RubyKey_RightCtrl;
		case 0xE035: return RubyKey_NumpadDivide;
		case 0xE038: return RubyKey_RightAlt;
		case 0xE047: return RubyKey_Home;
		case 0xE048: return RubyKey_UpArrow;
		case 0xE049: return RubyKey_PageUp;
		case 0xE04B: return RubyKey_LeftArrow;
		case 0xE04D: return RubyKey_RightArrow;
		case 0xE04F: return RubyKey_End;
		case 0xE050: return RubyKey_DownArrow;
		case 0xE051: return RubyKey_PageDown;
		case 0xE052: return RubyKey_Insert;
		case 0xE053: return RubyKey_Delete;
		case 0xE05B: return RubyKey_OSKey;
		case 0xE05C: return RubyKey_OSKeyRight;
		case 0xE05D: return RubyKey_Menu;

		default: return RubyKey_UnknownKey;
	}
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
			pThis->GetParent()->DispatchEvent<Event>( EventType::Close, EventCategory::EC_Ruby );

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
				pThis->GetParent()->DispatchEvent<RubyMaximizeEvent>( EventType::WindowMaximized, true );
			}
			else if( WParam == SIZE_MINIMIZED ) 
			{
				pThis->GetParent()->DispatchEvent<RubyMinimizeEvent>( EventType::WindowMinimized, true );
			}

			pThis->GetParent()->DispatchEvent<RubyWindowResizeEvent>( EventType::Resize, static_cast< uint32_t >( width ), static_cast< uint32_t >( height ) );
		} return false;

		//////////////////////////////////////////////////////////////////////////
		// Window Position & Focus

		case WM_MOVE: 
		{
			pThis->GetParent()->DispatchEvent<Event>( EventType::WindowMoved, EventCategory::EC_Ruby );

			if( pThis->GetParent()->GetCursorMode() == RubyCursorMode::Locked )
			{
				pThis->ConfigureClipRect();
				pThis->RecenterMousePos();
			}
		} return 0;

		//////////////////////////////////////////////////////////////////////////
		
		case WM_DISPLAYCHANGE: 
		{
			pThis->GetParent()->DispatchEvent<Event>( EventType::DisplayChanged, EventCategory::EC_Ruby );
		} break;

		case WM_KILLFOCUS: 
		{
			if( pThis->GetParent()->GetCursorMode() == RubyCursorMode::Locked )
			{
				pThis->SetMouseCursor( RubyCursorType::Arrow );
			}

			pThis->GetParent()->DispatchEvent<RubyFocusEvent>( EventType::WindowFocus, false );
		} return 0;

		case WM_SETFOCUS: 
		{
			if( pThis->GetParent()->GetCursorMode() == RubyCursorMode::Locked )
			{
				pThis->SetMouseCursor( RubyCursorType::None );
			}

			pThis->GetParent()->DispatchEvent<RubyFocusEvent>( EventType::WindowFocus, true );
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

				pThis->GetParent()->DispatchEvent<Event>( EventType::MouseEnterWindow, EventCategory::EC_Ruby );
			}

			if( pThis->GetParent()->GetCursorMode() == RubyCursorMode::Locked )
			{
				RubyIVec2 lastPos = pThis->GetParent()->GetLastMousePos();
				
				RubyIVec2 deltaPos = { x - lastPos.x, y - lastPos.y };
				RubyIVec2 lockedDelta = pThis->GetParent()->GetVirtualMousePos();
				lockedDelta += deltaPos;
				
				pThis->GetParent()->DispatchEvent<RubyMouseMoveEvent>( 
					EventType::MouseMoved, ( float ) lockedDelta.x, ( float ) lockedDelta.y );

				pThis->GetParent()->IntrnlSetLockedMousePos( lockedDelta );
			}
			else
			{
				pThis->GetParent()->DispatchEvent<RubyMouseMoveEvent>( EventType::MouseMoved, ( float ) x, ( float ) y );
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
			const RubyMouseButton btn = ( Msg == WM_LBUTTONDOWN ? RubyMouseButton_Left : Msg == WM_RBUTTONDOWN ? RubyMouseButton_Right : RubyMouseButton_Middle );

			if( ::GetCapture() == nullptr && pThis->GetParent()->GetCurrentMouseButtons().size() == 0 )
				::SetCapture( Handle );

			pThis->GetParent()->IntrnlSetMouseState( btn );
			pThis->GetParent()->DispatchEvent<RubyMouseEvent>( EventType::MousePressed, ( int ) btn );
		} return 0;

		case WM_XBUTTONDOWN:
		{
			const RubyMouseButton xbtn = GET_XBUTTON_WPARAM( WParam ) == XBUTTON1 ? RubyMouseButton_Extra1 : RubyMouseButton_Extra2;

			if( ::GetCapture() == nullptr && pThis->GetParent()->GetCurrentMouseButtons().size() == 0 )
				::SetCapture( Handle );

			pThis->GetParent()->IntrnlSetMouseState( xbtn );
			pThis->GetParent()->DispatchEvent<RubyMouseEvent>( EventType::MousePressed, ( int ) xbtn );
		} return 0;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			const RubyMouseButton btn = ( Msg == WM_LBUTTONUP ? RubyMouseButton_Left : Msg == WM_RBUTTONUP ? RubyMouseButton_Right : RubyMouseButton_Middle );

			pThis->GetParent()->IntrnlSetMouseState( btn, false );
			pThis->GetParent()->DispatchEvent<RubyMouseEvent>( EventType::MouseReleased, ( int )btn );
			
			if( ::GetCapture() == Handle && pThis->GetParent()->GetCurrentMouseButtons().size() == 0 )
				::ReleaseCapture();
		} return 0;

		case WM_XBUTTONUP:
		{
			const RubyMouseButton xbtn = GET_XBUTTON_WPARAM( WParam ) == XBUTTON1 ? RubyMouseButton_Extra1 : RubyMouseButton_Extra2;

			pThis->GetParent()->IntrnlSetMouseState( xbtn, false );
			pThis->GetParent()->DispatchEvent<RubyMouseEvent>( EventType::MouseReleased, ( int ) xbtn );

			if( ::GetCapture() == Handle && pThis->GetParent()->GetCurrentMouseButtons().size() == 0 )
				::ReleaseCapture();
		} return 0;

		case WM_MOUSELEAVE:
		{
			if( pThis->IsMouseTracked() )
			{
				pThis->SetTrackMouse( false );
			}

			pThis->GetParent()->DispatchEvent<Event>( EventType::MouseLeaveWindow, EventCategory::EC_Ruby );
		} break;
		
		// Vertical Scroll
		case WM_MOUSEWHEEL:
		{
			int yOffset = GET_WHEEL_DELTA_WPARAM( WParam );
			yOffset /= WHEEL_DELTA;

			pThis->GetParent()->DispatchEvent<RubyMouseScrollEvent>( EventType::MouseScroll, 0, yOffset );
		} return false;

		// Horizontal Scroll
		case WM_MOUSEHWHEEL: 
		{
			int xOffset = GET_WHEEL_DELTA_WPARAM( WParam );
			xOffset /= WHEEL_DELTA;

			pThis->GetParent()->DispatchEvent<RubyMouseScrollEvent>( EventType::MouseScroll, xOffset, 0 );
		} return false;

		// END: Mouse Events
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// Key Events

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			const UINT scanCode = HIWORD( LParam ) & ( KF_EXTENDED | 0xFF );
			const int Modifiers = HandleKeyMods();

			const RubyKey saturnKey = ConvertWinScancodeToRuby( scanCode );
#if !defined(SAT_DIST)
			if( saturnKey == RubyKey_UnknownKey )
			{
				const std::string hex = std::format( "{:08X}", scanCode );
				SAT_CORE_WARN( "[Ruby] Unknown Key!, Win32 scan code: WSC/0x{}", hex );
			}
#endif

			pThis->GetParent()->IntrnlSetKeyDown( saturnKey, true );
			pThis->GetParent()->DispatchEvent<RubyKeyEvent>( EventType::KeyPressed, saturnKey, scanCode, Modifiers );
		} return false;

		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			const UINT scanCode = HIWORD( LParam ) & ( KF_EXTENDED | 0xFF );
			const int Modifiers = HandleKeyMods();

			const RubyKey saturnKey = ConvertWinScancodeToRuby( scanCode );

			pThis->GetParent()->IntrnlSetKeyDown( saturnKey, false );
			pThis->GetParent()->DispatchEvent<RubyKeyEvent>( EventType::KeyReleased, saturnKey, scanCode, Modifiers );
		} return false;

		// The WM_CHAR message is sent when a printable character key is pressed.
		// Handle Ansi (Ascii) characters and UTF-8
		case WM_CHAR: 
		{
			const wchar_t wc = static_cast< wchar_t >( WParam );
			pThis->GetParent()->DispatchEvent<RubyCharacterEvent>( EventType::InputCharacter, wc );
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
//		: RubyBackendBase( m_WindowSpecification, pWindow )
// TODO: ^^ seems to give us garbage data... but works fine when not in the initialiser list
	{
		m_WindowSpecification = rSpec;
		m_pWindow = pWindow;
	}

	RubyWindowsBackend::~RubyWindowsBackend()
	{
		DestroyWindow();
	}

	void RubyWindowsBackend::Create()
	{
		const DWORD WindowStyle = ChooseStyle();
		const HWND ParentHWND = m_WindowSpecification.pParentWindow != nullptr ? m_WindowSpecification.pParentWindow->GetNativeHandle() : nullptr;

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
		{
			::SetWindowLong( m_Handle, GWL_STYLE, GetWindowLong( m_Handle, GWL_STYLE ) | WS_CAPTION );
			::SetWindowPos( m_Handle, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );
		}
	}

	DWORD RubyWindowsBackend::ChooseStyle()
	{
		switch( m_WindowSpecification.Style )
		{
			case RubyStyle::Default:
				return WS_OVERLAPPEDWINDOW;

			case RubyStyle::BorderlessNoResize:
				return WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_SYSMENU | WS_MINIMIZEBOX | WS_POPUP;

			case RubyStyle::Borderless:
				// Create the borderless window as a normal window however we will then set the required styles.
				// TODO: For some reason adding WS_CAPTION does not work, so we'll add it when set the style.
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
		SetMousePos( m_pWindow->GetWidth() * 0.5f, m_pWindow->GetHeight() * 0.5f );
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

	RubyVec2 RubyWindowsBackend::GetMousePos()
	{
		POINT pos{};
		if( ::GetCursorPos( &pos ) ) 
		{
			::ScreenToClient( m_Handle, &pos );

			return { static_cast< float >( pos.x ), static_cast< float >( pos.y ) };
		}

		return { 0.0f, 0.0f };
	}

	void RubyWindowsBackend::DestroyWindow()
	{
		if( m_Handle )
		{
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

			case RubyWindowShowCmd::NoActivate:
				::ShowWindow( m_Handle, SW_SHOWNA );
				break;

			case RubyWindowShowCmd::Fullscreen:
			{
				const HMONITOR Monitor = ::MonitorFromWindow( m_Handle, MONITOR_DEFAULTTONEAREST );

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
		// Process all the messages in the queue + process all windows owned by the current thread.
		MSG Message = {};
		while( ::PeekMessage( &Message, nullptr, 0, 0, PM_REMOVE ) > 0 )
		{
			::TranslateMessage( &Message );
			::DispatchMessage( &Message );
		}

		HWND activeWindow = ::GetActiveWindow();
		if( activeWindow )
		{
			RubyWindowsBackend* pThis = ( RubyWindowsBackend* ) ::GetPropW( activeWindow, L"RubyData" );
			if( pThis )
			{
				// Lock the mouse back to the center if it has moved.
				if( pThis->GetParent()->GetCursorMode() == RubyCursorMode::Locked )
				{
					const RubyIVec2 lastPos = pThis->GetParent()->GetLastMousePos();

					const uint32_t w = pThis->GetParent()->GetWidth() / 2u;
					const uint32_t h = pThis->GetParent()->GetHeight() / 2u;

					if( lastPos.x != w || lastPos.y != h )
					{
						pThis->RecenterMousePos();
					}
				}

				pThis->GetParent()->PollEvents();
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

		if( !::GetCursorPos( &MousePos ) )
			return false;

		// WindowFromPoint is needed because we may have multiple windows in the same area, so we need to check if the mouse is over this specific window.
		if( ::WindowFromPoint( MousePos ) != m_Handle )
			return false;

		::GetClientRect( m_Handle, &WindowRect );
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
		const std::wstring textDataW = Auxiliary::ConvertString( rTextData );
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
