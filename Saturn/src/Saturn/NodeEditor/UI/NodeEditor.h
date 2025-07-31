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

	class NodeEditor : public NodeEditorBase
	{
	public:
		NodeEditor();
		NodeEditor( AssetID ID );
		~NodeEditor();

		bool CanCreateLink( const Ref<Pin>& a, const Ref<Pin>& b );	

		// NodeEditorBase overrides
		virtual void OnImGuiRender() override;
		virtual void OnUpdate( Timestep ts ) override;
		virtual void OnEvent( Event& rEvent ) override {}

		// NodeEditor virtuals
		virtual void OnTopBarRender() {}
		// NOTE: The ImGui window has already begun when this function is called.
		virtual void OnExtraRender() {}
		virtual void OnNodeEditorEvent( NodeEditorAction action ) {}

		void Open( bool open ) { m_WindowOpen = open; }

		// Happens when the user clicks on the empty space.
		void SetCreateNewNodeFunction( std::function<Ref<NodeEditorNodeBase>()>&& rrCreateNewNodeFunction )
		{
			m_CreateNewNodeFunction = std::move( rrCreateNewNodeFunction );
		}

		void SetTopBarFunction( std::function<void()>&& rrTopbarItemsFunction )
		{
			m_TopbarItemsFunction = std::move( rrTopbarItemsFunction );
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

		void MarkDirty() { m_Dirty = true; }
		bool IsDirty() const { return m_Dirty; }

		void DeleteLink( UUID id, bool skipUndoRedo = false );
		void DeleteNode( UUID id, bool skipUndoRedo = false );

		void SetNodePosition( UUID nodeID, const ImVec2& rNewPosition );

	protected:
		// By default the NodeCache will save this node editor to a file called NCEditor.{ID}.nce OR {AssetID}.{ID}.nce
		// However, in some cases such as GraphSounds or BehaviourTrees we want a custom name.
		std::string m_CustomNameNC{};

		void OnChooseNewNode( Ref<NodeEditorNodeBase> node );
		std::vector<UUID> GetSelectedNodes();

	protected:
#if !defined(SAT_DIST)
		virtual void SerialiseData( std::ofstream& rStream, bool isForDist ) override;
		virtual void DeserialiseData( std::ifstream& rStream ) override;
#endif

	private:
		void CreateEditor();
		void Close();
		void DeleteDeadLinks( UUID nodeID );
		void CreateNewEditorIfNeeded();
		void DrawSimulatingCanvas();

	private:
		std::function<Ref<NodeEditorNodeBase>()> m_CreateNewNodeFunction;
		std::function<void()> m_TopbarItemsFunction;

		bool m_CreateNewNode = false;
		bool m_ShowUnsavedChanges = false;
		bool m_Dirty = false;
#if !defined(SAT_DIST)
		bool m_ShowRightClickContextMenu = false;
#endif

		Ref<Pin> m_NewLinkPin = nullptr;
		Ref<Pin> m_NewNodeLinkPin = nullptr;
#if !defined(SAT_DIST)
		// TODO: Weak ptr
		Ref<NodeEditorNodeBase> m_HoveredNode = nullptr;
#endif

		ImVec2 m_ViewportSize;

		Ref<Texture2D> m_ZoomTexture;
		Ref<Texture2D> m_CompileTexture;

		util::BlueprintNodeBuilder m_Builder;

		NodeEditorOutput m_OutputWindow;
		std::string m_InternalEditorID{};

	private:
		friend class NodeEditorCache;
		friend class NodeCacheSettings;
	};
}
