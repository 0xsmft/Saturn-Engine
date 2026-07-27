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
#include "EditorAboutWindowContents.h"

#include "ImGuiAuxiliary.h"

#include "Saturn/Core/App.h"

#include <imgui.h>

namespace Saturn {

	void EditorAboutWindowContents::DrawContents()
	{
		ImGui::Text( "Saturn Engine x64 %s (%s build)", Application::GetCurrentPlatformName(), Application::GetCurrentConfigName() );

		ImGui::Text( "Saturn Engine Version: %s (Internal Number: %i ident: %s)", SAT_CURRENT_VERSION_STRING, SAT_CURRENT_VERSION, SAT_CURRENT_VERSION_BUILD_TAG );

		ImGui::Text( "Built on: %s %s", __DATE__, __TIME__ );

		ImGui::Separator();

		ImGui::Text( "All icons in the editor are provided by icons8 via https://icons8.com/\nUsing the Tanah Basah set (https://icons8.com/icons/authors/v03BjHji0KTr/tanah-basah)" );

		ImGui::Separator();

		if( Auxiliary::TreeNode( "Third Party libraries" ) )
		{
			ImGui::Text( "acl & rtm" );
			ImGui::Text( "assimp" );
			ImGui::Text( "entt" );
			ImGui::Text( "filewatch" );
			ImGui::Text( "freetype" );
			ImGui::Text( "glm" );
			ImGui::Text( "imgui: %s (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM );
			ImGui::Text( "imgui_node_editor" );
			ImGui::Text( "ImGuizmo" );
			ImGui::Text( "imspinner" );
			ImGui::Text( "JoltPhysics" );
			ImGui::Text( "miniaudio" );
			ImGui::Text( "msdfgen" );
			ImGui::Text( "msdf-atlas-gen" );
			ImGui::Text( "nativefiledialog" );
			ImGui::Text( "Recast & detour" );
			ImGui::Text( "shaderc" );
			ImGui::Text( "spdlog" );
			ImGui::Text( "SPRIV-Cross" );
			ImGui::Text( "stb" );
			ImGui::Text( "steamworks" );
			ImGui::Text( "tracy" );
			ImGui::Text( "vma" );
			ImGui::Text( "zlib: Version 1.3.1, January 22nd, 2024" );

			Auxiliary::EndTreeNode();
		}

		ImGui::Separator();

		if( Auxiliary::TreeNode( "Past version numbers" ) )
		{
			ImGui::Text( "Saturn version 0.1.0 (%u)", SAT_VERSION_A_0_1_0 ); // March '24
			ImGui::Text( "Saturn version 0.1.1 (%u)", SAT_VERSION_A_0_1_1 ); // May '24
			ImGui::Text( "Saturn version 0.1.2 (%u)", SAT_VERSION_A_0_1_2 ); // July '24
			ImGui::Text( "Saturn version 0.1.3 (%u)", SAT_VERSION_A_0_1_3 ); // Sept '24
			ImGui::Text( "Saturn version 0.1.4 (%u)", SAT_VERSION_A_0_1_4 ); // Jan '25
			ImGui::Text( "Saturn version 0.2.0 (%u)", SAT_VERSION_A_0_2_0 ); // March '25
			ImGui::Text( "Saturn version 0.2.1 (%u)", SAT_VERSION_A_0_2_1 ); // May '25
			ImGui::Text( "Saturn version 0.2.2 (%u)", SAT_VERSION_A_0_2_2 ); // July '25
			ImGui::Text( "Saturn version 0.2.3 (%u)", SAT_VERSION_A_0_2_3 ); // Jan '26
			ImGui::Text( "Saturn version 0.2.4 (%u)", SAT_VERSION_A_0_2_4 ); // March '26
			ImGui::Text( "Saturn version 0.2.5 (%u)", SAT_VERSION_A_0_2_5 ); // May '26
			ImGui::Text( "Saturn version 0.2.6 (%u)", SAT_VERSION_A_0_2_6 ); // July '26

			Auxiliary::EndTreeNode();
		}
	}

}
