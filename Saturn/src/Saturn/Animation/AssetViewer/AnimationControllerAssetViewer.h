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

#include "Graph/Animation/AnimGraph.h"

#include "Saturn/ImGui/AssetViewer.h"
#include "Saturn/Animation/Animator.h"

namespace Saturn {

	class AnimationControllerAssetViewer : public AssetViewer
	{
	public:
		AnimationControllerAssetViewer( AssetID id );
		~AnimationControllerAssetViewer();

		void OnImGuiRender() override;
		inline void OnUpdate( Timestep ts ) {}
		virtual void OnEvent( Event& rEvent ) override;

#if !defined(SAT_DIST)
		void OnRuntimeStateChanged( RuntimeState newState, RuntimeState oldState ) override;

//		void AddBehviourTreeReference( Ref<BehaviourTree> asset );
#endif

	private:
		void AddAsset();
		void SetupNewNodeEditor();
		void SetupNodeEditorCallbacks();
	
		SharedPtr<NodeEditorNodeBase> DrawRootGraphNewNodeOptions();
		SharedPtr<NodeEditorNodeBase> DrawStateMachineNewNodeOptions();
		SharedPtr<NodeEditorNodeBase> DrawStateMachineStateNewNodeOptions();
		SharedPtr<NodeEditorNodeBase> DrawTransitionNewNodeOptions();

	private:
		Ref<Asset> m_Asset = nullptr;
		SharedPtr<AnimGraph> m_NodeEditor = nullptr;

//		Ref<BehaviourTreeEditorEvaluator> m_Runtime = nullptr;

		UUID m_RootNodeID = 0;
	};

}
