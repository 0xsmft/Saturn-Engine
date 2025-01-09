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

#include "Saturn/NodeEditor/Runtime/NodeEditorRuntime.h"

#include "Saturn/Core/Base.h"
#include "Saturn/Core/UUID.h"

#include <stack>

namespace Saturn {

	class Sound;
	class NodeEditorBase;
	class Node;
	class SoundGroup;

	class SoundEditorEvaluator : public NodeEditorRuntime
	{
	public:
		struct SoundEdEvaluatorInfo
		{
			Ref<SoundGroup> SoundGroup;
			UUID OutputNodeID;
		};

	public:
		SoundEditorEvaluator( const SoundEditorEvaluator& ) = delete;

		SoundEditorEvaluator( const SoundEdEvaluatorInfo& rInfo );
		~SoundEditorEvaluator();

		void SetTargetNodeEditor( Ref<NodeEditorBase> nodeEditor );
		Ref<NodeEditorBase>& GetTargetNodeEditor() { return m_NodeEditor; }

		[[nodiscard]] virtual NodeEditorCompilationStatus EvaluateEditor() override;

	public:
		// Sounds that have to be played in order to get the correct result.
		std::stack<UUID> SoundStack;
		
		// Sounds that are currently playing
		std::vector<Ref<Sound>> AliveSounds;

	private:
		void DestroyAliveSounds();
	
	private:
		Ref<NodeEditorBase> m_NodeEditor;
		SoundEdEvaluatorInfo m_Info;
	};
}