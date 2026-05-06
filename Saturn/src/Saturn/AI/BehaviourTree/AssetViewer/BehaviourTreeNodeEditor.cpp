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
#include "Nodes/BehaviourTreeSequenceNode.h"
#include "Nodes/BehaviourTreeSelectorNode.h"

#include "BehaviourTreeNodeEditor.h"
#include "BehaviourTreeNodeLibrary.h"

#include "Nodes/BehaviourTreeTaskNode.h"
#include "Nodes/BehaviourTreeNodeBase.h"

#include "Saturn/AI/BehaviourTree/Tasks/BehaviourTreeBaseTask.h"
#include "Saturn/AI/BehaviourTree/Tasks/BehaviourTreeCompositeTasks.h"

#include "Saturn/AI/AIAgentEntity.h"
#include "Saturn/AI/BehaviourTree/BehaviourTreeMemoryAssetViewer.h"

#include "Saturn/ImGui/ImGuiWindowManager.h"

#include "Saturn/GameFramework/SClass.h"

#if !defined(SAT_DIST)
#include "Saturn/Core/App.h"
#include "Saturn/ImGui/EditorEvents.h"
#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"
#endif

#include "Saturn/Core/Profiler.h"

namespace Saturn {

	BehaviourTreeNodeEditor::BehaviourTreeNodeEditor()
		: FDependentNodeEditorSuper()
	{
	}

	BehaviourTreeNodeEditor::BehaviourTreeNodeEditor( AssetID id )
		: FDependentNodeEditorSuper( id )
	{
	}

	BehaviourTreeNodeEditor::~BehaviourTreeNodeEditor()
	{
	}

	SharedPtr<NodeEditorNodeBase> BehaviourTreeNodeEditor::SetupNewNodeEditor()
	{
		return BehaviourTreeNodeLibrary::SpawnRootNode( SharedFromThis() );
	}

	void BehaviourTreeNodeEditor::TraverseBehaviourTree( const SharedPtr<NodeEditorNodeBase>& rRootNode )
	{
#if !defined(SAT_DIST)
		m_EvaluationOrder.clear();

		// BehaviourTree flow in the left direction
		// We use a deques as they don't not enforce FIFO or LIFO
		// The evaluation order can be different as each level of the tree can be different
		//                    ID,   LVL/DEPTH
		std::deque<std::pair<UUID, int>> temporaryStack;
		temporaryStack.emplace_back( rRootNode->ID, 0 );

		size_t nodeEvaluationOrder = 0;
		while( !temporaryStack.empty() )
		{
			const auto [currentID, currentLevel] = temporaryStack.back();
			temporaryStack.pop_back();

			// The evaluator only needs the nodes at level one
			// As nodes at level one have to be a sequence or a selector node
			if( currentLevel == 1 )
			{
				m_EvaluationOrder.emplace_back( currentID, currentLevel );
			}
			
			// Find neighbors from outputs then keep going down that path until there is no more neighbors
			SharedPtr<BehaviourTreeNodeBase> treeNode = FindNode( currentID ).As<BehaviourTreeNodeBase>();
			if( treeNode )
			{
				treeNode->EvaluationOrder = nodeEvaluationOrder++;

				auto rNeighbours = FindNeighborsLeft( treeNode );

				// Behaviour tree are left to right at the composite level
				std::sort( rNeighbours.begin(), rNeighbours.end(),
					[ this ]( const UUID& a, const UUID& b )
				{
					return ed::GetNodePosition( ed::NodeId( a ) ).x < ed::GetNodePosition( ed::NodeId( b ) ).x;
				} );

				switch( treeNode->ExecutionType )
				{
					default: break;

					case NodeExecutionType::BehaviourTreeSequenceNode:
					{
						SharedPtr<BehaviourTreeSequenceNode> seq = treeNode.As<BehaviourTreeSequenceNode>();
						if( seq )
						{
							seq->Reset();
							seq->AddChildren( rNeighbours );
						}
					} break;

					case NodeExecutionType::BehaviourTreeSelectorNode:
					{
						SharedPtr<BehaviourTreeSelectorNode> selector = treeNode.As<BehaviourTreeSelectorNode>();
						if( selector )
						{
							selector->Reset();
							selector->AddChildren( rNeighbours );
						}
					} break;
				}

				// Push right to left so that the left most node is visited first 
				for( auto itr = rNeighbours.rbegin(); itr != rNeighbours.rend(); ++itr )
				{
					temporaryStack.emplace_back( *itr, currentLevel + 1 );
				}
			}
		}
#endif
	}


#if !defined(SAT_DIST)
	void BehaviourTreeNodeEditor::OnTopBarRender()
	{
		if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Settings" ), { 24.0f, 24.0f } ) )
		{
			ImGui::OpenPopup( "##SETTINGSBTNE" );
		}

		if( ImGui::BeginItemTooltip() )
		{
			ImGui::Text( "Open the settings of this Node Editor" );

			ImGui::EndTooltip();
		}

		if( ImGui::BeginPopup( "##SETTINGSBTNE", ImGuiWindowFlags_NoSavedSettings ) )
		{
			ImGui::Checkbox( "Auto Evaluate", &m_AutoEvaluate );

			if( ImGui::BeginItemTooltip() )
			{
				ImGui::Text( "Re-evaluate the Graph when it is modified, turn this off when dealing with large behaviour trees." );

				ImGui::EndTooltip();
			}

			ImGui::EndPopup();
		}

		if( ImGui::Button( "Open Tree Memory" ) )
		{
			const Ref<Asset> asset = AssetManager::Get()->FindAsset( m_BehaviourTreeMemoryAssetID );
			if( asset )
			{
				const std::string windowName = std::format( "{0}##{1}", asset->Name, ( uint64_t ) m_BehaviourTreeMemoryAssetID );
				ImGuiWindowManager::Get()->OpenOrShowWindow<BehaviourTreeMemoryAssetViewer>( windowName, m_BehaviourTreeMemoryAssetID );

				ImGui::SetWindowFocus( windowName.c_str() );
			}
		}
	}

