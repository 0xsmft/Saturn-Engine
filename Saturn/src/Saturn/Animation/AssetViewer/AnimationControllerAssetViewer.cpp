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
#include "AnimationControllerAssetViewer.h"

#include "Saturn/Asset/AssetManager.h"

#include "Graph/Animation/AnimGraphNodeLibrary.h"
#include "Graph/Animation/AnimGraphOutputNode.h"
#include "Graph/Animation/AnimGraphStateMachinePlayerNode.h"
#include "Graph/StateMachine/AnimGraphStateMachineStateNode.h"
#include "Graph/StateMachine/AnimGraphStateMachineTransitionNode.h"

#include "Graph/StateMachine/StateMachineNodeLibrary.h"
#include "Graph/StateMachine/StateMachineStateNodeLibrary.h"
#include "Graph/StateMachine/TransitionNodeLibrary.h"

#include "Saturn/NodeEditor/NodeEditorHintNode.h"
#include "Saturn/NodeEditor/NodeEditorVariableNode.h"
#include "Saturn/NodeEditor/Serialisation/NodeCache.h"

#include "Saturn/NodeEditor/Maths/MathsNodes.h"

#include "Saturn/ImGui/UndoRedo/GlobalUndoRedoGroup.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Saturn {

	AnimationControllerAssetViewer::AnimationControllerAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::AnimationController;
		AddAsset();
	}

	void AnimationControllerAssetViewer::AddAsset()
	{
		m_Asset = AssetManager::Get()->FindAsset( m_AssetID );

		m_Open = true;
		m_Name = std::format( "{0}##{1}", m_Asset->Name, std::to_string( m_AssetID ) );

		m_NodeEditor = SharedPtr<AnimGraph>::Create( m_AssetID );
		const std::string filename = std::format( "{0}.sac", m_Asset->Name );

		if( NodeCacheEditor::ReadNodeEditorCache( m_NodeEditor, m_AssetID, filename ) )
		{
			m_RootNodeID = m_NodeEditor->FindNode( "Output Node" )->ID;
		}
		else
		{
			SetupNewNodeEditor();
		}

#if !defined(SAT_DIST)
		m_NodeEditor->NcSetCustomName( filename );
		m_NodeEditor->SetWindowName( m_Name );
		m_NodeEditor->OpenWindow( true );
#endif
		m_Open = true;

		SetupNodeEditorCallbacks();
	}

	AnimationControllerAssetViewer::~AnimationControllerAssetViewer()
	{
#if !defined(SAT_DIST)
		if( m_Dirty || m_NodeEditor->IsDirty() )
		{
			m_NodeEditor->SaveAndMarkClean();
		}

		GlobalUndoRedoGroup::Get()->RemoveIfActionHasIdentifier( m_AssetID );

		m_Asset = nullptr;
//		m_NodeEditor->SetRuntime( nullptr );
		m_NodeEditor = nullptr;

		AssetManager::Get()->Save();
#endif
	}

	void AnimationControllerAssetViewer::OnImGuiRender()
	{
#if !defined(SAT_DIST)
		// Draw main
		if( m_NodeEditor->IsOpen() )
		{
			m_NodeEditor->OnImGuiRender();
		}
		else
		{
			m_NodeEditor->OpenWindow( false );
			m_Open = false;
		}
#endif
	}

	void AnimationControllerAssetViewer::OnEvent( Event& rEvent )
	{
		if( m_NodeEditor )
		{
			m_NodeEditor->OnEvent( rEvent );
		}
	}

#if !defined(SAT_DIST)
	void AnimationControllerAssetViewer::OnRuntimeStateChanged( RuntimeState newState, RuntimeState oldState )
	{

	}
