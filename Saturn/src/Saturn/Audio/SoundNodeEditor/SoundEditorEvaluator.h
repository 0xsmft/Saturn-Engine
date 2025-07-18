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
#include <unordered_set>

namespace Saturn {

	class Sound;
	class NodeEditorBase;
	class NodeEditorNodeBase;
	class SoundGroup;
	class Link;

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

		void TraceEvaluationPath() override;
		void SetTargetNodeEditor( Ref<NodeEditorBase> nodeEditor );
		Ref<NodeEditorBase>& GetTargetNodeEditor() { return m_NodeEditor; }

		[[nodiscard]] virtual NodeEditorCompilationStatus EvaluateEditor() override;

		void AddNewSound( UUID id );
		void RegisterSound( size_t id );
		void UnregisterSound( size_t id );

		void OnSoundCompleted( UUID PlayerID );

		void Loop( bool loop ) { m_Looping = loop; }
		void TerminateEvaluation() override;

		[[nodiscard]] bool IsCompleted() const { return m_Completed; }

#if !defined(SAT_DIST)
		Ref<NodeEditorNodeBase> GetMostRecentNode() const
		{
			return nullptr;
		}
#endif

	public:
		// Sounds that are currently playing
		std::vector<Ref<Sound>> AliveSounds;
		std::unordered_set<size_t> SoundsPlaying;

#if !defined(SAT_DIST)
		std::unordered_map<UUID, NodeEvaluationState> EvaluatedPath;
#endif

	private:
		void DestroyAliveSounds();
		NodeEditorCompilationStatus EvalNoChecks();
		void PropagateNotEvaluated( Ref<Link> node, NodeEvaluationState state );

	private:
		SoundEdEvaluatorInfo m_Info;
		bool m_Looping = false;

		// Mark completed as true so the first time this runs it will evaluate.
		bool m_Completed = true;
	};

}
