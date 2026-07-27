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
#include "NodeEditor.h"

#include "Saturn/Core/Input.h"

#include "Saturn/Asset/AssetManager.h"

#if !defined(IMGUI_DEFINE_MATH_OPERATORS)
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include "Saturn/NodeEditor/Runtime/NodeEditorRuntime.h"
#include "Saturn/NodeEditor/Serialisation/NodeCache.h"
#include "Saturn/NodeEditor/NodeEditorBlueprintNode.h"
#include "Saturn/NodeEditor/UndoRedo/UndoRedoNodeEditorActions.h"
#include "Saturn/NodeEditor/PreCompiler/NodeEditorDefaultPreCompiler.h"
#include "Saturn/NodeEditor/PreCompiler/StandardErrorWarningToString.h"

#include "Saturn/NodeEditor/Debugging/NodeBreakPointManager.h"

#include "Saturn/ImGui/UndoRedo/GlobalUndoRedoGroup.h"

#include "Saturn/GameFramework/SClass.h"
#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "Saturn/Core/Profiler.h"

#include <backends/imgui_impl_vulkan.h>

namespace util = ax::NodeEditor::Utilities;

namespace Saturn {

	static constexpr inline bool operator==( const ImVec2& lhs, const ImVec2& rhs ) { return lhs.x == rhs.x && lhs.y == rhs.y; }
	static constexpr inline bool operator!=( const ImVec2& lhs, const ImVec2& rhs ) { return !( lhs == rhs ); }

	static void BuildNode( SharedPtr<NodeEditorNodeBase>& rNode )
	{
		for( auto& input : rNode->Inputs )
		{
			input->Node = rNode;
			input->Kind = PinKind::Input;
		}

		for( auto& output : rNode->Outputs )
		{
			output->Node = rNode;
			output->Kind = PinKind::Output;
		}
	}

	Ref<Texture2D> NodeEditor::GetBlueprintBackground()
	{
		auto icon = EditorIcons::GetIcon( "BlueprintBackground" );
		if( icon == nullptr )
		{
			const auto texture = Ref<Texture2D>::Create( "content/textures/editor/BlueprintBackground.png", AddressingMode::Repeat, TextureLoadFlags_LoadOnMainThread );

			EditorIcons::AddIcon( texture );

			ImGui_ImplVulkan_AddTexture( texture->GetSampler(), texture->GetImageView(), texture->GetDescriptorInfo().imageLayout );

			return texture;
		}

		return icon;
	}

	//////////////////////////////////////////////////////////////////////////
	// NODE EDITOR

	NodeEditor::NodeEditor( AssetID ID )
		: m_AssetID( ID ), m_OutputWindow( ID )
	{
		CreateEditor();
	}

	NodeEditor::NodeEditor()
		: m_AssetID( 0 ), m_OutputWindow( 0llu )
	{
		SetUserAuthorityFlag( NodeEditorUserAuthority::Full, true );
	}

	NodeEditor::~NodeEditor()
	{
		Close();

		//if( m_AssetID != 0 )
		GlobalUndoRedoGroup::Get()->RemoveIfActionHasIdentifier( m_AssetID );

		m_ZoomTexture = nullptr;
		m_CompileTexture = nullptr;
	}

	void NodeEditor::CreateEditor()
	{
		// Zoom Levels in imgui_node_editor work backwards
		ImVector<float> zoomLvls;
		zoomLvls.reserve( 6 );

		zoomLvls.push_back( 0.25f ); // Highest zoom level (least zoomed in)
		zoomLvls.push_back( 0.5f );
		zoomLvls.push_back( 0.75f );
		zoomLvls.push_back( 1.0f );
		zoomLvls.push_back( 1.25f );
		zoomLvls.push_back( 1.50f ); // Lowest zoom level (most zoomed in)

		ed::Config config;
		config.SettingsFile = nullptr;
		config.UserPointer = this;
		config.CustomZoomLevels = zoomLvls;

		config.SaveNodeSettings = [](
			ed::NodeId nodeId,
			const char* pData,
			size_t size,
			ed::SaveReasonFlags reason,
			void* pUserPointer ) -> bool
		{
			auto* pThis = static_cast< NodeEditor* >( pUserPointer );
			auto pNode = pThis->FindNode( UUID( nodeId.Get() ) );

			if( !pNode )
				return false;

			// Ignore events if we are debugging, simulating or suspended.
			if(
				pThis->IsStateFlagSet( NodeEditorState_Debugging ) ||
				pThis->IsStateFlagSet( NodeEditorState_Simulating ) ||
				pThis->IsStateFlagSet( NodeEditorState_Suspended ) )
			{
				return false;
			}

			if( ( reason & ed::SaveReasonFlags::EndDrag ) == ed::SaveReasonFlags::EndDrag )
			{
				pThis->OnNodeEditorEvent( NodeEditorAction::MoveNode );

#if !defined(SAT_DIST)
				Ref<UndoRedoActionModifyNodePosition> action = Ref<UndoRedoActionModifyNodePosition>::Create(
					pThis->SharedFromThis(),
					pNode,
					pNode->PositionBeforeMove );

				pNode->PositionBeforeMove = ed::GetNodePosition( nodeId );

				GlobalUndoRedoGroup::Get()->AddAction( action, pThis->GetAssetID() );
#endif
			}

			if( ( reason & ed::SaveReasonFlags::Selection ) == ed::SaveReasonFlags::Selection )
			{
				pThis->OnNodeEditorEvent( NodeEditorAction::SelectNode );
			}

			// Only mark dirty if we are not loading
			// imgui_node_editor will call this function when initialising
			if( !pThis->IsStateFlagSet( NodeEditorState_Loading ) )
			{
				pThis->MarkDirty();
			}

			return true;
		};

		m_Editor = ed::CreateEditor( &config );
		ed::SetCurrentEditor( m_Editor );

		m_ZoomTexture = EditorIcons::GetIcon( "Inspect" );
		m_CompileTexture = EditorIcons::GetIcon( "Compile" );

		const auto texture = GetBlueprintBackground();
		m_Builder = util::BlueprintNodeBuilder( ( ImTextureID ) texture->GetDescriptorSet(), texture->Width(), texture->Height() );

		m_OutputWindow.PushMessage( { .MessageText = "Initialised new editor!", .Type = NodeEditorMessageSeverity::Info } );

		m_InternalEditorID = std::format( "Nc##{0}", ( uint64_t ) m_AssetID );
	}

	void NodeEditor::Reload()
	{
		m_Dirty = false;

		ed::SetCurrentEditor( nullptr );

		ed::DestroyEditor( m_Editor );

		m_Editor = nullptr;

		CreateEditor();
	}

	void NodeEditor::Close()
	{
#if !defined(SAT_DIST)
		m_HoveredNode = nullptr;
#endif
		m_OutputWindow.ClearOutput();

		ed::DestroyEditor( m_Editor );
		ed::SetCurrentEditor( nullptr );
		m_Editor = nullptr;

		m_CopyPasteNodeClasses.clear();

		for( auto& [id, rNode] : m_Nodes )
		{
			rNode->Destroy();
		}

		m_Nodes.clear();
		m_Links.clear();
	}

	bool NodeEditor::CanCreateLink( const Ref<Pin>& a, const Ref<Pin>& b )
	{
		if( !a || !b || a == b || a->Kind == b->Kind || a->Type != b->Type || a->Node == b->Node )
			return false;

		return true;
	}

