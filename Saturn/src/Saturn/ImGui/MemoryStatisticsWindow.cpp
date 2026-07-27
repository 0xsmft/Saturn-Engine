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
#include "MemoryStatisticsWindow.h"

#include <imgui.h>

namespace Saturn {

	MemoryStatisticsWindow::MemoryStatisticsWindow()
		: ImGuiWindow( GetStaticName() )
	{
	}

	MemoryStatisticsWindow::MemoryStatisticsWindow( const std::string& rName )
		: ImGuiWindow( rName )
	{
	}

	void MemoryStatisticsWindow::OnImGuiRender()
	{
		if( m_FirstEverUpdate )
		{
			m_CurrentMemoryStatistics = SaturnMemoryStatisticsHelper::Query();
			m_FirstEverUpdate = false;
		}

		if( ImGui::Begin( m_Name.c_str(), &m_Open ) )
		{
			ImGui::Text( "Statistics update every second." );
			ImGui::Separator();

			ImGui::Text( "Page faults: %" PRIu64, m_CurrentMemoryStatistics.PageFaultCount );
			ImGui::Text( "Current working set: %" PRIu64 "MB", m_CurrentMemoryStatistics.CurrentWorkingSet / 1024 / 1024 );
			ImGui::Text( "Peak working set: %" PRIu64 "MB", m_CurrentMemoryStatistics.PeakWorkingSet / 1024 / 1024 );
		}

		ImGui::End();
	}

	void MemoryStatisticsWindow::OnUpdate( Timestep ts )
	{
		m_TimeSinceLastUpdate += ts;

		if( m_TimeSinceLastUpdate >= 1.0f )
		{
			m_CurrentMemoryStatistics = SaturnMemoryStatisticsHelper::Query();
			m_TimeSinceLastUpdate = 0.0f;
		}
	}

}
