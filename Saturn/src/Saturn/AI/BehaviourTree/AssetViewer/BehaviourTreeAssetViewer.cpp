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
#include "BehaviourTreeAssetViewer.h"

#include "Nodes/BehaviourTreeRootNode.h"
#include "Nodes/BehaviourTreeSelectorNode.h"
#include "Nodes/BehaviourTreeSequenceNode.h"
#include "Nodes/BehaviourTreeTaskNode.h"

#include "BehaviourTreeNodeLibrary.h"

#include "Saturn/NodeEditor/NodeEditorHintNode.h"
#include "Saturn/NodeEditor/Serialisation/NodeCache.h"

#include "Saturn/ImGui/UndoRedo/GlobalUndoRedoGroup.h"
#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Scene/Scene.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include <imgui_internal.h>

namespace Saturn {

	BehaviourTreeAssetViewer::BehaviourTreeAssetViewer( AssetID id )
		: AssetViewer( id )
	{
		m_AssetType = AssetType::BehaviourTree;
		AddBehaviourTree();
	}

	BehaviourTreeAssetViewer::~BehaviourTreeAssetViewer()
	{
#if !defined(SAT_DIST)
		if( m_Dirty || m_NodeEditor->IsDirty() )
		{
			m_NodeEditor->SaveAndMarkClean();
		}

		m_Asset = nullptr;

		GlobalUndoRedoGroup::Get()->RemoveIfActionHasIdentifier( m_AssetID );
		m_NodeEditor = nullptr;

		for( BehaviourTreeBaseTask* pTaskClass : m_ClassCache )
		{
			delete pTaskClass;
		}

		m_ClassCache.clear();
#endif
	}

