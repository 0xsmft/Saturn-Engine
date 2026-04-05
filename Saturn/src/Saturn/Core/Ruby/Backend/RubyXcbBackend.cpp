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
#include "RubyXcbBackend.h"

#include "Saturn/Core/Ruby/RubyWindow.h"
#include "Saturn/Core/StringAuxiliary.h"

#if defined( SAT_RBY_INCLUDE_VULKAN )
#include <vulkan_xcb.h>
#endif

//#include <libxkbcommon/xkbcommon-x11.h>
#include <X11/keysym.h>

#include <cstring>

static int HandleKeyMods()
{
	int Modifiers = Saturn::RubyKey_UnknownKey;
	return Modifiers;
}

static Saturn::RubyKey ConvertWinScancodeToRuby( uint8_t scanCode )
{
	using namespace Saturn;

	switch( scanCode )
    {
        // Letters
        case 16: return RubyKey_Q;   // Q
        case 17: return RubyKey_W;   // W
        case 18: return RubyKey_E;   // E
        case 19: return RubyKey_R;   // R
        case 20: return RubyKey_T;   // T
        case 21: return RubyKey_Y;   // Y
        case 22: return RubyKey_U;   // U
        case 23: return RubyKey_I;   // I
        case 24: return RubyKey_O;   // O
        case 25: return RubyKey_P;   // P
        case 30: return RubyKey_A;   // A
        case 31: return RubyKey_S;   // S
        case 32: return RubyKey_D;   // D
        case 33: return RubyKey_F;   // F
        case 34: return RubyKey_G;   // G
        case 35: return RubyKey_H;   // H
        case 36: return RubyKey_J;   // J
        case 37: return RubyKey_K;   // K
        case 38: return RubyKey_L;   // L
        case 44: return RubyKey_Z;   // Z
        case 45: return RubyKey_X;   // X
        case 46: return RubyKey_C;   // C
        case 47: return RubyKey_V;   // V
        case 48: return RubyKey_B;   // B
        case 49: return RubyKey_N;   // N
        case 50: return RubyKey_M;   // M

        // Numbers (top row)
        case 2: return RubyKey_Num1;
        case 3: return RubyKey_Num2;
        case 4: return RubyKey_Num3;
        case 5: return RubyKey_Num4;
        case 6: return RubyKey_Num5;
        case 7: return RubyKey_Num6;
        case 8: return RubyKey_Num7;
        case 9: return RubyKey_Num8;
        case 10: return RubyKey_Num9;
        case 11: return RubyKey_Num0;

        // Symbols
        case 12: return RubyKey_Minus;           // -
        case 13: return RubyKey_Equal;           // =
        case 26: return RubyKey_LeftBracket;     // [
        case 27: return RubyKey_RightBracket;    // ]
        case 39: return RubyKey_Apostrophe;      // '
        case 41: return RubyKey_Grave;           // `
        case 43: return RubyKey_Backslash;       // \ |
        case 51: return RubyKey_Comma;           // ,
        case 52: return RubyKey_Period;          // .
        case 53: return RubyKey_Slash;           // /

        // Function keys
        case 59: return RubyKey_F1;
        case 60: return RubyKey_F2;
        case 61: return RubyKey_F3;
        case 62: return RubyKey_F4;
        case 63: return RubyKey_F5;
        case 64: return RubyKey_F6;
        case 65: return RubyKey_F7;
        case 66: return RubyKey_F8;
        case 67: return RubyKey_F9;
        case 68: return RubyKey_F10;
        case 87: return RubyKey_F11;
        case 88: return RubyKey_F12;

        // Modifiers
        case 42: return RubyKey_LeftShift;
        case 54: return RubyKey_RightShift;
        case 29: return RubyKey_LeftCtrl;
        case 97: return RubyKey_RightCtrl;
        case 56: return RubyKey_LeftAlt;
        case 100: return RubyKey_RightAlt;

        // Lock keys
        case 58: return RubyKey_CapsLock;
        case 69: return RubyKey_NumLock;
        case 70: return RubyKey_ScrollLock;

        // Navigation / editing
        case 14: return RubyKey_Backspace;
        case 15: return RubyKey_Tab;
        case 28: return RubyKey_Enter;
        case 110: return RubyKey_Insert;
        case 111: return RubyKey_Delete;
        case 119: return RubyKey_Home;
        case 115: return RubyKey_End;
        case 112: return RubyKey_PageUp;
        case 117: return RubyKey_PageDown;

        // Arrows
        case 103: return RubyKey_UpArrow;
        case 108: return RubyKey_DownArrow;
        case 105: return RubyKey_LeftArrow;
        case 106: return RubyKey_RightArrow;

        // Numpad
        case 82: return RubyKey_Numpad0;
        case 79: return RubyKey_Numpad1;
        case 80: return RubyKey_Numpad2;
        case 81: return RubyKey_Numpad3;
        case 75: return RubyKey_Numpad4;
        case 76: return RubyKey_Numpad5;
        case 77: return RubyKey_Numpad6;
        case 71: return RubyKey_Numpad7;
        case 72: return RubyKey_Numpad8;
        case 73: return RubyKey_Numpad9;
        case 83: return RubyKey_NumpadDecimal;
        case 78: return RubyKey_NumpadAdd;
        case 74: return RubyKey_NumpadSubtract;
        case 55: return RubyKey_NumpadMultiply;
        case 181: return RubyKey_NumpadDivide;
        case 96: return RubyKey_NumpadEnter;

        // Misc / system keys
        case 1: return RubyKey_Esc;
        //case 70: return RubyKey_Pause;
        case 99: return RubyKey_PrintScreen;
        case 127: return RubyKey_Menu;

        // Space
        case 57: return RubyKey_Space;

        default: return RubyKey_UnknownKey;
    }
}

