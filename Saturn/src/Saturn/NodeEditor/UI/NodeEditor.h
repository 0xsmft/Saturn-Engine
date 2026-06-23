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

#pragma once

#include "Saturn/NodeEditor/NodeEditorDefinitions.h"
#include "Saturn/NodeEditor/NodeEditorNodeBase.h"
#include "Saturn/NodeEditor/NodeEditorCompilationStatus.h"
#include "Saturn/NodeEditor/NodeEditorVariable.h"
#include "Saturn/NodeEditor/PreCompiler/NodeEditorPreCompilerBase.h"
#include "Saturn/NodeEditor/NodeTaskCache.h"

// For AssetID
#include "Saturn/Asset/Asset.h"

#include "Saturn/Core/Event.h"
#include "Saturn/Core/Ruby/RubyEvent.h"

#include "NodeEditorOutputWindow.h"

#include "builders.h"

#include <stack>
#include <queue>

namespace Saturn {

	enum class NodeEditorAction : uint8_t
	{
		PostLoad,
		CreateLink,
		BreakLink,
		CreateNode,
		DestroyNode,
		MoveNode,
		PreEvaluate,
		OnEvaluate,
		PostEvaluateSuccess,
		PostEvaluateFailed,
		SelectNode,
		DeselectNode,
		SelectLink,
		DeselectLink
	};

	struct NodeEditorSearchCacher
	{
		ImGuiTextFilter Filter;
		std::vector<std::string> NodeNames;
		std::vector<std::string> PassedNodeNames;
	
		void FilterNames();
		void Clear();
	};

	typedef NodeEditor FDependentNodeEditorSuper;

	class Texture2D;

	struct NodeEditorCopyPasteInformation
	{
	public:
		// The selected node that was copied.
		WeakRef<const NodeEditorNodeBase> Node;

		// And it's children as well.
		std::vector<NodeEditorCopyPasteInformation> Children;

	public:
		NodeEditorCopyPasteInformation() = default;
	
		// Construct from strong reference.
		NodeEditorCopyPasteInformation( const SharedPtr<NodeEditorNodeBase> node ) 
			: Node( node )
		{
		}
	};

	class NodeEditor : public SObject, public EnabledSharedFromThis<NodeEditor>
	{
	public:
		static Ref<Texture2D> GetBlueprintBackground();

	public:
		NodeEditor();
		NodeEditor( AssetID ID );
		virtual ~NodeEditor();

		void OpenWindow( bool open ) { m_WindowOpen = open; }
		bool CanCreateLink( const Ref<Pin>& a, const Ref<Pin>& b );	

		// NodeEditorBase overrides
		virtual void OnImGuiRender();
		virtual void OnUpdate( Timestep ts );
		virtual void OnEvent( Event& rEvent );

		// NodeEditor virtuals
		virtual void OnTopBarRender() {}
		// NOTE: The ImGui window has already begun when this function is called.
		virtual void OnExtraRender() {}
		virtual void OnNodeEditorEvent( NodeEditorAction action ) {}
#if !defined(SAT_DIST)
		virtual void OnDebugBreak();
#endif

	public:
		bool IsLinked( UUID pinID );
		Ref<Pin> FindPin( UUID id );
		Ref<Link> FindLink( UUID id );
		Ref<Link> FindLinkByPin( UUID id );
		SharedPtr<NodeEditorNodeBase> FindNode( UUID id );
		SharedPtr<NodeEditorNodeBase> FindNode( const std::string& rName );
		SharedPtr<NodeEditorNodeBase> FindNodeByPin( UUID id );

		std::vector<Ref<Link>> FindLinksByPin( UUID id );

		// Search via all output/input pin
		std::vector<UUID> FindNeighborsViaInputs( SharedPtr<NodeEditorNodeBase> node );
		std::vector<UUID> FindNeighborsViaOutputs( SharedPtr<NodeEditorNodeBase> node );

		// Search via a single output/input pin
		std::vector<UUID> FindNeighborsInput( SharedPtr<NodeEditorNodeBase> node, size_t index );
		std::vector<UUID> FindNeighborsOutput( SharedPtr<NodeEditorNodeBase> node, size_t index );

