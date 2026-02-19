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
#include "AnimationController.h"

#include "AssetViewer/Graph/Animation/AnimGraph.h"
#include "SkeletonAsset.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/NodeEditor/Serialisation/NodeCache.h"

namespace Saturn {

	AnimationController::AnimationController( AssetID id )
		: m_ControllerAsset( AssetManager::Get()->FindAsset( id ) )
	{
	}

	AnimationController::~AnimationController()
	{		
		m_TaskHandler = nullptr;
		m_AnimationGraph = nullptr;
	}

	void AnimationController::Initialise( Ref<Animator> animator )
	{
		m_AnimationGraph = SharedPtr<AnimGraph>::Create( m_ControllerAsset->ID );
		// Read only...
		m_AnimationGraph->SetPrivileges( NodeEditorUserAuthority::Editing, false );

		const std::string filename = std::format( "{0}.sac", m_ControllerAsset->Name );
		if( NodeCacheEditor::ReadNodeEditorCache( m_AnimationGraph, m_ControllerAsset->ID, filename ) )
		{
			const auto order2 = m_AnimationGraph->TraverseAndCreateTasks();
			m_TaskHandler = Ref<AnimGraphTaskHandler>::Create( animator );
			m_TaskHandler->InitWithCustomOrder2( m_AnimationGraph, order2 );
		}
	}

	void AnimationController::Tick( Timestep ts )
	{
		if( Input::Get().KeyPressed( RubyKey_C ) )
		{
			m_AnimationGraph->FindDataHandle( "Speed" )->Set<float>( 67.0f );
		}
		
		if( Input::Get().KeyPressed( RubyKey_V ) )
		{
			m_AnimationGraph->FindDataHandle( "Speed" )->Set<float>( 21.0f );
		}

		m_TaskHandler->Tick( ts );
	}

}
