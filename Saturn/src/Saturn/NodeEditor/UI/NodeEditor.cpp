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

#include "Saturn/Core/OptickProfiler.h"

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
		: NodeEditorBase( ID )
	{
		m_AssetID = ID;

		CreateEditor();
	}

	NodeEditor::NodeEditor()
		: NodeEditorBase()
	{
		m_Editor = nullptr;

		SetPrivileges( NodeEditorUserAuthority::Full, true );
	}

	NodeEditor::~NodeEditor()
	{
		GlobalUndoRedoGroup::Get().RemoveIfActionHasIdentifier( m_AssetID );

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

				Ref<UndoRedoActionModifyNodePosition> action = Ref<UndoRedoActionModifyNodePosition>::Create(
					pThis,
					pNode,
					ed::GetNodePosition( nodeId ) );

				GlobalUndoRedoGroup::Get().AddAction( action, pThis->GetAssetID() );
			}

			if( ( reason & ed::SaveReasonFlags::Selection ) == ed::SaveReasonFlags::Selection )
			{
				pThis->OnNodeEditorEvent( NodeEditorAction::SelectNode );
			}

#if !defined(SAT_DIST)
			pNode->ActiveState.assign( pData, size );
			pNode->Position = ed::GetNodePosition( nodeId );
			pNode->Size = ed::GetNodeSize( nodeId );
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

		m_OutputWindow.SetWindowID( m_AssetID );
		m_OutputWindow.PushMessage( { .MessageText = "Initialised new editor!", .Type = NodeEditorMessageType::Info } );

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
		m_ActiveNodeEditorState = "";
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
		// Ensure our editor is the current one
		// We'll use a VariableGuard<ed::EditorContext*> when we can't be sure that we are the current node editor.
		ed::SetCurrentEditor( m_Editor );

		if( !m_WindowOpen )
			return;

		// Draw main window
		ImGui::Begin( m_Name.c_str(), &m_WindowOpen, m_Dirty ? ImGuiWindowFlags_UnsavedDocument : 0 );

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
				SaveSettings();
				NodeCacheEditor::WriteNodeEditorCache( this, m_CustomNameNC );

				m_Dirty = false;
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

		if( m_ViewportSize != ImGui::GetContentRegionAvail() )
			m_ViewportSize = ImGui::GetContentRegionAvail();

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
						m_OutputWindow.PushMessage( { .MessageText = "Successfully compiled and evaluated node editor!", .Type = NodeEditorMessageType::Info } );
					} break;

					case NodeEditorCompilationStatus::Failed: 
					{
						m_OutputWindow.PushMessage( { .MessageText = "Failed to compile node editor.", .Type = NodeEditorMessageType::Error } );
					} break;
				}
			}
			else
				m_OutputWindow.PushMessage( { .MessageText = "No active compiler was found!", .Type = NodeEditorMessageType::Error } );
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

		// Hand off to imgui_node_editor and draw the actual node editor and nodes
		ed::Begin( m_InternalEditorID.c_str(), ImGui::GetContentRegionAvail() );

		auto cursorTopLeft = ImGui::GetCursorScreenPos();

		for( auto& [id, rNode] : m_Nodes )
		{
			rNode->Render( m_Builder );
		}

		for( const auto& rLink : m_Links )
			ed::Link( ed::LinkId( rLink->ID ), ed::PinId( rLink->StartPinID ), ed::PinId( rLink->EndPinID ), rLink->Color );

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

								Ref<UndoRedoActionCreateLink> action = Ref<UndoRedoActionCreateLink>::Create( this, m_Links.back() );
								GlobalUndoRedoGroup::Get().AddAction( action, m_AssetID );
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

				ed::NodeId nodeId = 0;
				while( ed::QueryDeletedNode( &nodeId ) )
				{
					UUID id = nodeId.Get();

					const auto itr = std::find_if( m_Nodes.begin(), m_Nodes.end(),
						[id]( const auto& kv )
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

								Ref<UndoRedoActionDeleteNode> action = Ref<UndoRedoActionDeleteNode>::Create( this, rNode );
								GlobalUndoRedoGroup::Get().AddAction( action, m_AssetID );

								DeleteDeadLinks( id );

								OnNodeEditorEvent( NodeEditorAction::DestroyNode );

								rNode = nullptr;
								m_Nodes.erase( itr );

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

		// Extra information window
		if( ImGui::Begin( "Details" ) )
		{
			Auxiliary::ScopedDisabledFlag disabled( !HasPrivilege( NodeEditorUserAuthority::Editing ) );

			OnExtraRender();
		}

		ImGui::End();

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

			ImGui::Text( "Size X/%f Y/%f", m_HoveredNode->Size.x, m_HoveredNode->Size.y );
			ImGui::Text( "Pos X/%f Y/%f", m_HoveredNode->Position.x, m_HoveredNode->Position.y );

			disabled.Pop();
			ImGui::EndPopup();
		}

		ed::Resume();

		ed::End();

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

				if( m_Runtime )
				{
					m_Runtime->TraceEvaluationPath();
				}
			} break;
		}

		m_OutputWindow.Draw();

		ImGui::End(); // NODE_EDITOR

		// Window closed but we are dirty, show unsaved changes modal and keep window open
		if( !m_WindowOpen && m_Dirty )
		{
			m_WindowOpen = true;
		
			m_ShowUnsavedChanges = true;
		}
