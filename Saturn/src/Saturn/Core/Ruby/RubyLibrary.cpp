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
#include "RubyLibrary.h"

#if defined( SAT_PLATFORM_WINDOWS )
#include <Windows.h>
#include "Backend/RubyWindowsBackend.h"
#elif defined(SAT_PLATFORM_LINUX)
#include "RubyWindow.h"
#include <xcb/randr.h>
#endif

namespace Saturn {

#if defined( SAT_PLATFORM_WINDOWS )
	static BOOL CALLBACK MonitorEnumProc( HMONITOR Monitor, HDC HDCMonitor, LPRECT LPRCMonitor, LPARAM DWData )
	{
		RubyLibrary* pThis = ( RubyLibrary* ) DWData;

		MONITORINFOEX MonitorInfo{};
		MonitorInfo.cbSize = sizeof( MONITORINFOEX );

		if( ::GetMonitorInfo( Monitor, &MonitorInfo ) )
		{
			RubyMonitor monitor;
			monitor.Primary = MonitorInfo.dwFlags & MONITORINFOF_PRIMARY;
			monitor.Name = MonitorInfo.szDevice;

			DEVMODE DevMode{};
			DevMode.dmSize = sizeof( DevMode );

			::EnumDisplaySettings( MonitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &DevMode );
			monitor.MonitorPosition = { DevMode.dmPosition.x, DevMode.dmPosition.y };

			monitor.MonitorSize.x = MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left;
			monitor.MonitorSize.y = MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top;

			monitor.WorkSize.x = MonitorInfo.rcWork.right - MonitorInfo.rcWork.left;
			monitor.WorkSize.y = MonitorInfo.rcWork.bottom - MonitorInfo.rcWork.top;

			pThis->AddMonintor( monitor );
		}

		return TRUE;
	}
#endif

	//////////////////////////////////////////////////////////////////////////

	RubyLibrary::RubyLibrary()
	{
#if defined( SAT_PLATFORM_WINDOWS )
		GetAllMonitors();
#endif
	}

	void RubyLibrary::AddMonintor( const RubyMonitor& rMonitor )
	{
		m_Monitors.push_back( rMonitor );
	}

	void RubyLibrary::PollEvents()
	{
#if defined( SAT_PLATFORM_WINDOWS )
		RubyWindowsBackend::PollEvents();
#elif defined(SAT_PLATFORM_LINUX)
		for( auto* pWindow : m_Windows ) 
		{
			pWindow->PollEvents();
		} 
#endif
	}

	std::vector<RubyMonitor> RubyLibrary::GetAllMonitors()
	{
#if defined( SAT_PLATFORM_WINDOWS )
		int Monitors = ::GetSystemMetrics( SM_CMONITORS );

		if( m_Monitors.size() != Monitors )
		{
			m_Monitors.clear();
			m_Monitors.reserve( static_cast< size_t >( Monitors ) );

			LPARAM userData = (LPARAM)this;
			::EnumDisplayMonitors( NULL, NULL, MonitorEnumProc, userData );
		}
#elif defined(SAT_PLATFORM_LINUX)
		xcb_window_t rootWindow = static_cast<xcb_window_t>(
			reinterpret_cast<uintptr_t>(m_Windows[0]->GetNativeHandle())
		);
		xcb_randr_get_monitors_cookie_t cookie = xcb_randr_get_monitors(m_pConnection, rootWindow, 1);

		xcb_randr_get_monitors_reply_t* reply = xcb_randr_get_monitors_reply(m_pConnection, cookie, nullptr);

		int n = xcb_randr_get_monitors_monitors_length(reply);
		auto it =  xcb_randr_get_monitors_monitors_iterator(reply);

		m_Monitors.clear();
		m_Monitors.reserve(n);

		for (int i = 0; i < n; i++, xcb_randr_monitor_info_next(&it))
		{
			const xcb_randr_monitor_info_t& monitorInfo = *it.data;

			RubyMonitor& rMonitor = m_Monitors.emplace_back();
			rMonitor.Primary = monitorInfo.primary != 0;
			rMonitor.MonitorPosition = { monitorInfo.x, monitorInfo.y };
			rMonitor.MonitorSize = { monitorInfo.width, monitorInfo.height };
			rMonitor.WorkSize = rMonitor.MonitorSize; // no separate work area in X11
		}

		std::free(reply);
#endif

		return m_Monitors;
	}

	RubyMonitor& RubyLibrary::GetPrimaryMonitor()
	{
		if( !m_Monitors.size() )
			GetAllMonitors();

		auto Itr = std::find_if( m_Monitors.begin(), m_Monitors.end(),
			[]( auto& rMonitor ) 
			{ 
				return rMonitor.Primary; 
			} );

		return *( Itr );
	}

#if defined(SAT_PLATFORM_LINUX)
	bool RubyLibrary::TryOpenConnection()
	{
		if( m_pConnection )
			return true;

		m_pConnection = xcb_connect( 0, 0 );

		return m_pConnection != nullptr;
	}
#endif
}
