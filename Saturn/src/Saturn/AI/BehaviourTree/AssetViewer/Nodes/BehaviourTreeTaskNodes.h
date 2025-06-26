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

#include "BehaviourTreeNodeBase.h"

#include "Saturn/Asset/Asset.h"

#include <chrono>

namespace Saturn {

	class BehaviourTreeWaitNode : public BehaviourTreeNodeBase
	{
	public:
		BehaviourTreeWaitNode();
		virtual ~BehaviourTreeWaitNode();

		virtual NodeEvaluationState EvaluateNode( NodeEditorRuntime* pEvaluator ) override;
		virtual void OnSerialise( std::ofstream& rStream ) const override;
		virtual void OnDeserialise( IStream& rStream ) override;
		virtual BehaviourTreeBaseTask* ConvertToTask() override;
		virtual void PostDeserialise() override;

#if !defined(SAT_DIST)
		virtual void RenderDetails() override;
		virtual void OnRenderExtra() override;
#endif

	public:
		float WaitDuration = 1.0f;

	private:
		// TODO: Weak Ref #WREF_BehaviourTreeBaseTask
		Ref<BehaviourTreeMemoryVariableSpec> m_MemVariable;

	private:
		void CreateNode();
	};

	class BehaviourTreePlaySoundNode : public BehaviourTreeNodeBase
	{
	public:
		BehaviourTreePlaySoundNode();
		virtual ~BehaviourTreePlaySoundNode();

		virtual NodeEvaluationState EvaluateNode( NodeEditorRuntime* pEvaluator ) override;
		virtual void OnSerialise( std::ofstream& rStream ) const override;
		virtual void OnDeserialise( IStream& rStream ) override;
		virtual BehaviourTreeBaseTask* ConvertToTask() override;

#if !defined(SAT_DIST)
		// From NodeEditorTreeNode
		virtual void OnRenderExtra() override;
		// From BehaviourTreeNodeBase
		virtual void RenderDetails() override;
#endif

		void SetSoundID( AssetID id ) { m_SoundID = id; }
		AssetID GetSoundID() const { return m_SoundID; }

	private:
		void CreateNode();

	private:
		AssetID m_SoundID = 0;
	};

	// Move the AI Agent to a position
	class BehaviourTreeMoveToNode : public BehaviourTreeNodeBase
	{
	public:
		BehaviourTreeMoveToNode();
		virtual ~BehaviourTreeMoveToNode();

		virtual NodeEvaluationState EvaluateNode( NodeEditorRuntime* pEvaluator ) override;
		virtual void OnSerialise( std::ofstream& rStream ) const override;
		virtual void OnDeserialise( IStream& rStream ) override;
		virtual BehaviourTreeBaseTask* ConvertToTask() override;

#if !defined(SAT_DIST)
		// From NodeEditorTreeNode
		virtual void OnRenderExtra() override;
		// From BehaviourTreeNodeBase
		virtual void RenderDetails() override;
		// From NodeEditorNodeBase
		virtual void RenderContextWindow() override;
#endif

		void SetTargetPosition( const glm::vec3& rPosition ) { m_TargetPosition = rPosition; }
		glm::vec3 GetTargetPosition() const { return m_TargetPosition; }

	private:
		void CreateNode();

	private:
		glm::vec3 m_TargetPosition{};

		// TODO: Weak Ref #WREF_BehaviourTreeBaseTask
		Ref<BehaviourTreeMemoryVariableSpec> m_Variable;
	};

}