		template<typename Function>
		void TraverseFromStart( const SharedPtr<NodeEditorNodeBase>& rRootNode, NodeEditorFlowDirection dir, Function func )
		{
			switch( dir )
			{
				// NOTE: std::queue is FIFO
				case NodeEditorFlowDirection::StartFromRootNode:
				{
					std::queue<UUID> temporaryStack;
					temporaryStack.push( rRootNode->ID );

					while( !temporaryStack.empty() )
					{
						const auto currentID = temporaryStack.front();
						temporaryStack.pop();

						// Visit, add evaluation stack
						func( currentID );

						// Find neighbors from outputs and continue until there is no neighbors
						SharedPtr<NodeEditorNodeBase> currentNode = FindNode( currentID );
						const auto& rNeighbours = FindNeighborsViaOutputs( currentNode );

						for( auto Itr = rNeighbours.rbegin(); Itr != rNeighbours.rend(); Itr++ )
						{
							temporaryStack.push( *Itr );
						}
					}
				} break;

				// NOTE: std::stack is LIFO
				case NodeEditorFlowDirection::GoToRootNode:
				{
					std::stack<UUID> stack;
					stack.push( rRootNode->ID );

					while( !stack.empty() )
					{
						const auto currentID = stack.top();
						stack.pop();

						func( currentID );

						// Find neighbors from inputs and continue until there is no neighbors
						SharedPtr<NodeEditorNodeBase> currentNode = FindNode( currentID );
						for( const auto& rNeighbor : FindNeighborsViaInputs( currentNode ) )
						{
							stack.push( rNeighbor );
						}
					}
				} break;
			}
		}

		void CreateLink( const Ref<Pin>& rStart, const Ref<Pin>& rEnd, ImColor color );
		void CreateLinkWithID( UUID linkID, const Ref<Pin>& rStart, const Ref<Pin>& rEnd, ImColor color );

		void ShowFlow();
		void ShowFlow( const std::vector<Ref<Link>>& rLinks );
		void ShowFlow( const Ref<Link>& rLink );
		void ShowFlow( UUID linkID );

		NodeEditorState GetState() const { return ( NodeEditorState ) m_State; }
		bool IsStateFlagSet( NodeEditorState flag ) const { return ( m_State & flag ) != 0; }

		void SetState( NodeEditorState state )
		{
			if( m_State != state )
			{
				m_State = state;
			}
		}

		void SetStateFlag( std::underlying_type_t< NodeEditorState > states, bool val )
		{
			const auto flags = states;
			auto cur = ( std::underlying_type_t< NodeEditorState > )m_State;

			if( val )
				cur |= flags;
			else
				cur &= ~flags;

			m_State = ( NodeEditorState ) cur;
		}

		[[nodiscard]] bool HasUserAuthority( NodeEditorUserAuthority privilege ) const;
		void SetUserAuthorityFlag( NodeEditorUserAuthority privilege, bool value );

		Ref<Pin> GetOriginPinForNewNode() { return m_NewNodeLinkPin; }
		const Ref<Pin> GetOriginPinForNewNode() const { return m_NewNodeLinkPin; }

		void RejectAcceptedNewLinkForNewNode() { m_AcceptedNewLink = false; }

	public:
		AssetID GetAssetID() const { return m_AssetID; }
		NodeEditorVersion GetVersion() const { return m_Version; }

		const std::map<UUID, SharedPtr<NodeEditorNodeBase>>& GetNodes() const { return m_Nodes; }
		std::map<UUID, SharedPtr<NodeEditorNodeBase>>& GetNodes() { return m_Nodes; }

		std::unordered_map<UUID, Ref<NodeEditorVariable>> GetVariables() const { return m_EditorVariables; }

		[[nodiscard]] Ref<NodeEditorVariable> FindVariable( UUID id ) const;
		[[nodiscard]] Ref<NodeEditorVariable> FindVariable( const std::string& rName ) const;