#endif

	void AnimationControllerAssetViewer::SetupNewNodeEditor()
	{
		SharedPtr<AnimGraphOutputNode> outputNode = AnimGraphNodeLibrary::SpawnOutputNode( m_NodeEditor );
		m_RootNodeID = outputNode->ID;
	}

	SharedPtr<NodeEditorNodeBase> AnimationControllerAssetViewer::DrawRootGraphNewNodeOptions()
	{
		SharedPtr<NodeEditorNodeBase> result;

		ImGui::SeparatorText( "Animation Graph" );

		// #FixSharedPtrEquT2Op
		if( ImGui::MenuItem( "State Machine player" ) ) 
		{
			auto stateMachinePlayer = AnimGraphNodeLibrary::SpawnStateMachinePlayerNode( m_NodeEditor );
			stateMachinePlayer->PostPlace();

			result = stateMachinePlayer;
		}

		ImGui::SeparatorText( "Variables" );
		for( const auto& [id, rVariable] : m_NodeEditor->GetDataHandles() )
		{
			if( ImGui::MenuItem( rVariable->GetName().c_str() ) )
			{
				result = ( SharedPtr<NodeEditorVariableNode> )NodeEditorVariableNode::SpawnVariableNode( rVariable, m_NodeEditor );
			}

			if( ImGui::MenuItem( "Set Variable" ) )
				result = ( SharedPtr<NodeEditorSetVariableNode> )NodeEditorSetVariableNode::SpawnSetVariableNode( rVariable, m_NodeEditor );
		}

		ImGui::SeparatorText( "Maths" );
		if( auto mathResult = MathsNodesAuxiliary::DrawContextMenu( m_NodeEditor ); mathResult )
		{
			result = mathResult;
		}

		return result;
	}

	SharedPtr<NodeEditorNodeBase> AnimationControllerAssetViewer::DrawStateMachineNewNodeOptions()
	{
		SharedPtr<NodeEditorNodeBase> result;

		ImGui::SeparatorText( "State Machine" );

		// #FixSharedPtrEquT2Op
		if( ImGui::MenuItem( "New State" ) ) 
		{
			auto stateNode = StateMachineNodeLibrary::SpawnStateNode( m_NodeEditor );
			stateNode->PostPlace();

			result = stateNode;
		}

		return result;
	}

	SharedPtr<NodeEditorNodeBase> AnimationControllerAssetViewer::DrawStateMachineStateNewNodeOptions()
	{
		SharedPtr<NodeEditorNodeBase> result;

		ImGui::SeparatorText( "State Machine State" );

		// #FixSharedPtrEquT2Op
		if( ImGui::MenuItem( "Play Animation" ) )
			result = ( SharedPtr<NodeEditorNodeBase> )StateMachineStateNodeLibrary::SpawnPlayAnimNode( m_NodeEditor );

		ImGui::SeparatorText( "Variables" );
		for( const auto& [id, rVariable] : m_NodeEditor->GetDataHandles() )
		{
			if( ImGui::MenuItem( rVariable->GetName().c_str() ) )
			{
				result = ( SharedPtr<NodeEditorVariableNode> )NodeEditorVariableNode::SpawnVariableNode( rVariable, m_NodeEditor );
			}
		}

		return result;
	}

	SharedPtr<NodeEditorNodeBase> AnimationControllerAssetViewer::DrawTransitionNewNodeOptions()
	{
		SharedPtr<NodeEditorNodeBase> result;

		ImGui::SeparatorText( "Variables" );
		for( const auto& [id, rVariable] : m_NodeEditor->GetDataHandles() )
		{
			if( ImGui::MenuItem( rVariable->GetName().c_str() ) )
			{
				result = ( SharedPtr<NodeEditorVariableNode> )NodeEditorVariableNode::SpawnVariableNode( rVariable, m_NodeEditor );
			}
		}

		ImGui::SeparatorText( "Maths" );
		if( auto mathResult = MathsNodesAuxiliary::DrawContextMenu( m_NodeEditor ); mathResult )
		{
			result = mathResult;
		}

		return result;
	}

	void AnimationControllerAssetViewer::SetupNodeEditorCallbacks()
	{
#if !defined(SAT_DIST)
		m_NodeEditor->SetCreateNewNodeFunction(
			[ & ]() -> SharedPtr<NodeEditorNodeBase>
		{
			SharedPtr<NodeEditorNodeBase> result;

			// Root graph
			if( !m_NodeEditor->GetActiveSubGraph() )
			{
				result = DrawRootGraphNewNodeOptions();
			}
			else if( m_NodeEditor->GetActiveSubGraph()->GetClass() == AnimGraphStateMachinePlayerNode::StaticClass() )
			{
				result = DrawStateMachineNewNodeOptions();
			}
			else if( m_NodeEditor->GetActiveSubGraph()->GetClass() == AnimGraphStateMachineStateNode::StaticClass() )
			{
				result = DrawStateMachineStateNewNodeOptions();
			}
			else if( m_NodeEditor->GetActiveSubGraph()->GetClass() == AnimGraphStateMachineTransitionNode::StaticClass() )
			{
				result = DrawTransitionNewNodeOptions();
			}

			ImGui::SeparatorText( "Auxiliary" );

			if( ImGui::MenuItem( "Hint (Comment) Node" ) )
				result = NodeEditorHintNode::SpawnHintNode( m_NodeEditor );

			// Set parent object if needed.
			if( result && m_NodeEditor->GetActiveSubGraph() )
			{
				result->pParentObject = m_NodeEditor->GetActiveSubGraph().Get();
			}

			return result;
		} );

		m_NodeEditor->SetBreadCrumbsFunction( 
			[&]() -> void 
		{
			ImGui::SetCursorPos( { ImGui::GetStyle().WindowPadding.x + 2.0f, 20.0f + 64.0f } );
			ImGui::Text( "%s", m_Asset->Name.c_str() );
			const ImRect textRect = ImRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
			if( ImGui::IsItemHovered() )
			{
				ImGui::GetWindowDrawList()->AddRect( textRect.Min, textRect.Max, IM_COL32( 255, 255, 255, 255 ) );

				if( ImGui::IsMouseReleased( ImGuiMouseButton_Left ) )
				{
					m_NodeEditor->ClearSubGraphs();
				}
			}

			// Draw sub graphs.
			for( const auto& rGraph : m_NodeEditor->GetSubGraphs() )
			{
				ImGui::SameLine();
				ImGui::TextUnformatted( "\\" );
				ImGui::SameLine();

				ImGui::Text( "%s", rGraph->Name.c_str() );
				const ImRect textRect = ImRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
				if( ImGui::IsItemHovered() )
				{
					ImGui::GetWindowDrawList()->AddRect( textRect.Min, textRect.Max, IM_COL32( 255, 255, 255, 255 ) );

					if( ImGui::IsMouseReleased( ImGuiMouseButton_Left ) )
					{
						m_NodeEditor->PopActiveSubGraphTo( rGraph );
						return;
					}
				}
			}
		} );
#endif
	}

}
