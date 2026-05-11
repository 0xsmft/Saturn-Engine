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
#include "AnimGraphTransitionTasks.h"

#include "Saturn/NodeEditor/NodeEditorTaskHandler.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/AnimGraphStateMachineTransitionNode.h"
#include "Saturn/Animation/AssetViewer/Graph/StateMachine/AnimGraphTransitionGraphNodes.h"

namespace Saturn {

	AnimGraphTransitionTask::AnimGraphTransitionTask()
	{
	}

	AnimGraphTransitionTask::~AnimGraphTransitionTask()
	{
	}

	void AnimGraphTransitionTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditor* pEditor, NodeEditorNodeBase* pNode )
	{
		AnimGraphStateMachineTransitionNode* pTransitionNode = dynamic_cast< AnimGraphStateMachineTransitionNode* >( pNode );
		if( pTransitionNode )
		{
			m_NodeID = pTransitionNode->ID;
			m_FinalResultNodeID = pTransitionNode->GetOutputNodeID();
		}
	}

	NodeEditorTaskState AnimGraphTransitionTask::Tick( Timestep ts )
	{
		return NodeEditorTaskState::Completed;
	}

	void AnimGraphTransitionTask::Reset()
	{
	}
	
	//////////////////////////////////////////////////////////////////////////

	AnimGraphTransitionResultTask::AnimGraphTransitionResultTask()
	{
	}

	AnimGraphTransitionResultTask::~AnimGraphTransitionResultTask()
	{
	}

#if !defined(SAT_DIST)
	void AnimGraphTransitionResultTask::PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );
	
		// NB: FindLinkByPin is OK here, Pin does not have PinFlag_AcceptMultipleLinks flag.
		const auto link = pEditor->FindLinkByPin( pNode->Inputs[ 0 ]->ID );
		if( link && link->StartPinID )
		{
			auto otherPin = pEditor->FindPin( link->StartPinID );
			if( otherPin )
			{
				m_IncomingNodeID = otherPin->Node->ID;
			}
		}
	}
#endif

	void AnimGraphTransitionResultTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );

		AnimGraphTransitionResultTask* pThisOther = dynamic_cast< AnimGraphTransitionResultTask* >( pOther );
		if( pThisOther ) 
		{
			m_IncomingNodeID = pThisOther->m_IncomingNodeID;

			m_Result = pHandler->AccessLocator<bool>( m_IncomingNodeID, 0 );
		}
	}

	NodeEditorTaskState AnimGraphTransitionResultTask::Tick( Timestep ts )
	{
		return *m_Result ? NodeEditorTaskState::Completed : NodeEditorTaskState::Running;
	}

	void AnimGraphTransitionResultTask::Reset()
	{
	}

	void AnimGraphTransitionResultTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );

		RawSerialisation::WriteObjectChecked( m_IncomingNodeID, rStream );
	}

	void AnimGraphTransitionResultTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		RawSerialisation::ReadObjectChecked( m_IncomingNodeID, rStream );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( AnimGraphTransitionTask );
SAT_X31_CREATE_AUTO_REG( AnimGraphTransitionResultTask );