//////////////////////////////////////////////////////////////////////////

#define SAT_XCB_LEFT_MOUSE 1u
#define SAT_XCB_MIDDLE_MOUSE 2u
#define SAT_XCB_RIGHT_MOUSE 3u
#define SAT_XCB_WHEEL_UP_MOUSE 4u
#define SAT_XCB_WHEEL_DOWN_MOUSE 5u
#define SAT_XCB_XBTN_0_MOUSE 7u
#define SAT_XCB_XBTN_1_MOUSE 8u

xcb_atom_t XcbGetAtom(xcb_connection_t *conn, const char *name)
{
    xcb_intern_atom_cookie_t cookie =
        xcb_intern_atom(conn, 0, strlen(name), name);

    xcb_intern_atom_reply_t *reply =
        xcb_intern_atom_reply(conn, cookie, NULL);

    if (!reply)
        return XCB_NONE;

    xcb_atom_t atom = reply->atom;
    free(reply);
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

		RubyLibrary::Get().UnregisterWindow( m_pWindow );
	}

	void RubyXcbBackend::Create()
	{
		RubyLibrary::Get().RegisterWindow( m_pWindow );
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
		
		xcb_create_window(m_pConnection,
							XCB_COPY_FROM_PARENT,
							m_Handle,
							screen->root,
							0, 0,
							m_WindowSpecification.Width, m_WindowSpecification.Height,
							10,
							XCB_WINDOW_CLASS_INPUT_OUTPUT,
							screen->root_visual,
							mask, values 
		);                     

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

		// Map the window on the screen.
		xcb_map_window(m_pConnection, m_Handle);
		xcb_flush( m_pConnection );

		RubyLibrary::Get().GetAllMonitors();
	}
	
	void RubyXcbBackend::HandleXcbEvents()
	{
		xcb_generic_event_t* pEvent = nullptr;
		while( ( pEvent = xcb_poll_for_event( m_pConnection ) ) )
		{
			switch( pEvent->response_type & ~0x80 ) 
			{
				case XCB_BUTTON_PRESS:
				{
					xcb_button_press_event_t *ev = (xcb_button_press_event_t *)pEvent;

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

					m_pWindow->IntrnlSetMouseState( btn, true );
					m_pWindow->DispatchEvent<RubyMouseEvent>( EventType::MousePressed, ( int )btn );
				} break;
				
				case XCB_BUTTON_RELEASE:
				{
					xcb_button_release_event_t *ev = (xcb_button_release_event_t*)pEvent;

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

					m_pWindow->IntrnlSetMouseState( btn, false );
					m_pWindow->DispatchEvent<RubyMouseEvent>( EventType::MouseReleased, ( int )btn );
				} break;
				

				case XCB_ENTER_NOTIFY: 
				{
					m_pWindow->DispatchEvent<Event>( EventType::MouseEnterWindow, EventCategory::EC_Ruby );
				} break;
				
				case XCB_LEAVE_NOTIFY: 
				{
					m_pWindow->DispatchEvent<Event>( EventType::MouseLeaveWindow, EventCategory::EC_Ruby );
				} break;
				
				case XCB_MOTION_NOTIFY: 
				{
					xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t *)pEvent;
					
					const int x = ev->event_x;
					const int y = ev->event_y;
					
					if( m_pWindow->GetCursorMode() == RubyCursorMode::Locked ) 
					{
						// TODO: will do when ported.
					}
					else
					{
						m_pWindow->DispatchEvent<RubyMouseMoveEvent>( EventType::MouseMoved, ( float ) x, ( float ) y );
					}
					
					m_pWindow->IntrnlSetLastMousePos( { x, y } );
				} break;
				
				case XCB_FOCUS_IN: 
				{
					if( m_pWindow->GetCursorMode() == RubyCursorMode::Locked ) 
					{
						SetMouseCursor( RubyCursorType::Arrow );
					}
					
					m_pWindow->DispatchEvent<RubyFocusEvent>( EventType::WindowFocus, true );
				} break;
				
				case XCB_FOCUS_OUT: 
				{
					if( m_pWindow->GetCursorMode() == RubyCursorMode::Locked ) 
					{
						SetMouseCursor( RubyCursorType::None );
					}
					
					m_pWindow->DispatchEvent<RubyFocusEvent>( EventType::WindowFocus, false );
				} break;

				case XCB_KEY_PRESS: 
				{
					xcb_key_press_event_t *ev = (xcb_key_press_event_t *)pEvent;
					xcb_keysym_t keysym = xcb_key_symbols_get_keysym(m_Symbols, ev->detail, 0);
					const RubyKey saturnKey = ConvertWinScancodeToRuby( keysym );
					
					if( keysym ) 
					{
						if( keysym >= XK_space && keysym <= XK_asciitilde ) 
						{
							const wchar_t wc = (uint16_t)keysym;
							m_pWindow->DispatchEvent<RubyCharacterEvent>( EventType::InputCharacter, wc );
						}
					}
					
					m_pWindow->DispatchEvent<RubyKeyEvent>( EventType::KeyPressed, saturnKey, keysym, 0 );
					
				} break;
				
				case XCB_KEY_RELEASE: 
				{
					xcb_key_press_event_t *ev = (xcb_key_press_event_t *)pEvent;
					xcb_keysym_t keysym = xcb_key_symbols_get_keysym(m_Symbols, ev->detail, 0);
					const RubyKey saturnKey = ConvertWinScancodeToRuby( keysym );
					
					m_pWindow->DispatchEvent<RubyKeyEvent>( EventType::KeyReleased, saturnKey, keysym, 0 );
				} break;
				
				default: break;
			}

			std::free( pEvent );
		}
	}

	void RubyXcbBackend::FindMouseRestorePoint()
	{
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
		xcb_intern_atom_cookie_t wm_state_cookie = xcb_intern_atom(m_pConnection, 0, 13, "_NET_WM_STATE");
		xcb_intern_atom_cookie_t max_vert_cookie = xcb_intern_atom(m_pConnection, 0, 25, "_NET_WM_STATE_MAXIMIZED_VERT");
		xcb_intern_atom_cookie_t max_horz_cookie = xcb_intern_atom(m_pConnection, 0, 27, "_NET_WM_STATE_MAXIMIZED_HORZ");

		xcb_intern_atom_reply_t *wm_state_reply = xcb_intern_atom_reply(m_pConnection, wm_state_cookie, NULL);
		xcb_intern_atom_reply_t *max_vert_reply = xcb_intern_atom_reply(m_pConnection, max_vert_cookie, NULL);
		xcb_intern_atom_reply_t *max_horz_reply = xcb_intern_atom_reply(m_pConnection, max_horz_cookie, NULL);

		xcb_client_message_event_t event;
		event.response_type = XCB_CLIENT_MESSAGE;
		event.format = 32;
		event.window = m_Handle;
		event.type = wm_state_reply->atom;
		event.data.data32[0] = 1; // 1 = _NET_WM_STATE_ADD
		event.data.data32[1] = max_vert_reply->atom;
		event.data.data32[2] = max_horz_reply->atom;
		event.data.data32[3] = 0;
		event.data.data32[4] = 0;

		// Send to root window
		xcb_get_setup(m_pConnection);
		xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(m_pConnection)).data;
		xcb_send_event(m_pConnection, 0, m_Handle, XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY, (char *)&event);
		
		xcb_flush(m_pConnection);
	}

	void RubyXcbBackend::Minimize()
	{
		xcb_intern_atom_cookie_t cookie = xcb_intern_atom(m_pConnection, 0, std::strlen("WM_CHANGE_STATE"), "WM_CHANGE_STATE");
		xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(m_pConnection, cookie, NULL);

		xcb_atom_t atom = reply->atom;
		std::free(reply);

		xcb_client_message_event_t ev = {0};
		ev.response_type = XCB_CLIENT_MESSAGE;
		ev.window = m_Handle;
		ev.type = atom;
		ev.format = 32;
		ev.data.data32[0] = 3;

		xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(m_pConnection)).data;
		xcb_send_event(m_pConnection,
			0,
			m_Handle,
			XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
			XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
			(const char *)&ev);

		xcb_flush(m_pConnection);
	}

	void RubyXcbBackend::Restore()
	{
		xcb_atom_t active = XcbGetAtom(m_pConnection, "_NET_ACTIVE_WINDOW");

		xcb_client_message_event_t ev = {0};
		ev.response_type = XCB_CLIENT_MESSAGE;
		ev.window = m_Handle;
		ev.type = active;
		ev.format = 32;
		ev.data.data32[0] = 1; // source indication (application)
		ev.data.data32[1] = XCB_CURRENT_TIME;

		xcb_send_event(m_pConnection,
			0,
			m_Handle,
			XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
			XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
			(const char *)&ev);

		xcb_flush(m_pConnection);
	}

	WindowType RubyXcbBackend::GetNativeHandle()
	{
		return (void*)m_Handle;
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
		xcb_atom_t active_atom = XcbGetAtom(m_pConnection, "_NET_ACTIVE_WINDOW");

		xcb_get_property_cookie_t cookie =
			xcb_get_property(m_pConnection,
							 0,
							 m_Handle,
							 active_atom,
							 XCB_ATOM_WINDOW,
							 0,
							 1);

		xcb_get_property_reply_t *reply =
			xcb_get_property_reply(m_pConnection, cookie, NULL);

		if (!reply)
			return false;

		xcb_window_t *active = (xcb_atom_t*)xcb_get_property_value(reply);

		bool focused = false;

		if (active && *active == m_Handle)
			focused = true;

		std::free(reply);
		return focused;
	}

	VkResult RubyXcbBackend::CreateVulkanWindowSurface( VkInstance Instance, VkSurfaceKHR* pOutSurface )
	{
		VkXcbSurfaceCreateInfoKHR CreateInfo{ VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR };
		CreateInfo.window = m_Handle;
		CreateInfo.connection = m_pConnection;

		return vkCreateXcbSurfaceKHR( Instance, &CreateInfo, nullptr, pOutSurface );
	}

	void RubyXcbBackend::SetMouseCursor( RubyCursorType Cursor, RubyMouseCursorSetReason Reason )
	{
	}

	void RubyXcbBackend::UpdateCursorIcon()
	{
	}

	void RubyXcbBackend::SetMouseCursorMode( RubyCursorMode mode )
	{
	}

	void RubyXcbBackend::SetMousePos( double x, double y )
	{
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
		return { (float)reply->root_x, (float)reply->root_y };
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
	}

	void RubyXcbBackend::ResizeWindow( uint32_t Width, uint32_t Height )
	{
		uint32_t values[] = { Width, Height };
		xcb_configure_window( m_pConnection, m_Handle, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values );
	}

	RubyIVec2 RubyXcbBackend::GetSize()
	{
		const auto geoReq = xcb_get_geometry( m_pConnection, m_Handle );
		auto* pGeo = xcb_get_geometry_reply(m_pConnection, geoReq, NULL);
		
		return { pGeo->width, pGeo->height };
	}

	void RubyXcbBackend::MoveWindow( int x, int y )
	{
		uint32_t values[] = { (uint32_t)x, (uint32_t)y };
		xcb_configure_window( m_pConnection, m_Handle, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values );
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
		xcb_configure_window( m_pConnection, m_Handle, XCB_CONFIG_WINDOW_STACK_MODE, values);
	}

	RubyIVec2 RubyXcbBackend::GetWindowPos()
	{
		const auto geoReq = xcb_get_geometry( m_pConnection, m_Handle );
		auto* pGeo = xcb_get_geometry_reply(m_pConnection, geoReq, NULL);
		
		return { pGeo->x, pGeo->y };
	}

	bool RubyXcbBackend::MouseInRect()
	{
		return false;
	}

	void RubyXcbBackend::FlashAttention()
	{
	}

	void RubyXcbBackend::SetClipboardText( const std::string& rTextData )
	{
	}

	void RubyXcbBackend::SetClipboardText( const std::wstring& rTextData )
	{
	}

	const char* RubyXcbBackend::GetClipboardText()
	{
		return nullptr;
	}

	const wchar_t* RubyXcbBackend::GetClipboardTextW()
	{
		return nullptr;
	}

}
