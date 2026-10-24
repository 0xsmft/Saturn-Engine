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

#if defined(SAT_PLATFORM_LINUX)
#include "RubyXcbBackend.h"

#include "Saturn/Core/Ruby/RubyWindow.h"
#include "Saturn/Core/StringAuxiliary.h"

#if defined( SAT_RBY_INCLUDE_VULKAN )
#include <vulkan_xcb.h>
#endif

#include <xkbcommon/xkbcommon-x11.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include <xcb/xcb_cursor.h>

#include <cstring>

static Saturn::RubyKey ConvertX11KeycodeToRuby( uint8_t keycode )
{
	using namespace Saturn;

	switch( keycode )
	{
		// Numbers row
		case 10: return RubyKey_Num1;
		case 11: return RubyKey_Num2;
		case 12: return RubyKey_Num3;
		case 13: return RubyKey_Num4;
		case 14: return RubyKey_Num5;
		case 15: return RubyKey_Num6;
		case 16: return RubyKey_Num7;
		case 17: return RubyKey_Num8;
		case 18: return RubyKey_Num9;
		case 19: return RubyKey_Num0;

		// Symbols (top row)
		case 20: return RubyKey_Minus;
		case 21: return RubyKey_Equal;
		case 22: return RubyKey_Backspace;

		// Control row
		case 23: return RubyKey_Tab;
		case 24: return RubyKey_Q;
		case 25: return RubyKey_W;
		case 26: return RubyKey_E;
		case 27: return RubyKey_R;
		case 28: return RubyKey_T;
		case 29: return RubyKey_Y;
		case 30: return RubyKey_U;
		case 31: return RubyKey_I;
		case 32: return RubyKey_O;
		case 33: return RubyKey_P;
		case 34: return RubyKey_LeftBracket;
		case 35: return RubyKey_RightBracket;
		case 36: return RubyKey_Enter;

		// Home row
		case 37: return RubyKey_LeftCtrl;
		case 38: return RubyKey_A;
		case 39: return RubyKey_S;
		case 40: return RubyKey_D;
		case 41: return RubyKey_F;
		case 42: return RubyKey_G;
		case 43: return RubyKey_H;
		case 44: return RubyKey_J;
		case 45: return RubyKey_K;
		case 46: return RubyKey_L;
		case 47: return RubyKey_Semicolon;
		case 48: return RubyKey_Apostrophe;
		case 49: return RubyKey_Grave;

		// Bottom row
		case 50: return RubyKey_LeftShift;
		case 51: return RubyKey_Backslash;
		case 52: return RubyKey_Z;
		case 53: return RubyKey_X;
		case 54: return RubyKey_C;
		case 55: return RubyKey_V;
		case 56: return RubyKey_B;
		case 57: return RubyKey_N;
		case 58: return RubyKey_M;
		case 59: return RubyKey_Comma;
		case 60: return RubyKey_Period;
		case 61: return RubyKey_Slash;
		case 62: return RubyKey_RightShift;

		// Modifiers
		case 64: return RubyKey_LeftAlt;
		case 65: return RubyKey_Space;
		case 66: return RubyKey_CapsLock;
		case 108: return RubyKey_RightAlt;
		case 105: return RubyKey_RightCtrl;

		// Function keys
		case 67: return RubyKey_F1;
		case 68: return RubyKey_F2;
		case 69: return RubyKey_F3;
		case 70: return RubyKey_F4;
		case 71: return RubyKey_F5;
		case 72: return RubyKey_F6;
		case 73: return RubyKey_F7;
		case 74: return RubyKey_F8;
		case 75: return RubyKey_F9;
		case 76: return RubyKey_F10;
		case 95: return RubyKey_F11;
		case 96: return RubyKey_F12;

		// Navigation
		case 110: return RubyKey_Home;
		case 111: return RubyKey_UpArrow;
		case 112: return RubyKey_PageUp;

		case 113: return RubyKey_LeftArrow;
		case 114: return RubyKey_RightArrow;

		case 115: return RubyKey_End;
		case 116: return RubyKey_DownArrow;
		case 117: return RubyKey_PageDown;

		case 118: return RubyKey_Insert;
		case 119: return RubyKey_Delete;

		// Escape
		case 9: return RubyKey_Esc;

		// Numpad
		case 79: return RubyKey_Numpad7;
		case 80: return RubyKey_Numpad8;
		case 81: return RubyKey_Numpad9;
		case 82: return RubyKey_NumpadSubtract;

		case 83: return RubyKey_Numpad4;
		case 84: return RubyKey_Numpad5;
		case 85: return RubyKey_Numpad6;
		case 86: return RubyKey_NumpadAdd;

		case 87: return RubyKey_Numpad1;
		case 88: return RubyKey_Numpad2;
		case 89: return RubyKey_Numpad3;

		case 90: return RubyKey_Numpad0;
		case 91: return RubyKey_NumpadDecimal;

		case 106: return RubyKey_NumpadDivide;
		case 63:  return RubyKey_NumpadMultiply;
		case 104: return RubyKey_NumpadEnter;

		default:
			return RubyKey_UnknownKey;
	}
}

