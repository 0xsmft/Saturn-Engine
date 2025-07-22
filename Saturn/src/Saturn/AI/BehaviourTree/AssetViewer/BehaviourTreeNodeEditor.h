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
#include "Saturn/AI/BehaviourTree/BehaviourTreeMemory.h"

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

		// RUNTIME | SIMULATION ONLY
		void Tick( Timestep ts );
		void InitBBAndTasks();

		Ref<BehaviourTreeBaseTask> GetTaskFor( UUID node ) const;

	public:
		void TraverseBehaviourTree( const Ref<NodeEditorNodeBase>& rRootNode );

		void SetTargetAgent( AIAgentEntity* agent );
		[[nodiscard]] AIAgentEntity* GetTargetAgent() const;

		Ref<BehaviourTreeMemory> GetBlackboard() const { return	m_Blackboard; }
		Ref<BehaviourTreeMemorySpecification> GetBlackboardSpec() const { return m_BlackboardSpec; }

	protected:
		virtual void SerialiseData( std::ofstream& rStream, bool isForDist ) override;
		virtual void DeserialiseData( FDependentIStream& rStream ) override;

#if !defined(SAT_DIST)
	public:
		virtual void OnTopBarRender() override;
		virtual void OnExtraRender() override;
		virtual void OnNodeEditorEvent( NodeEditorAction action ) override;
	
	private:
		void ShowTreeFlow();
		void FindTreeFlow();
		void BuildFlow( Ref<BehaviourTreeNodeBase> node );
#endif

		void ResetAllTasks();

	private:
		Ref<BehaviourTreeBaseTask> m_CurrentTask;
		size_t m_CurrentTaskIndex = 0;
		
		AssetID m_BehaviourTreeMemoryAssetID = 0;

#if !defined(SAT_DIST)
		bool m_AutoEvaluate = true;
#endif

		// The current Agent that we are trying to control
		// #ReplaceRawPtrOrRefWithWeakRef, non owning ptr, should be converted to a weak ptr because we don't want to stop the entity from delete, we are "child" of it
		AIAgentEntity* m_pAIAgentEntity = nullptr;

		// All tasks in the tree
		//       NODE ID -> TASK*
		std::map<UUID, Ref<BehaviourTreeBaseTask>> m_Tasks;
		
		// All tasks at level one, level one meaning right below the root node
		//       INDEX -> TASK* (Index is relative to the m_Tasks map)
		std::map<size_t, Ref<BehaviourTreeBaseTask>> m_LevelOneTasks;

		std::vector<BehaviourTreeCompositeOrderInfo> m_EvaluationOrder;

#if !defined(SAT_DIST)
		std::vector<Ref<Link>> m_EditorLinkPath;
#endif

		Ref<BehaviourTreeMemorySpecification> m_BlackboardSpec;
		Ref<BehaviourTreeMemory> m_Blackboard;
	};

}
