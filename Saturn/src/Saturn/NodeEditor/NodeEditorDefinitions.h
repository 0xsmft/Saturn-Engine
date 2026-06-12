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

#include "Saturn/Core/VariableGuard.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_node_editor.h"

namespace ed = ax::NodeEditor;

namespace Saturn {

	enum class NodeEditorType : uint8_t
	{
		Unknown,
		Default,
		Material,
		Sound,
		BehaviourTree,
		AnimationController
	};

	enum NodeEditorState : uint8_t
	{
		// Default state when the editor is not in any specific state, we are simply just viewing the Node Editor.
		NodeEditorState_Editing = BIT( 0 ),

		// This state can only happen if evaluation was successful and we are ready to actually use the evaluated data
		// For example, GraphSounds are evaluated then they are ready to be simulated (played).
		// Same applies with behaviour trees, the get evaluated, the simulated in runtime.
		NodeEditorState_Simulating = BIT( 1 ),

		// Only true if we are simulating and the simulation is paused i.e. editor suspended or when the NodeEditor is loaded but is not being used.
		NodeEditorState_Suspended = BIT( 2 ),

		// Used when the node editor is being loaded from NC, shorted lived state (normally)
		NodeEditorState_Loading = BIT( 3 ),

		// Used when evaluation is in progress
		NodeEditorState_Evaluating = BIT( 4 ),

		// This state happens when a breakpoint is hit, it's used along with Simulating and Suspended
		NodeEditorState_Debugging = BIT( 5 ),
	};

	// NOTE: This enum does NOT have a bitwise OR (|) operator or a AND (&) operator.
	//       You must do m_Flags = m_Flags | <flag>, instead of m_Flags =| <flag>
	//		 yes, I know it's retarded.
	//
	// ~NodeEditorUserAuthority~
	// How much authority does the user have other this Node Editor
	// We can pick and choose what we want the user to be able todo.
	enum class NodeEditorUserAuthority : uint8_t
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

	enum class NodeEditorFlowDirection : uint8_t
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
		StartFromRootNode,

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
		GoToRootNode
	};

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

	// Asset version...
	// 256 (0-255) possible (major/breaking) changes,
	// please do not add new "versions" just because a small thing has changed,
	// only add new versions if a big breaking change has occurred OR
	// something new has to be serialised, the same applies with omissions.
	//
	// NOTE: This enum is not a bitfield! It is expected if we upgrade/downgrade we gain/lose the modifications that
	// are new/old.
	// 
	// e.g. if this asset version is 0 and we upgrade to Breakpoints (2) we get Subgraphs (1) as well. 
	//
	enum class NodeEditorVersion : uint8_t
	{
		// <0.2.5
		BeforeVersionWasAdded,

		// Subgraph feature added, 0.2.3
		Subgraphs,

		// Breakpoint feature added, 0.2.5
		Breakpoints,

		// Node editor task cache added, 0.2.5
		TaskCache,

		// PinType, PinKind, PinRenderType all changed to single-bytes, 0.2.5
		PinClassSizeChange,

		// Removed old JSON state flags, 0.2.6
		RemovedState,

		//^^^ only add new versions above here.... and not below here vvv
		Latest = RemovedState,
		Lowest = BeforeVersionWasAdded
	};

}
