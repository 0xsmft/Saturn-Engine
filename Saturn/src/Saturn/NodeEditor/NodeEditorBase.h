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

#include "Saturn/ImGui/AssetViewer.h"

#include "NodeEditorCompilationStatus.h"

#include "NodeEditorNodeBase.h"
#include "Link.h"

#include "imgui_node_editor.h"

#include <stack>
#include <queue>

namespace ed = ax::NodeEditor;

namespace Saturn {

	enum class NodeEditorType
	{
		Unknown,
		Default,
		Material,
		Sound,
		BehaviourTree
	};

	enum class NodeEditorState
	{
		// Default state when the editor is not in any specific state
		Editing,
		// This state can only happen if evaluation was successful and we are ready to actually use the evaluated data
		// For example, GraphSounds are evaluated then they are ready to be simulated (played).
		Simulating,
		// Only true if we are simulating and the simulation is paused i.e. editor suspended
		Suspended,
		// Used when the node editor is being loaded from NC
		Loading,
		// Used when evaluation is in progress
		Evaluating 
	};

	enum NodeEditorPrivileges_
	{
		// No editing or no evaluation
		NodeEditorPrivileges_ReadOnly = BIT( 0 ),
		// User can edit the nodes
		NodeEditorPrivileges_Editing = BIT( 1 ),
		// User can evaluate the editor
		NodeEditorPrivileges_Evaluation = BIT( 2 ),
		NodeEditorPrivileges_All = NodeEditorPrivileges_Editing | NodeEditorPrivileges_Evaluation,
	};

	// enum NodeEditorPrivileges_
	typedef int NodeEditorPrivileges;

	enum class NodeEditorFlowDirection 
	{
		// Backwards flow, start from the origin node. 
		// It is called "Left" because the origin node is on the left hand side of the node editor
		// Also known as:
		// Data propagation, forward execution
		//
		// ------------------>
		// 
		// ROOT NODE -> Node A -> Node B
		//                          /
		//                        Node D
		//
		Left,

		// Forwards, start from a node and work way back to origin/out node.
		// It is called "Right" because the origin node is on the right hand side of the node editor
		// Also known as:
		// Data resolution, backwards execution
		//
		// ------------------>
		// 
		// Node A -> Node B -> ROOT NODE
		//          /
		//      Node D
		//
		Right
	};

	template<typename EditorType, typename... V>
	struct NodeEditorNodeGroup {};

	class NodeEditorRuntime;
	class Texture2D;
	
	template<>
	class VariableGuard<ed::EditorContext*>
	{
	public:
		VariableGuard( ed::EditorContext* pTemporaryValue ) 
		{
			m_OldValue = ed::GetCurrentEditor();
			ed::SetCurrentEditor( pTemporaryValue );
		}

		~VariableGuard() 
		{
			ed::SetCurrentEditor( m_OldValue );
		}

	private:
		ed::EditorContext* m_OldValue = nullptr;
	};

	class NodeEditorBase : public RefTarget
	{
	public:
		NodeEditorBase();
		NodeEditorBase( AssetID id );
		virtual ~NodeEditorBase();

#if !defined(SAT_DIST)
		virtual void OnImGuiRender() = 0;
		virtual void OnUpdate( Timestep ts ) = 0;
		virtual void OnEvent( RubyEvent& rEvent ) = 0;
#else
		virtual void OnImGuiRender() {}
		virtual void OnUpdate( Timestep ts ) {}
		virtual void OnEvent( RubyEvent& rEvent ) {}
#endif

		static Ref<Texture2D> GetBlueprintBackground();

		bool IsLinked( UUID pinID );
		Ref<Pin> FindPin( UUID id );
		Ref<Link> FindLink( UUID id );
		Ref<NodeEditorNodeBase> FindNode( UUID id );
		Ref<NodeEditorNodeBase> FindNode( const std::string& rName );
		Ref<NodeEditorNodeBase> FindNodeByPin( UUID id );

		std::vector<Ref<Link>> FindLinksByPin( UUID id );

		void SetRuntime( Ref<NodeEditorRuntime> runtime );
		NodeEditorCompilationStatus Evaluate();

