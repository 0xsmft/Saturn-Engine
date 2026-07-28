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
#include "CrashReporterLayer.h"

#include "Saturn/Core/App.h"
#include "Saturn/Core/Ruby/RubyWindow.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h> 
#include <imgui_internal.h>

namespace Saturn {

	CrashReporterLayer::CrashReporterLayer( const std::filesystem::path& rErrorLogPath )
		: m_ErrorLogPath( rErrorLogPath )
	{
	}

	void CrashReporterLayer::OnAttach()
	{
		auto* pWindow = Application::Get()->GetWindow();
		pWindow->ChangeTitle( "Crash Reporter" );
		pWindow->CentreWindowXYInMonitor();
		pWindow->Show();

		if( !m_ErrorLogPath.empty() && std::filesystem::exists( m_ErrorLogPath ) )
		{
			std::ifstream stream( m_ErrorLogPath );
			m_CrashReportFileContent = std::string( std::istreambuf_iterator<char>( stream ), std::istreambuf_iterator<char>() );
		}
	}

	void CrashReporterLayer::OnDetach()
	{
	}

	CrashReporterLayer::~CrashReporterLayer()
	{
	}

	void CrashReporterLayer::OnUpdate( Timestep time )
	{
	}

	void CrashReporterLayer::OnImGuiRender()
	{
		const ImGuiViewport* pViewport = ImGui::GetMainViewport();
		const ImGuiID dockspaceID = ImGui::DockSpaceOverViewport( pViewport, ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoDockingOverMe | ImGuiDockNodeFlags_NoUndocking );

		ImGui::Begin( "##reporter", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar );
		ImGui::SetWindowDock( ImGui::GetCurrentWindow(), dockspaceID, ImGuiCond_FirstUseEver );

		const auto boldFont = ImGui::GetIO().Fonts->Fonts[ 1 ];
		ImGui::PushFont( boldFont );
		ImGui::Text( "A Saturn-Engine process has crashed." );
		ImGui::PopFont();

		const ImVec2 textSize = ImGui::CalcTextSize( "Close" );

		if( m_CrashReportFileContent.empty() )
		{
			ImGui::Text( "unable to load the log." );
		}
		else
		{
			ImGui::InputTextMultiline(
				"##text",
				m_CrashReportFileContent.data(),
				m_CrashReportFileContent.size() + 1,
				ImVec2( -FLT_MIN, ImGui::GetContentRegionAvail().y - ( ImGui::GetStyle().WindowPadding.y * 2.0f ) - textSize.y ),
				ImGuiInputTextFlags_ReadOnly
			);
		}

		ImGui::Separator();

		ImGui::BeginHorizontal( "##optionshz" );

		if( ImGui::Button( "Close" ) )
		{
			Application::Get()->Close();
		}

		{
			Auxiliary::ScopedDisabledFlag disabledIf( m_ErrorLogPath.empty() );

			if( ImGui::Button( "Show in explorer" ) )
			{
				Application::Get()->OpenNativeFileExplorer( m_ErrorLogPath, true );
			}
		}

		ImGui::EndHorizontal();

		ImGui::End();
	}

	void CrashReporterLayer::OnEvent( Event& rEvent )
	{
	}

}
