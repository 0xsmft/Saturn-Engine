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
#include "AIAgentEntity.h"

#include "BehaviourTree/BehaviourTree.h"

#if !defined(SAT_DIST)
#include "BehaviourTree/AssetViewer/BehaviourTreeAssetViewer.h"
#include "Saturn/ImGui/ImGuiWindowManager.h"
#endif

namespace Saturn {

	AIAgentEntity::AIAgentEntity()
	{
		AddComponent<StaticMeshComponent>();
		AddComponent<BehaviourTreeComponent>();
		AddComponent<CharacterMovementComponent>();
	}

	AIAgentEntity::~AIAgentEntity()
	{
	}

	void AIAgentEntity::BeginPlay()
	{
		Super::BeginPlay();
	}

	void AIAgentEntity::OnUpdate( Saturn::Timestep ts )
	{
		Super::OnUpdate( ts );

		if( m_BehaviourTree )
			m_BehaviourTree->Tick( ts );
	}

	void AIAgentEntity::OnPhysicsUpdate( Saturn::Timestep ts )
	{
		Super::OnPhysicsUpdate( ts );
	}

	void AIAgentEntity::StartBehaviourTree( AssetID id )
	{
		m_BehaviourTree = Ref<BehaviourTree>::Create( id );
		m_BehaviourTree->Initialise( SharedFromThis() );

#if !defined(SAT_DIST)
		// Add reference if asset viewer is open
		const std::string name = std::format( "{0}##{1}", m_BehaviourTree->GetUnderlyingAsset()->Name, ( uint64_t ) id );
		if( Ref<BehaviourTreeAssetViewer> window = ImGuiWindowManager::Get()->GetWindow<BehaviourTreeAssetViewer>( name ); window )
		{
			window->AddBehviourTreeReference( m_BehaviourTree );
		}
#endif
	}

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG_SPWN( AIAgentEntity );
