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
	void AnimGraphPlayAnimTask::PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );
	
		const AnimGraphStateMachinePlayAnimNode* pAGNode = dynamic_cast< AnimGraphStateMachinePlayAnimNode* >( pNode );

		if( pAGNode )
		{
			m_AnimationAssetID = pAGNode->Outputs[ 0 ].As<AnimGraphAnimationPin>()->GetAssetID();
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

			const AnimGraphPlayAnimTask* pAGOther = dynamic_cast< AnimGraphPlayAnimTask* >( pOther );
			if( pAGOther )
			{
				m_AnimationAssetID = pAGOther->m_AnimationAssetID;
				m_AnimationAsset = AssetManager::Get()->GetAssetAs<SkeletalAnimationAsset>( m_AnimationAssetID );
			}
		}
	}

	NodeEditorTaskState AnimGraphPlayAnimTask::Tick( Timestep ts )
	{
		if( m_AnimationAsset && m_Animator )
		{
			m_Animator->Loop( true );
//			m_Animator->SetPlaybackSpeed( pAGNode->GetPlaybackSpeed() );

			m_Animator->m_SingleAnimationAsset = m_AnimationAsset;
			m_Animator->m_Context.initialize( *static_cast< const acl::compressed_tracks* >( m_AnimationAsset->GetData() ) );
			m_Animator->TickSingleAnim( 0.0f );
	
			SAT_CORE_INFO( "Ticking Animation: {0}", m_AnimationAsset->Name );
		}

		m_CurrentState = NodeEditorTaskState::Completed;
//		if( m_Animator->IsCompleted() )
//		else
//			m_CurrentState = NodeEditorTaskState::Running;

		return m_CurrentState;
	}

	void AnimGraphPlayAnimTask::Reset()
	{
		m_CurrentState = NodeEditorTaskState::Unknown;
	}

	void AnimGraphPlayAnimTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );
	
		RawSerialisation::WriteObject( m_AnimationAssetID, rStream );
	}

	void AnimGraphPlayAnimTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		RawSerialisation::ReadObject( m_AnimationAssetID, rStream );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( AnimGraphPlayAnimTask );