static int HandleKeyMods( uint16_t state )
{
	int Modifiers = Saturn::RubyKey_UnknownKey;

	if( state & XCB_MOD_MASK_SHIFT )
	{
		Modifiers |= Saturn::RubyKey_LeftShift;
	}

	if( state & XCB_MOD_MASK_LOCK )
	{
		Modifiers |= Saturn::RubyKey_CapsLock;
	}

	if( state & XCB_MOD_MASK_1 )
	{
		Modifiers |= Saturn::RubyKey_LeftAlt;
	}

	if( state & XCB_MOD_MASK_CONTROL )
	{
		Modifiers |= Saturn::RubyKey_LeftCtrl;
	}

	return Modifiers;
}

//////////////////////////////////////////////////////////////////////////

#define SAT_XCB_LEFT_MOUSE 1u
#define SAT_XCB_MIDDLE_MOUSE 2u
#define SAT_XCB_RIGHT_MOUSE 3u
#define SAT_XCB_WHEEL_UP_MOUSE 4u
#define SAT_XCB_WHEEL_DOWN_MOUSE 5u
#define SAT_XCB_XBTN_0_MOUSE 7u
#define SAT_XCB_XBTN_1_MOUSE 8u

xcb_atom_t XcbGetAtom( xcb_connection_t* pConnection, const char* pName )
{
	xcb_intern_atom_cookie_t cookie =
		xcb_intern_atom( pConnection, 0, std::strlen( pName ), pName );

	xcb_intern_atom_reply_t* pReply =
		xcb_intern_atom_reply( pConnection, cookie, NULL );

	if( !pReply )
		return XCB_NONE;

	xcb_atom_t atom = pReply->atom;
	std::free( pReply );
	return atom;
}

//////////////////////////////////////////////////////////////////////////

namespace Saturn {

	RubyXcbBackend::RubyXcbBackend( const RubyWindowSpecification& rSpec, RubyWindow* pWindow )
	//		: RubyBackendBase( m_WindowSpecification, pWindow )
	// TODO: ^^ seems to give us garbage data... but works fine when not in the initialiser list
	{
		m_WindowSpecification = rSpec;
		m_pWindow = pWindow;
	}

	RubyXcbBackend::~RubyXcbBackend()
	{
		DestroyWindow();

		RubyLibrary::Get().UnregisterWindow( m_Handle );
	}

	struct RubyXcbMotfiHints
	{
		uint32_t   Flags;
		uint32_t   Functions;
		uint32_t   Decorations;
		int32_t    InputMode;
		uint32_t   Status;
	};

	void RubyXcbBackend::Create()
	{
		SAT_CORE_ASSERT( RubyLibrary::Get().TryOpenConnection() );

		m_pConnection = RubyLibrary::Get().GetConnection();

		const xcb_setup_t      *setup  = xcb_get_setup(m_pConnection);
		xcb_screen_iterator_t   iter   = xcb_setup_roots_iterator(setup);
		xcb_screen_t           *screen = iter.data;

		/* Create the window */
		m_Handle = xcb_generate_id( m_pConnection );
		
		uint32_t             mask = 0;
		uint32_t             values[2];
		
		mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
		values[0] = screen->white_pixel;
		values[1] = XCB_EVENT_MASK_EXPOSURE       | XCB_EVENT_MASK_BUTTON_PRESS   |
			  XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
			  XCB_EVENT_MASK_ENTER_WINDOW   | XCB_EVENT_MASK_LEAVE_WINDOW   |
			  XCB_EVENT_MASK_KEY_PRESS      | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_FOCUS_CHANGE;

		const xcb_window_t ParentHWND = m_WindowSpecification.pParentWindow != nullptr ? m_WindowSpecification.pParentWindow->GetNativeHandle() : screen->root;

		xcb_create_window(m_pConnection,
							XCB_COPY_FROM_PARENT,
							m_Handle,
							ParentHWND,
							0, 0,
							m_WindowSpecification.Width, m_WindowSpecification.Height,
							10,
							XCB_WINDOW_CLASS_INPUT_OUTPUT,
							screen->root_visual,
							mask, values 
		);                     

		RubyLibrary::Get().RegisterWindow( this );

		switch( m_WindowSpecification.Style )
		{
			default: break;

			case RubyStyle::BorderlessNoResize:
			case RubyStyle::Borderless:
			{
				RubyXcbMotfiHints hints{};
				hints.Flags = 2u;
				hints.Functions = 0u;
				hints.Decorations = 0u;
				hints.InputMode = 0;
				hints.Status = 0u;

				// Get cookie to modif hints.
				xcb_intern_atom_cookie_t cookie = xcb_intern_atom( m_pConnection, 0, strlen( "_MOTIF_WM_HINTS" ), "_MOTIF_WM_HINTS" );
				xcb_intern_atom_reply_t* pReply = xcb_intern_atom_reply( m_pConnection, cookie, nullptr );

				xcb_change_property( m_pConnection, XCB_PROP_MODE_REPLACE, m_Handle, pReply->atom, pReply->atom,  32, 5, &hints );
			} break;
		}

		// Set title.
		xcb_change_property(
			m_pConnection,
			XCB_PROP_MODE_REPLACE,
			m_Handle,
			XCB_ATOM_WM_NAME,
			XCB_ATOM_STRING,
			32,
			m_WindowSpecification.Name.size(),
			m_WindowSpecification.Name.data()
		);

		xcb_intern_atom_cookie_t windCloseCookie = xcb_intern_atom( m_pConnection, 0, 16, "WM_DELETE_WINDOW" );
		xcb_intern_atom_reply_t* pReply = xcb_intern_atom_reply( m_pConnection, windCloseCookie, NULL );

		xcb_change_property( m_pConnection, XCB_PROP_MODE_REPLACE, m_Handle, XCB_ATOM_WM_NAME, XCB_ATOM_ATOM, 32, 1, &pReply->atom );
		
		m_Symbols = xcb_key_symbols_alloc(m_pConnection);

		//if( m_WindowSpecification.ShowNow ) 
		{
			// Map the window on the screen.
			xcb_map_window( m_pConnection, m_Handle );
			xcb_flush( m_pConnection );
		}
	}
	
