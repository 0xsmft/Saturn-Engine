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

#include "Saturn/Core/Base.h"
#include "Saturn/Core/UUID.h"
#include "Saturn/GameFramework/SObject.h"

namespace Saturn {

	enum class NodeEditorTaskState
	{
		Unknown,
		Starting,
		Running,
		Completed, // success flag
		Failed
	};

	class NodeEditorNodeBase;
	class NodeEditorBase;
	class NodeEditorTaskHandler;

	SCLASS()
	class NodeEditorTaskBase : public SObject
	{
		SAT_DECLARE_CLASS( NodeEditorTaskBase, SObject );
	public:
		NodeEditorTaskBase() = default;
		virtual ~NodeEditorTaskBase() = default;

		virtual void InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pEditor, NodeEditorNodeBase* pNode ) {}
		virtual NodeEditorTaskState Tick( Timestep ts ) { return NodeEditorTaskState::Unknown; }
		virtual void Reset() {}

	public:
		[[nodiscard]] Saturn::UUID GetNodeID() const { return m_NodeID; }
		[[nodiscard]] NodeEditorTaskState GetState() const { return m_CurrentState; }

	public:
#if !defined(SAT_DIST)
		[[nodiscard]] virtual const char* GetTaskName() const { return "Base Task"; }
		[[nodiscard]] virtual bool IsSpawnableNode() const { return false; }
		virtual void OnRenderExtra() {}
		virtual void RenderDetails() {}
#endif

	protected:
		Saturn::UUID m_NodeID = 0;
		NodeEditorTaskState m_CurrentState = NodeEditorTaskState::Unknown;
	};
	
}