	void BehaviourTreeNodeEditor::OnExtraRender()
	{
		std::vector<UUID> nodes = GetSelectedNodes();
		if( nodes.size() == 1 )
		{
			SharedPtr<BehaviourTreeNodeBase> treeNode = FindNode( nodes[ 0 ] ).As<BehaviourTreeNodeBase>();
			if( treeNode && treeNode->RenderType == NodeRenderType::Tree )
			{
				ImGui::Text( "%s/%llu", treeNode->Name.c_str(), treeNode->ID );
				ImGui::Text( "Order: %i", treeNode->EvaluationOrder );
					
				ImGui::Separator();

				treeNode->RenderDetails();
			}
		}
		else
		{
			if( Auxiliary::TreeNode( "Behaviour Tree Memory (blackboard)" ) ) 
			{
				bool open = false;

				ImGui::BeginHorizontal( ( int ) m_AssetID );

				ImGui::Text( "Asset" );

				if( Auxiliary::ImageButton( EditorIcons::GetIcon( "Inspect" ), ImVec2( 24.0f, 24.0f ) ) )
				{
					open = true;
				}

				ImGui::Spring();

				UUID tempID = m_BehaviourTreeMemoryAssetID;
				if( Auxiliary::DrawAssetFinder( AssetType::BehaviourTreeMemory, &open, tempID ) )
				{
					AssetManager::Get()->UnregisterAssetDependency( m_AssetID, m_BehaviourTreeMemoryAssetID );
					
					MarkDirty();
					m_BehaviourTreeMemoryAssetID = tempID;

					AssetManager::Get()->RegisterAssetDependency( m_AssetID, m_BehaviourTreeMemoryAssetID );

					m_BlackboardSpec = AssetManager::Get()->GetAssetAs< BehaviourTreeMemorySpecification >( m_BehaviourTreeMemoryAssetID );
				}

				if( m_BehaviourTreeMemoryAssetID != 0 )
				{
					ImGui::InputText( "##asset", (char*)std::to_string( m_BehaviourTreeMemoryAssetID ).data(), 256, ImGuiInputTextFlags_ReadOnly );
				}
				else
				{
					ImGui::InputText( "##asset", (char*)"", 1, ImGuiInputTextFlags_ReadOnly );
				}

				ImGui::EndHorizontal();

				Auxiliary::EndTreeNode();
			}
		}
	}

