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

#include "sppch.h"
#include "AnimGraphPlayAnimTask.h"

#include "GraphTask.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Animation/SkeletalAnimationAsset.h"

#include "Saturn/Animation/AssetViewer/Graph/StateMachine/AnimGraphStateMachinePlayAnimNode.h"
#include "Saturn/Animation/AssetViewer/Graph/Animation/AnimGraphAnimationPin.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

#include "AnimGraphTaskHandler.h"

namespace Saturn {

	AnimGraphPlayAnimTask::AnimGraphPlayAnimTask()
	{
	}

	AnimGraphPlayAnimTask::~AnimGraphPlayAnimTask()
	{
		m_Animator = nullptr;
	}

#if !defined(SAT_DIST)
	void AnimGraphPlayAnimTask::SortOnCompleteTasksAndCovertToTasks( NodeEditor* pEditor, SharedPtr<NodeEditorNodeBase> firstNode )
	{
		std::deque<SharedPtr<NodeEditorNodeBase>> nodes;
		pEditor->TraverseFromStart( firstNode, NodeEditorFlowDirection::StartFromRootNode,
			[&]( const auto id ) 
		{
			nodes.push_back( pEditor->FindNode( id ) );
		} );

		m_OnCompleteTasks.reserve( nodes.size() );

		while( !nodes.empty() )
		{
			SharedPtr<NodeEditorNodeBase> sortedNode = nodes.back();
		
			auto* pTask = sortedNode->ConvertToTask();
			if( pTask )
			{
				pTask->PreInitialiseTask( pEditor, sortedNode.Get() );
				
				// NB: Converted to Ref!
				m_OnCompleteTasks.push_back( pTask );
			}

			nodes.pop_back();
		}
	}

	void AnimGraphPlayAnimTask::PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );
		
		const AnimGraphStateMachinePlayAnimNode* pAGNode = dynamic_cast< AnimGraphStateMachinePlayAnimNode* >( pNode );
		if( !pAGNode )
			return;

		// Set animation ID.
		m_AnimationAssetID = pAGNode->Outputs[ 0 ].As<AnimGraphAnimationPin>()->GetAssetID();
		m_ShouldLoop = pAGNode->IsLooping();

		// "OnComplete" tasks
		if( pEditor->IsLinked( pAGNode->Outputs[ 1 ]->ID ) )
		{
			// Find what the pin is linked to...
			// NB: FindLinkByPin is okay because this pin only accepts one link.
			const auto link = pEditor->FindLinkByPin( pAGNode->Outputs[ 1 ]->ID );
			
			const auto firstOnCompleteNode = pEditor->FindNodeByPin( link->EndPinID );
			if( firstOnCompleteNode )
			{
				SortOnCompleteTasksAndCovertToTasks( pEditor, firstOnCompleteNode );
			}
		}
	}
#endif

	void AnimGraphPlayAnimTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );

		const AnimGraphTaskHandler* pAG = dynamic_cast< AnimGraphTaskHandler* >( pHandler );
		if( pAG )
		{
			m_Animator = pAG->GetAnimator();

			AnimGraphPlayAnimTask* pAGOther = dynamic_cast< AnimGraphPlayAnimTask* >( pOther );
			if( pAGOther )
			{
				m_AnimationAssetID = pAGOther->m_AnimationAssetID;
				m_AnimationAsset = AssetManager::Get()->GetAssetAs<SkeletalAnimationAsset>( m_AnimationAssetID );
				m_ShouldLoop = pAGOther->m_ShouldLoop;
			
				// Construct new on complete tasks.
				m_OnCompleteTasks.reserve( pAGOther->m_OnCompleteTasks.size() );

				for( auto& rInnerTask : pAGOther->m_OnCompleteTasks )
				{
					auto* pNewTask = ( NodeEditorTaskBase* ) ClassMetadataHandler::Get().CreateClassObject( rInnerTask->GetClass(), this );
					pNewTask->InitialiseTaskWithOther( pHandler, rInnerTask.Get() );

					m_OnCompleteTasks.push_back( pNewTask );
				}
			}
		}
	}

	NodeEditorTaskState AnimGraphPlayAnimTask::Tick( Timestep ts )
	{
		if( m_AnimationAsset && m_Animator && !m_AnimationInitialised )
		{
			m_Animator->AnimGraph_SetSingleAnim( m_AnimationAsset, m_ShouldLoop );

			m_AnimationInitialised = true;
		}

		// Check if we are done...
		if( m_Animator->IsCompleted() )
		{
			TickOnCompleteTasks( ts );
			m_CurrentState = NodeEditorTaskState::Completed;
		}
		else
			m_CurrentState = NodeEditorTaskState::Running;

		return m_CurrentState;
	}

	void AnimGraphPlayAnimTask::TickOnCompleteTasks( Timestep ts )
	{
		// TODO: We need some sort of global value to tell how many tasks should be ticked in one cycle without
		//		 causing performance issues.
		//		 so for now if it's greater than 5 we do it over multiple cycles.
		if( m_OnCompleteTasks.size() <= 5 )
		{
			for( auto& rTask : m_OnCompleteTasks )
			{
				rTask->Tick( ts );
			}
		}
		else
		{
			for( size_t i = m_StartingIndexForOnComplete; i < 5; ++i, ++m_StartingIndexForOnComplete )
			{
				m_OnCompleteTasks[ m_StartingIndexForOnComplete ]->Tick( ts );
			}
		}
	}

	void AnimGraphPlayAnimTask::Reset()
	{
		m_CurrentState = NodeEditorTaskState::Unknown;
		m_AnimationInitialised = false;
	}

	void AnimGraphPlayAnimTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );
	
		RawSerialisation::WriteObject( m_OnCompleteTasks.size(), rStream );

		for( const auto& rInnerTask : m_OnCompleteTasks )
		{
			RawSerialisation::WriteObject( rInnerTask->GetClass()->GetHash(), rStream );
			rInnerTask->Serialise( rStream );
		}

		RawSerialisation::WriteObject( m_AnimationAssetID, rStream );
		RawSerialisation::WriteObject( m_ShouldLoop, rStream );
	}

	void AnimGraphPlayAnimTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		size_t size = 0llu;
		RawSerialisation::ReadObject( size, rStream );

		m_OnCompleteTasks.reserve( size );

		for( size_t i = 0; i < size; ++i )
		{
			uint64_t classHash = 0llu;
			RawSerialisation::ReadObject( classHash, rStream );

			SObject* pObject = ClassMetadataHandler::Get().CreateClassObject( classHash, this );
			SAT_CORE_VERIFY( pObject );

			NodeEditorTaskBase* pTask = dynamic_cast< NodeEditorTaskBase* >( pObject );
			SAT_CORE_VERIFY( pTask );

			pTask->Deserialise( rStream );

			m_OnCompleteTasks.push_back( pTask );
		}

		RawSerialisation::ReadObject( m_AnimationAssetID, rStream );
		RawSerialisation::ReadObject( m_ShouldLoop, rStream );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( AnimGraphPlayAnimTask );
