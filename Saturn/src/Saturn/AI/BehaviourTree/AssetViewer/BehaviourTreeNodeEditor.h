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

	class BehaviourTreeBaseTask;
	class AIAgentEntity;

#if !defined(SAT_DIST)
	typedef NodeEditor BehaviourTreeNodeEditorSuper;
#else
	typedef NodeEditorBase BehaviourTreeNodeEditorSuper;
#endif

	struct BehaviourTreeCompositeOrderInfo
	{
		UUID NodeID = 0;
		int Level = 0;

		static inline void Serialise( const BehaviourTreeCompositeOrderInfo& rObject, std::ofstream& rStream )
		{
			RawSerialisation::WriteObject( rObject.NodeID, rStream );
			RawSerialisation::WriteObject( rObject.Level, rStream );
		}

		static inline void Deserialise( BehaviourTreeCompositeOrderInfo& rObject, std::istream& rStream )
		{
			RawSerialisation::ReadObject( rObject.NodeID, rStream );
			RawSerialisation::ReadObject( rObject.Level, rStream );
		}
	};

	class BehaviourTreeNodeEditor : public BehaviourTreeNodeEditorSuper
	{
	public:
		BehaviourTreeNodeEditor();
		BehaviourTreeNodeEditor( AssetID id );
		virtual ~BehaviourTreeNodeEditor();

		void TraverseBehaviourTree( const Ref<NodeEditorNodeBase>& rRootNode );
		void InitBehaviourTree();

		void Tick( Timestep ts );

		BehaviourTreeBaseTask* GetTaskFor( UUID node );

		void ResetAllTasks();

		void SetTargetAgent( Ref<AIAgentEntity> agent ) { m_AIAgentEntity = agent; }
		[[nodiscard]] Ref<AIAgentEntity> GetTargetAgent() const;

#if !defined(SAT_DIST)
		virtual void OnTopBarRender() override;
		virtual void OnExtraRender() override;
		virtual void OnNodeEditorEvent( NodeEditorAction action ) override;

	protected:
		virtual void SerialiseData( std::ofstream& rStream ) override;
		virtual void DeserialiseData( std::ifstream& rStream ) override;
#endif

	private:
		BehaviourTreeBaseTask* m_CurrentTask = nullptr;
		size_t m_CurrentTaskIndex = 0;

#if !defined(SAT_DIST)
		bool m_AutoEvaluate = true;
#endif

		Ref<AIAgentEntity> m_AIAgentEntity;

		std::map<UUID, BehaviourTreeBaseTask*> m_Tasks;
		std::map<size_t, BehaviourTreeBaseTask*> m_LevelOneTasks;

		std::vector<BehaviourTreeCompositeOrderInfo> m_EvaluationOrder;
	};

}
