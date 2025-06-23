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

#pragma once

#include "Saturn/GameFramework/Core/GameScript.h"
#include "Saturn/Scene/Entity.h"

#include "Navigation/NavBoundsEntity.h"

namespace Saturn {

	class BehaviourTree;

	class AIAgentEntity : public Entity
	{
		//////////////////////////////////////////////////////////////////////////
		// Needed for game class.

		SAT_DECLARE_CLASS( AIAgentEntity, Entity );
	public:
		AIAgentEntity();
		AIAgentEntity( const std::string& rName, UUID rId );
		~AIAgentEntity();

		virtual void BeginPlay() override;
		virtual void OnUpdate( Saturn::Timestep ts ) override;
		virtual void OnPhysicsUpdate( Saturn::Timestep ts ) override;

		Ref<StaticMesh>& GetMesh() { return m_Mesh; }
		const Ref<StaticMesh>& GetMesh() const { return m_Mesh; }

	private:
		// TODO: Change to a base mesh class, we don't know what the user will have.
		Ref<StaticMesh> m_Mesh;
		// TODO: Move this to a movement component.
		PhysicsRigidBody* m_RigidBody = nullptr;

		Ref<NavBoundsEntity> m_NavBoundsEntity;
		Ref<BehaviourTree> m_BehaviourTree;
	};
	
}