		// Search via inputs
		std::vector<UUID> FindNeighborsRight( Ref<NodeEditorNodeBase> node );
		
		// Search via outputs
		std::vector<UUID> FindNeighborsLeft( Ref<NodeEditorNodeBase> node );

		template<typename Function>
		void TraverseFromStart( const Ref<NodeEditorNodeBase>& rRootNode, NodeEditorFlowDirection dir, Function func )
		{
			switch( dir )
			{
				// NOTE: std::queue is FIFO
				case NodeEditorFlowDirection::Left:
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
						Ref<NodeEditorNodeBase> currentNode = FindNode( currentID );
						const auto& rNeighbours = FindNeighborsLeft( currentNode );

						for( auto Itr = rNeighbours.rbegin(); Itr != rNeighbours.rend(); Itr++ )
						{
							temporaryStack.push( *Itr );
						}
					}
				} break;

				// NOTE: std::stack is LIFO
				case NodeEditorFlowDirection::Right:
				{
					std::stack<UUID> stack;
					stack.push( rRootNode->ID );

					while( !stack.empty() )
					{
						const auto currentID = stack.top();
						stack.pop();

						func( currentID );

						// Find neighbors from inputs and continue until there is no neighbors
						Ref<NodeEditorNodeBase> currentNode = FindNode( currentID );
						for( const auto& rNeighbor : FindNeighborsRight( currentNode ) )
						{
							stack.push( rNeighbor );
						}
					}
				} break;
			}
		}

		void CreateLink( const Ref<Pin>& rStart, const Ref<Pin>& rEnd );
		void CreateLinkWithID( UUID linkID, const Ref<Pin>& rStart, const Ref<Pin>& rEnd );
		
		void ShowFlow();
		void ShowFlow( const std::vector<Ref<Link>>& rLinks );
		void ShowFlow( const Ref<Link>& rLink );
		void ShowFlow( UUID id );

		NodeEditorState GetState() const { return m_State; }
		void SetState( NodeEditorState state )
		{
			if( m_State != state )
			{
				m_State = state;
//				m_Runtime->SetState( state );
			}
		}

		[[nodiscard]] bool HasPrivilege( NodeEditorPrivileges privilege ) const;
		void SetPrivileges( NodeEditorPrivileges privilege, bool value );

	public:
		AssetID GetAssetID() const { return m_AssetID; }

		const std::map<UUID, Ref<NodeEditorNodeBase>>& GetNodes() const { return m_Nodes; }
		std::map<UUID, Ref<NodeEditorNodeBase>>& GetNodes() { return m_Nodes; }

		const std::vector<Ref<Link>>& GetLinks() const { return m_Links; }
		std::vector<Ref<Link>>& GetLinks() { return m_Links; }

		void AddNode( Ref<NodeEditorNodeBase> node );

		bool IsOpen() const { return m_WindowOpen; }

		void SaveSettings();

	public:
#if !defined(SAT_DIST)
		virtual void SerialiseData( std::ofstream& rStream ) = 0;
		virtual void DeserialiseData( std::ifstream& rStream ) = 0;
#else
		virtual void SerialiseData( std::ofstream& rStream );
		void DeserialiseData( std::istream& rStream );
#endif

	protected:
		Ref<Link> FindLinkByPin( UUID id );

	protected:
		std::string m_Name;
		bool m_WindowOpen = false;
		uint32_t m_Version = SAT_CURRENT_VERSION;

		ed::EditorContext* m_Editor = nullptr;
		std::string m_ActiveNodeEditorState;

		std::map<UUID, Ref<NodeEditorNodeBase>> m_Nodes;
		std::vector<Ref<Link>> m_Links;

		Ref<NodeEditorRuntime> m_Runtime;

		AssetID m_AssetID = 0;

		NodeEditorState m_State = NodeEditorState::Editing;
		NodeEditorPrivileges m_Privileges = NodeEditorPrivileges_All;

	private:
		friend class NodeEditorCache;
		friend class NodeCacheEditor;
		friend class NodeCacheSettings;
	};

}
