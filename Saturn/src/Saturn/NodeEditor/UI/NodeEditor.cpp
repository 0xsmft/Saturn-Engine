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
#include "NodeEditor.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/ImGui/ImGuiAuxiliary.h"
#include "Saturn/ImGui/EditorIcons.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include "Saturn/NodeEditor/Runtime/NodeEditorRuntime.h"
#include "Saturn/NodeEditor/Serialisation/NodeCache.h"
#include "Saturn/NodeEditor/NodeEditorBlueprintNode.h"
#include "Saturn/NodeEditor/UndoRedo/UndoRedoNodeEditorActions.h"

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
			const auto texture = Ref<Texture2D>::Create( "content/textures/editor/BlueprintBackground.png", AddressingMode::Repeat, false );

			EditorIcons::AddIcon( texture );

			ImGui_ImplVulkan_AddTexture( texture->GetSampler(), texture->GetImageView(), texture->GetDescriptorInfo().imageLayout );

			return texture;
		}

		return icon;
	}

	//////////////////////////////////////////////////////////////////////////
	// NODE EDITOR

	NodeEditor::NodeEditor( AssetID ID )
		: NodeEditorBase( ID ), m_OutputWindow( ID )
	{
		m_AssetID = ID;

		CreateEditor();
	}

	NodeEditor::NodeEditor()
		: NodeEditorBase(), m_OutputWindow( 0llu )
	{
		m_Editor = nullptr;

		SetPrivileges( NodeEditorUserAuthority::Full, true );
	}

	NodeEditor::~NodeEditor()
	{
		GlobalUndoRedoGroup::Get()->RemoveIfActionHasIdentifier( m_AssetID );

		m_ZoomTexture = nullptr;
		m_CompileTexture = nullptr;
		
		Close();
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

#if !defined(SAT_DIST)
			pNode->ActiveState.assign( pData, size );
#endif

			// Only mark dirty if we are not loading
			// imgui_node_editor will call this function when initialising
			if( pThis->GetState() != NodeEditorState::Loading )
			{
				pThis->MarkDirty();
			}

			return true;
		};

		m_Editor = ed::CreateEditor( &config );
		ed::SetCurrentEditor( m_Editor );

		m_ZoomTexture = EditorIcons::GetIcon( "Inspect" );
		m_CompileTexture = EditorIcons::GetIcon( "NoIcon" );
		
		const auto texture = GetBlueprintBackground();
		m_Builder = util::BlueprintNodeBuilder( ( ImTextureID ) texture->GetDescriptorSet(), texture->Width(), texture->Height() );

		m_OutputWindow.PushMessage( { .MessageText = "Initialised new editor!", .Type = NodeEditorMessageSeverity::Info } );

		m_InternalEditorID = std::format( "Nc##{0}", (uint64_t)m_AssetID );
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

				ImGui::EndMenu();
			}

			if( ImGui::BeginMenu( "Window" ) )
			{
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
			if( HasPrivilege( NodeEditorUserAuthority::Editing ) )
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

			Auxiliary::DisabledFlag disabled( true );

			ImGui::Separator();

			ImGui::Text( "NC/%llu", m_HoveredNode->ID );
			ImGui::Text( "%s", m_HoveredNode->Name.c_str() );

			ImGui::Separator();

			const auto& rPosition = ed::GetNodePosition( ed::NodeId( m_HoveredNode->ID ) );
			const auto& rSize = ed::GetNodeSize( ed::NodeId( m_HoveredNode->ID ) );

			ImGui::Text( "Size X/%f Y/%f", rSize.x, rSize.y );
			ImGui::Text( "Pos X/%f Y/%f", rPosition.x, rPosition.y );

			disabled.Pop();
			ImGui::EndPopup();
		}

		ed::Resume();
		
		ed::End();

		if( m_BreadCrumbsFunction )
			m_BreadCrumbsFunction();

		HandleStateCanvasBorders();

		if( m_OutputWindow.IsOpen() ) m_OutputWindow.Draw();

		ImGui::End(); // NODE_EDITOR

		// Window closed but we are dirty, show unsaved changes modal and keep window open.
		if( !m_WindowOpen && m_Dirty )
		{
			m_WindowOpen = true;
		
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

	void NodeEditor::OnUpdate( Timestep ts )
	{
	}

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
		const auto wasConnectedToTheNode = [&]( const Ref<Link>& link )
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
			correspondingAsset->Version = m_Version;

			// #SaveAssetManagerOnJT
			AssetManager::Get()->Save();
		}
	}

	void NodeEditor::DeleteLink( UUID id, bool skipUndoRedo )
	{
		const auto Itr = std::find_if( m_Links.begin(), m_Links.end(),
			[id]( const auto& rLink )
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
		auto isDescendantOf = [](NodeEditorNodeBase* pTarget, const auto& rNode) -> bool
		{
			if( !pTarget ) return false;

			NodeEditorNodeBase* pCurrentParent = rNode->pParentObject;
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
			if( rNode->CanBeDeleted )
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

		const std::string canvasName = "SIMULATING";
		const ImVec2 textSize = ImGui::CalcTextSize( canvasName.c_str() );
		const float padding = 10.0f;

		const ImVec2 textPos = ImVec2( canvasRect.Min.x + padding, canvasRect.Min.y + padding );
		pDrawList->AddText( textPos, IM_COL32( 255, 255, 255, 255 ), canvasName.c_str() );
	}

	void NodeEditor::TryDrawUnsavedChangesModal()
	{
		if( m_ShowUnsavedChanges && HasPrivilege( NodeEditorUserAuthority::Editing ) )
			ImGui::OpenPopup( "Unsaved Changes" );

		// Unsaved changes modal
		// TODO: Center window with our main window
		ImGui::SetNextWindowPos( ImGui::GetWindowViewport()->GetCenter(), ImGuiCond_FirstUseEver );
		if( ImGui::BeginPopupModal( "Unsaved Changes", &m_ShowUnsavedChanges, ImGuiWindowFlags_NoSavedSettings ) )
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

		Auxiliary::DisabledFlag disabled( !HasPrivilege( NodeEditorUserAuthority::Evaluation ) );

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
		if( !m_CreateNewNode && HasPrivilege( NodeEditorUserAuthority::Editing ) )
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
						if( EndPin == StartPin )
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

							if( IsLinked( EndPin->ID ) && !EndPin->AcceptMultipleLinks )
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
									if( m_State == NodeEditorState::Simulating )
									{
										ed::StopFlow();

										// Terminate simulation
										m_Runtime->TerminateEvaluation();
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
						if( m_State == NodeEditorState::Simulating )
						{
							ed::StopFlow();

							// Terminate simulation
							m_Runtime->TerminateEvaluation();
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

						if( rNode->CanBeDeleted )
						{
							if( ed::AcceptDeletedItem() )
							{
								if( m_State == NodeEditorState::Simulating )
								{
									ed::StopFlow();

									// Terminate simulation
									m_Runtime->TerminateEvaluation();
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
		switch( m_State )
		{
			case NodeEditorState::Loading:
			case NodeEditorState::Editing:
			case NodeEditorState::Evaluating:
			case NodeEditorState::Suspended:
				break;

			case NodeEditorState::Simulating:
			{
				DrawSimulatingCanvas();

				// WARNING: TODO: Should sub-graphs be allowed to have their own runtime,
				//				  or should it be one runtime from the parent that will handle everything including sub-graphs?
				if( m_Runtime )
				{
					m_Runtime->TraceEvaluationPath();
				}
			} break;
		}
	}

	void NodeEditor::EdEvaluateEditor()
	{
		m_OutputWindow.ClearOutput();

		if( m_Runtime )
		{
			OnNodeEditorEvent( NodeEditorAction::PreEvaluate );

			NodeEditorCompilationStatus result = m_Runtime->EvaluateEditor();

			OnNodeEditorEvent( NodeEditorAction::PostEvaluate );

			switch( result )
			{
				case NodeEditorCompilationStatus::Success:
				{
					m_OutputWindow.PushMessage( { .MessageText = "Successfully compiled and evaluated node editor!", .Type = NodeEditorMessageSeverity::Info } );
				} break;

				case NodeEditorCompilationStatus::Failed:
				{
					m_OutputWindow.PushMessage( { .MessageText = "Failed to compile node editor.", .Type = NodeEditorMessageSeverity::Error } );
				} break;
			}
		}
		else
			m_OutputWindow.PushMessage( { .MessageText = "No active compiler was found!", .Type = NodeEditorMessageSeverity::Error } );
	}

	void NodeEditor::DrawDetailsWindow()
	{
		// Extra information window
		if( ImGui::Begin( "Details", &m_ShowDetailsInformation ) )
		{
			Auxiliary::ScopedDisabledFlag disabled( !HasPrivilege( NodeEditorUserAuthority::Editing ) );

			OnExtraRender();
		}

		ImGui::End();
	}

	void NodeEditor::DrawDataWindow()
	{
		if( ImGui::Begin( "Data", &m_ShowDataWindow ) )
		{
			Auxiliary::ScopedDisabledFlag disabled( !HasPrivilege( NodeEditorUserAuthority::Editing ) );

			if( Auxiliary::TreeNode( "Variables" ) )
			{
				for( auto& [id, rVariable] : m_DataHandles )
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
						m_DataHandles.erase( id );
						MarkDirty();

						ImGui::EndHorizontal();
						break;
					}

					ImGui::EndHorizontal();

					// No new line!
					if( rVariable->m_Name.empty() )
					{
						const std::string text = "The variable name cannot be empty!";

						const ImVec2 padding = ImGui::GetStyle().FramePadding;
						const ImVec2 textPosition = ImGui::GetCursorScreenPos();
						const ImVec2 textSize = ImGui::CalcTextSize( text.c_str() );

						const ImVec2 min = ImVec2( textPosition.x - padding.x, textPosition.y - padding.y );
						const ImVec2 max = ImVec2( textPosition.x + padding.x + textSize.x, textPosition.y + padding.y + textSize.y );

						ImGui::GetWindowDrawList()->AddRectFilled( min, max, IM_COL32( 200, 30, 60, 255 ), 2.0f, ImDrawFlags_RoundCornersAll );

						ImGui::TextUnformatted( text.c_str() );
					}
				}

				if( ImGui::SmallButton( "+" ) )
				{
					Ref<NodeEditorVariable> var = Ref<NodeEditorVariable>::Create( NodeEditorVariableDataType::Unknown );

					std::string name = "NewVariable";

					const auto count = std::count_if( m_DataHandles.begin(), m_DataHandles.end(),
						[ name ]( const auto& rCandidate )
					{
						return rCandidate.second->m_Name.contains( name );
					} );

					if( count >= 1 )
						name += std::to_string( count );

					var->m_Name = name;
					m_DataHandles[ var->GetUUID() ] = var;
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
				ImGui::Text( "Node Count %llu", m_Nodes.size() );
				ImGui::Separator();

				for( const auto& [id, rNode] : m_Nodes )
				{
					ImGui::PushID( ( int ) id );

					ImGui::Text( "%s", rNode->Name.c_str() );
					ImGui::Text( "ID/%llu", id );
					ImGui::Text( "Parent Object Name (if any) %s", rNode->pParentObject ? rNode->pParentObject->Name.c_str() : "<null>" );
					ImGui::Text( "SClass: %s", rNode->GetClass()->GetName().c_str() );

					if( Auxiliary::TreeNode( "Pins", false ) )
					{
						if( ImGui::TreeNode( "Outputs" ) )
						{
							for( const auto& rOutput : rNode->Outputs )
							{
								ImGui::Text( "%s", rOutput->Name.c_str() );
								ImGui::Text( "ID/%llu", rOutput->ID );
								ImGui::Text( "Accepts Multiple Links %i", rOutput->AcceptMultipleLinks );
							}

							Auxiliary::EndTreeNode();
							ImGui::Separator();
						}

						if( ImGui::TreeNode( "Inputs" ) )
						{
							for( const auto& rInput : rNode->Inputs )
							{
								ImGui::Text( "%s", rInput->Name.c_str() );
								ImGui::Text( "ID/%llu", rInput->ID );
								ImGui::Text( "Accepts Multiple Links %i", rInput->AcceptMultipleLinks );
							}

							Auxiliary::EndTreeNode();
							ImGui::Separator();
						}

						Auxiliary::EndTreeNode();
					}

					ImGui::PopID();
					ImGui::Separator();
				}

				Auxiliary::EndTreeNode();
			}
		}

		ImGui::End();
	}

#if !defined(SAT_DIST)

	void NodeEditor::OnChooseNewNode( SharedPtr<NodeEditorNodeBase> node )
	{
		BuildNode( node );

		m_CreateNewNode = false;

//		Ref<UndoRedoActionCreateNode> action = Ref<UndoRedoActionCreateNode>::Create( SharedFromThis(), node );
//		GlobalUndoRedoGroup::Get()->AddAction( action, m_AssetID );

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

		MarkDirty();
		OnNodeEditorEvent( NodeEditorAction::CreateNode );
	}

	std::vector<UUID> NodeEditor::GetSelectedNodes()
	{
		auto maxSize = m_Nodes.size();
		
		std::vector<ed::NodeId> temporary( maxSize );

		int selected = ed::GetSelectedNodes( temporary.data(), static_cast<int>( m_Nodes.size() ) );

		// Shrink to selected size
		temporary.resize( selected );

		std::vector<UUID> result;
		result.reserve( temporary.size() );

		for( const auto& id : temporary )
			result.emplace_back( id.Get() );

		return result;
	}

	//////////////////////////////////////////////////////////////////////////
	// SERIALISATION (DEBUG AND RELEASE)

	void NodeEditor::SerialiseData( std::ofstream& rStream, bool isForDist )
	{
		RawSerialisation::WriteString( m_Name, rStream );

		size_t mapSize = m_DataHandles.size();
		RawSerialisation::WriteObject( mapSize, rStream );

		for( const auto& [id, rHandle] : m_DataHandles )
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

			value->Serialise( rStream, isForDist );

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
		m_State = NodeEditorState::Loading;

		// NOTE: using the "this" keyword is fine here, 
		// ReadEditorSettings takes in a raw ptr
		NodeCacheSettings::ReadEditorSettings( this );

		m_Name = RawSerialisation::ReadString( rStream );

		CreateNewEditorIfNeeded();

		size_t mapSize = 0;
		RawSerialisation::ReadObject( mapSize, rStream );

		m_DataHandles.reserve( mapSize );

		for( size_t i = 0; i < mapSize; ++i )
		{
			UUID id = 0llu;
			RawSerialisation::ReadObjectChecked( id, rStream );

			Ref<NodeEditorVariable> var = Ref<NodeEditorVariable>::Create();
			NodeEditorVariable::Deserialise( var, rStream );

			m_DataHandles[ id ] = var;
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
				node = pNode;
			}
			else
			{
				node = NewObject<NodeEditorBlueprintNode>( this );
				SAT_CORE_WARN( "Could not find node editor node class hash {0}, so using NodeEditorBlueprintNode instead.", targetClassHash );
			}

//			AddNode( node );

			// NOTE: Although AddNode sets the pOuter, we want to override it to point to us (a NodeEditor), instead of NodeEditorBase
			node->Deserialise( rStream );
			node->PositionBeforeMove = ed::GetNodePosition( ed::NodeId( node->ID ) );

			UUID parentID = 0;
			if( m_Version >= SAT_VERSION_A_0_2_3 )
			{
				RawSerialisation::ReadObjectChecked( parentID, rStream );
			}

			parentToChildMap[ parentID ].push_back( key );

			m_Nodes[ key ] = node;
			BuildNode( node );
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

		m_State = NodeEditorState::Editing;
	}
#endif

}
