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
#include "BehaviourTreeBlackboardTask.h"

#include "Saturn/AI/BehaviourTree/AssetViewer/Nodes/BehaviourTreeNodeBase.h"
#include "Saturn/AI/BehaviourTree/AssetViewer/BehaviourTreeNodeEditor.h"
#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

namespace Saturn {

	BehaviourTreeBlackboardTask::BehaviourTreeBlackboardTask()
	{
	}

	BehaviourTreeBlackboardTask::~BehaviourTreeBlackboardTask()
	{
	}

#if !defined(SAT_DIST)
	void BehaviourTreeBlackboardTask::PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode )
	{
		Super::PreInitialiseTask( pEditor, pNode );

		BehaviourTreeNodeEditor* pBehNodeEd = dynamic_cast< BehaviourTreeNodeEditor* >( pEditor );
		if( pBehNodeEd && pBehNodeEd->GetBlackboardSpec() )
		{
			m_SpecBBID = pBehNodeEd->GetBlackboardSpec()->ID;
		}
	}
#endif

	void BehaviourTreeBlackboardTask::InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther )
	{
		Super::InitialiseTaskWithOther( pHandler, pOther );

		BehaviourTreeBlackboardTask* pThisOther = dynamic_cast< BehaviourTreeBlackboardTask* >( pOther );
		if( pThisOther )
		{
			Ref<BlackboardSpecificationAsset> bbSpec = AssetManager::Get()->GetAssetAs<BlackboardSpecificationAsset>( pThisOther->m_SpecBBID );

			if( bbSpec )
			{
				m_Blackboard = bbSpec->CreateBlackboard();
			}
		}
	} 

	void BehaviourTreeBlackboardTask::Serialise( std::ofstream& rStream ) const
	{
		Super::Serialise( rStream );

		RawSerialisation::WriteObject( m_SpecBBID, rStream );
	}

	void BehaviourTreeBlackboardTask::Deserialise( FDependentIStream& rStream )
	{
		Super::Deserialise( rStream );

		RawSerialisation::ReadObject( m_SpecBBID, rStream );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( BehaviourTreeBlackboardTask );
