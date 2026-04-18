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
#include "SandboxNodeEditorTasks.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#if !defined(SAT_DIST)
#include "SandboxNodeEditorNodes.h"
#include "Saturn/NodeEditor/NodeEditorBase.h"
#endif

#include "Saturn/NodeEditor/NodeEditorTaskHandler.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// SandboxNodeEditorNodeTask

	SandboxNodeEditorNodeTask::SandboxNodeEditorNodeTask()
	{
	}

	SandboxNodeEditorNodeTask::~SandboxNodeEditorNodeTask()
	{
	}

#if !defined(SAT_DIST)
	void SandboxNodeEditorNodeTask::PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		SandboxNodeEditorNode* pSandboxNode = dynamic_cast< SandboxNodeEditorNode* >( pNode );
		if( pSandboxNode )
		{
			m_Number = pSandboxNode->GetSpecialValue();
			m_NodeFlags = ( NodeEditorNodeFlags ) pSandboxNode->Flags;
		}
	}
#endif

	void SandboxNodeEditorNodeTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );

		SandboxNodeEditorNodeTask* pSandboxOther = dynamic_cast< SandboxNodeEditorNodeTask* >( pOther );
		if( pSandboxOther )
		{
			m_Number = pSandboxOther->m_Number;
			pHandler->RegisterLocator<uint64_t>( m_NodeID, &m_Number );
		}
	}

	NodeEditorTaskState SandboxNodeEditorNodeTask::Tick( Timestep ts )
	{
		return NodeEditorTaskState::Completed;
	}

	void SandboxNodeEditorNodeTask::Reset()
	{
		m_Number = 0llu;
	}

	void SandboxNodeEditorNodeTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );

		RawSerialisation::WriteObject( m_Number, rStream );
	}

	void SandboxNodeEditorNodeTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		RawSerialisation::ReadObject( m_Number, rStream );
	}

	//////////////////////////////////////////////////////////////////////////
	// SandboxNodeEditorOutputTask

	SandboxNodeEditorOutputTask::SandboxNodeEditorOutputTask()
	{
	}

	SandboxNodeEditorOutputTask::~SandboxNodeEditorOutputTask()
	{
	}

#if !defined(SAT_DIST)
	void SandboxNodeEditorOutputTask::PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		m_NodeFlags = ( NodeEditorNodeFlags ) pNode->Flags;

		if( !pNode || !pNode->Inputs.size() )
			return;

		// NB: FindLinkByPin is OK here, Pin does not have PinFlag_AcceptMultipleLinks flag.
		auto link = pEditor->FindLinkByPin( pNode->Inputs[ 0 ]->ID );
		if( link && link->StartPinID )
		{
			auto otherPin = pEditor->FindPin( link->StartPinID );
			if( otherPin )
			{
				m_IncomingNodeIDPin0 = otherPin->Node->ID;
			}
		}
	}
#endif

	void SandboxNodeEditorOutputTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );

		SandboxNodeEditorOutputTask* pSandboxOther = dynamic_cast< SandboxNodeEditorOutputTask* >( pOther );
		if( pSandboxOther )
		{
			m_IncomingNodeIDPin0 = pSandboxOther->m_IncomingNodeIDPin0;

			// We need to find what our input ptr is
			m_pInputNumber = pHandler->AccessLocator<uint64_t>( m_IncomingNodeIDPin0 );
		}
	}

	NodeEditorTaskState SandboxNodeEditorOutputTask::Tick( Timestep ts )
	{
		if( m_CurrentState != NodeEditorTaskState::Completed )
		{
			m_FinalNumber = ( *m_pInputNumber ) * 2;
		
			m_CurrentState = NodeEditorTaskState::Completed;
		}

		return m_CurrentState;
	}

	void SandboxNodeEditorOutputTask::Reset()
	{
		m_FinalNumber = 0llu;
	}

	void SandboxNodeEditorOutputTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );

		RawSerialisation::WriteObject( m_IncomingNodeIDPin0, rStream );
	}

	void SandboxNodeEditorOutputTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		RawSerialisation::ReadObject( m_IncomingNodeIDPin0, rStream );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( SandboxNodeEditorNodeTask );
SAT_X31_CREATE_AUTO_REG( SandboxNodeEditorOutputTask );
