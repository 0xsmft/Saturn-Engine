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
#include "SandboxNodeEditor.h"

#include "SandboxNodeEditorNodes.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

namespace Saturn {

	SandboxNodeEditor::SandboxNodeEditor()
		: NodeEditor( 0 )
	{
	}

	SandboxNodeEditor::~SandboxNodeEditor()
	{
		
	}

	void SandboxNodeEditor::BuildTaskCache()
	{
#if !defined(SAT_DIST)
		if( m_Dirty || m_TaskCache.IsListEmpty() )
		{
			// SHIT! Will be fixed sooner or later...
			const auto itr = std::find_if( m_Nodes.begin(), m_Nodes.end(),
				[]( const auto& rKV ) -> bool
			{
				return rKV.second->GetClass() == SandboxNodeEditorOutputNode::StaticClass();
			} );

			if( itr != m_Nodes.end() )
			{
				std::vector< SharedPtr<NodeEditorNodeBase> > ids;
				TraverseFromStart( itr->second, NodeEditorFlowDirection::GoToRootNode, [ & ]( const auto id )
				{
					ids.push_back( FindNode( id ) );
				} );

				std::reverse( ids.begin(), ids.end() );

				m_TaskCache.BuildMasterList( ids );
				SaveAndMarkClean();
			}
		}
#endif
	}

	void SandboxNodeEditor::OnUpdate( Timestep ts )
	{
		if( m_TaskHandler )
		{
			m_TaskHandler->Tick( ts );
		}
	}

	void SandboxNodeEditor::OnNodeEditorEvent( NodeEditorAction action )
	{
		switch( action )
		{
			case NodeEditorAction::PreEvaluate:
			{
				// SHIT! Will be fixed sooner or later...
				const auto itr = std::find_if( m_Nodes.begin(), m_Nodes.end(),
					[]( const auto& rKV ) -> bool
				{
					return rKV.second->GetClass() == SandboxNodeEditorOutputNode::StaticClass();
				} );

				if( itr != m_Nodes.end() )
				{
					std::vector< SharedPtr<NodeEditorNodeBase> > ids;
					TraverseFromStart( itr->second, NodeEditorFlowDirection::GoToRootNode, [ & ]( const auto id )
					{
						ids.push_back( FindNode( id ) );
					} );

					std::reverse( ids.begin(), ids.end() );

					m_PreCompiler->Init( ids );
				}
			} break;

			default:
				break;
		}
	}

#if !defined(SAT_DIST)
	void SandboxNodeEditor::OnImGuiRender()
	{
		NodeEditor::OnImGuiRender();
	
		if( m_ShowRuntimeControl ) DrawRuntimeControl();
	}

	void SandboxNodeEditor::OnTopBarRender()
	{
		if( ImGui::Button( "Open Runtime control" ) ) 
		{
			m_ShowRuntimeControl ^= 1;
		}

		ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );

		const std::string text = "DEBUGGING ONLY";

		const ImVec2 padding = ImGui::GetStyle().FramePadding;
		const ImVec2 textPosition = ImGui::GetCursorScreenPos();
		const ImVec2 textSize = ImGui::CalcTextSize( text.c_str() );

		const ImVec2 min = ImVec2( textPosition.x - padding.x, textPosition.y - padding.y );
		const ImVec2 max = ImVec2( textPosition.x + padding.x + textSize.x, textPosition.y + padding.y + textSize.y );

		ImGui::GetWindowDrawList()->AddRectFilled( min, max,
			IM_COL32( 200, 30, 60, 255 ), 2.0f, ImDrawFlags_RoundCornersAll );

		ImGui::TextUnformatted( text.c_str() );

		ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );

		if( ImGui::Button( "Clear" ) ) 
		{
			ClearEditor();
		}
	}

#endif

	void SandboxNodeEditor::DrawRuntimeControl()
	{
#if !defined(SAT_DIST)
		if( ImGui::Begin( "Runtime Control##sndbxne", &m_ShowRuntimeControl, ImGuiWindowFlags_NoSavedSettings ) )
		{
			if( ImGui::Button( "Build NodeTaskCache" ) )
			{
				m_TaskCache.Clear();
				BuildTaskCache();
			}

			if( ImGui::Button( "Clear NodeTaskCache" ) )
			{
				m_TaskCache.Clear();
			}

			if( ImGui::Button( "Simulate Runtime" ) )
			{
				m_TaskHandler = Ref<SandboxNodeEditorTaskHandler>::Create();

				if( m_TaskCache.IsListEmpty() || m_TaskCache.IsDirty() )
				{
					BuildTaskCache();

					m_TaskHandler->Init( m_TaskCache );
				}

				SetStateFlag( NodeEditorState_Simulating, true );
			}

			if( ImGui::Button( "Terminate Runtime" ) )
			{
				m_TaskHandler = nullptr;
				SetStateFlag( NodeEditorState_Simulating, false );
			}
		}

		ImGui::End();
#endif
	}

	void SandboxNodeEditor::ClearEditor()
	{
		UUID outputNodeID = 0;
		for( auto itr = m_Nodes.begin(); itr != m_Nodes.end(); )
		{
			auto& rNodeID = itr->first;
			SharedPtr<NodeEditorNodeBase> node = itr->second;

			if( node->GetClass() == SandboxNodeEditorOutputNode::StaticClass() ) 
			{
				outputNodeID = rNodeID;
				
				// Move on.
				++itr;
				continue;
			}

			node->Destroy();
			DeleteDeadLinks( rNodeID );

			itr = m_Nodes.erase( itr );
		}

		if( m_Nodes.size() )
		{
			auto outputNode = m_Nodes.at( outputNodeID );
			
			SAT_CORE_ASSERT( outputNode->GetClass() == SandboxNodeEditorOutputNode::StaticClass(), "Unexpected node SClass for output node!" );

			DeleteDeadLinks( outputNode->ID );
		}
	}

}