#endif
	}

	void NodeEditor::OnUpdate( Timestep ts )
	{
	}

	void NodeEditor::ThrowError( const std::string& rMessage )
	{
		m_OutputWindow.PushMessage( { .MessageText = rMessage, .Type = NodeEditorMessageType::Error } );
	}

	void NodeEditor::ThrowWarning( const std::string& rMessage )
	{
		m_OutputWindow.PushMessage( { .MessageText = rMessage, .Type = NodeEditorMessageType::Warning } );
	}

	void NodeEditor::PushInfoMessage( const std::string& rMessage )
	{
		m_OutputWindow.PushMessage( { .MessageText = rMessage, .Type = NodeEditorMessageType::Info } );
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

				Ref<UndoRedoActionDeleteLink> action = Ref<UndoRedoActionDeleteLink>::Create( this, link );
				GlobalUndoRedoGroup::Get().AddAction( action, m_AssetID );
			}

			m_Links.erase( Itr );
		}

		OnNodeEditorEvent( NodeEditorAction::BreakLink );
	}

	void NodeEditor::DeleteNode( UUID id, bool skipUndoRedo /*= false */ )
	{
#if !defined(SAT_DIST)
		const auto Itr = std::find_if( m_Nodes.begin(), m_Nodes.end(),
			[ id ]( const auto& rNode )
		{
			return rNode.first == id;
		} );

		if( Itr != m_Nodes.end() )
		{
			auto& rNode = ( Itr->second );

			if( rNode->CanBeDeleted )
			{
				if( !skipUndoRedo )
				{
					Ref<UndoRedoActionDeleteNode> action = Ref<UndoRedoActionDeleteNode>::Create( this, rNode );
					GlobalUndoRedoGroup::Get().AddAction( action, m_AssetID );
				}

				DeleteDeadLinks( id );

				rNode = nullptr;
				m_Nodes.erase( Itr );

				OnNodeEditorEvent( NodeEditorAction::DestroyNode );
			}
		}
#endif
	}

	void NodeEditor::SetNodePosition( UUID nodeID, const ImVec2& rNewPosition )
	{
		VariableGuard<ed::EditorContext*> guard( m_Editor );

		ed::SetNodePosition( ed::NodeId( nodeID ), rNewPosition );
	}

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

#if !defined(SAT_DIST)

	void NodeEditor::OnChooseNewNode( SharedPtr<NodeEditorNodeBase> node )
	{
		BuildNode( node );

		m_CreateNewNode = false;

		Ref<UndoRedoActionCreateNode> action = Ref<UndoRedoActionCreateNode>::Create( this, node );
		GlobalUndoRedoGroup::Get().AddAction( action, m_AssetID );

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
		NodeEditorBase::SerialiseData( rStream, isForDist );
	}

	void NodeEditor::DeserialiseData( std::ifstream& rStream )
	{
		m_State = NodeEditorState::Loading;

		NodeCacheSettings::ReadEditorSettings( this );

		m_Name = RawSerialisation::ReadString( rStream );

		CreateNewEditorIfNeeded();

		size_t mapSize = 0;
		RawSerialisation::ReadObject( mapSize, rStream );

		for( size_t i = 0; i < mapSize; i++ )
		{
			UUID key = 0;
			RawSerialisation::ReadUUID( key, rStream );

			uint64_t targetClassHash = 0;
			RawSerialisation::ReadObject( targetClassHash, rStream );

			NodeEditorNodeBase* pNode = dynamic_cast< NodeEditorNodeBase* >( ClassMetadataHandler::Get().CreateClassObject( targetClassHash ) );

			SharedPtr<NodeEditorNodeBase> node = pNode;
			if( node )
			{
				AddNode( node );
			}
			else
			{
				node = SharedPtr<NodeEditorBlueprintNode>::Create();
			}

			// NOTE: Although AddNode sets the pOuter, we want to override it to point to us (a NodeEditor), instead of NodeEditorBase
			node->pOuter = this;
			node->Deserialise( rStream );

			m_Nodes[ key ] = node;
			BuildNode( node );
		}

		mapSize = 0;
		RawSerialisation::ReadObject( mapSize, rStream );

		m_Links.resize( mapSize );

		for( size_t i = 0; i < mapSize; i++ )
		{
			Ref<Link> link = Ref<Link>::Create();

			Link::Deserialise( link, rStream );

			m_Links[ i ] = link;
		}

		m_State = NodeEditorState::Editing;
	}
#endif
}
