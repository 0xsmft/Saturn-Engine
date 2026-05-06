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

#include "Saturn/NodeEditor/UI/NodeEditor.h"
#include "Saturn/Core/IndexedMap.h"

#include "Saturn/Animation/AssetViewer/AnimGraphTaskAndNodeInfo.h"

namespace Saturn {

	class SGraphTask;

	class AnimGraph : public FDependentNodeEditorSuper
	{
	public:
		using AnimGraphSortMap = IndexedMap<UUID, AnimGraphNodeAndTaskInfo>;

	public:
		AnimGraph();
		AnimGraph( AssetID id );
		virtual ~AnimGraph();

		SharedPtr<NodeEditorNodeBase> SetupNewNodeEditor();

		AnimGraphSortMap TraverseAndCreateTasks();

		void MarkNodeAsEntry( SharedPtr<NodeEditorNodeBase> node ) { m_StateMachineEntryNode = node; }
		SharedPtr<NodeEditorNodeBase> GetEntryNode() const { return m_StateMachineEntryNode; }

#if !defined(SAT_DIST)
	public:
		void BuildTaskCache();

	public:
		virtual void OnExtraRender() override;
		virtual void OnNodeEditorEvent( NodeEditorAction action ) override;

	protected:
		//////////////////////////////////////////////////////////////////////////
		void SerialiseData( std::ofstream& rStream, bool isForDist ) override;
		void DeserialiseData( FDependentIStream& rStream ) override;
	protected:
		virtual void DrawGraph() override;

	private:
		void DrawStateMachineNodes();
#endif

	private:
		// Sorting
		void SortAnimGraph( AnimGraphSortMap& rMap );
		void SortStateMachineEntry( AnimGraphSortMap& rMap );
		void SortTransitionNodeOrStateMachineAfterEntryRec( UUID id, AnimGraphSortMap& rMap );

	private:
		UUID m_TransitionStartNode = 0;
		SharedPtr<NodeEditorNodeBase> m_StateMachineEntryNode;

		bool m_StateNodeHovered = false;
		bool m_CanResetHoveredNode = false;
	};

}
