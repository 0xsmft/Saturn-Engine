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

#include "Saturn/NodeEditor/UI/NodeEditor.h"

namespace Saturn {

	class AnimGraph : public FDependentNodeEditorSuper
	{
	public:
		AnimGraph();
		AnimGraph( AssetID id );
		virtual ~AnimGraph();

		std::vector<UUID> TraverseAnimGraph();

		void MarkNodeAsEntry( SharedPtr<NodeEditorNodeBase> node ) { m_EntryNode = node; }
		SharedPtr<NodeEditorNodeBase> GetEntryNode() const { return m_EntryNode; }

#if !defined(SAT_DIST)
	public:
		virtual void OnExtraRender() override;
		virtual void OnNodeEditorEvent( NodeEditorAction action ) override;
#endif

	protected:
		//////////////////////////////////////////////////////////////////////////
		void SerialiseData( std::ofstream& rStream, bool isForDist ) override;
		void DeserialiseData( FDependentIStream& rStream ) override;

	protected:
		virtual void DrawGraph() override;

	private:
		void DrawStateMachineNodes();

	private:
		UUID m_TransitionStartNode = 0;
		SharedPtr<NodeEditorNodeBase> m_EntryNode;
	};

}
