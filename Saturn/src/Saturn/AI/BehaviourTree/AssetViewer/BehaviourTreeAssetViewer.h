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

#include "Nodes/BehaviourTreeSequenceNode.h"
#include "BehaviourTreeNodeEditor.h"

#include "Saturn/AI/BehaviourTree/BehaviourTree.h"

namespace Saturn {

	class BehaviourTreeEditorEvaluator;

	class BehaviourTreeAssetViewer : public	AssetViewer
	{
	public:
		BehaviourTreeAssetViewer( AssetID id );
		~BehaviourTreeAssetViewer();

		void OnImGuiRender() override;
		inline void OnUpdate( Timestep ts ) {}
		inline void OnEvent( RubyEvent& rEvent ) {}

#if !defined(SAT_DIST)
		void OnRuntimeStateChanged( RuntimeState newState, RuntimeState oldState ) override;

		void AddBehviourTreeReference( Ref<BehaviourTree> asset );
#endif

	private:
		void AddBehaviourTree();
		void SetupNewNodeEditor();
		void SetupNodeEditorCallbacks();

	private:
		Ref<Asset> m_Asset = nullptr;
		Ref<BehaviourTreeNodeEditor> m_NodeEditor = nullptr;
		Ref<BehaviourTreeEditorEvaluator> m_Runtime = nullptr;

		UUID m_RootNodeID = 0;

#if !defined(SAT_DIST)
		// The asset that is referencing this viewer
		// For example we could have the same sound spec asset being used in different places so which one are we trying to view (only available when in Runtime).

		// TODO: Weak Ref #WREF_BehaviourTreeBaseTask
		std::vector<Ref<BehaviourTree>> m_ReferencingAssets;

		Ref<BehaviourTreeNodeEditor> m_OriginalNodeEditor = nullptr;
#endif
	};	

}