	void BehaviourTreeNodeEditor::OnNodeEditorEvent( NodeEditorAction action )
	{
		if( !m_AutoEvaluate )
			return;

		switch( action )
		{
			case NodeEditorAction::CreateLink:
			case NodeEditorAction::BreakLink:
			case NodeEditorAction::CreateNode:
			case NodeEditorAction::DestroyNode:
			case NodeEditorAction::MoveNode:
				Evaluate();
				break;

			case NodeEditorAction::PreEvaluate:
			{
				std::vector<SharedPtr<NodeEditorNodeBase>> order;
				Sort( order );

				m_PreCompiler->Init( order );
			} break;

			case NodeEditorAction::PostEvaluateSuccess:
			{
				BuildTaskCache();
				SaveAndMarkClean();
			} break;

			case NodeEditorAction::SelectNode:
			case NodeEditorAction::DeselectNode:
			case NodeEditorAction::SelectLink:
			case NodeEditorAction::DeselectLink:
			default: break;
		}
	}

	void BehaviourTreeNodeEditor::OnDebugBreak()
	{
		FDependentNodeEditorSuper::OnDebugBreak();
	}

#endif

	void BehaviourTreeNodeEditor::BuildTaskCache()
	{
		std::vector<SharedPtr<NodeEditorNodeBase>> order;
		Sort( order );

		m_TaskCache.BuildMasterList( order );
	}

	void BehaviourTreeNodeEditor::Sort( std::vector<SharedPtr<NodeEditorNodeBase>>& rOrder )
	{
		if( const auto rootNode = FindNode( "Root Node" ) ) 
		{
			TraverseBehaviourTree( rootNode );

			for( const auto& rCompositeInfo : m_EvaluationOrder )
			{
				SortFrom( rOrder, FindNode( rCompositeInfo.NodeID ) );
			}
		}
	}

	void BehaviourTreeNodeEditor::SortFrom( std::vector<SharedPtr<NodeEditorNodeBase>>& rOrder, SharedPtr<NodeEditorNodeBase> node )
	{
		// Push this node...
		rOrder.push_back( node );

		/*
		switch( node->ExecutionType )
		{
			default: break;

			case NodeExecutionType::BehaviourTreeSequenceNode:
			{
				SharedPtr<BehaviourTreeSequenceNode> seq = node.As<BehaviourTreeSequenceNode>();
				if( seq )
				{
					for( const auto& rChild : seq->GetChildren() )
					{
						SortFrom( rOrder, FindNode( rChild ) );
					}
				}
			} break;

			case NodeExecutionType::BehaviourTreeSelectorNode:
			{
				SharedPtr<BehaviourTreeSelectorNode> selector = node.As<BehaviourTreeSelectorNode>();
				if( selector )
				{
					for( const auto& rChild : selector->GetChildren() )
					{
						SortFrom( rOrder, FindNode( rChild ) );
					}
				}
			} break;
		}
		*/
	}

#if !defined(SAT_DIST)
	void BehaviourTreeNodeEditor::ShowTreeFlow()
	{
	}

	void BehaviourTreeNodeEditor::FindTreeFlow()
	{
	}

	void BehaviourTreeNodeEditor::BuildFlow( SharedPtr<BehaviourTreeNodeBase> node )
	{
	}
#endif

	void BehaviourTreeNodeEditor::SerialiseData( std::ofstream& rStream, bool isForDist )
	{
		FDependentNodeEditorSuper::SerialiseData( rStream, isForDist );
		RawSerialisation::WriteVector( m_EvaluationOrder, rStream );

		RawSerialisation::WriteObject( m_BehaviourTreeMemoryAssetID, rStream );
	}

	void BehaviourTreeNodeEditor::DeserialiseData( FDependentIStream& rStream )
	{
		FDependentNodeEditorSuper::DeserialiseData( rStream );
		RawSerialisation::ReadVector( m_EvaluationOrder, rStream );

		RawSerialisation::ReadObject( m_BehaviourTreeMemoryAssetID, rStream );

#if !defined(SAT_DIST)
		m_BlackboardSpec = AssetManager::Get()->GetAssetAs< BehaviourTreeMemorySpecification>( m_BehaviourTreeMemoryAssetID );

		if( m_BlackboardSpec )
		{
			for( auto&& [id, rNode] : m_Nodes )
			{
				SharedPtr<BehaviourTreeNodeBase> treeNode = rNode.As<BehaviourTreeNodeBase>();
				if( treeNode )
				{
					treeNode->PostDeserialise();
				}
			}
		}
#endif
	}

}
