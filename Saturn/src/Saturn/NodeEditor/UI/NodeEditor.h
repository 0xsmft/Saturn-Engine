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

#include "Saturn/NodeEditor/NodeEditorBase.h"
#include "Saturn/NodeEditor/NodeEditorCompilationStatus.h"

#include "NodeEditorOutput.h"

#include "builders.h"

namespace Saturn {

	enum class NodeEditorAction 
	{
		CreateLink,
		BreakLink,
		CreateNode,
		DestroyNode,
		MoveNode,
		PreEvaluate,
		PostEvaluate,
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

#if !defined(SAT_DIST)
	typedef NodeEditor FDependentNodeEditorSuper;
#else
	typedef NodeEditorBase FDependentNodeEditorSuper;
#endif

	// The NodeEditor class is a graphical representation of NodeEditorBase
	class NodeEditor : public NodeEditorBase
	{
	public:
		static Ref<Texture2D> GetBlueprintBackground();

	public:
		NodeEditor();
		NodeEditor( AssetID ID );
		~NodeEditor();

		void OpenWindow( bool open ) { m_WindowOpen = open; }
		bool CanCreateLink( const Ref<Pin>& a, const Ref<Pin>& b );	

		// NodeEditorBase overrides
		virtual void OnImGuiRender() override;
		virtual void OnUpdate( Timestep ts ) override;
		virtual void OnEvent( Event& rEvent ) override;

		// NodeEditor virtuals
		virtual void OnTopBarRender() {}
		// NOTE: The ImGui window has already begun when this function is called.
		virtual void OnExtraRender() {}
		virtual void OnNodeEditorEvent( NodeEditorAction action ) {}
#if !defined(SAT_DIST)
		virtual void OnDebugBreak();
#endif

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
#if !defined(SAT_DIST)
		virtual void SerialiseData( std::ofstream& rStream, bool isForDist ) override;
		virtual void DeserialiseData( std::ifstream& rStream ) override;
#endif

	private:
		void CreateEditor();

	protected:
		void Close();
		void DeleteDeadLinks( UUID nodeID );
		void CreateNewEditorIfNeeded();
		void DrawSimulatingCanvas();
		void DrawDebuggingCanvas();
		void TryDrawUnsavedChangesModal();
		void DrawTopBarChildInternal();
		void HandleCreate();
		void HandleStateCanvasBorders();
		void EdEvaluateEditor();
		void DrawDetailsWindow();
		void DrawDataWindow();
		void DrawDebugWindow();
		void DrawBeginSearchWindow();
		void DrawSearchResultsWindow();

		virtual void DrawGraph();

	protected:
		NodeEditorSearchCacher m_SearchCacher;

		std::function<SharedPtr<NodeEditorNodeBase>()> m_CreateNewNodeFunction;
		std::function<void()> m_TopbarItemsFunction;
		std::function<void()> m_BreadCrumbsFunction;

		bool m_ShowSearchResultsWindow = false;
		bool m_IsSearching = false;
		bool m_CreateNewNode = false;
		bool m_ShowUnsavedChanges = false;
		bool m_Dirty = false;
#if !defined(SAT_DIST)
		bool m_ShowRightClickContextMenu = false;
		bool m_PendingBreakHandle = false;
#endif
		bool m_ShowDebugInformation = false;
		bool m_ShowDetailsInformation = false;
		bool m_ShowDataWindow = false;

		Ref<Pin> m_NewLinkPin = nullptr;
		Ref<Pin> m_NewNodeLinkPin = nullptr;
#if !defined(SAT_DIST)
		// TODO: Weak ptr #ReplaceRawPtrOrRefWithWeakRef
		SharedPtr<NodeEditorNodeBase> m_HoveredNode = nullptr;

		SharedPtr<NodeEditorNodeBase> m_ActiveSubGraph;
		// Sub-graph path
		std::vector<SharedPtr<NodeEditorNodeBase>> m_SubGraphs;
#endif

		ImVec2 m_ViewportSize;

		Ref<Texture2D> m_ZoomTexture;
		Ref<Texture2D> m_CompileTexture;

		util::BlueprintNodeBuilder m_Builder;

		NodeEditorOutput m_OutputWindow;

		// Internal imgui_node_editor ID, NOT to be confused with the Window Name (m_Name)
		std::string m_InternalEditorID{};

	private:
		friend class NodeEditorCache;
		friend class NodeCacheSettings;
	};
}
