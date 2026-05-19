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

#include "NodeEditorNodeFlags.h"

#include "Saturn/Core/Base.h"
#include "Saturn/Core/UUID.h"
#include "Saturn/GameFramework/SObject.h"

// I don't like the fact we have to include <fstream>... (which is pulled in from the file below)
#include "Saturn/Serialisation/Raw/RawSerialisationBase.h"

namespace Saturn {

	enum class NodeEditorTaskState : uint8_t
	{
		Unknown,
		Starting,
		Running,
		Completed, // success flag
		DebugBreakRequested,
		Failed,

		TransitionNotComplete,
		TransitionCannotTransition,
		TransitionShouldTransition,
		TransitionShouldTransitionLast,
	};

	class NodeEditorNodeBase;
	class NodeEditor;
	class NodeEditorTaskHandler;

	//
	// NodeEditorTaskBase
	// 
	// A NodeEditorTaskBase is a the runtime representation of a Node.
	//
	SCLASS()
	class NodeEditorTaskBase : public SObject
	{
		SAT_DECLARE_CLASS_MOVE( NodeEditorTaskBase, SObject );
	public:
		NodeEditorTaskBase() = default;
		virtual ~NodeEditorTaskBase() = default;

	public:
#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode );
#endif
		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther );

		// Called every frame unless the node has the ConstantEvaluated flag.
		virtual NodeEditorTaskState Tick( Timestep ts ) { return NodeEditorTaskState::Unknown; }
		virtual void Reset() {}

	public:
		virtual void Serialise( std::ofstream& rStream ) const;
		virtual void Deserialise( FDependentIStream& rStream );

	public:
		[[nodiscard]] Saturn::UUID GetNodeID() const { return m_NodeID; }
		[[nodiscard]] NodeEditorNodeFlags GetNodeFlags() const { return m_NodeFlags; }
		[[nodiscard]] NodeEditorTaskState GetState() const { return m_CurrentState; }

#if !defined(SAT_DIST)
		void SetDebugName( const std::string& rName ) { m_DebugName = rName; }
#endif

	protected:
#if !defined(SAT_DIST)
		std::string m_DebugName;
#endif

		// The original Node ID, we need this just in case we need to access any data that is linked to a Node ID.
		Saturn::UUID m_NodeID = 0;
		NodeEditorTaskHandler* m_pHandler = nullptr;
		NodeEditorNodeFlags m_NodeFlags = NodeFlags_Default;
		NodeEditorTaskState m_CurrentState = NodeEditorTaskState::Unknown;
	};
	
}