	void BehaviourTreeAssetViewer::OnImGuiRender()
	{
#if !defined(SAT_DIST)
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

	void BehaviourTreeAssetViewer::AddBehaviourTree()
	{
		const Ref<Asset> asset = AssetManager::Get()->FindAsset( m_AssetID );
		m_Asset = asset;

		m_Name = std::format( "{0}##{1}", m_Asset->Name, ( uint64_t ) m_AssetID );

		m_NodeEditor = SharedPtr<BehaviourTreeNodeEditor>::Create( m_AssetID );
		const std::string filename = std::format( "{0}.sbt", m_Asset->Name );

		if( NodeCacheEditor::ReadNodeEditorCache( m_NodeEditor, m_AssetID, filename ) )
		{
			m_RootNodeID = m_NodeEditor->FindNode( "Root Node" )->ID;
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

#if !defined(SAT_DIST)
		// Discover all classes that are based from BehaviourTreeBaseTask for our context menu.
		const auto map = ClassMetadataHandler::Get().GetAllClassesBasedFrom<BehaviourTreeBaseTask>();
		for( auto* pClass : map )
		{
			BehaviourTreeBaseTask* pObject = dynamic_cast<BehaviourTreeBaseTask*>( ClassMetadataHandler::Get().CreateClassObject( pClass ) );
			
			if( pObject ) 
			{
				// Create default task object from SClass
				m_ClassCache.push_back( pObject );
			}
		}
#endif
	}

	void BehaviourTreeAssetViewer::SetupNewNodeEditor()
	{
		m_RootNodeID = m_NodeEditor->SetupNewNodeEditor()->ID;
		MarkDirty();
	}

	void BehaviourTreeAssetViewer::SetupNodeEditorCallbacks()
	{
#if !defined(SAT_DIST)
		m_NodeEditor->SetCreateNewNodeFunction(
			[ & ]() -> SharedPtr<NodeEditorNodeBase>
		{
			SharedPtr<NodeEditorNodeBase> result;

			ImGui::SeparatorText( "Basic/Composite" );
			
			if( ImGui::MenuItem( "Selector" ) )
				result = BehaviourTreeNodeLibrary::SpawnSelectorNode( m_NodeEditor );

			if( ImGui::MenuItem( "Sequence" ) )
				result = BehaviourTreeNodeLibrary::SpawnSequenceNode( m_NodeEditor );

			ImGui::SeparatorText( "Tasks" );
			for( auto* pClass : m_ClassCache )
			{
				if( !pClass->IsSpawnableNode() ) continue;

				if( ImGui::MenuItem( pClass->GetTaskName() ) ) 
				{
					// NOTE: Raw ptr converted to Ref<> by BehaviourTreeTaskNode
					// Create new task object
					SObject* pNewTaskObject = ClassMetadataHandler::Get().CreateClassObject( pClass->GetClass()->GetHash() );

					// Then, create the new node
					BehaviourTreeTaskNode* pNode = NewObject<BehaviourTreeTaskNode>( m_NodeEditor.Get(), ( BehaviourTreeBaseTask* ) pNewTaskObject );

					SharedPtr<BehaviourTreeTaskNode> sp = pNode;

					result = sp;
					m_NodeEditor->AddNode( sp );

					break;
				}
			}

			ImGui::SeparatorText( "Auxiliary" );

			if( ImGui::MenuItem( "Hint (Comment) Node" ) )
				result = NodeEditorHintNode::SpawnHintNode( m_NodeEditor );

			return result;
		} );

		m_NodeEditor->SetTopBarFunction( [ & ]()
		{
			if( !g_ActiveScene->IsRuntimeActive() )
				return;

			ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical );

			// Debug References
			ImGui::Text( "References" );

			ImGui::SetNextItemWidth( 134.0F );

			std::string previewVal;
			if( m_SelectedReferenceObject )
				previewVal = m_SelectedReferenceObject->GetDebugName();

			if( ImGui::BeginCombo( "##References", previewVal.data(), ImGuiComboFlags_WidthFitPreview ) )
			{
				if( ImGui::Selectable( "Main Asset (removes reference)" ) )
				{
					m_SelectedReferenceObject = nullptr;
					m_NodeEditor->ResetDebugging();
				}

				ImGui::Separator();

				for( const auto& rBehaviourTree : m_ReferencingObjects )
				{
					const bool selected = m_SelectedReferenceObject == rBehaviourTree;

					if( ImGui::Selectable( rBehaviourTree->GetDebugName().c_str(), selected ) )
					{
						m_SelectedReferenceObject = rBehaviourTree;

//						m_NodeEditor->SetCurrentDebuggingEditor( rBehaviourTree->GetNodeEditor() );
					}
				}

				ImGui::EndCombo();
			}
		} );
#endif
	}

#if !defined(SAT_DIST)
	void BehaviourTreeAssetViewer::OnRuntimeStateChanged( RuntimeState newState, RuntimeState oldState )
	{
		switch( newState )
		{
			default:
			case RuntimeState::Starting:
			case RuntimeState::NoState:
			case RuntimeState::Suspended:
				break;

			case RuntimeState::Running:
			{
				if( oldState == RuntimeState::Starting || oldState == RuntimeState::NoState ) 
				{
					m_SelectedReferenceObject = nullptr;

					m_NodeEditor->SetUserAuthorityFlag( NodeEditorUserAuthority::Editing, false );
				}
			} break;

			case RuntimeState::Ending:
			{
				m_NodeEditor->AllowEditingAndDisableDebugging();

				m_SelectedReferenceObject = nullptr;
				m_ReferencingObjects.clear();
			} break;
		}
	}

	void BehaviourTreeAssetViewer::AddBehviourTreeReference( Ref<BehaviourTree> bt )
	{
		m_ReferencingObjects.push_back( bt );
	}

	void BehaviourTreeAssetViewer::SetActiveReference( Ref<BehaviourTree> bt )
	{
		const auto itr = std::find( m_ReferencingObjects.begin(), m_ReferencingObjects.end(), bt );
		if( itr != m_ReferencingObjects.end() )
		{
			m_SelectedReferenceObject = bt;
		}
	}

	void BehaviourTreeAssetViewer::OnDebugBreak()
	{
		m_NodeEditor->OnDebugBreak();
	}

#endif
}