	void NodeEditor::OnImGuiRender()
	{
#if !defined(SAT_DIST)
		if( !m_WindowOpen )
			return;

		ImGuiWindowFlags mainWindowFlags = ImGuiWindowFlags_MenuBar;
		if( m_Dirty )
			mainWindowFlags |= ImGuiWindowFlags_UnsavedDocument;

		// Draw main window
		ImGui::Begin( m_Name.c_str(), &m_WindowOpen, mainWindowFlags );

		if( ImGui::BeginMenuBar() )
		{
			if( ImGui::BeginMenu( "File" ) )
			{
				if( ImGui::MenuItem( "Save" ) )
				{
					SaveAndMarkClean();
				}

				if( ImGui::MenuItem( "Close" ) )
				{
					m_WindowOpen = false;
				}

				if( ImGui::MenuItem( "Find" ) )
				{
					m_IsSearching ^= 1;
				}

				ImGui::EndMenu();
			}

			if( ImGui::BeginMenu( "Test" ) )
			{
				if( ImGui::MenuItem( "Evaluate" ) )
				{
					EdEvaluateEditor();
				}

				ImGui::EndMenu();
			}

			if( ImGui::BeginMenu( "View" ) )
			{
				if( ImGui::MenuItem( "Zoom to Content" ) )
				{
					ed::NavigateToContent( 0.25f );
				}

				ImGui::SeparatorText( "Windows" );

				if( ImGui::MenuItem( "Show Output Windows" ) )
				{
					m_OutputWindow.ShowOrHide();
				}

				if( ImGui::MenuItem( "Show Debug Information" ) )
				{
					m_ShowDebugInformation ^= 1;
				}

				if( ImGui::MenuItem( "Show Details Windows" ) )
				{
					m_ShowDetailsInformation ^= 1;
				}

				if( ImGui::MenuItem( "Show Variables Windows" ) )
				{
					m_ShowDataWindow ^= 1;
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		// Ensure our editor is the current one
		// We'll use a VariableGuard<ed::EditorContext*> when we can't be sure that we are the current node editor.
		ed::SetCurrentEditor( m_Editor );

		if( m_PendingBreakHandle )
		{
			// Hacky way for us to find what node caused the break point.
			if( m_HoveredNode )
			{
				ed::SelectNode( ed::NodeId( m_HoveredNode->ID ) );
				ed::NavigateToSelection();

				ImGui::FocusWindow( ImGui::GetCurrentWindow() );

				m_HoveredNode = nullptr;
			}

			m_PendingBreakHandle = false;
		}

		TryDrawCompileErrorModal();
		TryDrawUnsavedChangesModal();

		if( m_ViewportSize != ImGui::GetContentRegionAvail() )
			m_ViewportSize = ImGui::GetContentRegionAvail();

		DrawTopBarChildInternal();

		// Hand off to imgui_node_editor and draw the actual node editor and nodes
		ed::Begin( m_InternalEditorID.c_str(), ImGui::GetContentRegionAvail() );

		const auto cursorTopLeft = ImGui::GetCursorScreenPos();

		DrawGraph();

		ImGui::SetCursorScreenPos( cursorTopLeft );

		ed::Suspend();

		ed::NodeId activeID{};
		if( ed::ShowNodeContextMenu( &activeID ) )
		{
			ImGui::OpenPopup( "NE_NodeAction" );
			m_HoveredNode = FindNode( UUID( activeID.Get() ) );
		}

		if( ed::ShowBackgroundContextMenu() )
		{
			ImGui::OpenPopup( "Create New Node" );
			m_NewNodeLinkPin = nullptr;
		}
		ed::Resume();

		ed::Suspend();

		if( m_ShowDetailsInformation ) DrawDetailsWindow();
		if( m_ShowDataWindow )         DrawDataWindow();
		if( m_ShowDebugInformation )   DrawDebugWindow();

		// Create new node context popup window
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 8.0f, 8.0f ) );
		if( ImGui::BeginPopup( "Create New Node" ) )
		{
			if( HasUserAuthority( NodeEditorUserAuthority::Editing ) )
			{
				const ImVec2 mousePos = ImGui::GetMousePosOnOpeningCurrentPopup();
				SharedPtr<NodeEditorNodeBase> node = nullptr;

				if( m_CreateNewNodeFunction )
					node = m_CreateNewNodeFunction();

				if( node )
				{
					ed::SetNodePosition( ed::NodeId( node->ID ), ed::ScreenToCanvas( mousePos ) );
					OnChooseNewNode( node );
				}
			}
			else
			{
				ImGui::Text( "Insufficient User Authority to add a new node." );
			}

			ImGui::EndPopup();
		}
		else
			m_CreateNewNode = false;

		ImGui::PopStyleVar();

		// Node context window popup
		if( ImGui::BeginPopup( "NE_NodeAction" ) )
		{
			m_HoveredNode->RenderContextWindow();

			ImGui::SeparatorText( "Breakpoints" );

			if( NodeBreakPointManager::Get().HasBreakPoint( m_HoveredNode->ID ) )
			{
				auto& rBreakpoint = NodeBreakPointManager::Get().GetBreakPoint( m_HoveredNode->ID );

				if( ImGui::BeginMenu( "Breakpoint settings" ) )
				{
					if( rBreakpoint.Active )
					{
						if( ImGui::MenuItem( "Disable breakpoint" ) )
						{
							rBreakpoint.Active = false;

							MarkDirty();
						}
					}
					else
					{
						if( ImGui::MenuItem( "Enable breakpoint" ) )
						{
							rBreakpoint.Active = true;

							MarkDirty();
						}
					}

					if( ImGui::BeginMenu( "Breakpoint type" ) )
					{
						if( ImGui::MenuItem( "Normal" ) )
						{
							rBreakpoint.Type = NodeBreakPointType::Normal;
							MarkDirty();
						}

						if( ImGui::MenuItem( "Single fire" ) )
						{
							rBreakpoint.Type = NodeBreakPointType::SingleFire;
							MarkDirty();
						}

						if( ImGui::MenuItem( "Conditional" ) )
						{
							rBreakpoint.Type = NodeBreakPointType::Conditional;
							MarkDirty();
						}

						ImGui::EndMenu();
					}

					ImGui::EndMenu();
				}

				if( ImGui::Selectable( "Remove breakpoint" ) )
				{
					NodeBreakPointManager::Get().Remove( m_HoveredNode->ID );
					MarkDirty();
				}

				ImGui::Separator();

				{
					Auxiliary::ScopedDisabledFlag disabled( true );
					ImGui::Text( "Hit Count: %i", rBreakpoint.HitCount );
				}
			}
			else
			{
				if( ImGui::Selectable( "Add breakpoint" ) )
				{
					NodeBreakPointManager::Get().AddBreakPoint( m_HoveredNode->ID, NodeBreakPointType::Normal );
					MarkDirty();
				}
			}

			{
				ImGui::SeparatorText( "Debug Info" );
				Auxiliary::ScopedDisabledFlag disabled( true );

				ImGui::Text( "NC/%" PRIu64, m_HoveredNode->ID );
				ImGui::Text( "%s", m_HoveredNode->Name.c_str() );

				ImGui::Separator();

				const auto& rPosition = ed::GetNodePosition( ed::NodeId( m_HoveredNode->ID ) );
				const auto& rSize = ed::GetNodeSize( ed::NodeId( m_HoveredNode->ID ) );

				ImGui::Text( "Size X/%f Y/%f", rSize.x, rSize.y );
				ImGui::Text( "Pos X/%f Y/%f", rPosition.x, rPosition.y );

			}

			ImGui::EndPopup();
		}

		ed::Resume();

		ed::End();

		if( m_BreadCrumbsFunction )
			m_BreadCrumbsFunction();

		HandleStateCanvasBorders();

		if( m_IsSearching )				DrawBeginSearchWindow();
		if( m_ShowSearchResultsWindow ) DrawSearchResultsWindow();
		if( m_OutputWindow.IsOpen() )	m_OutputWindow.Draw();

		ImGui::End(); // NODE_EDITOR

		// Window closed but we are dirty, show unsaved changes modal and keep window open.
		if( !m_WindowOpen && m_Dirty )
		{
			m_WindowOpen = true;

			if( m_HasPreCompileErrors )
				m_ShowErrorPopup = true;

			m_ShowUnsavedChanges = true;
		}
#endif
	}

	void NodeEditor::DrawGraph()
	{
		for( auto& [id, rNode] : m_Nodes )
		{
			rNode->Render( m_Builder );
		}

		for( const auto& rLink : m_Links )
			ed::Link( ed::LinkId( rLink->ID ), ed::PinId( rLink->StartPinID ), ed::PinId( rLink->EndPinID ), rLink->Color );

		HandleCreate();
	}

	SharedPtr<NodeEditorNodeBase> NodeEditor::CopyNode( const SharedPtr<const NodeEditorNodeBase> originalNode )
	{
		SObject* newObject = ClassMetadataHandler::Get().CreateClassObject( originalNode->GetClass(), this );
		if( !newObject )
			return nullptr;

		SharedPtr<NodeEditorNodeBase> spNode( ( NodeEditorNodeBase* ) newObject );
		AddNode( spNode );

		// Copy name
		// TOOD: Make a function for this? OnNodePasted?
		spNode->Name = originalNode->Name;

		ed::SetNodePosition( ed::NodeId( spNode->ID ), ed::GetNodePosition( ed::NodeId( originalNode->ID ) ) );

		return spNode;
	}

	SharedPtr<NodeEditorNodeBase> NodeEditor::CopyPaste_CopyNode( const NodeEditorCopyPasteInformation& rCopyInfo )
	{
		const auto nodeToCopy = rCopyInfo.Node.Access();
		if( !nodeToCopy )
			return nullptr;

		// Copy the parent node.
		auto newlyCreatedNode = CopyNode( nodeToCopy );
		if( !newlyCreatedNode )
			return nullptr;

		for( const auto& [parentNode, childrenInfo] : rCopyInfo.Children )
		{
			for( const auto& childInfo : childrenInfo )
			{
				auto childNode = CopyPaste_CopyNode( childInfo );
				if( childNode )
				{
					childNode->pParentObject = newlyCreatedNode.Get();
				}
			}
		}

		return newlyCreatedNode;
	}

	void NodeEditor::CopyPasteBuildChildrenList( SharedPtr<NodeEditorNodeBase> parentNode, NodeEditorCopyPasteInformation& rInfo )
	{
		// TODO: FIX THIS, ITS SLOW, m_Nodes is the list of _all_ nodes!
		//		 maybe we should have a map that allows us to only get the nodes at a certain sub-graph parent id.
		for( const auto& [id, rCandidate] : m_Nodes )
		{
			// IsDirectDescendantOf
			if( rCandidate->pParentObject == parentNode.Get() )
			{
				auto& rChildInfo = rInfo.Children.emplace_back();
				rChildInfo.Node = rCandidate;

				// Now get the child's children and so on.
				CopyPasteBuildChildrenList( rCandidate, rChildInfo );
			}
		}
	}

	void NodeEditor::OnUpdate( Timestep ts )
	{
	}

	void NodeEditor::OnEvent( Event& rEvent )
	{
		switch( rEvent.Type )
		{
			case EventType::KeyPressed:
			{
				OnKeyPressed( ( RubyKeyEvent& ) rEvent );
			} break;

			default:
				break;
		}
	}

#if !defined(SAT_DIST)
	void NodeEditor::OnDebugBreak()
	{
		SetUserAuthorityFlag( NodeEditorUserAuthority::Editing, false );

		SetStateFlag( NodeEditorState_Evaluating, false );
		SetStateFlag( NodeEditorState_Debugging | NodeEditorState_Suspended, true );

		m_PendingBreakHandle = true;
	}
#endif

	void NodeEditor::ThrowError( const std::string& rMessage )
	{
		m_OutputWindow.PushMessage( { .MessageText = rMessage, .Type = NodeEditorMessageSeverity::Error } );
	}

	void NodeEditor::ThrowWarning( const std::string& rMessage )
	{
		m_OutputWindow.PushMessage( { .MessageText = rMessage, .Type = NodeEditorMessageSeverity::Warning } );
	}

	void NodeEditor::PushInfoMessage( const std::string& rMessage )
	{
		m_OutputWindow.PushMessage( { .MessageText = rMessage, .Type = NodeEditorMessageSeverity::Info } );
	}

	void NodeEditor::DeleteDeadLinks( UUID nodeID )
	{
		const auto wasConnectedToTheNode = [ & ]( const Ref<Link>& link )
		{
			return ( !FindPin( link->StartPinID ) ) || ( !FindPin( link->EndPinID ) )
				|| FindPin( link->StartPinID )->Node->ID == nodeID
				|| FindPin( link->EndPinID )->Node->ID == nodeID;
		};

		const auto removeIt = std::remove_if( m_Links.begin(), m_Links.end(), wasConnectedToTheNode );
		m_Links.erase( removeIt, m_Links.end() );
	}

	void NodeEditor::SaveAndMarkClean()
	{
		SaveSettings();
		NodeCacheEditor::WriteNodeEditorCache( SharedFromThis(), m_CustomNameNC );

		m_Dirty = false;

		if( m_AssetID )
		{
			// Find and bump asset version.
			Ref<Asset> correspondingAsset = AssetManager::Get()->FindAsset( m_AssetID );
			correspondingAsset->Version = AssetVersion::Latest;

			// NB: Already done by WriteNodeEditorCache!
//			m_Version = NodeEditorVersion::Latest;

			// #SaveAssetManagerOnJT
			AssetManager::Get()->Save();
		}
	}

	void NodeEditor::DeleteLink( UUID id, bool skipUndoRedo )
	{
		const auto Itr = std::find_if( m_Links.begin(), m_Links.end(),
			[ id ]( const auto& rLink )
		{
			return rLink->ID == id;
		} );

		if( Itr != m_Links.end() )
		{
			// Because "DeleteLink" is called from the undo/redo stack we don't want to add a new action.
			if( !skipUndoRedo )
			{
				Ref<Link> link = *Itr;

				Ref<UndoRedoActionDeleteLink> action = Ref<UndoRedoActionDeleteLink>::Create( SharedFromThis(), link );
				GlobalUndoRedoGroup::Get()->AddAction( action, m_AssetID );
			}

			m_Links.erase( Itr );
		}

		OnNodeEditorEvent( NodeEditorAction::BreakLink );
	}

	void NodeEditor::DeleteNode( UUID id, bool skipUndoRedo /*= false */ )
	{
#if !defined(SAT_DIST)
		auto isDescendantOf = []( NodeEditorNodeBase* pTarget, const auto& rCandidate ) -> bool
		{
			if( !pTarget ) return false;

			NodeEditorNodeBase* pCurrentParent = rCandidate->pParentObject;
			while( pCurrentParent )
			{
				if( pCurrentParent == pTarget )
				{
					return true;
				}

				pCurrentParent = pCurrentParent->pParentObject;
			}

			return false;
		};

		const auto itr = std::find_if( m_Nodes.begin(), m_Nodes.end(),
			[ id ]( const auto& rNode )
		{
			return rNode.first == id;
		} );
		if( itr != m_Nodes.end() )
		{
			auto& rNode = ( itr->second );
			if( !rNode->IsFlagSet( NodeFlags_Irremovable ) )
			{
				std::vector<NodeEditorNodeBase*> children;
				for( const auto& [id, rCandidate] : m_Nodes )
				{
					if( isDescendantOf( rNode.Get(), rCandidate ) && rCandidate != rNode )
						children.push_back( rCandidate.Get() );
				}

				// If this node has children (making it a sub-graph) we need to create a different undo/redo action
				if( children.size() )
				{
					for( const auto pChild : children )
					{
						pChild->Destroy();

						DeleteDeadLinks( pChild->ID );
						m_Nodes.erase( pChild->ID );
					}

					children.clear();
				}

				if( !skipUndoRedo )
				{
					//					Ref<UndoRedoActionDeleteNode> action = Ref<UndoRedoActionDeleteNode>::Create( SharedFromThis(), rNode );
					//					GlobalUndoRedoGroup::Get()->AddAction( action, m_AssetID );
				}

				// Remove breakpoints, if any.
				NodeBreakPointManager::Get().Remove( id );

				rNode->Destroy();
				DeleteDeadLinks( id );

				rNode = nullptr;
				m_Nodes.erase( itr );

				OnNodeEditorEvent( NodeEditorAction::DestroyNode );
			}
		}
#endif
	}

	void NodeEditor::SetNodePosition( UUID nodeID, const ImVec2& rNewPosition )
	{
		VariableGuard<ed::EditorContext*, ed::EditorContext*> guard( m_Editor );

		ed::SetNodePosition( ed::NodeId( nodeID ), rNewPosition );
	}

#if !defined(SAT_DIST)
	void NodeEditor::AddSubGraph( SharedPtr<NodeEditorNodeBase> graph )
	{
		if( std::find( m_SubGraphs.begin(), m_SubGraphs.end(), graph ) == m_SubGraphs.end() )
			m_SubGraphs.push_back( graph );
	}

	void NodeEditor::RemoveSubGraph( SharedPtr<NodeEditorNodeBase> graph )
	{
		m_SubGraphs.erase( std::remove( m_SubGraphs.begin(), m_SubGraphs.end(), graph ), m_SubGraphs.end() );
	}

	void NodeEditor::ChangeEditorNextFrame( SharedPtr<NodeEditorNodeBase> graph )
	{
		m_ActiveSubGraph = graph;
	}

	void NodeEditor::ClearSubGraphs()
	{
		m_SubGraphs.clear();
		m_ActiveSubGraph = nullptr;
	}

	void NodeEditor::PopActiveSubGraphTo( SharedPtr<NodeEditorNodeBase> graph )
	{
		m_ActiveSubGraph = graph;

		// Remove everything after the graph.
		auto itr = std::find( m_SubGraphs.begin(), m_SubGraphs.end(), graph );
		if( itr != m_SubGraphs.end() )
		{
			m_SubGraphs.erase( std::next( itr ), m_SubGraphs.end() );
		}
	}

	void NodeEditor::AllowEditingAndDisableDebugging()
	{
		ResetDebugging();

		// Clear state, reset to editing.
		SetState( NodeEditorState_Editing );
		SetUserAuthorityFlag( NodeEditorUserAuthority::Editing, true );
	}

	void NodeEditor::SetCurrentDebuggingEditor( SharedPtr<NodeEditor> nodeEditor )
	{
		m_OldEditor = m_Editor;
		m_Editor = nodeEditor->m_Editor;
	}

	void NodeEditor::ResetDebugging()
	{
		// Probably didn't have a valid debugging editor and the user tried to remove the reference.
		if( !m_OldEditor )
			return;

		m_Editor = m_OldEditor;
		m_OldEditor = nullptr;
	}

#endif

	void NodeEditor::CreateNewEditorIfNeeded()
	{
		if( !m_Editor )
			CreateEditor();
	}

	void NodeEditor::DrawSimulatingCanvas()
	{
		auto canvasRect = ImRect( ed::GetRectMin(), ed::GetRectMax() );
		ImDrawList* pDrawList = ImGui::GetWindowDrawList();

		const ImU32 borderColor = IM_COL32( 100, 150, 255, 255 );
		const float thickness = 6.0F;
		const float rounding = 12.0F;

		// Draw
		pDrawList->AddRect( canvasRect.Min, canvasRect.Max, borderColor, rounding, ImDrawFlags_RoundCornersAll, thickness );

		const std::string canvasName = "SIMULATING | READ ONLY";
		const ImVec2 textSize = ImGui::CalcTextSize( canvasName.c_str() );
		const float padding = 10.0f;

		const ImVec2 textPos = ImVec2( canvasRect.Min.x + padding, canvasRect.Min.y + padding );
		pDrawList->AddText( textPos, IM_COL32( 255, 255, 255, 255 ), canvasName.c_str() );
	}

	void NodeEditor::DrawDebuggingCanvas()
	{
		auto canvasRect = ImRect( ed::GetRectMin(), ed::GetRectMax() );
		ImDrawList* pDrawList = ImGui::GetWindowDrawList();

		const ImU32 borderColor = IM_COL32( 255, 0, 0, 255 );
		const float thickness = 6.0F;
		const float rounding = 12.0F;

		// Draw
		pDrawList->AddRect( canvasRect.Min, canvasRect.Max, borderColor, rounding, ImDrawFlags_RoundCornersAll, thickness );

		const std::string canvasName = "DEBUGGING | PAUSED | READ ONLY";
		const ImVec2 textSize = ImGui::CalcTextSize( canvasName.c_str() );
		const float padding = 10.0f;

		const ImVec2 textPos = ImVec2( canvasRect.Min.x + padding, canvasRect.Min.y + padding );
		pDrawList->AddText( textPos, IM_COL32( 255, 255, 255, 255 ), canvasName.c_str() );
	}

	void NodeEditor::TryDrawUnsavedChangesModal()
	{
		if( !m_ShowErrorPopup && m_ShowUnsavedChanges && HasUserAuthority( NodeEditorUserAuthority::Editing ) )
			ImGui::OpenPopup( "Unsaved Changes" );

		// Unsaved changes modal
		// TODO: Center window with our main window
		ImGui::SetNextWindowPos( ImGui::GetWindowViewport()->GetCenter(), ImGuiCond_FirstUseEver );
		if( ImGui::BeginPopupModal( "Unsaved Changes", nullptr, ImGuiWindowFlags_NoSavedSettings ) )
		{
			ImGui::Text( "You have unsaved changes to this editor." );
			ImGui::Text( "Would you like to save before closing?" );

			ImGui::BeginHorizontal( "##DirtyModalOpt" );

			if( ImGui::Button( "Save" ) )
			{
				SaveAndMarkClean();

				m_WindowOpen = false;
				m_ShowUnsavedChanges = false;
				ImGui::CloseCurrentPopup();
			}

			if( ImGui::Button( "Discard changes" ) )
			{
				m_Dirty = false;
				m_WindowOpen = false;

				m_ShowUnsavedChanges = false;
				ImGui::CloseCurrentPopup();
			}

			if( ImGui::Button( "Cancel" ) )
			{
				m_ShowUnsavedChanges = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}
	}

	void NodeEditor::TryDrawCompileErrorModal()
	{
		if( m_ShowErrorPopup && HasUserAuthority( NodeEditorUserAuthority::Editing ) )
			ImGui::OpenPopup( "PreCompile error" );

		ImGui::SetNextWindowPos( ImGui::GetWindowViewport()->GetCenter(), ImGuiCond_FirstUseEver );
		if( ImGui::BeginPopupModal( "PreCompile error", nullptr, ImGuiWindowFlags_NoSavedSettings ) )
		{
			ImGui::Text( "There are errors in the NodeEditor" );
			ImGui::Text( "Would you like to fix them before closing?" );

			ImGui::BeginHorizontal( "##DirtyModalOpt" );

			if( ImGui::Button( "Yes" ) )
			{
				m_ShowErrorPopup = false;
				ImGui::CloseCurrentPopup();
			}

			if( ImGui::Button( "No" ) )
			{
				m_WindowOpen = true;
				m_ShowErrorPopup = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndHorizontal();

			ImGui::EndPopup();
		}
	}

	void NodeEditor::DrawTopBarChildInternal()
	{
		ImGui::PushStyleColor( ImGuiCol_ChildBg, ImVec4( 0.0f, 0.0f, 0.0f, 0.0f ) );

		ImGui::BeginChild( "Topbar", ImVec2( 0.0f, 30.0f ), 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar );
		ImGui::BeginHorizontal( "##TopbarItems" );

		if( Auxiliary::ImageButton( m_ZoomTexture, { 24, 24 } ) )
			ed::NavigateToContent( 0.25f );

		if( ImGui::IsItemHovered() )
		{
			ImGui::BeginTooltip();

			ImGui::Text( "Zoom in and find the content." );

			ImGui::EndTooltip();
		}

		Auxiliary::DisabledFlag disabled( !HasUserAuthority( NodeEditorUserAuthority::Evaluation ) );

		if( Auxiliary::ImageButton( m_CompileTexture, { 24.0f, 24.0f } ) )
		{
			EdEvaluateEditor();
		}

		if( ImGui::IsItemHovered() )
		{
			ImGui::BeginTooltip();

			ImGui::Text( "Compile and evaluate the node editor." );

			ImGui::EndTooltip();
		}

		disabled.Pop();

		OnTopBarRender();

		if( m_TopbarItemsFunction )
			m_TopbarItemsFunction();

		ImGui::EndHorizontal();
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

	void NodeEditor::HandleCreate()
	{
#if !defined(SAT_DIST)
		if( !m_CreateNewNode && HasUserAuthority( NodeEditorUserAuthority::Editing ) )
		{
			if( ed::BeginCreate( ImColor( 255, 255, 255 ), 2.0f ) )
			{
				auto showLabel = []( const char* label, ImColor color )
				{
					ImGui::SetCursorPosY( ImGui::GetCursorPosY() - ImGui::GetTextLineHeight() );
					auto size = ImGui::CalcTextSize( label );

					auto padding = ImGui::GetStyle().FramePadding;
					auto spacing = ImGui::GetStyle().ItemSpacing;

					ImGui::SetCursorPos( ImGui::GetCursorPos() + ImVec2( spacing.x, -spacing.y ) );

					auto rectMin = ImGui::GetCursorScreenPos() - padding;
					auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

					auto drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled( rectMin, rectMax, color, size.y * 0.15f );
					ImGui::TextUnformatted( label );
				};

				ed::PinId StartPinId = 0;
				ed::PinId EndPinId = 0;

				if( ed::QueryNewLink( &StartPinId, &EndPinId ) )
				{
					Ref<Pin> StartPin = nullptr;
					Ref<Pin> EndPin = nullptr;

					StartPin = FindPin( UUID( StartPinId.Get() ) );
					EndPin = FindPin( UUID( EndPinId.Get() ) );

					m_NewLinkPin = StartPin ? StartPin : EndPin;

					// If we started from an input swap the start to the output side
					if( StartPin->Kind == PinKind::Input )
					{
						std::swap( StartPin, EndPin );
						std::swap( StartPinId, EndPinId );
					}

					if( StartPin && EndPin )
					{
						// Pin is the same, reject.
						if( EndPin == StartPin || EndPin->Node == StartPin->Node )
						{
							showLabel( "x Cannot link to self!", ImColor( 45, 32, 32, 180 ) );
							ed::RejectNewItem( ImColor( 225, 0, 0 ), 2.0f );
						}
						else if( EndPin->Kind == StartPin->Kind )  // Same kind, input/output into input/output.
						{
							showLabel( "x Incompatible Pin Kind, input/output into input/output", ImColor( 45, 32, 32, 180 ) );

							ed::RejectNewItem( ImColor( 225, 0, 0 ), 2.0f );
						}
						else if( EndPin->Type != StartPin->Type )
						{
							showLabel( "x Incompatible Pin Type!", ImColor( 45, 32, 32, 180 ) );

							ed::RejectNewItem( ImColor( 225, 128, 128 ), 2.0f );
						}
						else // Valid type, accept (create new link)
						{
							bool shouldDelete = false;

							if( IsLinked( EndPin->ID ) && !EndPin->IsFlagSet( PinFlag_AcceptMultipleLinks ) )
							{
								showLabel( "+ Replace old link with current link", ImColor( 32, 45, 32, 180 ) );
								shouldDelete = true;
							}
							else
							{
								showLabel( "+ Create Link", ImColor( 32, 45, 32, 180 ) );
							}

							if( ed::AcceptNewItem( ImColor( 128, 255, 128 ), 4.0f ) )
							{
								if( shouldDelete )
								{
									if( IsStateFlagSet( NodeEditorState_Simulating ) )
									{
										ed::StopFlow();
									}

									ed::BreakLinks( EndPinId );
									DeleteLink( FindLinkByPin( EndPin->ID )->ID );

									OnNodeEditorEvent( NodeEditorAction::BreakLink );
								}

								UUID start = UUID( StartPinId.Get() );
								UUID end = UUID( EndPinId.Get() );

								m_Links.push_back( Ref<Link>::Create( UUID(), start, end, StartPin->GetPinColor() ) );

								MarkDirty();

								Ref<UndoRedoActionCreateLink> action = Ref<UndoRedoActionCreateLink>::Create( SharedFromThis(), m_Links.back() );
								GlobalUndoRedoGroup::Get()->AddAction( action, m_AssetID );
								OnNodeEditorEvent( NodeEditorAction::CreateLink );
							}
						}
					}
				}

				// If the link is not connected, user maybe want to create a new node rather than link it.
				ed::PinId id = 0;
				if( ed::QueryNewNode( &id ) )
				{
					m_NewLinkPin = FindPin( UUID( id.Get() ) );

					// Show popup.
					if( m_NewLinkPin )
						showLabel( "+ Create Node", ImColor( 32, 45, 32, 180 ) );

					if( ed::AcceptNewItem() )
					{
						m_CreateNewNode = true;

						m_NewNodeLinkPin = FindPin( UUID( id.Get() ) );
						m_NewLinkPin = nullptr;

						ed::Suspend();
						ImGui::OpenPopup( "Create New Node" );
						ed::Resume();
					}
				}
			}
			else
				m_NewLinkPin = nullptr;

			ed::EndCreate();

			if( ed::BeginDelete() )
			{
				ed::LinkId linkId = 0;
				while( ed::QueryDeletedLink( &linkId ) )
				{
					if( ed::AcceptDeletedItem() )
					{
						if( IsStateFlagSet( NodeEditorState_Simulating ) )
						{
							ed::StopFlow();
						}

						DeleteLink( linkId.Get() );
						OnNodeEditorEvent( NodeEditorAction::BreakLink );

						MarkDirty();
					}
				}

				// If the user is deleting a node from the editor handle it here.
				ed::NodeId nodeId = 0;
				while( ed::QueryDeletedNode( &nodeId ) )
				{
					UUID id = nodeId.Get();

					const auto itr = std::find_if( m_Nodes.begin(), m_Nodes.end(),
						[ id ]( const auto& kv )
					{
						return kv.first == id;
					} );

					if( itr != m_Nodes.end() )
					{
						auto& rNode = ( itr->second );

						if( !rNode->IsFlagSet( NodeFlags_Irremovable ) )
						{
							if( ed::AcceptDeletedItem() )
							{
								if( IsStateFlagSet( NodeEditorState_Simulating ) )
								{
									ed::StopFlow();
								}

								// TODO: Not the best way, 
								// because DeleteNode finds the node and checks if it can be deleted, we've already done that...
								DeleteNode( id );
								MarkDirty();
							}
						}
						else
						{
							ed::RejectDeletedItem();
						}
					}
				}
			}
			ed::EndDelete();
		}
#endif
	}

	void NodeEditor::HandleStateCanvasBorders()
	{
		if( IsStateFlagSet( NodeEditorState_Debugging ) )
		{
			DrawDebuggingCanvas();
			return;
		}

		if( IsStateFlagSet( NodeEditorState_Simulating ) )
		{
			DrawSimulatingCanvas();
			return;
		}
	}

	void NodeEditor::EdEvaluateEditor()
	{
		if( !HasUserAuthority( NodeEditorUserAuthority::Editing ) )
			return;

		m_OutputWindow.ClearOutput();

		if( !m_PreCompiler )
			m_PreCompiler = Ref<NodeEditorDefaultPreCompiler>::Create( SharedFromThis() );

		OnNodeEditorEvent( NodeEditorAction::PreEvaluate );

		const auto& result = m_PreCompiler->PreCompile();
		m_HasPreCompileErrors = !result.Succeeded;

		OnNodeEditorEvent( m_HasPreCompileErrors ? NodeEditorAction::PostEvaluateFailed : NodeEditorAction::PostEvaluateSuccess );

		for( const auto& rMessage : result.Messages )
		{
			const NodeEditorMessageSeverity severity = ( ( rMessage.Category & NodeEdPreCompCategory_Warning ) == 0 ) ? NodeEditorMessageSeverity::Error : NodeEditorMessageSeverity::Warning;

			m_OutputWindow.PushMessage( { .MessageText = Auxiliary::NodeEditorPreCompResultToString( rMessage ), .Type = severity } );
		}
	}

	void NodeEditor::DrawDetailsWindow()
	{
		// Extra information window
		if( ImGui::Begin( "Details", &m_ShowDetailsInformation ) )
		{
			Auxiliary::ScopedDisabledFlag disabled( !HasUserAuthority( NodeEditorUserAuthority::Editing ) );

			OnExtraRender();
		}

		ImGui::End();
	}

	void NodeEditor::DrawDataWindow()
	{
		if( ImGui::Begin( "Data", &m_ShowDataWindow ) )
		{
			Auxiliary::ScopedDisabledFlag disabled( !HasUserAuthority( NodeEditorUserAuthority::Editing ) );

			if( Auxiliary::TreeNode( "Variables" ) )
			{
				for( auto& [id, rVariable] : m_EditorVariables )
				{
					ImGui::BeginHorizontal( ( int ) id );
					if( Auxiliary::InputText( "##editname", &rVariable->m_Name ) )
					{
						MarkDirty();
					}

					ImGui::Spring();
					ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical, 1.0f );
					ImGui::Spring();

					const std::string currentType = NodeEditorVariableDataTypeToString( rVariable->GetType() );
					const std::string dataTypeID = std::format( "##DataType/{0}", ( uint64_t ) id );
					ImGui::SetNextItemWidth( 164.0f );
					if( ImGui::BeginCombo( dataTypeID.c_str(), currentType.c_str() ) )
					{
						for( size_t i = 0; i < std::underlying_type_t<NodeEditorVariableDataType>( NodeEditorVariableDataType::Unknown ); ++i )
						{
							const std::string itemName = NodeEditorVariableDataTypeToString( ( NodeEditorVariableDataType ) i );
							if( ImGui::Selectable( itemName.c_str() ) )
							{
								rVariable->m_DataType = ( NodeEditorVariableDataType ) i;
								MarkDirty();
							}
						}

						ImGui::EndCombo();
					}

					ImGui::Spring();

					if( ImGui::SmallButton( "-" ) )
					{
						m_EditorVariables.erase( id );
						MarkDirty();

						ImGui::EndHorizontal();
						break;
					}

					ImGui::EndHorizontal();

					// No new line!
					if( rVariable->m_Name.empty() )
					{
						const char* pText = "The variable name cannot be empty!";

						const ImVec2 padding = ImGui::GetStyle().FramePadding;
						const ImVec2 textPosition = ImGui::GetCursorScreenPos();
						const ImVec2 textSize = ImGui::CalcTextSize( pText );

						const ImVec2 min = ImVec2( textPosition.x - padding.x, textPosition.y - padding.y );
						const ImVec2 max = ImVec2( textPosition.x + padding.x + textSize.x, textPosition.y + padding.y + textSize.y );

						ImGui::GetWindowDrawList()->AddRectFilled( min, max, IM_COL32( 200, 30, 60, 255 ), 2.0f, ImDrawFlags_RoundCornersAll );

						ImGui::TextUnformatted( pText );
					}
				}

				if( ImGui::SmallButton( "+" ) )
				{
					Ref<NodeEditorVariable> var = Ref<NodeEditorVariable>::Create( NodeEditorVariableDataType::Unknown );

					std::string name = "NewVariable";

					const auto count = std::count_if( m_EditorVariables.begin(), m_EditorVariables.end(),
						[ name ]( const auto& rCandidate )
					{
						return rCandidate.second->m_Name.contains( name );
					} );

					if( count >= 1 )
						name += std::to_string( count );

					var->m_Name = name;
					m_EditorVariables[ var->GetUUID() ] = var;
					MarkDirty();
				}

				Auxiliary::EndTreeNode();
			}
		}

		ImGui::End();
	}

	void NodeEditor::DrawDebugWindow()
	{
		if( ImGui::Begin( "Debug Information", &m_ShowDebugInformation ) )
		{
			if( Auxiliary::TreeNode( "Editor Information" ) )
			{
				ImGui::Text( "Name: %s", m_Name.c_str() );
				ImGui::Text( "Custom Name (NC): %s", m_CustomNameNC.c_str() );
				ImGui::Text( "Internal Editor ID: %s", m_InternalEditorID.c_str() );

				ImGui::Text( "Internal Node Count (Live): %i", ed::GetNodeCount() );

				Auxiliary::EndTreeNode();
			}

			if( Auxiliary::TreeNode( "Nodes" ) )
			{
				ImGui::Text( "Node Count %" PRIu64, m_Nodes.size() );
				ImGui::Separator();

				for( const auto& [id, rNode] : m_Nodes )
				{
					ImGui::PushID( ( int ) id );

					ImGui::Text( "%s", rNode->Name.c_str() );
					ImGui::Text( "ID/%" PRIu64, id );
					ImGui::Text( "Parent Object Name (if any) %s", rNode->pParentObject ? rNode->pParentObject->Name.c_str() : "<null>" );
					ImGui::Text( "SClass: %s", rNode->GetClass()->GetName().c_str() );

					if( Auxiliary::TreeNode( "Pins", false ) )
					{
						auto drawPinFlagText = []( uint8_t flags )
						{
							ImGui::Text( "Flags:" );

							if( ( flags & PinFlag_DefaultSet ) != 0 )
							{
								ImGui::BulletText( "Default Flags" );
							}

							if( ( flags & PinFlag_AcceptMultipleLinks ) != 0 )
							{
								ImGui::BulletText( "Accepts Multiple Links" );
							}

							if( ( flags & PinFlag_RequiredForEvaluation ) != 0 )
							{
								ImGui::BulletText( "Required For Evaluation" );
							}
						};

						if( ImGui::TreeNode( "Outputs" ) )
						{
							for( const auto& rOutput : rNode->Outputs )
							{
								ImGui::Text( "%s", rOutput->Name.c_str() );
								ImGui::Text( "ID/%" PRIu64, rOutput->ID );

								if( IsLinked( rOutput->ID ) )
								{
									ImGui::TextColored( ImVec4{ 0.0F, 1.0F, 0.0, 1.0F }, "Linked" );
								}
								else
								{
									ImGui::TextColored( ImVec4{ 1.0F, 0.0F, 0.0, 1.0F }, "Not Linked" );
								}

								drawPinFlagText( rOutput->PinFlags );

								ImGui::Separator();
							}

							Auxiliary::EndTreeNode();
							ImGui::Separator();
						}

						if( ImGui::TreeNode( "Inputs" ) )
						{
							for( const auto& rInput : rNode->Inputs )
							{
								ImGui::Text( "%s", rInput->Name.c_str() );
								ImGui::Text( "ID/%" PRIu64, rInput->ID );

								if( IsLinked( rInput->ID ) )
								{
									ImGui::TextColored( ImVec4{ 0.0F, 1.0F, 0.0, 1.0F }, "Linked" );
								}
								else
								{
									ImGui::TextColored( ImVec4{ 1.0F, 0.0F, 0.0, 1.0F }, "Not Linked" );
								}

								drawPinFlagText( rInput->PinFlags );

								ImGui::Separator();
							}

							Auxiliary::EndTreeNode();
							ImGui::Separator();
						}

						Auxiliary::EndTreeNode();
					}

					ImGui::PopID();
					ImGui::Separator();
				}

				if( Auxiliary::TreeNode( "Extra extra information (may be slow)", false ) )
				{
					uint64_t numberOfUnlinkedInputs = 0;
					for( const auto& [id, rNode] : m_Nodes )
					{
						for( const auto& rInput : rNode->Inputs )
						{
							if( !IsLinked( rInput->ID ) )
							{
								ImGui::Text( "Node %s (%" PRIu64 ") pin ID %" PRIu64 "is not linked.", rNode->Name.c_str(), id, rInput->ID );
								++numberOfUnlinkedInputs;
							}
						}
					}

					ImGui::Text( "Number of unlinked inputs %" PRIu64, numberOfUnlinkedInputs );

					Auxiliary::EndTreeNode();
				}

				Auxiliary::EndTreeNode();
			}
		}

		ImGui::End();
	}

	void NodeEditor::DrawBeginSearchWindow()
	{
		if( ImGui::Begin( "Find##NE_SEARCH", &m_IsSearching, ImGuiWindowFlags_NoSavedSettings ) )
		{
			if( ImGui::IsWindowAppearing() )
			{
				ImGui::SetKeyboardFocusHere( 0 );
			}

			m_SearchCacher.Filter.Draw( "##search" );

			if( ImGui::Button( "Go" ) )
			{
				m_SearchCacher.NodeNames.reserve( m_Nodes.size() );

				for( const auto& [ID, rNode] : m_Nodes )
				{
					m_SearchCacher.NodeNames.emplace_back( rNode->Name );
				}

				m_SearchCacher.FilterNames();

				m_ShowSearchResultsWindow = true;
				m_IsSearching = false;

				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();

			if( ImGui::Button( "Close" ) )
			{
				m_IsSearching = false;
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::End();
	}

	void NodeEditor::DrawSearchResultsWindow()
	{
		if( ImGui::Begin( "Search Results", &m_ShowSearchResultsWindow ) )
		{
			for( const auto& rNodeName : m_SearchCacher.PassedNodeNames )
			{
				if( ImGui::Selectable( rNodeName.c_str() ) )
				{
					const auto node = FindNode( rNodeName );
					if( node )
					{
						ed::SelectNode( ed::NodeId( node->ID ) );
						ed::NavigateToSelection();
					}
				}
			}
		}

		if( !m_ShowSearchResultsWindow )
		{
			m_SearchCacher.Clear();
		}

		ImGui::End();
	}

	void NodeEditor::OnKeyPressed( RubyKeyEvent& rKeyEvent )
	{
		if( Input::Get().KeyPressed( RubyKey_LeftCtrl ) || Input::Get().KeyPressed( RubyKey_RightCtrl ) )
		{
			switch( rKeyEvent.GetKeycode() )
			{
				case RubyKey_C:
				{
					const auto nodes = GetSelectedNodes();

					m_CopyPasteNodeClasses.clear();
					m_CopyPasteNodeClasses.reserve( nodes.size() );

					for( const auto& rNodeID : nodes )
					{
						const auto node = FindNode( rNodeID );
						SAT_CORE_ASSERT( node );

						// Ignore any nodes that are not meant to be pasted.
						if( node->IsFlagSet( NodeFlags_RejectCopyPaste ) )
							continue;

						auto& rInfo = m_CopyPasteNodeClasses.emplace_back( node );
						CopyPasteBuildChildrenList( node, rInfo );
					}
				} break;

				case RubyKey_V:
				{
					auto& rIO = ImGui::GetIO();
					const auto canvasMousePos = ed::ScreenToCanvas( rIO.MousePos );

					for( const auto& rCopyInfo : m_CopyPasteNodeClasses )
					{
						if( const auto newlyCreatedNode = CopyPaste_CopyNode( rCopyInfo ) ) 
						{
							ed::SetNodePosition( ed::NodeId( newlyCreatedNode->ID ), canvasMousePos );
						}
					}

					// Don't clear out list because we may want to paste the same set of nodes again.
//					m_CopyPasteNodeClasses.clear();
				} break;

				default:
					break;
			}
		}
	}

	void NodeEditor::OnChooseNewNode( SharedPtr<NodeEditorNodeBase> node )
	{
		BuildNode( node );

		m_CreateNewNode = false;

		//		Ref<UndoRedoActionCreateNode> action = Ref<UndoRedoActionCreateNode>::Create( SharedFromThis(), node );
		//		GlobalUndoRedoGroup::Get()->AddAction( action, m_AssetID );

		if( m_AcceptedNewLink )
		{
			if( auto& startPin = m_NewNodeLinkPin )
			{
				auto& pins = startPin->Kind == PinKind::Input ? node->Outputs : node->Inputs;

				for( auto& pin : pins )
				{
					if( CanCreateLink( startPin, pin ) )
					{
						auto& endPin = pin;

						UUID startID = startPin->ID;
						UUID endID = endPin->ID;

						// Start of the link must be the output pin
						if( startPin->Kind == PinKind::Input )
							std::swap( startID, endID );

						m_Links.push_back( Ref<Link>::Create( UUID(), startID, endID, startPin->GetPinColor() ) );

						break;
					}
				}
			}
		}
		else
		{
			// Reset back to default state
			m_AcceptedNewLink = true;
		}

		MarkDirty();
		OnNodeEditorEvent( NodeEditorAction::CreateNode );
	}

	std::vector<UUID> NodeEditor::GetSelectedNodes()
	{
		const auto maxSize = m_Nodes.size();

		std::vector<ed::NodeId> temporary( maxSize );

		const int selected = ed::GetSelectedNodes( temporary.data(), static_cast< int >( m_Nodes.size() ) );

		// Shrink to selected size
		temporary.resize( selected );

		std::vector<UUID> result;
		result.reserve( temporary.size() );

		for( const auto& id : temporary )
			result.emplace_back( id.Get() );

		return result;
	}

	//////////////////////////////////////////////////////////////////////////
	// NODE EDITOR BASE

	void NodeEditor::SaveSettings()
	{
		NodeCacheSettings::WriteEditorSettings( SharedFromThis() );
	}

	bool NodeEditor::IsLinked( UUID pinID )
	{
		if( !pinID )
			return false;

		const auto Itr = std::find_if( m_Links.begin(), m_Links.end(),
			[ pinID ]( const auto& rLink )
		{
			return rLink->StartPinID == pinID || rLink->EndPinID == pinID;
		} );

		if( Itr != m_Links.end() )
			return true;

		return false;
	}

	Ref<Pin> NodeEditor::FindPin( UUID id )
	{
		if( !id )
			return nullptr;

		for( const auto& [nodeId, rNode] : m_Nodes )
		{
			for( const auto& pin : rNode->Inputs )
			{
				if( pin->ID == id )
				{
					return pin;
				}
			}

			for( const auto& pin : rNode->Outputs )
			{
				if( pin->ID == id )
				{
					return pin;
				}
			}
		}

		return nullptr;
	}

	Ref<Link> NodeEditor::FindLink( UUID id )
	{
		const auto Itr = std::find_if( m_Links.begin(), m_Links.end(),
			[ id ]( const auto& rLink )
		{
			return rLink->ID == id;
		} );

		if( Itr != m_Links.end() )
			return *Itr;

		return nullptr;
	}

	SharedPtr<NodeEditorNodeBase> NodeEditor::FindNode( UUID id )
	{
		const auto Itr = m_Nodes.find( id );

		if( Itr != m_Nodes.end() )
			return Itr->second;

		return nullptr;
	}

	SharedPtr<NodeEditorNodeBase> NodeEditor::FindNode( const std::string& rName )
	{
		for( auto& [id, node] : m_Nodes )
		{
			if( node->Name == rName )
				return node;
		}

		return nullptr;
	}

	Ref<Link> NodeEditor::FindLinkByPin( UUID id )
	{
		if( id == 0 )
			return nullptr;

		if( !IsLinked( id ) )
			return nullptr;

		const auto Itr = std::find_if( m_Links.begin(), m_Links.end(),
			[ id ]( const auto& rLink )
		{
			return rLink->StartPinID == id || rLink->EndPinID == id;
		} );

		if( Itr != m_Links.end() )
			return *Itr;

		return nullptr;
	}

	SharedPtr<NodeEditorNodeBase> NodeEditor::FindNodeByPin( UUID id )
	{
		if( auto rPin = FindPin( id ) )
			return rPin->Node;

		return nullptr;
	}

	std::vector<Ref<Link>> NodeEditor::FindLinksByPin( UUID id )
	{
		std::vector<Ref<Link>> result;

		if( id == 0 )
			return result;

		for( const auto& rLink : m_Links )
		{
			if( rLink->StartPinID == id || rLink->EndPinID == id )
			{
				result.push_back( rLink );
			}
		}

		return result;
	}

	std::vector<UUID> NodeEditor::FindNeighborsViaInputs( SharedPtr<NodeEditorNodeBase> node )
	{
		std::vector<UUID> ids;

		for( const auto& rInput : node->Inputs )
		{
			if( !IsLinked( rInput->ID ) )
				continue;

			// If the pin is linked find the other end of it and add it to our list.
			const auto links = FindLinksByPin( rInput->ID );
			for( const auto& rLink : links )
			{
				const bool isStart = rLink->StartPinID == rInput->ID;
				SharedPtr<NodeEditorNodeBase> otherNode = FindNodeByPin( isStart ? rLink->EndPinID : rLink->StartPinID );

				ids.push_back( otherNode->ID );
			}
		}

		return ids;
	}

	std::vector<UUID> NodeEditor::FindNeighborsViaOutputs( SharedPtr<NodeEditorNodeBase> node )
	{
		std::vector<UUID> ids;

		for( const auto& rOutput : node->Outputs )
		{
			if( !IsLinked( rOutput->ID ) )
				continue;

			// If the pin is linked find the other end of it and add it to our list.
			const auto links = FindLinksByPin( rOutput->ID );

			for( const auto& rLink : links )
			{
				const bool isStart = rLink->StartPinID == rOutput->ID;
				SharedPtr<NodeEditorNodeBase> otherNode = FindNodeByPin( isStart ? rLink->EndPinID : rLink->StartPinID );

				ids.push_back( otherNode->ID );
			}
		}

		return ids;
	}

	std::vector<UUID> NodeEditor::FindNeighborsInput( SharedPtr<NodeEditorNodeBase> node, size_t index )
	{
		std::vector<UUID> ids;

		if( index >= node->Inputs.size() )
			return ids;

		const auto& rPin = node->Inputs[ index ];
		if( IsLinked( rPin->ID ) )
		{
			// If the pin is linked find the other end of it and add it to our list.
			const auto links = FindLinksByPin( rPin->ID );

			for( const auto& rLink : links )
			{
				const bool isStart = rLink->StartPinID == rPin->ID;
				SharedPtr<NodeEditorNodeBase> otherNode = FindNodeByPin( isStart ? rLink->EndPinID : rLink->StartPinID );

				ids.push_back( otherNode->ID );
			}
		}

		return ids;
	}

	std::vector<UUID> NodeEditor::FindNeighborsOutput( SharedPtr<NodeEditorNodeBase> node, size_t index )
	{
		std::vector<UUID> ids;

		if( index >= node->Outputs.size() )
			return ids;

		const auto& rPin = node->Outputs[ index ];
		if( IsLinked( rPin->ID ) )
		{
			// If the pin is linked find the other end of it and add it to our list.
			const auto links = FindLinksByPin( rPin->ID );

			for( const auto& rLink : links )
			{
				const bool isStart = rLink->StartPinID == rPin->ID;
				SharedPtr<NodeEditorNodeBase> otherNode = FindNodeByPin( isStart ? rLink->EndPinID : rLink->StartPinID );

				ids.push_back( otherNode->ID );
			}
		}

		return ids;
	}

	void NodeEditor::CreateLink( const Ref<Pin>& rStart, const Ref<Pin>& rEnd, ImColor color )
	{
		m_Links.push_back( Ref<Link>::Create( UUID(), rStart->ID, rEnd->ID, color ) );
	}

	void NodeEditor::CreateLinkWithID( UUID linkID, const Ref<Pin>& rStart, const Ref<Pin>& rEnd, ImColor color )
	{
		m_Links.push_back( Ref<Link>::Create( linkID, rStart->ID, rEnd->ID, color ) );
	}

	void NodeEditor::ShowFlow()
	{
		for( const auto& rLink : m_Links )
			ed::Flow( ed::LinkId( rLink->ID ) );
	}

	void NodeEditor::ShowFlow( const std::vector<Ref<Link>>& rLinks )
	{
		for( const auto& rLink : rLinks )
			ed::Flow( ed::LinkId( rLink->ID ) );
	}

	void NodeEditor::ShowFlow( const Ref<Link>& rLink )
	{
		ed::Flow( ed::LinkId( rLink->ID ) );
	}

	void NodeEditor::ShowFlow( UUID linkID )
	{
		ed::Flow( ed::LinkId( linkID ) );
	}

	bool NodeEditor::HasUserAuthority( NodeEditorUserAuthority privilege ) const
	{
		return ( m_Privileges & privilege ) == privilege;
	}

	void NodeEditor::SetUserAuthorityFlag( NodeEditorUserAuthority privilege, bool value )
	{
		if( value )
			m_Privileges = m_Privileges | privilege;
		else
			m_Privileges = m_Privileges & ~privilege;
	}

	Ref<NodeEditorVariable> NodeEditor::FindVariable( UUID id ) const
	{
		auto itr = m_EditorVariables.find( id );
		return itr == m_EditorVariables.end() ? nullptr : itr->second;
	}

	Ref<NodeEditorVariable> NodeEditor::FindVariable( const std::string& rName ) const
	{
		for( const auto& [uuid, var] : m_EditorVariables )
		{
			if( var->GetName() == rName )
				return var;
		}

		return nullptr;
	}

	void NodeEditor::AddNode( SharedPtr<NodeEditorNodeBase> node )
	{
		if( IsStateFlagSet( NodeEditorState_Loading ) )
			return;

		m_Nodes[ node->ID ] = node;

		BuildNode( node );

#if !defined(SAT_DIST)
		VariableGuard<ed::EditorContext*, ed::EditorContext*> guard( m_Editor );
		node->PositionBeforeMove = ed::GetNodePosition( ed::NodeId( node->ID ) );

		// TODO: Currently no way for us to preemptively set a position of a node before the first frame is drawn.
//		if( node->Position.x != 0.0f && node->Position.y != 0.0f )
//			ed::SetNodePosition( ed::NodeId( node->ID ), node->Position );
#endif

//		node->pOuter = this;
	}

	//////////////////////////////////////////////////////////////////////////
	// SERIALISATION (DEBUG AND RELEASE, ON DIST NODE EDITORS DO NOT EXIST)

	void NodeEditor::SerialiseData( std::ofstream& rStream )
	{
		RawSerialisation::WriteString( m_Name, rStream );

		size_t mapSize = m_EditorVariables.size();
		RawSerialisation::WriteObject( mapSize, rStream );

		for( const auto& [id, rHandle] : m_EditorVariables )
		{
			RawSerialisation::WriteObjectChecked( id, rStream );
			NodeEditorVariable::Serialise( rHandle, rStream );
		}

		mapSize = m_Nodes.size();
		rStream.write( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

		for( const auto& [key, value] : m_Nodes )
		{
			RawSerialisation::WriteUUID( key, rStream );

			RawSerialisation::WriteObject( value->GetClass()->GetHash(), rStream );

			value->Serialise( rStream );

			if( value->pParentObject )
				RawSerialisation::WriteObjectChecked( value->pParentObject->ID, rStream );
			else
				RawSerialisation::WriteObject( 0llu, rStream );
		}

		mapSize = m_Links.size();
		RawSerialisation::WriteObject( mapSize, rStream );

		for( auto& rLinks : m_Links )
		{
			Link::Serialise( rLinks, rStream );
		}
	}

	void NodeEditor::DeserialiseData( std::ifstream& rStream )
	{
#if !defined(SAT_DIST)
		m_State = NodeEditorState_Loading;

		// NOTE: using the "this" keyword is fine here, 
		//		 ReadEditorSettings takes in a raw ptr
		NodeCacheSettings::ReadEditorSettings( this );

		m_Name = RawSerialisation::ReadString( rStream );

		CreateNewEditorIfNeeded();

		size_t mapSize = 0;
		RawSerialisation::ReadObject( mapSize, rStream );

		m_EditorVariables.reserve( mapSize );

		for( size_t i = 0; i < mapSize; ++i )
		{
			UUID id = 0llu;
			RawSerialisation::ReadObjectChecked( id, rStream );

			Ref<NodeEditorVariable> var = Ref<NodeEditorVariable>::Create();
			NodeEditorVariable::Deserialise( var, rStream );

			m_EditorVariables[ id ] = var;
		}

		mapSize = 0;
		RawSerialisation::ReadObject( mapSize, rStream );

		std::unordered_map<UUID, std::vector<UUID>> parentToChildMap;

		for( size_t i = 0; i < mapSize; ++i )
		{
			UUID key = 0;
			RawSerialisation::ReadUUID( key, rStream );

			uint64_t targetClassHash = 0;
			RawSerialisation::ReadObject( targetClassHash, rStream );

			NodeEditorNodeBase* pNode = dynamic_cast< NodeEditorNodeBase* >( ClassMetadataHandler::Get().CreateClassObject( targetClassHash, this ) );

			SharedPtr<NodeEditorNodeBase> node;
			if( pNode )
			{
				node = SharedPtr<NodeEditorNodeBase>( pNode );
			}
			else
			{
				node = SharedPtr<NodeEditorNodeBase>( NewObject<NodeEditorBlueprintNode>( this ) );
				SAT_CORE_WARN( "Could not find node editor node class hash {0}, so using NodeEditorBlueprintNode instead.", targetClassHash );
			}

			BuildNode( node );

			node->Deserialise( rStream );
			node->PositionBeforeMove = ed::GetNodePosition( ed::NodeId( node->ID ) );

			UUID parentID = 0;
			if( m_Version >= NodeEditorVersion::Subgraphs )
			{
				RawSerialisation::ReadObjectChecked( parentID, rStream );
			}

			parentToChildMap[ parentID ].push_back( key );

			m_Nodes[ key ] = node;
		}

		// Sub-graph parent
		for( const auto& [parentID, children] : parentToChildMap )
		{
			for( const auto& rChild : children )
			{
				m_Nodes[ rChild ]->pParentObject = parentID == 0 ? nullptr : m_Nodes[ parentID ].Get();
			}
		}

		mapSize = 0;
		RawSerialisation::ReadObject( mapSize, rStream );

		m_Links.resize( mapSize );

		for( size_t i = 0; i < mapSize; ++i )
		{
			Ref<Link> link = Ref<Link>::Create();

			Link::Deserialise( link, rStream );

			m_Links[ i ] = link;
		}

		m_State = NodeEditorState_Editing;

		OnNodeEditorEvent( NodeEditorAction::PostLoad );
#endif
	}

	//////////////////////////////////////////////////////////////////////////
	// NodeEditorSearchCacher

	void NodeEditorSearchCacher::FilterNames()
	{
		// TODO: Yea, this could be slow, we could either find a new way or 
		// use the job system for this.

		// Assume we'll eliminate half of the nodes...
		PassedNodeNames.reserve( NodeNames.size() / 2 );

		for( const auto& rName : NodeNames )
		{
			if( Filter.PassFilter( rName.data() ) )
			{
				PassedNodeNames.push_back( rName );
			}
		}
	}

	void NodeEditorSearchCacher::Clear()
	{
		PassedNodeNames.clear();
		NodeNames.clear();
		Filter.Clear();
	}

}