	void RubyXcbBackend::HandleXcbEvents()
	{
		xcb_generic_event_t* pEvent = nullptr;
		while( ( pEvent = xcb_poll_for_event( RubyLibrary::Get().GetConnection() ) ) )
		{
			switch( pEvent->response_type & ~0x80 ) 
			{
				case XCB_CONFIGURE_NOTIFY:
				{
					xcb_configure_notify_event_t* ev = (xcb_configure_notify_event_t*)pEvent;
					
					xcb_window_t XcbWindow = ev->event;
					auto* pThis = RubyLibrary::Get().FindWindowFromXcbHnd( XcbWindow );
					if( !pThis )
						break;

					if( pThis->GetParent()->GetCursorMode() == RubyCursorMode::Locked ) 
					{
						pThis->ConfigureClipRect();
						pThis->RecenterMousePos();
					}

					pThis->GetParent()->DispatchEvent<RubyWindowResizeEvent>( EventType::Resize, ev->width, ev->height );
				} break;

				case XCB_EXPOSE:
				{
					xcb_expose_event_t* ev = (xcb_expose_event_t*)pEvent;

					xcb_window_t XcbWindow = ev->window;
					auto* pThis = RubyLibrary::Get().FindWindowFromXcbHnd( XcbWindow );
					if( !pThis )
						break;

					pThis->GetParent()->DispatchEvent<RubyWindowResizeEvent>( EventType::Resize, ev->width, ev->height );
				} break;

				case XCB_BUTTON_PRESS:
				{
					xcb_button_press_event_t* ev = (xcb_button_press_event_t *)pEvent;

					xcb_window_t XcbWindow = ev->event;
					auto* pThis = RubyLibrary::Get().FindWindowFromXcbHnd( XcbWindow );

					if( !pThis )
						break;

					RubyMouseButton btn = RubyMouseButton_Unknown;
					switch( ev->detail )
					{
						case SAT_XCB_LEFT_MOUSE:
							btn = RubyMouseButton_Left;
							break;
						
						case SAT_XCB_MIDDLE_MOUSE:
							btn = RubyMouseButton_Middle;
							break;
							
						case SAT_XCB_RIGHT_MOUSE:
							btn = RubyMouseButton_Right;
							break;
							
						case SAT_XCB_WHEEL_UP_MOUSE:
							pThis->GetParent()->DispatchEvent<RubyMouseScrollEvent>( EventType::MouseScroll, 0, 1.0 );
							return;

						case SAT_XCB_WHEEL_DOWN_MOUSE:
							pThis->GetParent()->DispatchEvent<RubyMouseScrollEvent>( EventType::MouseScroll, 0, -1.0 );
							return;

						case SAT_XCB_XBTN_0_MOUSE:
							btn = RubyMouseButton_Extra1;
							break;
							
						case SAT_XCB_XBTN_1_MOUSE:
							btn = RubyMouseButton_Extra2;
							break;
					}

					pThis->GetParent()->IntrnlSetMouseState( btn, true );
					pThis->GetParent()->DispatchEvent<RubyMouseEvent>( EventType::MousePressed, ( int )btn );
				} break;
				
				case XCB_BUTTON_RELEASE:
				{
					xcb_button_release_event_t* ev = (xcb_button_release_event_t*)pEvent;

					xcb_window_t XcbWindow = ev->event;
					auto* pThis = RubyLibrary::Get().FindWindowFromXcbHnd( XcbWindow );

					if( !pThis )
						break;

					RubyMouseButton btn = RubyMouseButton_Unknown;
					switch( ev->detail ) 
					{
						case SAT_XCB_LEFT_MOUSE:
							btn = RubyMouseButton_Left;
							break;
						
						case SAT_XCB_MIDDLE_MOUSE:
							btn = RubyMouseButton_Middle;
							break;
							
						case SAT_XCB_RIGHT_MOUSE:
							btn = RubyMouseButton_Right;
							break;
							
						case SAT_XCB_XBTN_0_MOUSE:
							btn = RubyMouseButton_Extra1;
							break;
							
						case SAT_XCB_XBTN_1_MOUSE:
							btn = RubyMouseButton_Extra2;
							break;
					}

					pThis->GetParent()->IntrnlSetMouseState( btn, false );
					pThis->GetParent()->DispatchEvent<RubyMouseEvent>( EventType::MouseReleased, ( int )btn );
				} break;
				

				case XCB_ENTER_NOTIFY: 
				{
					xcb_enter_notify_event_t* ev = (xcb_enter_notify_event_t*)pEvent;

					xcb_window_t XcbWindow = ev->event;
					auto* pThis = RubyLibrary::Get().FindWindowFromXcbHnd( XcbWindow );

					if( !pThis )
						break;

					pThis->GetParent()->DispatchEvent<Event>( EventType::MouseEnterWindow, EventCategory::EC_Ruby );
				} break;
				
				case XCB_LEAVE_NOTIFY: 
				{
					xcb_leave_notify_event_t* ev = (xcb_leave_notify_event_t*)pEvent;

					xcb_window_t XcbWindow = ev->event;
					auto* pThis = RubyLibrary::Get().FindWindowFromXcbHnd( XcbWindow );

					if( !pThis )
						break;

					pThis->GetParent()->DispatchEvent<Event>( EventType::MouseLeaveWindow, EventCategory::EC_Ruby );
				} break;
				
				case XCB_MOTION_NOTIFY: 
				{
					xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t *)pEvent;
					
					xcb_window_t XcbWindow = ev->event;
					auto* pThis = RubyLibrary::Get().FindWindowFromXcbHnd( XcbWindow );

					if( !pThis )
						break;

					const int x = ev->event_x;
					const int y = ev->event_y;

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
				} break;
				
				case XCB_FOCUS_IN: 
				{
					xcb_focus_in_event_t* ev = (xcb_focus_in_event_t *)pEvent;

					xcb_window_t XcbWindow = ev->event;
					auto* pThis = RubyLibrary::Get().FindWindowFromXcbHnd( XcbWindow );

					if( !pThis )
						break;

					if( pThis->GetParent()->GetCursorMode() == RubyCursorMode::Locked )
					{
						pThis->GetParent()->SetMouseCursor( RubyCursorType::Arrow );
					}
					
					pThis->GetParent()->DispatchEvent<RubyFocusEvent>( EventType::WindowFocus, true );
				} break;
				
				case XCB_FOCUS_OUT: 
				{
					xcb_focus_out_event_t* ev = (xcb_focus_out_event_t *)pEvent;

					xcb_window_t XcbWindow = ev->event;
					auto* pThis = RubyLibrary::Get().FindWindowFromXcbHnd( XcbWindow );

					if( !pThis )
						break;

					if( pThis->GetParent()->GetCursorMode() == RubyCursorMode::Locked )
					{
						pThis->GetParent()->SetMouseCursor( RubyCursorType::None );
					}
					
					pThis->GetParent()->DispatchEvent<RubyFocusEvent>( EventType::WindowFocus, false );
				} break;

				case XCB_KEY_PRESS: 
				{
					xcb_key_press_event_t* ev = ( xcb_key_press_event_t* ) pEvent;

					xcb_window_t XcbWindow = ev->event;
					auto* pThis = RubyLibrary::Get().FindWindowFromXcbHnd( XcbWindow );

					if( !pThis )
						break;

					xkb_keycode_t keycode = ev->detail;
					enum xkb_key_direction direction = XKB_KEY_DOWN;

					xkb_state_update_key( RubyLibrary::Get().GetXkbState(), keycode, direction );

					xkb_keysym_t keysym = xkb_state_key_get_one_sym( RubyLibrary::Get().GetXkbState(), keycode );

					uint32_t codepoint = xkb_keysym_to_utf32( keysym );
					if( codepoint != 0 ) 
					{
						const wchar_t wc = ( wchar_t )codepoint;
						pThis->GetParent()->DispatchEvent<RubyCharacterEvent>( EventType::InputCharacter, wc );
					}

					const RubyKey saturnKey = ConvertX11KeycodeToRuby( keycode );
					pThis->GetParent()->DispatchEvent<RubyKeyEvent>( EventType::KeyPressed, saturnKey, saturnKey, HandleKeyMods( ev->state ) );
				} break;
				
				case XCB_KEY_RELEASE: 
				{
					xcb_key_release_event_t *ev = (xcb_key_release_event_t *)pEvent;

					xcb_window_t XcbWindow = ev->event;
					auto* pThis = RubyLibrary::Get().FindWindowFromXcbHnd( XcbWindow );

					if( !pThis )
						break;

					xkb_keycode_t keycode = ev->detail;
					enum xkb_key_direction direction = XKB_KEY_UP;

					xkb_state_update_key( RubyLibrary::Get().GetXkbState(), keycode, direction );

					const RubyKey saturnKey = ConvertX11KeycodeToRuby( ev->detail );
					pThis->GetParent()->DispatchEvent<RubyKeyEvent>( EventType::KeyReleased, saturnKey, saturnKey, HandleKeyMods( ev->state ) );
				} break;
				
				default: break;
			}

			std::free( pEvent );
		}
	}

	void RubyXcbBackend::FindMouseRestorePoint()
	{
		if( m_pWindow->GetLastCursorMode() < RubyCursorMode::Locked ) 
		{
			const auto mousePos = GetMousePos();
			m_MouseRestorePoint = { ( int ) mousePos.x, ( int ) mousePos.y };
		}
	}

	void RubyXcbBackend::SetResizeCursor( RubyCursorType Type )
	{
	}

	void RubyXcbBackend::ResetResizeCursor()
	{
	}

	void RubyXcbBackend::ConfigureClipRect()
	{
	}

	void RubyXcbBackend::RecenterMousePos()
	{
		SetMousePos( m_pWindow->GetWidth() * 0.5f, m_pWindow->GetHeight() * 0.5f );
	}

	void RubyXcbBackend::DisableCursor()
	{
		FindMouseRestorePoint();

		UpdateCursorIcon();

		// Keep the mouse in the center of the window so we don't move out of the window.
		RecenterMousePos();

		ConfigureClipRect();
	}

	void RubyXcbBackend::SetTitle( const std::string& rTitle )
	{
		xcb_change_property(
			m_pConnection,
			XCB_PROP_MODE_REPLACE,
			m_Handle,
			XCB_ATOM_WM_NAME,
			XCB_ATOM_STRING,
			32,
			rTitle.size(),
			rTitle.data()
		);
	}

	void RubyXcbBackend::SetTitle( const std::wstring& rTitle )
	{
		xcb_change_property(
			m_pConnection,
			XCB_PROP_MODE_REPLACE,
			m_Handle,
			XCB_ATOM_WM_NAME,
			XCB_ATOM_STRING,
			32,
			rTitle.size(),
			rTitle.data()
		);
	}

	void RubyXcbBackend::Maximize()
	{
		xcb_atom_t state = XcbGetAtom( m_pConnection, "_NET_WM_STATE" );
		xcb_atom_t vert  = XcbGetAtom( m_pConnection, "_NET_WM_STATE_MAXIMIZED_VERT" );
		xcb_atom_t horz  = XcbGetAtom( m_pConnection, "_NET_WM_STATE_MAXIMIZED_HORZ" );

		xcb_client_message_event_t event;
		event.response_type = XCB_CLIENT_MESSAGE;
		event.format = 32;
		event.window = m_Handle;
		event.type = state;
		event.data.data32[ 0 ] = 1;
		event.data.data32[ 1 ] = vert;
		event.data.data32[ 2 ] = horz;
		event.data.data32[ 3 ] = 1;
		event.data.data32[ 4 ] = 0;

		xcb_screen_t* screen = nullptr;
		xcb_screen_iterator_t iter = xcb_setup_roots_iterator( xcb_get_setup( m_pConnection ) );
		screen = iter.data;

		xcb_send_event( 
			m_pConnection, 
			0, 
			screen->root, 
			XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY, 
			reinterpret_cast<const char*>( &event ) 
		);

		xcb_flush( m_pConnection );
	}

	void RubyXcbBackend::Minimize()
	{
		xcb_atom_t changeState = XcbGetAtom( m_pConnection, "WM_CHANGE_STATE" );

		xcb_client_message_event_t ev = {0};
		ev.response_type = XCB_CLIENT_MESSAGE;
		ev.window = m_Handle;
		ev.type = changeState;
		ev.format = 32;
		ev.data.data32[0] = 3;

		xcb_screen_t *screen = xcb_setup_roots_iterator( xcb_get_setup( m_pConnection ) ).data;
		xcb_send_event( m_pConnection,
			0,
			screen->root,
			XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
			XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
			(const char *)&ev );

		xcb_flush( m_pConnection );
	}

	void RubyXcbBackend::Restore()
	{
		xcb_atom_t state = XcbGetAtom( m_pConnection, "_NET_WM_STATE" );
		xcb_atom_t vert  = XcbGetAtom( m_pConnection, "_NET_WM_STATE_MAXIMIZED_VERT" );
		xcb_atom_t horz  = XcbGetAtom( m_pConnection, "_NET_WM_STATE_MAXIMIZED_HORZ" );

		xcb_client_message_event_t event;
		event.response_type = XCB_CLIENT_MESSAGE;
		event.format = 32;
		event.window = m_Handle;
		event.type = state;
		event.data.data32[ 0 ] = 0;
		event.data.data32[ 1 ] = vert;
		event.data.data32[ 2 ] = horz;
		event.data.data32[ 3 ] = 1;
		event.data.data32[ 4 ] = 0;

		xcb_screen_t* screen = xcb_setup_roots_iterator( xcb_get_setup( m_pConnection ) ).data;
		xcb_send_event( m_pConnection,
			0,
			screen->root,
			XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
			XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
			(const char *)&event );

		xcb_flush( m_pConnection );
	}

	WindowType RubyXcbBackend::GetNativeHandle()
	{
		return m_Handle;
	}

	bool RubyXcbBackend::Minimized()
	{
		xcb_atom_t net_wm_state = XcbGetAtom(m_pConnection, "_NET_WM_STATE");
		xcb_atom_t hidden = XcbGetAtom(m_pConnection, "_NET_WM_STATE_HIDDEN");

		xcb_get_property_cookie_t cookie =
			xcb_get_property(m_pConnection, 0, m_Handle,
							 net_wm_state,
							 XCB_ATOM_ATOM,
							 0, 32);

		xcb_get_property_reply_t *reply =
			xcb_get_property_reply(m_pConnection, cookie, NULL);

		if (!reply)
			return false;

		int len = xcb_get_property_value_length(reply) / sizeof(xcb_atom_t);
		xcb_atom_t *atoms = (xcb_atom_t*)xcb_get_property_value(reply);

		bool minimized = false;
		for (int i = 0; i < len; i++) 
		{
			if (atoms[i] == hidden) 
			{
				minimized = true;
				break;
			}
		}

		std::free(reply);
		return minimized;
	}

	bool RubyXcbBackend::Maximized()
	{
		xcb_atom_t net_wm_state = XcbGetAtom(m_pConnection, "_NET_WM_STATE");
		xcb_atom_t max_vert = XcbGetAtom(m_pConnection, "_NET_WM_STATE_MAXIMIZED_VERT");
		xcb_atom_t max_horz = XcbGetAtom(m_pConnection, "_NET_WM_STATE_MAXIMIZED_HORZ");

		xcb_get_property_cookie_t cookie =
			xcb_get_property(m_pConnection, 0, m_Handle,
							 net_wm_state,
							 XCB_ATOM_ATOM,
							 0, 32);

		xcb_get_property_reply_t *reply =
			xcb_get_property_reply(m_pConnection, cookie, NULL);

		if (!reply)
			return false;

		int len = xcb_get_property_value_length(reply) / sizeof(xcb_atom_t);
		xcb_atom_t *atoms = (xcb_atom_t*)xcb_get_property_value(reply);

		bool vert = false;
		bool horz = false;

		for (int i = 0; i < len; i++) 
		{
			if (atoms[i] == max_vert)
				vert = true;
			if (atoms[i] == max_horz)
				horz = true;
		}

		std::free(reply);

		return vert && horz;
	}

	bool RubyXcbBackend::Focused()
	{
		xcb_get_input_focus_cookie_t cookie = xcb_get_input_focus( m_pConnection );
		xcb_get_input_focus_reply_t* pFocusReply = xcb_get_input_focus_reply( m_pConnection, cookie, nullptr );

		bool focused = false;
		if( pFocusReply )
		{
			focused = ( pFocusReply->focus == m_Handle );

			std::free( pFocusReply );
		}

		return focused;
	}

	VkResult RubyXcbBackend::CreateVulkanWindowSurface( VkInstance Instance, VkSurfaceKHR* pOutSurface )
	{
		VkXcbSurfaceCreateInfoKHR CreateInfo{ VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR };
		CreateInfo.window = m_Handle;
		CreateInfo.connection = m_pConnection;

		return vkCreateXcbSurfaceKHR( Instance, &CreateInfo, nullptr, pOutSurface );
	}

	static xcb_cursor_t CreateXcbCursor( 
		xcb_connection_t* pConnection, 
		xcb_screen_t* pScreen, 
		const char* pCursorName )
	{
		xcb_cursor_context_t* pCursorContext = nullptr;
		if( xcb_cursor_context_new( pConnection, pScreen, &pCursorContext ) != 0 )
		{
			SAT_CORE_ERROR( "Failed to create XCB cursor context." );
			return XCB_NONE;
		}

		xcb_cursor_t cursor = 0u;
		if( pCursorName == nullptr ) 
		{
			cursor = xcb_cursor_load_cursor( pCursorContext, "none" );			
		}
		else
		{
			cursor = xcb_cursor_load_cursor( pCursorContext, pCursorName );
		}

		xcb_cursor_context_free( pCursorContext );
		return cursor;
	}

	static void SetXcbCursor( xcb_connection_t* pConnection, xcb_window_t window, const char* pCursorName )
	{
		xcb_screen_t* pScreen = xcb_setup_roots_iterator( xcb_get_setup( pConnection ) ).data;
		xcb_cursor_t cursor = CreateXcbCursor( pConnection, pScreen, pCursorName );
		
		xcb_change_window_attributes( 
			pConnection, 
			window, 
			XCB_CW_CURSOR, 
			&cursor );

		xcb_free_cursor( pConnection, cursor );

		xcb_flush( pConnection );
	}

	void RubyXcbBackend::SetMouseCursor( RubyCursorType Cursor, RubyMouseCursorSetReason Reason )
	{
		if( m_BlockMouseCursor || m_CurrentCursorType == Cursor )
			return;

		m_CurrentCursorType = Cursor;

		switch( Cursor ) 
		{
			case RubyCursorType::None:
			{
				SetXcbCursor( m_pConnection, m_Handle, nullptr );
			} break;

			case RubyCursorType::Arrow:
			{
				SetXcbCursor( m_pConnection, m_Handle, "left_ptr" );
			} break;

			case RubyCursorType::IBeam:
			{
				SetXcbCursor( m_pConnection, m_Handle, "xterm" );
			} break;

			case RubyCursorType::Hand: 
			{
				SetXcbCursor( m_pConnection, m_Handle, "hand2" );
			} break;

			case RubyCursorType::ResizeEW:
			{
				SetXcbCursor( m_pConnection, m_Handle, "sb_h_double_arrow" );
			} break;

			case RubyCursorType::ResizeNS:
			{
				SetXcbCursor( m_pConnection, m_Handle, "sb_v_double_arrow" );
			} break;

			case RubyCursorType::ResizeNESW:
			{
				SetXcbCursor( m_pConnection, m_Handle, "bottom_left_corner" );
			} break;

			case RubyCursorType::ResizeNWSE:
			{
				SetXcbCursor( m_pConnection, m_Handle, "bottom_right_corner" );
			} break;

			case RubyCursorType::NotAllowed:
			{
				SetXcbCursor( m_pConnection, m_Handle, "circle" );
			} break;

			default:
				break;
		}
	}

	void RubyXcbBackend::UpdateCursorIcon()
	{
		if( m_pWindow->GetCursorMode() == RubyCursorMode::Locked ) 
		{
			SetXcbCursor( m_pConnection, m_Handle, nullptr );
		}
	}

	void RubyXcbBackend::SetMouseCursorMode( RubyCursorMode mode )
	{
		switch( mode )
		{
			case RubyCursorMode::Normal:
			{
				if( m_pWindow->GetLastCursorMode() == RubyCursorMode::Locked )
				{
					SetMousePos( m_MouseRestorePoint.x, m_MouseRestorePoint.y );

					m_MouseRestorePoint = {};
					m_pWindow->m_LockedMousePosition = {};
				}

				SetMouseCursor( RubyCursorType::Arrow );
			} break;

			case RubyCursorMode::Hidden:
			{
				SetMouseCursor( RubyCursorType::None );
			} break;

			case RubyCursorMode::Locked:
			{
				if( !Focused() )
					break;

				GetParent()->IntrnlSetLockedMousePos( GetMousePos().To<RubyIVec2>() );

				DisableCursor();
			} break;
		}
	}

	void RubyXcbBackend::SetMousePos( double x, double y )
	{	
		m_pWindow->IntrnlSetLastMousePos( { ( int ) x, ( int ) y } );

		xcb_warp_pointer(m_pConnection,
				 XCB_NONE,
				 m_Handle,
				 0, 0, 0, 0,
				(uint16_t)x, (uint16_t)y);
		xcb_flush(m_pConnection);
	}

	RubyVec2 RubyXcbBackend::GetMousePos()
	{
		const auto cookieReq = xcb_query_pointer( m_pConnection, m_Handle );
		auto reply = xcb_query_pointer_reply( m_pConnection, cookieReq, NULL );

		xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(m_pConnection)).data;

		xcb_translate_coordinates_cookie_t cookie = xcb_translate_coordinates(
			m_pConnection,
			screen->root,
			m_Handle,
			reply->root_x,
			reply->root_y
		);

		xcb_translate_coordinates_reply_t* tsReply = xcb_translate_coordinates_reply(
			m_pConnection, cookie, nullptr
		);

		return { (float)tsReply->dst_x, (float)tsReply->dst_y };
	}

	void RubyXcbBackend::DestroyWindow()
	{
		xcb_destroy_window( m_pConnection, m_Handle );
		xcb_flush( m_pConnection );
	}

	void RubyXcbBackend::CloseWindow()
	{
		m_ShouldClose = true;
	}

	void RubyXcbBackend::PresentWindow( RubyWindowShowCmd Command )
	{
		switch( Command )
		{
			case RubyWindowShowCmd::NoActivate:
			case RubyWindowShowCmd::Default:
				xcb_map_window( m_pConnection, m_Handle );
				break;

			case RubyWindowShowCmd::Fullscreen:
			{
				xcb_atom_t stateAtom = XcbGetAtom( m_pConnection, "_NET_WM_STATE" );
				xcb_atom_t fullscreenAtom = XcbGetAtom( m_pConnection, "_NET_WM_STATE_FULLSCREEN" );

				xcb_screen_t* pScreen = xcb_setup_roots_iterator( xcb_get_setup( m_pConnection ) ).data;

				xcb_client_message_event_t event{ 0} ;
				event.response_type = XCB_CLIENT_MESSAGE;
				event.format = 32;
				event.window = m_Handle;
				event.type = stateAtom;
				event.data.data32[ 0 ] = 1; // 1 == add.
				event.data.data32[ 1 ] = fullscreenAtom;
				event.data.data32[ 2 ] = XCB_ATOM_NONE;
				event.data.data32[ 3 ] = 1;
			
				xcb_send_event( 
					m_pConnection, 
					0, 
					pScreen->root, 
					XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY, 
					reinterpret_cast<const char*>( &event ) );
			} break;
		}

		xcb_flush( m_pConnection );
	}

	void RubyXcbBackend::HideWindow()
	{
		xcb_unmap_window( m_pConnection, m_Handle );
		xcb_flush( m_pConnection );
	}

	void RubyXcbBackend::SetIcon( Ref<class Texture2D> icon )
	{

	}

	void RubyXcbBackend::ResizeWindow( uint32_t Width, uint32_t Height )
	{
		uint32_t values[] = { Width, Height };
		xcb_configure_window( m_pConnection, m_Handle, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values );
		xcb_flush( m_pConnection );
	}

	RubyIVec2 RubyXcbBackend::GetSize() const
	{
		const auto geoReq = xcb_get_geometry( m_pConnection, m_Handle );
		auto* pGeo = xcb_get_geometry_reply(m_pConnection, geoReq, NULL);
		
		return { pGeo->width, pGeo->height };
	}

	void RubyXcbBackend::MoveWindow( int x, int y )
	{
		uint32_t values[] = { (uint32_t)x, (uint32_t)y };
		xcb_configure_window( m_pConnection, m_Handle, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values );
		xcb_flush( m_pConnection );
	}

	void RubyXcbBackend::PollEvents()
	{
		HandleXcbEvents();
	}

	bool RubyXcbBackend::PendingClose()
	{
		return m_ShouldClose;
	}

	void RubyXcbBackend::Focus()
	{
		const static uint32_t values[] = { XCB_STACK_MODE_ABOVE };
		xcb_configure_window( m_pConnection, m_Handle, XCB_CONFIG_WINDOW_STACK_MODE, values );
	}

	RubyIVec2 RubyXcbBackend::GetWindowPos()
	{
		const auto geoReq = xcb_get_geometry( m_pConnection, m_Handle );
		auto* pGeo = xcb_get_geometry_reply(m_pConnection, geoReq, NULL);
		
		xcb_get_setup(m_pConnection);
		xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(m_pConnection)).data;

		xcb_translate_coordinates_cookie_t cookie = xcb_translate_coordinates(
			m_pConnection,
			m_Handle,
			screen->root,
			0, 0
		);

		xcb_translate_coordinates_reply_t* reply = xcb_translate_coordinates_reply(m_pConnection, cookie, NULL);

		return { reply->dst_x, reply->dst_y };
	}

	bool RubyXcbBackend::MouseInRect()
	{
		const auto cookieReq = xcb_query_pointer( m_pConnection, m_Handle );
		auto reply = xcb_query_pointer_reply( m_pConnection, cookieReq, nullptr );

		bool inRect = false;
		if( reply )
		{
			if( reply->root == m_Handle )
				inRect = true;

			std::free( reply );
		}

		return inRect;
	}

	void RubyXcbBackend::FlashAttention()
	{
		xcb_atom_t stateAtom = XcbGetAtom( m_pConnection, "_NET_WM_STATE" );
		xcb_atom_t demandsAttention = XcbGetAtom( m_pConnection, "_NET_WM_STATE_DEMANDS_ATTENTION" );

		xcb_client_message_event_t event = { 0 };
		event.response_type = XCB_CLIENT_MESSAGE;
		event.format = 32;
		event.window = m_Handle;
		event.type = stateAtom;
		event.data.data32[ 0 ] = 1; // 1 == add.
		event.data.data32[ 1 ] = demandsAttention;
		event.data.data32[ 2 ] = XCB_ATOM_NONE;
		event.data.data32[ 3 ] = 1;
		
		xcb_screen_t* pScreen = xcb_setup_roots_iterator( xcb_get_setup( m_pConnection ) ).data;

		xcb_send_event( 
			m_pConnection, 
			0, 
			pScreen->root, 
			XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY, 
			reinterpret_cast<const char*>( &event ) );

		xcb_flush( m_pConnection );
	}

	void RubyXcbBackend::SetClipboardText( const std::string& rTextData )
	{
	}

	void RubyXcbBackend::SetClipboardText( const std::wstring& rTextData )
	{
	}

	std::string RubyXcbBackend::GetClipboardText()
	{
		return {};
	}

	std::wstring RubyXcbBackend::GetClipboardTextW()
	{
		return {};
	}

	RubyVec2 RubyXcbBackend::GetFramebufferScale() const
	{
		return { 1, 1 };
	}

	RubyIVec2 RubyXcbBackend::GetFramebufferSize() const
	{
		return GetSize();
	}
}

#endif
