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
#include "PhysicsCharacterMovement.h"

#include "PhysicsMaterialAsset.h"
#include "PhysicsFoundation.h"
#include "PhysicsScene.h"
#include "PhysicsAuxiliary.h"

#include "Saturn/Scene/Entity.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Asset/AssetManager.h"

namespace Saturn {

	PhysicsCharacterMovement::PhysicsCharacterMovement( AssetID materialAsset, float height, float radius )
		: m_MaterialID( materialAsset ), m_Height( height ), m_Radius( radius )
	{
		m_Gravity = glm::length( glm::vec3{ 0.0f, -9.81f, 0.0f } );
	}

	PhysicsCharacterMovement::~PhysicsCharacterMovement()
	{
	}

	void PhysicsCharacterMovement::OnUpdate( Timestep ts )
	{
	}

	static Ref<PhysicsMaterialAsset> GetMaterial( AssetID materialID )
	{
		Ref<PhysicsMaterialAsset> materialAsset;

		Ref<Project> activeProject = Project::GetActiveProject();
		if( materialID == 0 || materialID == activeProject->GetDefaultPhysicsMaterialAsset() )
		{
			materialAsset = AssetManager::Get()->GetAssetAs<PhysicsMaterialAsset>( activeProject->GetDefaultPhysicsMaterialAsset() );
		}
		else
		{
			materialAsset = AssetManager::Get()->GetAssetAs<PhysicsMaterialAsset>( materialID );
		}

		return materialAsset;
	}

	void PhysicsCharacterMovement::CreateController( PhysicsScene* pScene, SharedPtr<Entity> entity, const glm::vec3& rOriginPosition )
	{
	}

	void PhysicsCharacterMovement::Move( const glm::vec3& rDisplacement )
	{
		m_Displacement += rDisplacement;
	}

	void PhysicsCharacterMovement::Jump( float pwr )
	{
		m_DownSpeed = -1.0f * pwr;
	}

	void PhysicsCharacterMovement::Teleport( const glm::vec3& rPosition )
	{
	}

	bool PhysicsCharacterMovement::IsGrounded() const
	{
		return m_CollisionFlags & PhysControllerCollision_Down;
	}

	glm::vec3 PhysicsCharacterMovement::GetPosition() const
	{
		return glm::vec3();
	}

}