		const std::vector<Ref<Link>>& GetLinks() const { return m_Links; }
		std::vector<Ref<Link>>& GetLinks() { return m_Links; }

		const NodeTaskCache GetNodeTaskCache() const { return m_TaskCache; }

		void AddNode( SharedPtr<NodeEditorNodeBase> node );

		bool IsOpen() const { return m_WindowOpen; }

		void SaveSettings();
	public:
		// Happens when the user clicks on the empty space.
		void SetCreateNewNodeFunction( std::function<SharedPtr<NodeEditorNodeBase>()>&& rrCreateNewNodeFunction )
		{
			m_CreateNewNodeFunction = std::move( rrCreateNewNodeFunction );
		}

		void SetTopBarFunction( std::function<void()>&& rrTopbarItemsFunction )
		{
			m_TopbarItemsFunction = std::move( rrTopbarItemsFunction );
		}

		void SetBreadCrumbsFunction( std::function<void()>&& rrBreadCrumbsFunction )
		{
			m_BreadCrumbsFunction = std::move( rrBreadCrumbsFunction );
		}

		std::string& GetEditorState() { return m_ActiveNodeEditorState; }
		const std::string& GetEditorState() const { return m_ActiveNodeEditorState; }
		void SetEditorState( const std::string& rState ) { m_ActiveNodeEditorState = rState; }

		void ThrowError( const std::string& rMessage );
		void ThrowWarning( const std::string& rMessage );
		void PushInfoMessage( const std::string& rMessage );

		void Reload();

		void SetWindowName( const std::string& rName ) { m_Name = rName; }
		void NcSetCustomName( const std::string& rName ) { m_CustomNameNC = rName; }

		[[nodiscard]] const std::string& GetWindowName() const { return m_Name; }

		void MarkDirty() { m_Dirty = true; }
		bool IsDirty() const { return m_Dirty; }

		void SaveAndMarkClean();

		void DeleteLink( UUID id, bool skipUndoRedo = false );
		void DeleteNode( UUID id, bool skipUndoRedo = false );

		void SetNodePosition( UUID nodeID, const ImVec2& rNewPosition );

#if !defined(SAT_DIST)
		void AddSubGraph( SharedPtr<NodeEditorNodeBase> graph );
		void RemoveSubGraph( SharedPtr<NodeEditorNodeBase> graph );
		void ChangeEditorNextFrame( SharedPtr<NodeEditorNodeBase> graph );
		void ClearSubGraphs();
		void PopActiveSubGraphTo( SharedPtr<NodeEditorNodeBase> graph );
		[[nodiscard]] std::vector<SharedPtr<NodeEditorNodeBase>> GetSubGraphs() const { return m_SubGraphs; }
		
		SharedPtr<NodeEditorNodeBase> GetActiveSubGraph() { return m_ActiveSubGraph; }

		void AllowEditingAndDisableDebugging();
		void SetCurrentDebuggingEditor( SharedPtr<NodeEditor> nodeEditor );
		void ResetDebugging();
#endif

		std::vector<UUID> GetSelectedNodes();

	protected:
		// Node Cache filename
		// By default the NodeCache will save this node editor to a file called NCEditor.{ID}.nce OR {AssetID}.{ID}.nce
		// However, in some cases such as GraphSounds or BehaviourTrees we want a custom name to match with the Asset filename because in such cases the NodeEditor is the Asset data.
		std::string m_CustomNameNC{};

		void OnChooseNewNode( SharedPtr<NodeEditorNodeBase> node );

	protected:
		virtual void SerialiseData( std::ofstream& rStream );
		virtual void DeserialiseData( std::ifstream& rStream );

	private:
		void CreateEditor();

	protected:
		void Close();
		void DeleteDeadLinks( UUID nodeID );
		void CreateNewEditorIfNeeded();
		void DrawSimulatingCanvas();
		void DrawDebuggingCanvas();
		void TryDrawUnsavedChangesModal();
		void TryDrawCompileErrorModal();
		void DrawTopBarChildInternal();
		void HandleCreate();
		void HandleStateCanvasBorders();
		void EdEvaluateEditor();
		void DrawDetailsWindow();
		void DrawDataWindow();
		void DrawDebugWindow();
		void DrawBeginSearchWindow();
		void DrawSearchResultsWindow();

