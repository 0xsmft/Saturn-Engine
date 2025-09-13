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
#include "AnimationControllerAssetViewer.h"

#include "Saturn/Asset/AssetManager.h"

#include "Graph/Animation/AnimGraphNodeLibrary.h"
#include "Graph/Animation/AnimGraphOutputNode.h"
#include "Graph/StateMachine/AnimGraphStateMachineStateNode.h"

#include "Saturn/NodeEditor/NodeEditorHintNode.h"

#include <imgui.h>

namespace Saturn {

	AnimationControllerAssetViewer::AnimationControllerAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::AnimationController;
		AddAsset();
	}

	void AnimationControllerAssetViewer::AddAsset()
	{
		m_Asset = AssetManager::Get().FindAsset( m_AssetID );

		m_Open = true;
		m_Name = std::format( "{0}##{1}", m_Asset->Name, std::to_string( m_AssetID ) );

		m_NodeEditor = SharedPtr<AnimGraphStateMachineGraph>::Create( m_AssetID );

		SetupNewNodeEditor();

#if !defined(SAT_DIST)
//		m_NodeEditor->NcSetCustomName( filename );
		const std::string nodeEdWindowName = std::format( "Final Out##{0}", std::to_string( m_AssetID ) );

		m_NodeEditor->SetWindowName( nodeEdWindowName );
		m_NodeEditor->Open( true );
#endif
		m_Open = true;
		m_CurrentGraph = m_NodeEditor;

		SetupNodeEditorCallbacks();
	}

	AnimationControllerAssetViewer::~AnimationControllerAssetViewer()
	{
		m_CurrentGraph = nullptr;
	}

	void AnimationControllerAssetViewer::OnImGuiRender()
	{
		if( ImGui::Begin( m_Name.c_str(), &m_Open ) )
		{
			// Draw main
			if( m_CurrentGraph->IsOpen() )
			{
				m_CurrentGraph->OnImGuiRender();
			}
			else
			{
				m_CurrentGraph->Open( false );
				m_Open = false;
			}

			ImGui::End();
		}

		const auto& rSelected = m_NodeEditor->GetSelectedNodes();
		for( const auto& rNode : rSelected )
		{
			if( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
			{
				SharedPtr<NodeEditorNodeBase> baseNode = m_NodeEditor->FindNode( rNode );
				if( baseNode->ExecutionType == NodeExecutionType::AnimGraphStateMachineStateNode )
				{
					AnimGraphStateMachineStateNode* pStateNode = dynamic_cast< AnimGraphStateMachineStateNode* >( baseNode.Get() );
					if( pStateNode )
					{
						m_CurrentGraph = ( SharedPtr<NodeEditor> )pStateNode->GetGraph();
						m_CurrentGraph->Open( true );
					}
				}
			}
		}

	}

	void AnimationControllerAssetViewer::OnEvent( Event& rEvent )
	{
		if( m_NodeEditor )
		{
			m_NodeEditor->OnEvent( rEvent );
		}
	}

	void AnimationControllerAssetViewer::OnRuntimeStateChanged( RuntimeState newState, RuntimeState oldState )
	{

	}

	void AnimationControllerAssetViewer::SetupNewNodeEditor()
	{
		SharedPtr<AnimGraphOutputNode> outputNode = AnimGraphNodeLibrary::SpawnOutputNode( m_NodeEditor );
		m_RootNodeID = outputNode->ID;


		SharedPtr<AnimGraphStateMachineStateNode> sm0 = AnimGraphNodeLibrary::SpawnStateMachineStateNode( m_NodeEditor );
//		SharedPtr<AnimGraphStateMachineStateNode> sm1 = AnimGraphNodeLibrary::SpawnStateMachineStateNode( m_NodeEditor );
	}

	void AnimationControllerAssetViewer::SetupNodeEditorCallbacks()
	{
#if !defined(SAT_DIST)
		m_NodeEditor->SetCreateNewNodeFunction(
			[ & ]() -> SharedPtr<NodeEditorNodeBase>
		{
			SharedPtr<NodeEditorNodeBase> result;

			if( ImGui::MenuItem( "State Machine player" ) )
				result = ( SharedPtr<NodeEditorNodeBase> )AnimGraphNodeLibrary::SpawnStateMachinePlayerNode( m_NodeEditor );
			
			ImGui::SeparatorText( "Auxiliary" );

			if( ImGui::MenuItem( "[DEBUG] CREATE STATE MACHINE STATE NODE" ) )
				result = ( SharedPtr<NodeEditorNodeBase> )AnimGraphNodeLibrary::SpawnStateMachineStateNode( m_NodeEditor );
			
			if( ImGui::MenuItem( "Hint (Comment) Node" ) )
				result = NodeEditorHintNode::SpawnHintNode( m_NodeEditor );

			return result;
		} );
#endif
	}

}
