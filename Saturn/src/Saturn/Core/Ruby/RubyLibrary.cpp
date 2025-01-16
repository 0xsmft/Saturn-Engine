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

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace Saturn {

#if defined(_WIN32)
	BOOL CALLBACK MonitorEnumProc( HMONITOR Monitor, HDC HDCMonitor, LPRECT LPRCMonitor, LPARAM DWData )
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
		GetAllMonitors();
	}

	void RubyLibrary::AddMonintor( const RubyMonitor& rMonitor )
	{
		m_Monitors.push_back( rMonitor );
	}

	std::vector<RubyMonitor> RubyLibrary::GetAllMonitors()
	{
		int Monitors = GetSystemMetrics( SM_CMONITORS );

		if( m_Monitors.size() != Monitors )
		{
			m_Monitors.clear();
			m_Monitors.reserve( static_cast<size_t>( Monitors ) );

#if defined(_WIN32)
			LPARAM userData = (LPARAM)this;
			::EnumDisplayMonitors( NULL, NULL, MonitorEnumProc, userData );
#endif
		}

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
}