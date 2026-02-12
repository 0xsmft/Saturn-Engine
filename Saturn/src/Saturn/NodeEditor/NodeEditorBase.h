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

#include "NodeEditorCompilationStatus.h"
#include "NodeEditorNodeBase.h"
#include "NodeEditorVariable.h"
#include "Link.h"

#include "Saturn/Core/VariableGuard.h"
#include "Saturn/ImGui/AssetViewer.h"

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
		BehaviourTree,
		AnimationController
	};

	enum class NodeEditorState
	{
		// Default state when the editor is not in any specific state
		Editing,
		// This state can only happen if evaluation was successful and we are ready to actually use the evaluated data
		// For example, GraphSounds are evaluated then they are ready to be simulated (played).
		Simulating,
		// Only true if we are simulating and the simulation is paused i.e. editor suspended or when the NodeEditor is loaded but is not being used.
		Suspended,
		// Used when the node editor is being loaded from NC
		Loading,
		// Used when evaluation is in progress
		Evaluating 
	};

	// NOTE: This enum does NOT have a bitwise OR (|) operator or a AND (&) operator.
	//       You must do m_Flags = m_Flags | <flag>, instead of m_Flags =| <flag>
	//
	// ~NodeEditorUserAuthority~
	// How much authority does the user have other this Node Editor
	// We can pick and choose what we want the user to be able todo.
	enum class NodeEditorUserAuthority
	{
		// User can edit the nodes
		Editing = BIT( 0 ),
		// User can evaluate the editor
		Evaluation = BIT( 1 ),
		
		Full = Editing | Evaluation,
	};

	inline constexpr NodeEditorUserAuthority operator|( NodeEditorUserAuthority lhs, NodeEditorUserAuthority rhs )
	{
		using U = std::underlying_type_t<NodeEditorUserAuthority>;
		return static_cast< NodeEditorUserAuthority >( static_cast< U >( lhs ) | static_cast< U >( rhs ) );
	}

	inline constexpr NodeEditorUserAuthority operator&( NodeEditorUserAuthority lhs, NodeEditorUserAuthority rhs )
	{
		using U = std::underlying_type_t<NodeEditorUserAuthority>;
		return static_cast< NodeEditorUserAuthority >( static_cast< U >( lhs ) & static_cast< U >( rhs ) );
	}
	
	inline constexpr NodeEditorUserAuthority operator~( NodeEditorUserAuthority rhs )
	{
		using U = std::underlying_type_t<NodeEditorUserAuthority>;
		return static_cast< NodeEditorUserAuthority >( ~static_cast< U >( rhs ) );
	}

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
		//                        Node C
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
		//      Node C
		//
		Right
	};

	template<typename EditorType, typename... V>
	struct NodeEditorNodeGroup {};

	class NodeEditorRuntime;
	class Texture2D;
	
	// VariableGuard template specialisation for ed::EditorContext*
	template<>
	class VariableGuard<ed::EditorContext*, ed::EditorContext*>
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

	// The base class for all Node Editors (Node Graphs).
	// NodeEditorBase does not inherit from ImGuiWindow because NodeEditorBase is more of the backend and doesn't
	// need to always be a window.
	// NodeEditorBase does not draw anything, it simply provides the logical code for nodes without any rendering
	// For example, you can setup a NodeEditorBase the same way as a NodeEditor would of been created and use it the same way just without any graphical representation. 
	// On dist, this class will ALWAYS be used in place of NodeEditor
	// 
	// NodeEditors are based from an SObject, which is okay however we are wasting 32 whole ass bytes :(
	// The reason why we are based from an SObject is that we want Tasks* and Nodes to share the same ancestor, so m_pParentObject = NodeEditorBase, before this Nodes had their own pointer to us, and tasks did not, every task could have a pointer to NodeEditorBase however Tasks are due a rewrite in their own respect, so maybe this won't last long as well.
	//
	class NodeEditorBase : public SObject, public EnabledSharedFromThis<NodeEditorBase>
	{
	public:
		NodeEditorBase();
		NodeEditorBase( AssetID id );
		virtual ~NodeEditorBase();

#if !defined(SAT_DIST)
		virtual void OnImGuiRender() = 0;
		virtual void OnUpdate( Timestep ts ) = 0;
		virtual void OnEvent( Event& rEvent ) = 0;
#else
		virtual void OnImGuiRender() {}
		virtual void OnUpdate( Timestep ts ) {}
		virtual void OnEvent( Event& rEvent ) {}
#endif

		bool IsLinked( UUID pinID );
		Ref<Pin> FindPin( UUID id );
		Ref<Link> FindLink( UUID id );
		SharedPtr<NodeEditorNodeBase> FindNode( UUID id );
		SharedPtr<NodeEditorNodeBase> FindNode( const std::string& rName );
		SharedPtr<NodeEditorNodeBase> FindNodeByPin( UUID id );

		std::vector<Ref<Link>> FindLinksByPin( UUID id );

		void SetRuntime( Ref<NodeEditorRuntime> runtime );
		NodeEditorCompilationStatus Evaluate();

		// Search via inputs
		std::vector<UUID> FindNeighborsRight( SharedPtr<NodeEditorNodeBase> node );

		// Search via outputs
		std::vector<UUID> FindNeighborsLeft( SharedPtr<NodeEditorNodeBase> node );

		template<typename Function>
		void TraverseFromStart( const SharedPtr<NodeEditorNodeBase>& rRootNode, NodeEditorFlowDirection dir, Function func )
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
						SharedPtr<NodeEditorNodeBase> currentNode = FindNode( currentID );
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
						SharedPtr<NodeEditorNodeBase> currentNode = FindNode( currentID );
						for( const auto& rNeighbor : FindNeighborsRight( currentNode ) )
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

		NodeEditorState GetState() const { return m_State; }
		void SetState( NodeEditorState state )
		{
			if( m_State != state )
			{
				m_State = state;
//				m_Runtime->SetState( state );
			}
		}

		[[nodiscard]] bool HasPrivilege( NodeEditorUserAuthority privilege ) const;
		void SetPrivileges( NodeEditorUserAuthority privilege, bool value );

	public:
		AssetID GetAssetID() const { return m_AssetID; }

		const std::map<UUID, SharedPtr<NodeEditorNodeBase>>& GetNodes() const { return m_Nodes; }
		std::map<UUID, SharedPtr<NodeEditorNodeBase>>& GetNodes() { return m_Nodes; }

		std::unordered_map<UUID, Ref<NodeEditorVariable>> GetDataHandles() const { return m_DataHandles; }

		[[nodiscard]] Ref<NodeEditorVariable> FindDataHandle( UUID id ) const;
		[[nodiscard]] Ref<NodeEditorVariable> FindDataHandle( const std::string& rName ) const;

		const std::vector<Ref<Link>>& GetLinks() const { return m_Links; }
		std::vector<Ref<Link>>& GetLinks() { return m_Links; }

		void AddNode( SharedPtr<NodeEditorNodeBase> node );

		bool IsOpen() const { return m_WindowOpen; }

		void SaveSettings();

	public:
		virtual void SerialiseData( std::ofstream& rStream, bool isForDist );

#if !defined(SAT_DIST)
		virtual void DeserialiseData( std::ifstream& rStream ) = 0;
#else
		virtual void DeserialiseData( std::istream& rStream );
#endif

	protected:
		Ref<Link> FindLinkByPin( UUID id );

	protected:
		std::string m_Name;
		bool m_WindowOpen = false;
		uint32_t m_Version = SAT_CURRENT_VERSION;

		ed::EditorContext* m_Editor = nullptr;
		std::string m_ActiveNodeEditorState;

		std::unordered_map<UUID, Ref<NodeEditorVariable>> m_DataHandles;
		std::map<UUID, SharedPtr<NodeEditorNodeBase>> m_Nodes;
		std::vector<Ref<Link>> m_Links;

		Ref<NodeEditorRuntime> m_Runtime;

		AssetID m_AssetID = 0;

		NodeEditorState m_State = NodeEditorState::Editing;
		// User has full authority over this node editor by default
		NodeEditorUserAuthority m_Privileges = NodeEditorUserAuthority::Full;

	private:
		friend class NodeEditorCache;
		friend class NodeCacheEditor;
		friend class NodeCacheSettings;
	};

}
