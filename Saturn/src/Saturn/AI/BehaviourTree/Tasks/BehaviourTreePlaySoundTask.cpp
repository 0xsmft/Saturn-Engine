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
#include "BehaviourTreePlaySoundTask.h"

#include "Saturn/AI/BehaviourTree/AssetViewer/Nodes/BehaviourTreeNodeBase.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Audio/AudioSystem.h"

namespace Saturn {

	BehaviourTreePlaySoundTask::BehaviourTreePlaySoundTask( AssetID assetID )
	{
		Ref<Asset> asset = AssetManager::Get()->FindAsset( assetID );
		if( asset->Type == AssetType::GraphSound )
		{
			m_Sound = AudioSystem::Get().PlayGraphSound( assetID, UUID() );
		}
		else if( asset->Type == AssetType::Sound ) 
		{
			m_Sound = AudioSystem::Get().RequestNewSound( assetID, UUID(), false );
		}
	}

	void BehaviourTreePlaySoundTask::InitialiseTask( NodeEditorTaskHandler* pHandler, NodeEditorBase* pEditor, NodeEditorNodeBase* pNode )
	{
		m_NodeID = pNode->ID;
	}

	BehaviourTreePlaySoundTask::~BehaviourTreePlaySoundTask()
	{
		Reset();
		m_Sound = nullptr;
	}

	NodeEditorTaskState BehaviourTreePlaySoundTask::Tick( Timestep ts )
	{
		if( m_Sound->IsPlaying() )
		{
			m_CurrentState = NodeEditorTaskState::Running;
			return m_CurrentState;
		}

		m_Sound->Play( 0 );

		m_CurrentState = NodeEditorTaskState::Starting;
		return m_CurrentState;
	}

	void BehaviourTreePlaySoundTask::Reset()
	{
		AudioSystem::Get().UnloadSound( m_Sound );
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

//SAT_X31_CREATE_AUTO_REG( BehaviourTreePlaySoundTask );