		void OnKeyPressed( RubyKeyEvent& rKeyEvent );

		virtual void DrawGraph();

		SharedPtr<NodeEditorNodeBase> CopyNode( const SharedPtr<const NodeEditorNodeBase> originalNode );

		// For use by copy and paste only!
		SharedPtr<NodeEditorNodeBase> CopyPaste_CopyNode( const NodeEditorCopyPasteInformation& rCopyInfo );

		void CopyPasteBuildChildrenList( SharedPtr<NodeEditorNodeBase> parentNode, NodeEditorCopyPasteInformation& rInfo );

	protected:
		std::string m_Name;
		// Internal imgui_node_editor ID, NOT to be confused with the Window Name (m_Name)
		std::string m_InternalEditorID{};

		ed::EditorContext* m_Editor = nullptr;
		// m_OldEditor is only valid if we switch editor during a debugging session (in the Editor).
		ed::EditorContext* m_OldEditor = nullptr;
		std::string m_ActiveNodeEditorState;

		std::unordered_map<UUID, Ref<NodeEditorVariable>> m_EditorVariables;
		std::map<UUID, SharedPtr<NodeEditorNodeBase>> m_Nodes;
		std::vector<Ref<Link>> m_Links;

		// List of nodes that will be pasted upon a Ctrl+V command
		// gets filled by the Ctrl+C command.
		std::vector<NodeEditorCopyPasteInformation> m_CopyPasteNodeClasses;

		// Temporary task cache, only exists for serialisation.
		// or for editor simulation.
		NodeTaskCache m_TaskCache;

		NodeEditorSearchCacher m_SearchCacher;

		std::function<SharedPtr<NodeEditorNodeBase>()> m_CreateNewNodeFunction;
		std::function<void()> m_TopbarItemsFunction;
		std::function<void()> m_BreadCrumbsFunction;

		bool m_WindowOpen = false;
		bool m_ShowSearchResultsWindow = false;
		bool m_IsSearching = false;
		bool m_CreateNewNode = false;
		bool m_ShowUnsavedChanges = false;
		bool m_Dirty = false;
		bool m_ShowRightClickContextMenu = false;
		bool m_PendingBreakHandle = false;
		bool m_ShowDebugInformation = false;
		bool m_ShowDetailsInformation = false;
		bool m_ShowDataWindow = false;
		bool m_HasPreCompileErrors = false;
		bool m_ShowErrorPopup = false;
		
		// Did we accept the new link when we created a new node.
		// Rare case but in the case of the set variable node it will always reject this.
		bool m_AcceptedNewLink = true;

		// Start in the editing state.
		NodeEditorState m_State = NodeEditorState_Editing;

		// User has full authority over this node editor by default
		NodeEditorUserAuthority m_Privileges = NodeEditorUserAuthority::Full;

		NodeEditorVersion m_Version = NodeEditorVersion::Latest;

		Ref<Pin> m_NewLinkPin = nullptr;
		Ref<Pin> m_NewNodeLinkPin = nullptr;
#if !defined(SAT_DIST)
		// TODO: Weak ptr #ReplaceRawPtrOrRefWithWeakRef
		SharedPtr<NodeEditorNodeBase> m_HoveredNode = nullptr;

		SharedPtr<NodeEditorNodeBase> m_ActiveSubGraph;
		// Sub-graph path
		std::vector<SharedPtr<NodeEditorNodeBase>> m_SubGraphs;
#endif

		Ref<NodeEditorPreCompilerBase> m_PreCompiler;

		ImVec2 m_ViewportSize;

		Ref<Texture2D> m_ZoomTexture;
		Ref<Texture2D> m_CompileTexture;
		AssetID m_AssetID = 0llu;

		util::BlueprintNodeBuilder m_Builder;

		NodeEditorOutputWindow m_OutputWindow;

	private:
		friend class NodeCacheEditor;
		friend class NodeCacheSettings;
	};
}
