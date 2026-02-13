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

	void AnimGraphPlayAnimTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		const AnimGraphStateMachinePlayAnimNode* pAGNode = dynamic_cast< AnimGraphStateMachinePlayAnimNode* >( pNode );
		const AnimGraphTaskHandler* pAGTaskHandler = dynamic_cast< AnimGraphTaskHandler* >( pHandler );
		
		if( pAGTaskHandler )
		{
			m_Animator = pAGTaskHandler->GetAnimator();
		}

		if( pAGNode )
		{
			m_AnimationAsset = AssetManager::Get()->GetAssetAs<SkeletalAnimationAsset>( pAGNode->Outputs[ 0 ].As<AnimGraphAnimationPin>()->GetAssetID() );
		}

		/*
		if( m_AnimationAsset && m_Animator )
		{
			m_Animator->Loop( true );
//			m_Animator->SetPlaybackSpeed( pAGNode->GetPlaybackSpeed() );

			m_Animator->m_SingleAnimationAsset = m_AnimationAsset;
			m_Animator->m_Context.initialize( *static_cast< const acl::compressed_tracks* >( m_AnimationAsset->GetData() ) );
			m_Animator->TickSingleAnim( 0.0f );
		}
		*/
	}

	NodeEditorTaskState AnimGraphPlayAnimTask::Tick( Timestep ts )
	{
		SAT_CORE_INFO( "AnimGraphPlayAnimTask::Tick" );

		if( m_AnimationAsset && m_Animator )
		{
			m_Animator->Loop( true );
//			m_Animator->SetPlaybackSpeed( pAGNode->GetPlaybackSpeed() );

			m_Animator->m_SingleAnimationAsset = m_AnimationAsset;
			m_Animator->m_Context.initialize( *static_cast< const acl::compressed_tracks* >( m_AnimationAsset->GetData() ) );
			m_Animator->TickSingleAnim( 0.0f );
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

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( AnimGraphPlayAnimTask );
