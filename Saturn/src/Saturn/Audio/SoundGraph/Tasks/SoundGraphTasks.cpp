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
#include "SoundGraphTasks.h"

#include "Saturn/NodeEditor/NodeEditorBase.h"

#include "Saturn/Audio/AudioSystem.h"

#include "Saturn/Audio/SoundGraph/SoundGraphTaskHandler.h"

#if !defined(SAT_DIST)
#include "Saturn/Audio/SoundGraph/Nodes/SoundGraphNodes.h"
#endif

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include "Saturn/Core/Random.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// SGraphSoundOutputTask

	SGraphSoundOutputTask::SGraphSoundOutputTask()
	{
	}

	SGraphSoundOutputTask::~SGraphSoundOutputTask()
	{
	}

#if !defined(SAT_DIST)
	void SGraphSoundOutputTask::PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );
	}
#endif

	void SGraphSoundOutputTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );
	}

	NodeEditorTaskState SGraphSoundOutputTask::Tick( Timestep ts )
	{
		return NodeEditorTaskState::Completed;
	}

	void SGraphSoundOutputTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );
	}

	void SGraphSoundOutputTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );
	}

	//////////////////////////////////////////////////////////////////////////
	// SGraphSoundPlayerTask
	
	SGraphSoundPlayerTask::SGraphSoundPlayerTask()
	{
	}

	SGraphSoundPlayerTask::~SGraphSoundPlayerTask()
	{
	}

#if !defined(SAT_DIST)
	void SGraphSoundPlayerTask::PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );

		SoundPlayerNode* pSoundNode = dynamic_cast< SoundPlayerNode* >( pNode );
		if( pSoundNode )
		{
			m_SpecAssetID = pSoundNode->GetAssetID();
			m_Spatialisation = pSoundNode->Inputs[ 0 ].As<BoolPin>()->Data;
		}
	}
#endif

	void SGraphSoundPlayerTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );

		SoundGraphTaskHandler* pSGHandler = dynamic_cast< SoundGraphTaskHandler* >( m_pHandler );
		if( pSGHandler )
		{
			SGraphSoundPlayerTask* pThisOther = dynamic_cast< SGraphSoundPlayerTask* >( pOther );
			if( pThisOther )
			{
				m_SpecAssetID = pThisOther->m_SpecAssetID;
				m_Spatialisation = pThisOther->m_Spatialisation;

				m_SoundIndex = pSGHandler->AddNewSound( m_SpecAssetID, m_Spatialisation );
				pSGHandler->RegisterLocator<size_t>( m_NodeID, 0, &m_SoundIndex );
		
				m_CurrentState = NodeEditorTaskState::Completed;
			}
		}
	}

	NodeEditorTaskState SGraphSoundPlayerTask::Tick( Timestep ts )
	{
		return m_CurrentState;
	}

	void SGraphSoundPlayerTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );

		RawSerialisation::WriteObject( m_SpecAssetID, rStream );
		RawSerialisation::WriteObject( m_Spatialisation, rStream );
	}

	void SGraphSoundPlayerTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		RawSerialisation::ReadObject( m_SpecAssetID, rStream );
		RawSerialisation::ReadObject( m_Spatialisation, rStream );
	}

	//////////////////////////////////////////////////////////////////////////
	// SGraphSoundRandomSoundTask

	SGraphSoundRandomSoundTask::SGraphSoundRandomSoundTask()
	{
	}

	SGraphSoundRandomSoundTask::~SGraphSoundRandomSoundTask()
	{
	}

#if !defined(SAT_DIST)
	void SGraphSoundRandomSoundTask::PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );

		// NB: FindLinkByPin is OK here, Pin does not have PinFlag_AcceptMultipleLinks flag.
		auto link = pEditor->FindLinkByPin( pNode->Inputs[ 0 ]->ID );
		if( link && link->StartPinID )
		{
			auto otherPin = pEditor->FindPin( link->StartPinID );
			if( otherPin )
			{
				m_PinANode = otherPin->Node->ID;
			}
		}

		link = pEditor->FindLinkByPin( pNode->Inputs[ 1 ]->ID );
		if( link && link->StartPinID )
		{
			auto otherPin = pEditor->FindPin( link->StartPinID );
			if( otherPin )
			{
				m_PinBNode = otherPin->Node->ID;
			}
		}
	}
#endif

	void SGraphSoundRandomSoundTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );

		SoundGraphTaskHandler* pSGHandler = dynamic_cast< SoundGraphTaskHandler* >( m_pHandler );
		if( pSGHandler )
		{
			SGraphSoundRandomSoundTask* pThisOther = dynamic_cast< SGraphSoundRandomSoundTask* >( pOther );
			if( pThisOther )
			{
				m_PinANode = pThisOther->m_PinANode;
				m_PinBNode = pThisOther->m_PinBNode;

				m_pIndexA = pSGHandler->AccessLocator<size_t>( m_PinANode, 0 );
				m_pIndexB = pSGHandler->AccessLocator<size_t>( m_PinBNode, 0 );
			}
		}
	}

	NodeEditorTaskState SGraphSoundRandomSoundTask::Tick( Timestep ts )
	{
		if( m_CurrentState == NodeEditorTaskState::Unknown )
		{
			m_ChosenIndex = Random::RandomElementInRange( glm::min( *m_pIndexA, *m_pIndexB ), glm::max( *m_pIndexA, *m_pIndexB ) );

			SoundGraphTaskHandler* pSGHandler = dynamic_cast< SoundGraphTaskHandler* >( m_pHandler );
			if( pSGHandler )
			{
				pSGHandler->RegisterLocator<size_t>( m_NodeID, 0, &m_ChosenIndex );
			}

			m_CurrentState = NodeEditorTaskState::Completed;
		}

		return m_CurrentState;
	}

	void SGraphSoundRandomSoundTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );

		RawSerialisation::WriteObject( m_PinANode, rStream );
		RawSerialisation::WriteObject( m_PinBNode, rStream );
	}

	void SGraphSoundRandomSoundTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		RawSerialisation::ReadObject( m_PinANode, rStream );
		RawSerialisation::ReadObject( m_PinBNode, rStream );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( SGraphSoundOutputTask );
SAT_X31_CREATE_AUTO_REG( SGraphSoundPlayerTask );
SAT_X31_CREATE_AUTO_REG( SGraphSoundRandomSoundTask );
